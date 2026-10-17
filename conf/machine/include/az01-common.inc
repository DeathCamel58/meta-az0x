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
# The kernel is built from source by default (linux-yocto 6.18 with this
# layer's device trees and drivers), or the vendor's compiled kernel can be
# staged instead; see az01-kernel.inc. No bootloader is built: the vendor's
# u-boot loads the kernel and device tree from the rootfs, and at flash time
# only the rootfs partition is replaced. See ../../../README.md.

# rk3288 basics from meta-rockchip: SOC_FAMILY, rockchip MACHINEOVERRIDES and
# the cortexa17 tune include. We deliberately do NOT require rockchip-wic.inc
# or rockchip-extlinux.inc - both pull in u-boot.
require conf/machine/include/rk3288.inc

# Every az01-family board carries this override, so the prebuilt kernel recipe
# and the board recipes can say ":az01" once instead of listing four machines.
MACHINEOVERRIDES =. "az01:"

# The rootfs partition every image must fit; only the block COUNT is
# per-board (AZ0X_ROOTFS_KIB, set in each board conf).
require conf/machine/include/az0x-rootfs.inc

# --- Toolchain / ABI -------------------------------------------------------
# Vendor binaries report: Tag_CPU_name "7VE", Tag_CPU_arch v7, Thumb-2,
# Tag_FP_arch VFPv4, Tag_ABI_VFP_args "VFP registers" (hard float).
# rk3288.inc's default is cortexa17thf-neon, which omits vfpv4 - override so
# anything we build here is ABI-identical to what is already on the device.
DEFAULTTUNE = "cortexa17thf-neon-vfpv4"

# --- Kernel ----------------------------------------------------------------
# Provider is switchable; see the include. Default is linux-yocto from source.
require conf/machine/include/az01-kernel.inc

# --- Bootloader ------------------------------------------------------------
# Explicitly none. The vendor's u-boot, trust and loader partitions are left
# untouched by the repack step.
PREFERRED_PROVIDER_virtual/bootloader = ""
UBOOT_MACHINE = ""
MACHINE_FEATURES:remove = "efi"
EXTRA_IMAGEDEPENDS = ""
IMAGE_BOOT_FILES = ""

# --- Boot ------------------------------------------------------------------
# The vendor u-boot loads zImage and the dtb OUT OF THE ROOTFS partition's
# /boot, through the boot script it finds there, so all three must be in every
# image - drop any of them and the device will not boot. Which recipe supplies
# the dtbs: the kernel's own kernel-devicetree, plus AZ0X_DTB_INSTALL for the
# case of a source kernel booting the vendor dtbs (az01-vendor-devicetree).
# az0x-signed boots a FIT from its own partition and needs no boot script.
AZ0X_BOOTSCRIPT_INSTALL ?= "az01-bootscript"
AZ0X_DTB_INSTALL ?= ""
MACHINE_ESSENTIAL_EXTRA_RDEPENDS += " \
    kernel-image \
    kernel-devicetree \
    ${AZ0X_DTB_INSTALL} \
    ${AZ0X_BOOTSCRIPT_INSTALL} \
"

# Board-tuned firmware (Broadcom Wi-Fi/Bluetooth NVRAM and patchram, touch,
# DSP) from the vendor files; linux-firmware has nothing that brings this
# hardware up.
MACHINE_ESSENTIAL_EXTRA_RDEPENDS += "az0x-firmware"

# Which source provides the BCM43456 Wi-Fi / BCM4345C5 Bluetooth payloads.
# "vendor" keeps the blobs az0x-firmware copies out of the vendor tree;
# "public" uses the pinned, CLM-carrying set from bcm43456-firmware-public and
# az0x-firmware omits the overlapping files, so exactly one package installs
# them. A machine sets this when its part is covered by the public set.
AZ0X_WIFI_FIRMWARE ?= "vendor"
MACHINE_ESSENTIAL_EXTRA_RDEPENDS += "${@bb.utils.contains('AZ0X_WIFI_FIRMWARE', 'public', 'bcm43456-firmware-public', '', d)}"
# The optional AP6256 module data (and its board-qualified links) belongs to
# the public set only; BCM43456_AP6256_BOARDS is set per machine, and a machine
# may select the public set without listing any board.
MACHINE_ESSENTIAL_EXTRA_RDEPENDS += "${@'bcm43456-firmware-public-nvram-ap6256' if d.getVar('AZ0X_WIFI_FIRMWARE') == 'public' and (d.getVar('BCM43456_AP6256_BOARDS') or '').split() else ''}"

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

# Named ALSA devices for boards whose channel map is known, picked at boot by
# product code; see az01-alsa-config.
MACHINE_ESSENTIAL_EXTRA_RRECOMMENDS += "az01-alsa-config"

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
