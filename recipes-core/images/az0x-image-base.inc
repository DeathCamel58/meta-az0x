# Shared base for every AZ0x image flavor, across both machines.
#
#   az0x-image.bb         (flavor a) minimal / de-branded - the default,
#                         builds for whichever MACHINE is selected.
#   az01-image-engine.bb  (flavor b) adds the vendor Engine application stack.
#                         az01-only: the grafted tree is armv7hf JP13 material.
#
# One image recipe serves every MACHINE; Yocto is per-MACHINE, so
# `MACHINE=az01-player bitbake az0x-image` and any other board each emit their
# own rootfs, sized and populated from that board's own vendor artifacts.

inherit core-image

# --- Filesystem geometry ---------------------------------------------------
# Matched per machine to the vendor rootfs read out with dumpe2fs, so a repack
# is a drop-in partition swap. Both vendor filesystems are journal-less (flash
# wear) and reserve no blocks for root.
#
#   az01-premium (jp13/jp14)  1024 x 504276   ~492 MiB
#   az01-player  (jp07/08/11) 1024 x 504076   ~492 MiB
#   az01-console (jc11/16/nh08) 1024 x 503936 ~492 MiB
#   az01-mpc     (acv*)       1024 x 430936   ~421 MiB
#   az04         (rmz2)       4096 x 205995   ~805 MiB
#
# Each board declares its own AZ0X_ROOTFS_KIB / blocksize / inode ratio in
# conf/machine/, so there are no per-machine overrides here at all.
IMAGE_FSTYPES = "ext4"

AZ0X_ROOTFS_KIB ?= "0"
AZ0X_EXT4_BLOCKSIZE ?= "4096"
AZ0X_EXT4_INODE_RATIO ?= "4096"

IMAGE_ROOTFS_SIZE = "${AZ0X_ROOTFS_KIB}"
IMAGE_ROOTFS_MAXSIZE = "${AZ0X_ROOTFS_KIB}"
IMAGE_OVERHEAD_FACTOR = "1.0"
IMAGE_ROOTFS_EXTRA_SPACE = "0"

EXTRA_IMAGECMD:ext4 = "-b ${AZ0X_EXT4_BLOCKSIZE} -O ^has_journal -m 0 -i ${AZ0X_EXT4_INODE_RATIO}"

# Both families mount / read-only (see each vendor /etc/fstab).
IMAGE_FEATURES += "read-only-rootfs"

# --- Kernel bits that must be in the rootfs --------------------------------
# az01: the vendor boot.scr.uimg loads zImage and the dtb OUT OF THE ROOTFS
#   partition's /boot, so kernel-image and kernel-devicetree are mandatory -
#   drop them and the device will not boot.
# az04: the kernel, dtb, cmdline overlay and initramfs live in a signed FIT on
#   a separate 'boot' partition, and the vendor's own rootfs ships an EMPTY
#   /boot. So only the modules belong in the filesystem.
AZ0X_KERNEL_INSTALL ?= "kernel-modules"
AZ0X_KERNEL_INSTALL:az01 = " \
    kernel-image \
    kernel-devicetree \
    kernel-modules \
"
AZ0X_KERNEL_INSTALL:az04 = "kernel-modules"

# --- Baseline userspace shared by both flavors -----------------------------
# NOTE: this deliberately does NOT use CORE_IMAGE_BASE_INSTALL. That pulls in
# packagegroup-base-extended, whose wifi/bluetooth packagegroups RRECOMMEND
# kernel-module-* package names that only exist when a kernel is actually
# built and split by kernel.bbclass. We reuse a prebuilt kernel, so those
# names never exist and the image fails to resolve. Each flavor therefore
# sets IMAGE_INSTALL explicitly from the pieces below.
AZ0X_BASE_INSTALL = " \
    packagegroup-core-boot \
    kmod \
    util-linux \
    e2fsprogs \
    e2fsprogs-e2fsck \
    dosfstools \
    alsa-utils \
"

# Firmware blobs, supplied by the machine's prebuilt kernel recipe straight
# from the vendor rootfs. Deliberately NOT linux-firmware: the az01 brcmfmac
# NVRAM and BT patchram are board-tuned and poky has no bcm43456 package at
# all, and az04 needs the ARM Mali CSF firmware. Neither has an upstream
# package that will actually bring the hardware up.
AZ0X_FIRMWARE_INSTALL = "az0x-firmware"
