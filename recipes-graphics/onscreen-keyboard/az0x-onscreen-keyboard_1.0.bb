SUMMARY = "Make the on-screen keyboard raise automatically on text focus"
DESCRIPTION = "matchbox-keyboard offers two independent ways to appear, and on \
a device with no physical keyboard you want both: \
\
1. A panel toggle button. That comes from the 'keyboard' applet, which Sato's \
   /etc/matchbox/session only adds when HAVE_KEYBOARD != 1 in the formfactor \
   config - see the formfactor bbappend in this layer. \
\
2. Automatic raise when a text field takes focus. That is a GTK input-method \
   module, matchbox-keyboard-im, which installs libmbgtk3-im-invoker.so and \
   registers itself in immodules.cache as 'matchbox-im-invoker'. Being \
   registered is not enough: GTK picks gtk-im-context-simple unless \
   GTK_IM_MODULE names something else, so without this the module is present, \
   cached, and never used. \
\
This recipe supplies (2) as a drop-in on the X server unit, so every GTK client \
in the session inherits it. A drop-in rather than editing /etc/default/ \
xserver-nodm, which belongs to xserver-nodm-init."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit features_check systemd
REQUIRED_DISTRO_FEATURES = "x11 systemd"

AZ0X_GTK_IM_MODULE ?= "matchbox-im-invoker"

S = "${WORKDIR}"
PACKAGE_ARCH = "${MACHINE_ARCH}"

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
    install -d ${D}${systemd_system_unitdir}/xserver-nodm.service.d
    cat > ${D}${systemd_system_unitdir}/xserver-nodm.service.d/10-gtk-im-module.conf <<DROPIN
# Select matchbox-keyboard's GTK input method so the on-screen keyboard raises
# itself when a text field takes focus. Without this GTK uses
# gtk-im-context-simple and the keyboard only ever appears via the panel
# toggle applet.
[Service]
Environment=GTK_IM_MODULE=${AZ0X_GTK_IM_MODULE}
DROPIN
}

FILES:${PN} = "${systemd_system_unitdir}/xserver-nodm.service.d"

RDEPENDS:${PN} = "matchbox-keyboard matchbox-keyboard-im"
