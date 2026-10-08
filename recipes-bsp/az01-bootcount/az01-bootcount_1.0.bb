SUMMARY = "Clear the AZ01 bootloader's boot counter after a successful boot"
DESCRIPTION = "The AZ01 u-boot counts boots in a PMU register that survives \
warm resets (u-boot bootcount_syscon format, PMU syscon offset 0xa0). Unless \
the OS clears it, a few consecutive warm reboots make the bootloader enter \
recovery (fastboot) instead of booting. This installs a small helper and a \
boot-time unit that clears the count once userspace is up. Power cycles clear \
the register anyway, which is why the problem only shows up on warm reboots."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://az01-bootcount.c \
    file://az01-bootcount-reset.service \
    file://az01-bootcount-reset.init \
"

S = "${WORKDIR}"

COMPATIBLE_MACHINE = "az01"
PACKAGE_ARCH = "${MACHINE_ARCH}"

inherit systemd update-rc.d

SYSTEMD_SERVICE:${PN} = "az01-bootcount-reset.service"
INITSCRIPT_NAME = "az01-bootcount-reset"
INITSCRIPT_PARAMS = "start 99 2 3 4 5 ."

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -Wall -Wextra -o az01-bootcount ${S}/az01-bootcount.c
}

do_install() {
    install -D -m 0755 ${B}/az01-bootcount ${D}${sbindir}/az01-bootcount

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        install -D -m 0644 ${S}/az01-bootcount-reset.service \
            ${D}${systemd_system_unitdir}/az01-bootcount-reset.service
    fi
    if ${@bb.utils.contains('DISTRO_FEATURES', 'sysvinit', 'true', 'false', d)}; then
        install -D -m 0755 ${S}/az01-bootcount-reset.init \
            ${D}${sysconfdir}/init.d/az01-bootcount-reset
    fi
}
