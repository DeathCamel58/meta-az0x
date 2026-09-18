SUMMARY = "Vendor u-boot boot script for AZ01 devices"
DESCRIPTION = "Installs the vendor's boot.scr.uimg into /boot. \
\
This is load-bearing: the vendor u-boot scans the rootfs partition for \
/boot/boot.scr.uimg and that script is what loads the kernel and dtb. Without \
it the device does not boot, whichever kernel provider is selected. \
\
Split out of linux-az01-prebuilt for the same reason as az0x-firmware - it is \
not a kernel artifact, and a from-source kernel build does not produce one."

LICENSE = "Proprietary"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESPATH = "${AZ0X_VENDOR_DIR}/${AZ0X_BOARD_DIR}"
SRC_URI = "file://boot/boot.scr.uimg"
S = "${WORKDIR}"

PACKAGE_ARCH = "${MACHINE_ARCH}"
COMPATIBLE_MACHINE = "az01"

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
    install -d ${D}/boot
    install -m 0644 ${S}/boot/boot.scr.uimg ${D}/boot/
    chown -R root:root ${D}
}

FILES:${PN} = "/boot/boot.scr.uimg"

INSANE_SKIP:${PN} += "arch ldflags already-stripped buildpaths"

# Not every az01 board ships a boot script. JC11S (Prime 4 Plus) boots a signed
# FIT straight from its own kernel partition, so there is no boot.scr.uimg in
# the vendor tree at all. Skip the recipe rather than fail parsing for every
# machine: SRC_URI is checksummed at parse time, so a missing file is a hard
# parse error even when nothing installs this package.
python () {
    import os
    d_ = d.getVar('AZ0X_VENDOR_DIR') or ''
    board = d.getVar('AZ0X_BOARD_DIR') or ''
    if board and not os.path.exists(os.path.join(d_, board, 'boot', 'boot.scr.uimg')):
        raise bb.parse.SkipRecipe(
            "%s ships no boot/boot.scr.uimg; this board boots without one" % board)
}
