#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exercise the actual patched read callback against a simulated eFuse bank.

No device is opened. Checks every valid byte range, invalid/overflow ranges,
allocation/clock failures, and that every control write disables programming.
Run: python3 tools/test_secure_efuse.py
"""
from pathlib import Path
import subprocess
import tempfile

PATCH = Path(__file__).resolve().parents[1] / "recipes-kernel/linux/az01-kernel-files/0014-nvmem-rockchip-add-RK3288-secure-bank-word-reads.patch"

MOCK = r"""
#define _DEFAULT_SOURCE
#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint32_t __le32;
#define RK3288_A_SHIFT 6
#define RK3288_A_MASK 0x3ffu
#define RK3288_PGENB (1u << 3)
#define RK3288_LOAD (1u << 2)
#define RK3288_STROBE (1u << 1)
#define RK3288_CSB 1u
#define REG_EFUSE_CTRL 0
#define REG_EFUSE_DOUT 4
#define GFP_KERNEL 0
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define cpu_to_le32(v) htole32(v)
struct mutex { int held; };
struct clk { int enabled; };
struct rockchip_efuse_chip { void *base; struct clk *clk; struct mutex lock; };
static uint8_t rom[128], registers[8];
static uint32_t ctrl, latch;
static unsigned reads, writes, enables, allocations;
static int fail_alloc, fail_clk;
static void *kmalloc_array(size_t n, size_t size, int flags) {
    (void)flags;
    if (fail_alloc) return NULL;
    void *p = malloc(n * size);
    assert(p); allocations++; return p;
}
static void kfree(void *p) { assert(allocations); allocations--; free(p); }
static void mutex_lock(struct mutex *m) { assert(!m->held); m->held = 1; }
static void mutex_unlock(struct mutex *m) { assert(m->held); m->held = 0; }
static int clk_prepare_enable(struct clk *c) {
    if (fail_clk) return -EIO;
    assert(!c->enabled); c->enabled = 1; enables++; return 0;
}
static void clk_disable_unprepare(struct clk *c) { assert(c->enabled); c->enabled = 0; }
static void udelay(unsigned n) { assert(n >= 1); }
static uint32_t readl(void *address) {
    if (address == registers) return ctrl;
    assert(address == registers + 4);
    assert(ctrl & RK3288_STROBE);
    reads++; return latch;
}
static void writel(uint32_t value, void *address) {
    assert(address == registers);  /* Never write the data latch. */
    assert(value & RK3288_PGENB);  /* Programming ALWAYS disabled. */
    assert(!(value & ~((RK3288_A_MASK << RK3288_A_SHIFT) | 15u)));
    writes++; ctrl = value;
    if (value & RK3288_STROBE) {
        assert(value & RK3288_LOAD);
        unsigned word = (value >> RK3288_A_SHIFT) & RK3288_A_MASK;
        assert(word < 32);
        unsigned offset = word * 4;
        latch = (uint32_t)rom[offset] | ((uint32_t)rom[offset+1] << 8)
              | ((uint32_t)rom[offset+2] << 16) | ((uint32_t)rom[offset+3] << 24);
    }
}
static void reset(void) {
    assert(!allocations);
    ctrl = latch = reads = writes = enables = 0;
    fail_alloc = fail_clk = 0;
}
"""

TEST = r"""
int main(void) {
    struct clk clock = {0};
    struct rockchip_efuse_chip chip = { registers, &clock, {0} };
    uint8_t out[130];
    unsigned ranges = 0;
    for (unsigned i = 0; i < sizeof(rom); i++) rom[i] = (uint8_t)(i * 37 + 11);
    for (unsigned offset = 0; offset <= 128; offset++) {
        for (unsigned size = 0; size <= 128 - offset; size++) {
            reset(); memset(out, 0xaa, sizeof(out));
            assert(!rockchip_rk3288_secure_efuse_read(&chip, offset, out + 1, size));
            assert(!memcmp(out + 1, rom + offset, size));
            assert(out[0] == 0xaa);
            for (unsigned i = size + 1; i < sizeof(out); i++) assert(out[i] == 0xaa);
            assert(!chip.lock.held && !clock.enabled && !allocations);
            if (size) {
                assert(enables == 1);
                assert(reads == (offset % 4 + size + 3) / 4);
                assert(ctrl == (RK3288_PGENB | RK3288_CSB));
            } else assert(!writes && !enables);
            ranges++;
        }
    }
    unsigned invalid[][2] = { {128, 1}, {129, 0}, {127, 2}, {UINT_MAX, 1}, {0, UINT_MAX} };
    for (unsigned i = 0; i < sizeof(invalid)/sizeof(*invalid); i++) {
        reset();
        assert(rockchip_rk3288_secure_efuse_read(&chip, invalid[i][0], out, invalid[i][1]) == -EINVAL);
        assert(!writes && !enables && !allocations);
    }
    reset(); fail_alloc = 1;
    assert(rockchip_rk3288_secure_efuse_read(&chip, 0, out, 128) == -ENOMEM);
    assert(!writes && !enables && !chip.lock.held && !allocations);
    reset(); fail_clk = 1;
    assert(rockchip_rk3288_secure_efuse_read(&chip, 0, out, 128) == -EIO);
    assert(!writes && !clock.enabled && !chip.lock.held && !allocations);
    puts("secure eFuse: all byte ranges, bounds, read-only control and failure cleanup passed");
    printf("Validated %u valid ranges\n", ranges);
    return 0;
}
"""


def main():
    additions = "\n".join(line[1:] for line in PATCH.read_text().splitlines()
                          if line.startswith("+") and not line.startswith("+++"))
    start = additions.index("static int rockchip_rk3288_secure_efuse_read(")
    end = additions.index("\n}", start) + 2
    callback = additions[start:end]
    with tempfile.TemporaryDirectory(prefix="az0x-efuse-test-") as directory:
        source = Path(directory) / "test.c"
        binary = Path(directory) / "test"
        source.write_text(MOCK + callback + "\n" + TEST)
        subprocess.run(["cc", "-std=gnu11", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-g", str(source),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
