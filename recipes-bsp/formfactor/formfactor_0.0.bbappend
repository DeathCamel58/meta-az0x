# Supply a machconfig for the az01 family. Without it poky's defaults apply
# (HAVE_KEYBOARD=1, HAVE_TOUCHSCREEN=0), which drops the on-screen keyboard
# applet from the Sato panel and leaves a mouse cursor on a touch-only device.
#
# The file lives under files/az01/ rather than per-board: az01-common.inc adds
# an "az01" MACHINEOVERRIDE, and FILESOVERRIDES includes MACHINEOVERRIDES, so
# all four boards pick it up from the one path.
FILESEXTRAPATHS:prepend:az01 := "${THISDIR}/files:"
