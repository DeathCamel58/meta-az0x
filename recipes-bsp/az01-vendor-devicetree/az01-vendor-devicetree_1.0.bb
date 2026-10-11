SUMMARY = "Vendor device trees for AZ01 boards"
DESCRIPTION = "Installs the vendor's prebuilt *.dtb into /boot. \
\
Used when the kernel is built from source but we are not yet confident in our \
own reconstructed DTS. The vendor u-boot does `load ... ${fdtfile} || exit`, \
so a missing or wrong dtb name is a silent non-boot; shipping the vendor set \
means whatever name u-boot asks for is present. \
\
This conflicts on /boot/*.dtb with both kernel providers' own kernel-devicetree \
package, so exactly one of the three must be installed - see AZ0X_DTB_INSTALL."

LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

FILESPATH = "${AZ0X_VENDOR_DIR}/${AZ0X_BOARD_DIR}"
SRC_URI = "file://boot/"
S = "${WORKDIR}"

PACKAGE_ARCH = "${MACHINE_ARCH}"
COMPATIBLE_MACHINE = "az01"

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
    install -d ${D}/boot
    install -m 0644 ${S}/boot/*.dtb ${D}/boot/
    chown -R root:root ${D}
}

FILES:${PN} = "/boot/*.dtb"

INSANE_SKIP:${PN} += "arch ldflags already-stripped buildpaths"

# Vendor files are staged by hand (see README, "Vendor files"). Without them
# the file:// SRC_URI fails checksumming at parse time, which would stop the
# parse for every machine. Skip the recipe with a pointer instead.
python () {
    import os
    path = os.path.join(d.getVar('AZ0X_VENDOR_DIR') or '',
                        d.getVar('AZ0X_BOARD_DIR') or '', 'boot')
    if not (d.getVar('AZ0X_BOARD_DIR') or '') or not os.path.exists(path):
        raise bb.parse.SkipRecipe(
            "vendor files not staged: %s is missing (see the meta-az0x README, "
            "\"Vendor files\")" % path)
}
