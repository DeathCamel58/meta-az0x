# Build linux-yocto-rt for the AZ01 boards. Inert unless AZ0X_KERNEL_PROVIDER
# selects it (the recipe SkipRecipes unless PREFERRED_PROVIDER names it).
#
# The RT recipe has its own filename, so the plain linux-yocto_%.bbappend does
# not apply to it - bitbake requires the bbappend prefix to match, and
# "linux-yocto_%.bb".startswith("linux-yocto-r") is False. Without this file a
# AZ0X_KERNEL_PROVIDER="yocto-rt" build would get neither the AZ01 defconfig,
# config fragment and device-tree/driver patches from az01-kernel.inc, nor the
# COMPATIBLE_MACHINE entries that let it build for our boards.
require az01-kernel.inc

# Real-time preemption model; see the fragment for why the defconfig alone
# does not select it.
SRC_URI:append:az01 = " file://az01-rt.cfg"

# The same prefix rule hides meta-rockchip's linux-yocto_%.bbappend from this
# recipe, but the machine still inherits rk3288.inc's
#   KERNEL_FEATURES:append:rk3288 = " bsp/rockchip/remove-non-rockchip-arch-arm.scc"
# whose .scc lives in that bbappend's rockchip-kmeta. Without it
# do_kernel_metadata fails with "Feature ... not found". Pull the same kmeta in
# so both providers configure identically. The directory is located through
# BBPATH (every layer appends its LAYERDIR) rather than a hard-coded layer path;
# meta-rockchip is a LAYERDEPENDS of this layer, so it is always present.
AZ0X_ROCKCHIP_KMETA_DIR := "${@os.path.dirname(bb.utils.which(d.getVar('BBPATH'), 'recipes-kernel/linux/linux-yocto/rockchip-kmeta') or '')}"
FILESEXTRAPATHS:prepend:rockchip := "${AZ0X_ROCKCHIP_KMETA_DIR}:"
SRC_URI:append:rockchip = " file://rockchip-kmeta;type=kmeta;name=rockchip-kmeta;destsuffix=rockchip-kmeta"
