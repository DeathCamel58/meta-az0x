# Flavor (b): feature-parity Engine graft

The tree ships **flavor (a)** — a minimal, de-branded rootfs — as the default.
This directory holds the scaffolding to turn it into **flavor (b)**, which puts
the vendor DJ application stack back on top of the custom base.

## Why this is an add-on and not a fork

`recipes-core/images/az0x-image-base.inc` holds everything both flavors share
(ext4 geometry, read-only rootfs, the prebuilt kernel packages). The two image
recipes differ by one line: `az01-image-engine.bb` adds
`IMAGE_INSTALL += "az01-engine-graft"`.

## To enable it

1. `cp az01-engine-graft.bb.sample az01-engine-graft.bb`
2. Fill in the `do_install` sections that are commented out. The parts you need
   to enumerate against a real vendor rootfs:
   - which `usr/lib/systemd/system/*.service` units start Engine,
   - the exact `LD_PRELOAD` shim set (`dtshim`, `drmatomic`, `alsashim`) and the
     environment file that references them,
   - whether the Qt/EGLFS runtime is stock (rebuild it from meta-qt5 against the
     same tune) or vendor-patched (graft it).
3. `bitbake az01-image-engine`

Until step 1 happens, `az01-image-engine.bb` deliberately `SkipRecipe`s with an
explanatory message rather than failing with a missing-provider error.

## ABI note

Every grafted binary was linked against glibc 2.39 / gcc 13.4 /
`arm-poky-linux-gnueabi`, armv7ve + Thumb-2 + NEON + VFPv4, hard-float. The
`DEFAULTTUNE = "cortexa17thf-neon-vfpv4"` in `conf/machine/az01.conf` and the
scarthgap pins across all five layers are what keep that true. Do not float a
layer onto a newer branch and expect grafted binaries to keep working.
