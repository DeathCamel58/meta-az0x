# Shared bits for building a real kernel for the AZ01 family from source.
# Required by both linux-yocto_%.bbappend and linux-yocto-rt_%.bbappend.
#
# Only in effect when AZ0X_KERNEL_PROVIDER selects one of these recipes; both
# poky recipes SkipRecipe unless PREFERRED_PROVIDER_virtual/kernel names them,
# so this is inert on the default prebuilt path.

FILESEXTRAPATHS:prepend := "${THISDIR}/az01-kernel-files:"

# poky's COMPATIBLE_MACHINE for these recipes is an anchored regex over qemu*
# only, so every board has to be added explicitly - the same thing
# meta-rockchip does for its own boards in linux-yocto_%.bbappend.
COMPATIBLE_MACHINE:az01-player = "az01-player"
COMPATIBLE_MACHINE:az01-premium = "az01-premium"
COMPATIBLE_MACHINE:az01-console = "az01-console"
COMPATIBLE_MACHINE:az01-mpc = "az01-mpc"

# Config fragment. multi_v7_defconfig (from rk3288.inc's KBUILD_DEFCONFIG) is
# the base - that is exactly how tinker-board, the RK3288 reference board,
# builds: no .scc BSP and no KMACHINE, because a defconfig is a complete
# substitute for a kmeta BSP definition.
SRC_URI:append:az01 = " file://az01-base.cfg"

# The board device trees. kernel-devicetree.bbclass literally runs
# `oe_runmake rockchip/<name>.dtb` in the kernel tree, so the .dts must exist
# inside arch/arm/boot/dts/rockchip/ AND be listed in that directory's
# Makefile - there is no variable that injects an external .dts. Hence a
# git-format patch that does both, which is the meta-arm pattern.
#
# The patch ships the whole AZ01 and AZ05 families as a layered tree
# (rk3288-az01.dtsi + option dtsis + per-board base dtsi + board dts), not 31
# near-identical decompiler dumps. Only the board named by AZ0X_BOARD_DTB is
# actually compiled, but shipping the family means any machine conf can point
# at any board without touching this recipe.
#
# The trees are generated, and regenerating them is reproducible: see
# tools/dts/README.md. Each one is verified to rebuild to a DTB semantically
# identical to the vendor's shipped blob before it lands here.
SRC_URI:append:az01 = " file://0001-ARM-dts-rockchip-add-inMusic-AZ01-AZ05-device-trees.patch"
