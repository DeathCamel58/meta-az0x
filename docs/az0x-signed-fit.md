# From-source kernel FIT for az0x-signed

## Why

`az0x-signed` (Prime 4+, JC11S and siblings) does not boot a zImage out of the
rootfs. Its u-boot loads a FIT named `kernel.fit` from the dedicated ext2
`kernel` partition and verifies it against key `denon-1` before starting. On the
vendor path that FIT is inMusic's own signed blob and there is nothing to build.

The project's goal is to run a **from-source PREEMPT-RT kernel** on this board
too. Signing the kernel and getting it onto the device is outside this layer's
scope and is left to an external boot-activation tool. What was missing on the
Yocto side was the FIT in the first place: a from-source provider built a plain
zImage, and the image guard refused a non-prebuilt kernel for this machine.

This change closes that gap. It permits the from-source path deliberately and
has the layer emit an unsigned FIT in the vendor shape.

## What changed

| File | Change |
|---|---|
| `conf/machine/include/az01-kernel.inc` | Comment now states az0x-signed boots a signed FIT and that the from-source path emits an unsigned one. |
| `conf/machine/az0x-signed.conf` | Header documents the opt-in from-source/FIT path. |
| `recipes-core/images/az0x-image-base.inc` | Guard comment describes the FIT path; the opt-in and its error are unchanged. |
| `README.md` | "Booting what you build" documents the FIT and the `DEPLOY_DIR_IMAGE` artifacts. |
| `recipes-kernel/linux/az01-kernel.inc` | Requires `az0x-signed-fit.inc`, so both `linux-yocto` and `linux-yocto-rt` get it. |
| `recipes-kernel/linux/az0x-signed-fit.inc` | **New.** FIT policy and the `do_deploy:append:az0x-signed` task. |
| `recipes-kernel/linux/az0x-signed-fit-files/fdt-cmdline.dts.in` | **New.** One-fragment `/chosen/bootargs` overlay template. |
| `recipes-kernel/linux/az0x-signed-fit-files/kernel.its.in` | **New.** FIT image-description template. |

## The FIT shape

The vendor u-boot boots `bootm 0x800800#conf-${board}#cmdline`, so it selects
configurations by name. The vendor FIT has these nodes:

```text
/images/kernel            type kernel,   load/entry 0x02000000
/images/fdt-<board>       type flat_dt,  load 0x01f00000
/images/fdt-cmdline       type flat_dt,  load 0x01fc2000, /chosen bootargs

/configurations/conf-<board>   default, kernel + fdt-<board>
/configurations/cmdline        fdt-cmdline only
```

The emitted FIT reproduces exactly that. The command-line overlay sets
`root=PARTLABEL=denonlinux` (`AZ0X_FIT_BOOTARGS`), the single line that redirects
the system from the on-device rootfs to the USB stick; the GPT partition name
and the ext4 label must both be `denonlinux`.

## Why a small `mkimage` task and not `kernel-fitimage.bbclass`

The standard `kernel-fitimage.bbclass` was evaluated first. It cannot produce
this shape:

- its image nodes are named `fdt-<dtbfile-with-extension>` and its configs
  `conf-<dtbfile>`, not `fdt-cmdline` / `cmdline`;
- it has no hook for an arbitrary extra flat_dt overlay image — the only
  overlay-ish addition, a `.dtbo` under `EXTERNAL_KERNEL_DEVICETREE`, still
  lands under `fdt-<name>.dtbo` and `conf-<name>.dtbo`.

u-boot's bootcmd looks up `conf-<board>` and `cmdline` by name, so a
deterministic `mkimage -f` over a checked-in `.its` is the minimal way to
match it. `kernel-fitimage.bbclass` remains the right choice for boards that
boot a conventional FIT; this one specifically does not.

## Why `do_deploy:append` and not a task

By the time `do_deploy:append` runs, `kernel.bbclass` has already put the kernel
image into `DEPLOYDIR` as the `${KERNEL_IMAGETYPE}` symlink and
`kernel-devicetree.bbclass` has put `<AZ0X_BOARD_DTB>.dtb` there. Reading the
deployed artifacts avoids guessing at `${B}/arch/arm/boot/...` paths, keeps the
assembly in the kernel's own deploy sstate, and needs no extra task or
`EXTRA_IMAGEDEPENDS` wiring. The whole body is gated on the `az0x-signed`
override and on `AZ0X_FIT_ENABLE`, so no other machine changes behaviour; on the
prebuilt path the containing recipes are not built at all.

## Verified so far

- `bitbake virtual/kernel` with `AZ0X_KERNEL_PROVIDER = "yocto-rt"` built
  `linux-yocto-rt` 6.6.151-rt31 (`SMP PREEMPT_RT`), and the deployed
  `kernel.fit` had the image and configuration nodes listed above, with
  `fdt-cmdline` carrying `root=PARTLABEL=denonlinux rootwait rw`.
- That kernel's zImage with `rk3288-az01-jc11s.dtb` and a USB rootfs, run in
  an rk3288 emulator in direct-Linux mode, reached a `ttyS3` login prompt.
  That mode bypasses u-boot, so it says nothing about the FIT or its
  signature check.
- The FIT itself has **not** been booted on hardware through this path.

## How to validate on a full build

On a real build tree:

```sh
MACHINE=az0x-signed \
AZ0X_KERNEL_PROVIDER=yocto-rt \
AZ0X_KERNEL_ALLOW_UNSIGNED=1 \
bitbake az0x-image
```

Then inspect the deployed artifacts:

```sh
ls tmp/deploy/images/az0x-signed/kernel.fit \
   tmp/deploy/images/az0x-signed/fdt-cmdline.dtbo \
   tmp/deploy/images/az0x-signed/kernel.its
dumpimage -l tmp/deploy/images/az0x-signed/kernel.fit
dtc -I dtb -O dts tmp/deploy/images/az0x-signed/fdt-cmdline.dtbo
```

Expected: `kernel`, `fdt-rk3288-az01-jc11s`, `fdt-cmdline` images;
`conf-rk3288-az01-jc11s` (default) and `cmdline` configurations; the overlay's
`bootargs` equal to `AZ0X_FIT_BOOTARGS`. Confirm the kernel image hash actually
matches the freshly built `zImage` and that the `fdt-` hash matches
`rk3288-az01-jc11s.dtb`.

Negative checks worth doing in CI:

- `MACHINE=az01-player AZ0X_KERNEL_PROVIDER=yocto `… must **not** emit a FIT
  and must build exactly as before;
- `MACHINE=az0x-signed` with `AZ0X_KERNEL_PROVIDER=yocto-rt` and **no**
  `AZ0X_KERNEL_ALLOW_UNSIGNED` must still be a parse-time error.

On hardware, a serial console is the only way to see a FIT the bootloader
rejects. Load addresses are copied from the vendor FIT and are unverified on a
from-source kernel (see Risks).

## Risks

- **Load addresses.** `0x02000000` / `0x01f00000` / `0x01fc2000` are taken from
  the vendor FIT, not measured for a from-source zImage. If the kernel must be
  decompressed or relocated differently the boot may hang. Verify on hardware
  before publishing artifacts.
- **Signing is out of scope.** This layer deliberately does not sign. Whatever
  gets the FIT onto the device must accept an unsigned FIT in this shape.
- **`rootwait rw` and console.** `AZ0X_FIT_BOOTARGS` defaults to
  `root=PARTLABEL=denonlinux rootwait rw` with no `console=`, relying on the
  jc11s device tree's `/chosen/stdout-path = "serial3:115200n8"` (uart3,
  `ttyS3`, which matches the vendor's own `console=ttyS3,115200n8`). The
  machine's getty is on `ttyS3` for the same reason. If an explicit
  `console=` is needed, this is the variable to set.
- **Kernel size.** A from-source RT kernel must still fit the fixed-size
  `kernel` partition. Watch the deployed `kernel.fit` size.

## Open questions

1. Does the signing step need the FIT to already contain `signature-0` nodes
   (so it can be re-pointed) rather than a clean unsigned FIT?
