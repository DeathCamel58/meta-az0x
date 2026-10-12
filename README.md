# meta-az0x

A Yocto Project BSP layer for inMusic Brands "EngineOS" devices: the Denon DJ,
Akai MPC, Numark and Rane hardware built on the AZ0x platform (Rockchip RK3288
for AZ01, RK3588 for AZ04).

**What it provides:** machine configurations, a Linux kernel built from source
with device trees and drivers for these boards, packaging for the vendor
firmware the hardware needs, board support (boot counter, panel rotation,
touch, jog-wheel display, network defaults), and two reference images.

**What it does not provide:** a distribution or applications. It is used with
Poky (or your own distro) like any other BSP layer, and application layers sit
on top. It never builds a bootloader; at flash time only the root filesystem
partition is replaced, and the device's own U-Boot boots it.

## Machines

| MACHINE | Product line | HWIDs | Default kernel | Verified on hardware |
|---|---|---|---|---|
| `az01-premium` | Planck AZ01 Premium (Denon DJ SC6000, SC6000M) | jp13, jp14 | from source | SC6000: display, touch, audio, USB hub and drive bay, control surface, jog-wheel display, reboot / reboot-to-fastboot / power-off |
| `az01-player` | Planck AZ01 Player (Denon DJ Prime Go and siblings) | jp07, jp08, jp11 | from source | Prime Go: display, touch, audio, DC-jack power, battery |
| `az01-console` | Planck AZ01 Console | jc11, jc16, nh08 | from source | not yet |
| `az01-mpc` | Akai MPC (MPC upgrade image) | acv5, acv8, acva, acva2, acvb, acvm, acvr, acv5s | vendor 6.18.26 | not yet |
| `az0x-signed` | signed-boot generation | jc11s, jp11s, jp20, jp21, jp21x, jp23 | vendor 6.1.111 | rootfs only, see [Booting what you build](#booting-what-you-build) |
| `az04` | Planck AZ04 (Rane SYSTEM ONE) | rmz2 | vendor 6.18.9 | scaffold only |

The from-source kernel is linux-yocto 6.6 (PREEMPT_RT optional). `az01-mpc`
keeps its vendor kernel by default because it is newer than ours; `az0x-signed`
and `az04` because their bootloaders only start signed kernels.

## Getting started

The layer is used with Poky. Everything below assumes the layers are cloned
side by side in one directory, which is what the template configuration expects.

### 1. Clone the layers

```sh
mkdir az0x && cd az0x
git clone -b scarthgap https://git.yoctoproject.org/poky
git clone -b scarthgap https://git.openembedded.org/meta-openembedded
git clone -b scarthgap https://git.yoctoproject.org/meta-arm
git clone -b scarthgap https://git.yoctoproject.org/meta-rockchip
git clone -b scarthgap-v1.0 https://github.com/DeathCamel58/meta-az0x
```

This release was built and tested against these commits. The `scarthgap`
branches only take fixes, so their tips normally work too; check these out if
they do not:

| layer | commit |
|---|---|
| poky | `4fb7016a07507683359ebd620f1951fd5ea44bfa` (yocto-5.0.20-167) |
| meta-openembedded | `0f00f8b9a21950640da8c5707343e5540133f86e` |
| meta-arm | `0a4b911d9fd63ab29c4532a907fae3282dbdcfe4` |
| meta-rockchip | `9690180b7e9953361958c962b80d891eda8c1028` |

### 2. Stage the vendor files

The devices need firmware that upstream does not carry (board-tuned Broadcom
Wi-Fi/Bluetooth files, touch firmware, a DSP blob), and the `prebuilt` kernel
option uses the vendor's own kernel. These are proprietary, so they are not in
this repository: you extract them from a firmware update for your device. See
[Vendor files](#vendor-files) for what each machine needs and where it goes.
By default the layer reads them from `vendor/` next to `meta-az0x`:

```
az0x/
├── poky/  meta-openembedded/  meta-arm/  meta-rockchip/  meta-az0x/
└── vendor/
    └── az01-premium/  (one directory per machine)
```

### 3. Create a build directory

```sh
TEMPLATECONF=$PWD/meta-az0x/conf/templates/default source poky/oe-init-build-env build
```

This creates `build/conf/` with every layer above already listed and the
settings the images need (systemd, merged /usr, PAM). `MACHINE` defaults to
`az01-premium`; change it in `conf/local.conf` or per command.

### 4. Build

```sh
bitbake az0x-image-desktop                       # SC6000
MACHINE=az01-player bitbake az0x-image-desktop   # Prime Go
```

The result is `tmp/deploy/images/<machine>/<image>-<machine>.rootfs.ext4`, a
raw ext4 image of the root filesystem partition.

### 5. Flash

The device's U-Boot has a fastboot mode that writes the image straight to the
`rootfs` partition. To enter it, run `reboot loader` on a running system, run
`fastboot 0` at the U-Boot prompt on the serial console, or on a Prime Go
power off, then hold `< Load` and `Load >` while pressing power. Then:

```sh
fastboot oem inmusic-unlock-magic-7de5fbc22b8c524e   # allow writes; no data is wiped
fastboot flash rootfs tmp/deploy/images/az01-premium/az0x-image-desktop-az01-premium.rootfs.ext4
fastboot reboot
```

Confirmed on JP11 (Prime Go) and JP13 (SC6000). Only the root filesystem is
replaced; the bootloader and the user data partition are not touched.

> **Do not use `oem format`, `oem resetenv`, `oem update-bootloader` or
> `oem flash-bootloader`.** The same unlock enables them and all four are
> destructive.

## Images

- `az0x-image`: a minimal, read-only root filesystem with a serial console and
  SSH, and no vendor applications.
- `az0x-image-desktop`: the Sato desktop, rotated for the panel, with touch
  input and an on-screen keyboard.
- `az01-image-engine`: a scaffold for grafting the vendor Engine application
  stack onto the image; it needs a graft recipe that is not part of this layer
  (see `recipes-az0x/engine/README.md`). `az01-premium` only.

## Kernel

`AZ0X_KERNEL_PROVIDER` selects who supplies the kernel, device trees and
modules:

| value | what you get |
|---|---|
| `yocto` | linux-yocto 6.6 with this layer's device trees and drivers (default on `az01-premium`, `az01-player`, `az01-console`) |
| `yocto-rt` | the same with PREEMPT_RT |
| `prebuilt` | the vendor's own compiled kernel, device trees and modules, staged from `vendor/` (default on `az01-mpc`, `az0x-signed`, `az04`) |

On the AZ01 boards U-Boot loads `zImage` and the device tree from the root
filesystem, so replacing the root filesystem replaces the kernel too. The
kernel changes live in `recipes-kernel/linux/az01-kernel-files/` as one patch
per driver: the AZ01/AZ05 device trees (generated from the vendor's compiled
trees, see `tools/dts/README.md`), the DC-jack power driver, the panel drivers,
the audio codec and machine driver, the USB hub attach driver, and fixes to
in-tree drivers. Each patch states its own `Upstream-Status`.

Choosing the source kernel on `az01-mpc` (a downgrade from 6.18) or on
`az0x-signed` (an unsigned kernel) is refused unless you also set
`AZ0X_KERNEL_ALLOW_DOWNGRADE = "1"` or `AZ0X_KERNEL_ALLOW_UNSIGNED = "1"`,
so neither happens by accident.

The source kernel also provides GPL read-only RK3288 secure-eFuse support for
board metadata. See [driver details and verification](docs/rk3288-secure-efuse.md).

## Booting what you build

Whether a device starts what this layer produces depends on the device, and
the two halves, root filesystem and kernel, are independent.

**The root filesystem is open everywhere tested.** Nothing re-verifies it at
boot: no dm-verity, no IMA/EVM appraisal, no signed root hash. A modified root
filesystem boots. On the newer signed-container devices the check that does
exist lives in the *updater*, which refuses an unsigned image from EngineOS
5.0.0 onwards; installing after downgrading to 4.3.2 sidesteps it.

**The kernel is not open everywhere.**

| board | how the kernel is loaded | can you boot your own? |
|---|---|---|
| `az01-premium` / `player` / `console` / `mpc` | U-Boot loads `zImage` and the dtb **from the root filesystem** and calls `bootz` on a raw image: no FIT, no signature | **yes**, proven on a Prime Go and an SC6000 |
| `az0x-signed` (Prime 4 Plus and siblings) | U-Boot loads `kernel.fit` from a dedicated kernel partition and its control FDT carries `required = "conf"` against key `denon-1` (`sha256,rsa2048`) | **not with stock U-Boot**; a tester has booted a kernel from this layer on a Prime 4 Plus by other means |
| `az04` | signed FIT, same enforcement | **not with stock U-Boot** |

For `az0x-signed` the layer still builds a kernel when asked
(`AZ0X_KERNEL_PROVIDER = "yocto"` or `"yocto-rt"` plus
`AZ0X_KERNEL_ALLOW_UNSIGNED = "1"`). It is deployed as an unsigned kernel FIT
in the vendor's shape (see `docs/az0x-signed-fit.md`), which this layer does not
sign or install; without a way to do that, stock U-Boot refuses it and the
device looks dead with no console output.

## Vendor files

Every machine needs a `vendor/<machine>/` directory with the device's own files,
copied from the root filesystem of a vendor firmware update for that device:

```
vendor/<machine>/lib/firmware/            all machines
vendor/<machine>/boot/                    zImage-<ver> or Image, *.dtb, boot.scr.uimg
vendor/<machine>/lib/modules/<ver>/       for the prebuilt kernel
```

| machine | taken from | notes |
|---|---|---|
| `az01-premium` | JP13 (SC6000) 5.0.x | the only family with Paragon UFSD (NTFS/HFS+) |
| `az01-player` | JP07 5.0.x | the same files serve JP08 and JP11 |
| `az01-console` | JC11 5.0.x | carries the az05 NH08 tree too |
| `az01-mpc` | ACV5 3.9.x | kernel 6.18.26, no eta5805 DSP firmware |
| `az0x-signed` | JC11S | no boot script: the kernel is a signed FIT |
| `az04` | RMZ2 | aarch64 |

Extracting an update image is described in
<https://github.com/DeathCamel58/denondj-firmware-modding>. Each machine
declares its vendor directory, kernel version and root filesystem size in its
`conf/machine/*.conf` (`AZ0X_BOARD_DIR`, `AZ0X_BOARD_KVER`, `AZ0X_ROOTFS_KIB`).
`AZ0X_VENDOR_DIR` moves the whole tree elsewhere. A recipe whose files are
missing is skipped with a message naming the path it expected, rather than
failing the parse for every machine.

Hardware facts gathered while bringing the boards up, such as the boot counter
and the power latch, are in `docs/az01-hardware-facts.md`.

## Dependencies

```
URI: https://git.yoctoproject.org/poky                  layers: meta, meta-poky
URI: https://git.openembedded.org/meta-openembedded     layer: meta-oe
URI: https://git.yoctoproject.org/meta-rockchip
URI: https://git.yoctoproject.org/meta-arm               layers: meta-arm, meta-arm-toolchain (for meta-rockchip)
branch: scarthgap
```

`LAYERDEPENDS_az0x = "core rockchip openembedded-layer"`. The layer targets
scarthgap (Yocto 5.0 LTS) only: the devices run glibc 2.39 and gcc 13, and
anything grafted from the vendor userspace must stay ABI-compatible.

## License

The layer's own metadata is MIT; see `COPYING.MIT`. Kernel patches are
GPL-2.0-only, like the kernel. The vendor files the layer packages are
proprietary and are not distributed here.

## Contributing

Issues and pull requests: <https://github.com/DeathCamel58/meta-az0x>.
Kernel patches follow the kernel's style, one patch per driver, each with an
`Upstream-Status` line.
