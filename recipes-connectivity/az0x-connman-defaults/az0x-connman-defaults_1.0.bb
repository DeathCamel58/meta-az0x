SUMMARY = "Seed ConnMan technology defaults on AZ0x devices"
DESCRIPTION = "Seeds ConnMan's technology settings at boot so both radios \
come up enabled. Needed because the rootfs is read-only and /var/lib is a \
tmpfs, so ConnMan's own persistence is lost on every boot and it falls back to \
Enable=false, which soft-blocks the radio via rfkill. \
\
Wi-Fi and Bluetooth are both affected, and Bluetooth is the less obvious of \
the two: the vendor bootargs carry rfkill.default_state=0, so hci0 registers \
soft-blocked, and ConnMan then writes [Bluetooth] Enable=false of its own \
accord and keeps it blocked. Clearing the block with rfkill does not hold - \
ConnMan simply reapplies it. Seeding Enable=true is what actually sticks, and \
once the block clears BlueZ powers the adapter itself with no configuration \
(there is no /etc/bluetooth/main.conf here; its AutoEnable default does it). \
\
Measured on an SC6000: with Enable=true seeded, hci0 comes up UP RUNNING and \
connmanctl reports the bluetooth technology Powered = True."

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
