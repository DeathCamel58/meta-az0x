# meta-az0x

Yocto/OpenEmbedded layer for building custom root filesystems for inMusic
Brands "EngineOS" devices — Denon DJ, Akai MPC, Numark and Rane hardware built
on the AZ0x platform.

The layer builds **both** the root filesystem and, where the device allows it,
the kernel and device trees from source. `AZ0X_KERNEL_PROVIDER` selects which:

| value | what you get |
|---|---|
| `prebuilt` | the vendor's compiled kernel, dtbs, boot script and modules staged as opaque binaries (default) |
| `yocto` | linux-yocto 6.6 built from source, with our device trees and drivers |
| `yocto-rt` | the same, PREEMPT_RT |

No bootloader is ever compiled. At flash time only the rootfs partition is
replaced.

**Building a kernel and being able to boot it are separate questions.** See
[Booting what you build](#booting-what-you-build).

## Machines

| MACHINE | Vendor product line | HWIDs | SoC / tune | Kernel |
|---|---|---|---|---|
| `az01-premium` | Planck AZ01 Premium | jp13, jp14 | RK3288 armv7hf `cortexa17thf-neon-vfpv4` | 6.6.119 |
| `az01-player` | Planck AZ01 Player | jp07, jp08, jp11 | same | 6.6.119 |
| `az01-console` | Planck AZ01 Console | jc11, jc16, nh08 | same | 6.6.119 |
| `az01-mpc` | MPC upgrade image | acv5/acv8/acva/acva2/acvb/acvm/acvr/acv5s | same | 6.18.26 |
| `az0x-signed` | signed-boot generation | jc11s, jp11s, jp20, jp21, jp21x, jp23 | same | 6.1.111 |
| `az04` | Planck AZ04 | rmz2 | RK3588 aarch64 `cortexa76-cortexa55` | 6.18.9 |

Three machines run a **fully from-source stack on real hardware** — our kernel,
our device tree, our userspace:

- `az01-player` on a Denon DJ Prime Go: display, touch, audio, DC-jack power
  and battery.
- `az01-premium` on a Denon DJ SC6000: display, touch, audio, and the USB hub
  that carries the user ports, the internal drive bay and the control surface.
- `az0x-signed` on a Denon DJ Prime 4 Plus, with `linux-yocto-rt`, booted by a
  tester using embargoed workarounds — root filesystem on a USB stick through
  that same hub.

`az04` is a scaffold only.

The source kernel also provides GPL read-only RK3288 secure-eFuse support for
board metadata. See [driver details and verification](docs/rk3288-secure-efuse.md).

## Booting what you build

Whether a device will start what this layer produces depends on the device, and
the two halves are independent.

**The rootfs is open everywhere tested.** Nothing re-verifies it at boot — no
dm-verity mapping, no IMA/EVM appraisal, no signed root hash. A modified rootfs
boots. On the newer signed-container devices the check that does exist lives in
the *updater*, which refuses an unsigned image from EngineOS 5.0.0 onwards;
installing after downgrading to 4.3.2 sidesteps it.

**The kernel is not open everywhere.**

| board | how the kernel is loaded | can you boot your own? |
|---|---|---|
| `az01-premium` / `player` / `console` / `mpc` | u-boot loads `zImage` and the dtb **from the rootfs** and calls `bootz` on a raw image — no FIT, no signature | **yes**, proven on a Prime Go |
| `az0x-signed` (Prime 4 Plus and siblings) | u-boot loads `kernel.fit` from a dedicated kernel partition and its control FDT carries `required = "conf"` against key `denon-1` (`sha256,rsa2048`) | **not with stock u-boot** — but proven bootable on a Prime 4 Plus using embargoed workarounds |
| `az04` | signed FIT, same enforcement | **not with stock u-boot** |

For the signed boards the layer will still build you a kernel — set
`AZ0X_KERNEL_PROVIDER = "yocto"` (**or `"yocto-rt"` for the PREEMPT-RT build**)
and `AZ0X_KERNEL_ALLOW_UNSIGNED = "1"`, which exists precisely so that choice
is deliberate. On `az0x-signed` a from-source kernel is not deployed as a bare
zImage: the layer assembles a **kernel FIT** in the vendor shape — image nodes
`kernel`, `fdt-<board>` and `fdt-cmdline`, configurations `conf-<board>` and
`cmdline` — with a command-line overlay that sets `root=PARTLABEL=denonlinux`.
This layer does not sign it; signing it and writing it to the `kernel`
partition is left to an external boot-activation tool. Without such a boot
path the stock u-boot refuses the image before it starts, and the symptom
is a device that looks dead with no console output. That is why the opt-in is
required rather than assumed.

The `/boot/zImage` and dtbs that land in the rootfs on those boards are build
leftovers; u-boot never reads them. The FIT and its inputs land in
`DEPLOY_DIR_IMAGE` as `kernel.fit`, `fdt-cmdline.dtbo` and `kernel.its`.

## Images

- `az0x-image` — minimal, de-branded; builds for any MACHINE above.
- `az01-image-engine` — feature-parity flavor that grafts the vendor Engine
  application stack on top. Scaffolded; needs a graft recipe (see
  `recipes-az0x/engine/README.md`). `az01-premium` only.

## Dependencies

```
URI: https://git.yoctoproject.org/poky                (meta, meta-poky)
URI: https://git.openembedded.org/meta-openembedded   (meta-oe)
URI: https://git.yoctoproject.org/meta-rockchip
URI: https://git.yoctoproject.org/meta-arm            (required by meta-rockchip)
branch: scarthgap
```

`LAYERDEPENDS_az0x = "core rockchip openembedded-layer"`. Pin everything to
`scarthgap` (5.0 LTS): the devices run glibc 2.39 / gcc 13.4, and anything
grafted in later must stay ABI-compatible.

## Vendor artifacts are NOT in this repository

Every machine needs a `vendor/<AZ0X_BOARD_DIR>/` tree — the device's own
`zImage`/`Image`, its `*.dtb`, `boot.scr.uimg`, `lib/modules/<ver>/` and
`lib/firmware/`. These are proprietary vendor binaries (Paragon UFSD,
Broadcom/Cypress/ARM Mali firmware) and are deliberately excluded.

`AZ0X_VENDOR_DIR` defaults to `${LAYERDIR}/../vendor`. Stage them from an
extracted vendor rootfs — see
<https://github.com/DeathCamel58/denondj-firmware-modding> — so that:

```
vendor/az01-player/boot/{zImage-<ver>,*.dtb,boot.scr.uimg}
vendor/az01-player/lib/modules/<ver>/
vendor/az01-player/lib/firmware/
```

Each board declares three facts in its `conf/machine/*.conf` — `AZ0X_BOARD_DIR`,
`AZ0X_BOARD_KVER` and `AZ0X_ROOTFS_KIB` — and nothing else is board-specific.
Adding a board is a vendor directory plus a short machine conf.

## Adding the layer

```sh
bitbake-layers add-layer /path/to/meta-az0x
MACHINE=az01-player bitbake az0x-image
```

## License

MIT, except where a recipe states otherwise; see `COPYING.MIT`. The vendor
binaries this layer packages are proprietary and are not distributed here.

## Maintainer

Please submit changes via <https://github.com/DeathCamel58>.
