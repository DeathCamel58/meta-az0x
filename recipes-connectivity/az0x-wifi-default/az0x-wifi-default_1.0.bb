SUMMARY = "Enable Wi-Fi by default on AZ0x devices"
DESCRIPTION = "Seeds ConnMan's technology settings at boot so the Wi-Fi radio \
comes up enabled. Needed because the rootfs is read-only and /var/lib is a \
tmpfs, so ConnMan's own persistence is lost on every boot and it defaults to \
WiFi Enable=false, which soft-blocks the radio via rfkill."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://connman-settings file://az0x-connman.conf"
S = "${WORKDIR}"

inherit systemd

do_install() {
    install -d ${D}${datadir}/az0x
    install -m 0644 ${S}/connman-settings ${D}${datadir}/az0x/connman-settings

    install -d ${D}${nonarch_libdir}/tmpfiles.d
    install -m 0644 ${S}/az0x-connman.conf ${D}${nonarch_libdir}/tmpfiles.d/
}

FILES:${PN} = "${datadir}/az0x ${nonarch_libdir}/tmpfiles.d"
RDEPENDS:${PN} = "connman"
