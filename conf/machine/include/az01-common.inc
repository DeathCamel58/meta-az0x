# Shared machine setup for every AZ01-family board: Rockchip RK3288, armv7hf.
#
# The az01 family spans many products. They share a byte-identical kernel and
# firmware set and differ only in which device trees ship, which modules are
# licensed in, and the exact rootfs geometry - so all of that lives in the
# per-board machine conf, and everything common lives here.
#
#   az01-premium.conf   "Planck AZ01 Premium"  jp13, jp14
#   az01-player.conf    "Planck AZ01 Player"   jp07, jp08, jp11
#   az01-console.conf   "Planck AZ01 Console"  jc11, jc16, nh08
#   az01-mpc.conf       "MPC upgrade image"    acv5/acv8/acva/acva2/acvb/acvm/acvr/acv5s
#
# The machine names are the vendor's own product lines, taken from each AZ01
# container header. Each board conf declares three facts and nothing else:
# AZ0X_BOARD_DIR (its vendor/ subdir), AZ0X_BOARD_KVER and AZ0X_ROOTFS_KIB.
#
# Every board here REUSES the vendor's compiled kernel, device trees, boot
# script and kernel modules as opaque prebuilts. Nothing builds a kernel or a
# bootloader: at flash time the vendor's own BOOT partitions are kept and only
# the rootfs partition is replaced. See ../../../README.md.

# rk3288 basics from meta-rockchip: SOC_FAMILY, rockchip MACHINEOVERRIDES and
# the cortexa17 tune include. We deliberately do NOT require rockchip-wic.inc
# or rockchip-extlinux.inc - both pull in u-boot.
require conf/machine/include/rk3288.inc

# Every az01-family board carries this override, so the prebuilt kernel recipe
# and the image base can say ":az01" once instead of listing four machines.
MACHINEOVERRIDES =. "az01:"

# ext4 geometry common to the whole family; only the block COUNT is per-board
# (AZ0X_ROOTFS_KIB, set in each board conf).
AZ0X_EXT4_BLOCKSIZE = "1024"
AZ0X_EXT4_INODE_RATIO = "4096"

# --- Toolchain / ABI -------------------------------------------------------
# Vendor binaries report: Tag_CPU_name "7VE", Tag_CPU_arch v7, Thumb-2,
# Tag_FP_arch VFPv4, Tag_ABI_VFP_args "VFP registers" (hard float).
# rk3288.inc's default is cortexa17thf-neon, which omits vfpv4 - override so
# anything we build here is ABI-identical to what is already on the device.
DEFAULTTUNE = "cortexa17thf-neon-vfpv4"

# --- Kernel ----------------------------------------------------------------
# Provider is switchable; see the include. Default is the vendor prebuilt.
require conf/machine/include/az01-kernel.inc

# --- Bootloader ------------------------------------------------------------
# Explicitly none. The vendor's u-boot, trust and loader partitions are left
# untouched by the repack step.
PREFERRED_PROVIDER_virtual/bootloader = ""
UBOOT_MACHINE = ""
MACHINE_FEATURES:remove = "efi"
EXTRA_IMAGEDEPENDS = ""
IMAGE_BOOT_FILES = ""

# --- Image -----------------------------------------------------------------
# A bare ext4 filesystem is exactly what the AZ01 container wants for its
# rootfs partition. No wic, no sdimg, no partition table.
IMAGE_FSTYPES = "ext4"
IMAGE_CLASSES:remove = "image_types_wic"

# --- Console / hardware ----------------------------------------------------
# rk3288.inc sets 115200;ttyS2, which matches the vendor boot.scr.uimg and the
# dtb chosen node (stdout-path = "serial2:115200n8"). bootargs carry no
# console=, so this DT path is what actually provides the console.
SERIAL_CONSOLES = "115200;ttyS2"

MACHINE_FEATURES = "usbhost usbgadget alsa screen ext2 rtc"

MACHINE_ESSENTIAL_EXTRA_RDEPENDS += "${AZ0X_ESSENTIAL_MODULES}"

# The bootloader counts warm reboots in a PMU register and enters recovery
# (fastboot) after a few unless the OS clears it; see az01-bootcount.
MACHINE_ESSENTIAL_EXTRA_RRECOMMENDS += "az01-bootcount"

# --- X11 ---------------------------------------------------------------------
# meta-rockchip's rockchip-defaults.inc lists xserver-xorg-xvfb in XSERVER, but
# poky's xserver-xorg does not enable the xvfb PACKAGECONFIG by default, so that
# package is never produced. Any x11 image on a rockchip machine then fails
# do_rootfs with "nothing provides xserver-xorg-xvfb".
#
# Xvfb is a headless virtual framebuffer X server - pointless on a board with a
# real panel - so drop it from the list rather than switch the PACKAGECONFIG on
# and ship a server we would never run.
XSERVER:remove = "xserver-xorg-xvfb"
