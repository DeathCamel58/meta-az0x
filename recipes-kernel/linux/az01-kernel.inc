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
