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
COMPATIBLE_MACHINE:az0x-signed = "az0x-signed"

# Config fragment. multi_v7_defconfig (from rk3288.inc's KBUILD_DEFCONFIG) is
# the base - that is exactly how tinker-board, the RK3288 reference board,
# builds: no .scc BSP and no KMACHINE, because a defconfig is a complete
# substitute for a kmeta BSP definition.
# Base kernel configuration.
#
# This replaces meta-rockchip's multi_v7_defconfig. That is a generic
# multi-SoC config: it brought in ~595 modules, of which roughly 548 were
# drivers for hardware this board does not have (OMAP, i.MX, Exynos and so on),
# while leaving things the vendor builds in sitting at =m. Comparing against
# the vendor's own config - read out of its zImage via IKCONFIG - it runs
# 1152 =y / 74 =m where we ran 1619 =y / 595 =m.
#
# This defconfig was generated with `bitbake -c savedefconfig virtual/kernel`
# from the known-good configuration, so adopting it is a no-op by construction.
# Trimming happens on top of it, deliberately and one step at a time.
#
# KBUILD_DEFCONFIG must be cleared: when set it names a file under
# arch/arm/configs/ and wins over a defconfig supplied through SRC_URI.
KBUILD_DEFCONFIG:az01 = ""
SRC_URI:append:az01 = " file://defconfig"

# poky.conf appends features/debug/debug-kernel.scc to KERNEL_EXTRA_FEATURES,
# for its SPDX kernel source manifests. That fragment is applied after the
# defconfig above and turns DWARF debug info back on, which undoes the
# defconfig's CONFIG_DEBUG_INFO_NONE. This is an appliance kernel; drop it.
# (DEBUG_KERNEL itself stays - EXPERT selects it.)
KERNEL_FEATURES:remove:az01 = "features/debug/debug-kernel.scc"

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

# The DC-jack power supply. Nothing in-tree covers it: the jack is sensed by a
# single SARADC channel behind a divider, with no charger IC. This also unblocks
# the SBS battery, which names the ac-power node in its power-supplies list and
# defers forever until that supply registers.
SRC_URI:append:az01 = " file://0002-power-supply-add-inMusic-AZ01-DC-jack-driver.patch"

# The DSI panel. Nothing in-tree drives it, and without a drm_panel the whole
# display pipeline stays down: rockchip-drm is component-based and will not
# finish binding until the panel registers, so there is no card0 at all.
SRC_URI:append:az01 = " file://0003-drm-panel-add-URT-UMOH-9522MD-T.patch"

# Audio. Two drivers, because the hardware is split that way: a codec with no
# control interface (master clock plus enable/mute/reset GPIOs) and a machine
# driver joining it to the Rockchip I2S controller.
#
# simple-audio-card cannot do this. The vendor device tree names both ends with
# flat inmusic,cpu / inmusic,codec phandles rather than the subnode form that
# binding requires, and reshaping the DT would cost the round-trip fidelity we
# verified across the whole board family. Nothing in-tree covers the codec
# either - max98357a is the closest and has no clock or reset handling.
SRC_URI:append:az01 = " file://0004-ASoC-codecs-add-the-inMusic-AZ0x-audio-codec.patch"
SRC_URI:append:az01 = " file://0005-ASoC-rockchip-add-the-inMusic-AZ0x-machine-driver.patch"

# Without this the audio above builds, registers a card and streams - into
# nothing. The vendor device tree sets slew-rate on the I2S pin configs,
# mainline's rockchip pinctrl implements slew rate for no SoC at all, and one
# rejected parameter makes the pinctrl core revert the entire state, mux
# included. The pins never reach the I2S function.
SRC_URI:append:az01 = " file://0006-pinctrl-rockchip-accept-and-ignore-slew-rate.patch"


# The USB hub. On the boards that have one - jp13 and relatives - every
# user-facing USB port, the internal SATA drive bay and the control surface sit
# behind an SMBus-configured hub that does not attach upstream until told to.
# Nothing in-tree covers it: onboard_usb_hub does power and reset sequencing
# only and never touches i2c, and usb251xb speaks a different register layout
# for the USB 2.0 parts. See the driver header for why it must not reset the
# hub.
SRC_URI:append:az01 = " file://0007-usb-misc-add-the-inMusic-AZ01-USB-hub-attach-driver.patch"
