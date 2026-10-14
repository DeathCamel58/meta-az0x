SUMMARY = "Per-board ALSA devices for AZ01 boards"
DESCRIPTION = "The AZ01 codec exposes one multi-channel PCM per board, and \
what each channel drives differs from board to board. This installs named \
ALSA devices for each board whose channel map has been measured, and \
selects the right set at boot from the device tree's product code, since \
one image serves several boards."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://az01-alsa-select \
    file://az01-alsa-select.service \
    file://az01-alsa-select.init \
    file://boards/JP13.conf \
"

S = "${WORKDIR}"

COMPATIBLE_MACHINE = "az01"
PACKAGE_ARCH = "${MACHINE_ARCH}"

inherit systemd update-rc.d

SYSTEMD_SERVICE:${PN} = "az01-alsa-select.service"
INITSCRIPT_NAME = "az01-alsa-select"
INITSCRIPT_PARAMS = "start 38 S ."

do_install() {
    install -D -m 0755 ${S}/az01-alsa-select ${D}${libexecdir}/az01-alsa-select

    install -d ${D}${datadir}/az01-alsa/boards
    install -m 0644 ${S}/boards/*.conf ${D}${datadir}/az01-alsa/boards/

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install -D -m 0644 ${S}/az01-alsa-select.service \
            ${D}${systemd_system_unitdir}/az01-alsa-select.service
    fi
    if ${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        install -D -m 0755 ${S}/az01-alsa-select.init \
            ${D}${sysconfdir}/init.d/az01-alsa-select
    fi
}

FILES:${PN} += " \
    ${libexecdir}/az01-alsa-select \
    ${datadir}/az01-alsa \
"
