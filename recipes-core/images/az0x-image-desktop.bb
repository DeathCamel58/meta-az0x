SUMMARY = "AZ0x root filesystem with a desktop environment"
DESCRIPTION = "Flavor (c): the minimal az0x base plus a graphical desktop. \
STUB - the frame is wired up and the Weston tier builds, but nothing here has \
been run on hardware yet. Read the space and tier notes below before using it."

LICENSE = "MIT"

require az0x-image-base.inc

IMAGE_FEATURES += "debug-tweaks ssh-server-openssh"

# --- Which desktop -----------------------------------------------------------
# Set AZ0X_DESKTOP in local.conf. Tiers, cheapest first:
#
#   weston  Wayland compositor, DRM/KMS backend.        <- default, recommended
#           No extra layer and no x11. The distro already has the wayland and
#           opengl DISTRO_FEATURES, but it needs ONE more:
#
#               DISTRO_FEATURES:append = " pam"
#
#           packagegroup-core-weston requires pam whenever the init manager is
#           systemd (it is here), for the session weston-init sets up. This is
#           deliberately NOT forced by this recipe: pam changes login handling
#           for every image built from this tree, so it is your call to make in
#           local.conf rather than a side effect of selecting a desktop.
#
#   xfce    Full X11 desktop. Needs meta-xfce in BBLAYERS *and* x11 put back
#           into DISTRO_FEATURES - local.conf currently removes it. Big.
#
#   gnome   Full GNOME. Needs meta-gnome in BBLAYERS. Very big; see the space
#           note - this will not fit an az01 rootfs partition.
#
AZ0X_DESKTOP ?= "weston"

AZ0X_DESKTOP_INSTALL_weston = " \
    packagegroup-core-weston \
    weston-init \
    wayland-utils \
"
AZ0X_DESKTOP_INSTALL_xfce = " \
    packagegroup-xfce-base \
"
AZ0X_DESKTOP_INSTALL_gnome = " \
    packagegroup-gnome-desktop \
"

AZ0X_DESKTOP_INSTALL = "${@d.getVar('AZ0X_DESKTOP_INSTALL_' + (d.getVar('AZ0X_DESKTOP') or '')) or ''}"

# Touch input. The az01 panels are touchscreens (ILI2117 on jp11), and libinput
# is what Weston uses to pick them up. udev rules come from the base.
AZ0X_DESKTOP_INPUT ?= "libinput"

IMAGE_INSTALL = " \
    ${AZ0X_KERNEL_INSTALL} \
    ${AZ0X_BASE_INSTALL} \
    ${AZ0X_FIRMWARE_INSTALL} \
    ${AZ0X_DESKTOP_INSTALL} \
    ${AZ0X_DESKTOP_INPUT} \
    ${AZ0X_DEBUG_INSTALL} \
    ${CORE_IMAGE_EXTRA_INSTALL} \
"

# Inherited from az0x-image.bb's list; set to "" in local.conf to claw back
# space, which you will probably need to do.
AZ0X_DEBUG_INSTALL ?= ""

# --- Space ------------------------------------------------------------------
# This is the binding constraint, not the software. The az01 rootfs partitions
# are fixed by the vendor layout and IMAGE_ROOTFS_MAXSIZE is pinned to match:
#
#   az01-premium / player / console   ~492 MiB
#   az01-mpc                          ~421 MiB
#   az04                              ~805 MiB
#
# The minimal az0x-image already uses ~112 MiB, leaving roughly 380 MiB on the
# az01 boards. Weston plus Mesa should fit; a full X11 or GNOME desktop will
# not. If the rootfs outgrows the partition the build fails at do_image_ext4
# rather than producing something unflashable - that is deliberate.
#
# If you need more room the options are, in order: drop AZ0X_DEBUG_INSTALL,
# drop ssh-server-openssh, or stop trying to fit a full DE in half a gigabyte.

# --- Graphics ---------------------------------------------------------------
# Hardware GL comes for free on the az01 boards: meta-rockchip's
# mesa_%.bbappend does PACKAGECONFIG:append:rk3288 = " kmsro panfrost", and our
# machines carry the rk3288 override via SOC_FAMILY. The device is already
# proven to bind panfrost and create fb0 under the vendor kernel.
#
# NOTE for az04: that same bbappend has no rk3588 entry, so Mesa would fall
# back to swrast there. az04 is not flashable anyway (see README), but do not
# expect accelerated GL from this recipe on it without adding panfrost to
# PACKAGECONFIG yourself.

# --- Guard rails ------------------------------------------------------------
# Fail at parse time with something readable rather than a wall of missing
# providers three minutes into a build.
python () {
    want = d.getVar('AZ0X_DESKTOP') or ''
    tiers = ('weston', 'xfce', 'gnome')
    if want not in tiers:
        raise bb.parse.SkipRecipe(
            "AZ0X_DESKTOP is '%s'; expected one of %s" % (want, ', '.join(tiers)))

    collections = (d.getVar('BBFILE_COLLECTIONS') or '').split()
    needed_layer = {'xfce': 'xfce-layer', 'gnome': 'gnome-layer'}.get(want)
    if needed_layer and needed_layer not in collections:
        raise bb.parse.SkipRecipe(
            "AZ0X_DESKTOP = '%s' needs the %s layer. It is already cloned at "
            "meta-openembedded/meta-%s but is not in bblayers.conf - add it "
            "with 'bitbake-layers add-layer'." % (want, needed_layer, want))

    features = (d.getVar('DISTRO_FEATURES') or '').split()
    init = d.getVar('VIRTUAL-RUNTIME_init_manager') or ''
    if want == 'weston' and init == 'systemd' and 'pam' not in features:
        raise bb.parse.SkipRecipe(
            "AZ0X_DESKTOP = 'weston' with systemd needs the pam DISTRO_FEATURE "
            "(packagegroup-core-weston requires it for the session). Add "
            "'DISTRO_FEATURES:append = \" pam\"' to local.conf. It is not "
            "forced here because pam affects every image in this tree.")

    if want == 'xfce' and 'x11' not in features:
        raise bb.parse.SkipRecipe(
            "AZ0X_DESKTOP = 'xfce' is an X11 desktop, but local.conf removes "
            "x11 from DISTRO_FEATURES. Put it back, or use the weston tier "
            "which needs no x11.")
}
