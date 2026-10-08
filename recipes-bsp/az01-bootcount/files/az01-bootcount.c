/* az01-bootcount - show or clear the AZ01 bootloader's boot counter.
 *
 * The AZ01 u-boot keeps a boot counter in the RK3288 PMU syscon (base
 * 0xff730000, offset 0xa0), using u-boot's bootcount_syscon format: the
 * upper 16 bits hold the 0xb001 magic and the lower 16 bits the count. The
 * bootloader increments it on every boot; a power cycle clears it, a warm
 * reset does not. After a few warm reboots without a reset it decides boots
 * are failing and enters recovery (fastboot) instead of booting the kernel.
 *
 * The OS is expected to clear the counter once it has booted successfully.
 * `reset` writes magic | 0, and only if the register already holds the magic:
 * on anything else it reports and leaves the register alone.
 *
 *   az01-bootcount [show]   print the raw register and the count
 *   az01-bootcount reset    clear the count (exit 0 when nothing to do)
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PMU_BASE        0xff730000UL
#define PMU_MAP_LEN     0x1000UL
#define BOOTCOUNT_OFF   0xa0UL
#define BOOTCOUNT_MAGIC 0xb001u

/* Only touch the register on the board family whose bootloader uses it. */
static int is_az01(void)
{
    char buf[512];
    ssize_t n, i;
    int az01 = 0, rk3288 = 0;
    int fd = open("/proc/device-tree/compatible", O_RDONLY);

    if (fd < 0)
        return 0;
    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return 0;
    buf[n] = '\0';
    /* NUL-separated string list */
    for (i = 0; i < n; i += (ssize_t)strlen(buf + i) + 1) {
        if (!strcmp(buf + i, "inmusic,az01"))
            az01 = 1;
        else if (!strcmp(buf + i, "rockchip,rk3288"))
            rk3288 = 1;
    }
    return az01 && rk3288;
}

int main(int argc, char **argv)
{
    const char *cmd = argc > 1 ? argv[1] : "show";
    int reset = !strcmp(cmd, "reset");
    volatile uint32_t *reg;
    uint32_t val;
    void *map;
    int fd;

    if (!reset && strcmp(cmd, "show")) {
        fprintf(stderr, "usage: %s [show|reset]\n", argv[0]);
        return 2;
    }
    if (!is_az01()) {
        fprintf(stderr, "az01-bootcount: not an AZ01 (RK3288) board; "
                        "nothing to do\n");
        return reset ? 0 : 1;
    }

    fd = open("/dev/mem", (reset ? O_RDWR : O_RDONLY) | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "az01-bootcount: /dev/mem: %s\n", strerror(errno));
        return 1;
    }
    map = mmap(NULL, PMU_MAP_LEN, PROT_READ | (reset ? PROT_WRITE : 0),
               MAP_SHARED, fd, (off_t)PMU_BASE);
    close(fd);
    if (map == MAP_FAILED) {
        fprintf(stderr, "az01-bootcount: mmap PMU: %s\n", strerror(errno));
        return 1;
    }
    reg = (volatile uint32_t *)((volatile uint8_t *)map + BOOTCOUNT_OFF);
    val = *reg;

    if ((val >> 16) != BOOTCOUNT_MAGIC) {
        printf("az01-bootcount: register 0x%08x has no bootcount magic; "
               "not touching it\n", (unsigned)val);
        munmap(map, PMU_MAP_LEN);
        return reset ? 0 : 1;
    }
    if (!reset) {
        printf("az01-bootcount: 0x%08x (count %u)\n", (unsigned)val,
               (unsigned)(val & 0xffffu));
        munmap(map, PMU_MAP_LEN);
        return 0;
    }
    if ((val & 0xffffu) == 0) {
        printf("az01-bootcount: count already 0\n");
        munmap(map, PMU_MAP_LEN);
        return 0;
    }

    *reg = (uint32_t)BOOTCOUNT_MAGIC << 16;
    if (*reg != (uint32_t)BOOTCOUNT_MAGIC << 16) {
        fprintf(stderr, "az01-bootcount: write did not stick (0x%08x)\n",
                (unsigned)*reg);
        munmap(map, PMU_MAP_LEN);
        return 1;
    }
    printf("az01-bootcount: count %u -> 0\n", (unsigned)(val & 0xffffu));
    munmap(map, PMU_MAP_LEN);
    return 0;
}
