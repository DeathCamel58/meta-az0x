SUMMARY = "Keep the SC6000 jog-wheel display from resetting while idle"
DESCRIPTION = "The round display in the SC6000 jog wheel is a USB-MIDI \
device that resets itself about every 10 seconds until a host talks to it, \
re-enumerating each time. Applications that open it during that loop end up \
with a dead MIDI port. This udev rule sends it a MIDI device inquiry when it \
appears, which stops the loop until an application starts driving it."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://az01-wheel-display-release \
    file://90-az01-wheel-display.rules \
"

S = "${UNPACKDIR}"

COMPATIBLE_MACHINE = "az01"
PACKAGE_ARCH = "${MACHINE_ARCH}"

do_install() {
    install -D -m 0755 ${S}/az01-wheel-display-release \
        ${D}${libexecdir}/az01-wheel-display-release
    install -D -m 0644 ${S}/90-az01-wheel-display.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/90-az01-wheel-display.rules
}

FILES:${PN} = " \
    ${libexecdir}/az01-wheel-display-release \
    ${nonarch_base_libdir}/udev/rules.d/90-az01-wheel-display.rules \
"
