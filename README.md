# meta-az0x

Yocto/OpenEmbedded layer for building custom root filesystems for inMusic
Brands "EngineOS" devices — Denon DJ, Akai MPC, Numark and Rane hardware built
on the AZ0x platform.

The layer **reuses each device's vendor-compiled kernel, device trees, boot
script and kernel modules as opaque prebuilts.** No kernel and no bootloader is
ever compiled: only userspace is built, and at flash time only the rootfs
partition is replaced.

## Machines

| MACHINE | Vendor product line | HWIDs | SoC / tune | Kernel |
|---|---|---|---|---|
| `az01-premium` | Planck AZ01 Premium | jp13, jp14 | RK3288 armv7hf `cortexa17thf-neon-vfpv4` | 6.6.119 |
| `az01-player` | Planck AZ01 Player | jp07, jp08, jp11 | same | 6.6.119 |
| `az01-console` | Planck AZ01 Console | jc11, jc16, nh08 | same | 6.6.119 |
| `az01-mpc` | MPC upgrade image | acv5/acv8/acva/acva2/acvb/acvm/acvr/acv5s | same | 6.18.26 |
| `az04` | Planck AZ04 | rmz2 | RK3588 aarch64 `cortexa76-cortexa55` | 6.18.9 |

`az01-player` is **confirmed booting on real hardware** (Denon DJ Prime Go, to a
login prompt). `az04` is a scaffold only — those devices enforce RSA-2048 FIT
signing and dm-verity, so a custom rootfs cannot boot without the vendor key.

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
