# RK3288 secure eFuse support

## Ownership and licence

This is shared AZ01/AZ05 board support, so it lives in `meta-az0x` and is
applied to both from-source kernel providers through `az01-kernel.inc`:

`recipes-kernel/linux/az01-kernel-files/0014-nvmem-rockchip-add-RK3288-secure-bank-word-reads.patch`

The patch extends Linux's existing `drivers/nvmem/rockchip-efuse.c`, preserving
its **GPL-2.0-only** SPDX licence and Rockchip copyright attribution. The
resulting `nvmem_rockchip_efuse` module declares `GPL v2`. There is no proprietary
kernel module, application shim, firmware image or embedded board data in this
change. Vendor applications remain separate from the open driver.

## The two RK3288 banks

| Bank | Compatible | Addressing | Size on JC11S |
|---|---|---|---|
| Ordinary | `rockchip,rk3288-efuse` | Byte addresses, byte data latch | 32 bytes |
| Secure | `rockchip,rk3288-secure-efuse` | Word addresses, 32-bit data latch | 128 bytes |

The secure-bank device already exists in the board DT at `0xffb10000`, but
mainline 6.6 had no matching callback. Matching it to the ordinary byte reader
would produce incorrect metadata.

The stock inMusic **6.6.119** driver on the Prime uses **direct MMIO** for this
bank. Its word-aligned reads and byte slicing were checked against that
kernel's `rockchip_rk3288_secure_efuse_read`. Generic Rockchip vendor trees also
contain a SiP secure-monitor variant, but that is not the path used here; the
port introduces no SiP calls or trusted-firmware dependency.

## Read-only behaviour

- The existing nvmem configuration remains `read_only = true`, with no write
  callback.
- Requests are bounded to the 128-byte bank and read through an aligned
  temporary word buffer, including arbitrary unaligned byte ranges.
- A per-device mutex serialises secure-bank reads and clock operations.
- Every read-control register write retains `PGENB` (programming disabled).
  These register writes select addresses and strobe the read latch; they do
  not program eFuses. Standby is restored before disabling the clock.
- Allocation and clock failures return without strobing the hardware and
  release their resources.
- `NVMEM_DEVID_AUTO` lets both banks register simultaneously. Without this,
  both attempt to create `rockchip-efuse0` and the second probe fails.

The driver has no storage-device code and does not access or repair eMMC.

## Build and test

```sh
python3 tools/test_secure_efuse.py
bitbake virtual/kernel
bitbake package-index
```

The test extracts the actual added callback from the patch, compiles it against
a simulated register bank with AddressSanitizer/UndefinedBehaviorSanitizer,
and checks all **8,385 valid byte ranges**, invalid/overflow bounds, allocation
and clock failures, programming-disable control bits, standby and cleanup.

For the tested module-enabled RT configuration, install the matching module
package on the USB root disk:

```sh
opkg install kernel-module-nvmem-rockchip-efuse
modprobe nvmem-rockchip-efuse
```

## Prime 4+ verification (2026-10-10)

- Built against `6.6.151-rt31-yocto-preempt-rt` and deployed the module without
  flashing or rebooting the kernel.
- Both banks bound; their sysfs `nvmem` files are read-only and report 32 and
  128 bytes respectively. The secure-bank read succeeded with nonzero data;
  raw contents were not printed, saved or included in the repository.
- The sandboxed vendor Engine app passed its board check and reached Qt/QML
  initialisation. It subsequently aborted when its IRQ-affinity configuration
  hit the sandbox's read-only `/proc/irq`; that is application integration,
  owned by OptimusLinux, rather than an eFuse-driver failure.
- eMMC remained locked read-only with zero write I/O during these tests.

Only the JC11S hardware has been exercised. The compatible also exists in the
layer's other RK3288 board trees; those still need real-device validation.
