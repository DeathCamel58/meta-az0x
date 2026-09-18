SUMMARY = "Vendor firmware blobs for AZ0x devices"
DESCRIPTION = "The board-tuned firmware the device needs and upstream \
linux-firmware does not carry: Broadcom/Cypress brcmfmac NVRAM and BT \
patchram, Ilitek touch firmware, the eta5805 DSP blob, and on az04 the ARM \
Mali CSF firmware. \
\
This used to be a side-effect of linux-az01-prebuilt. It is its own recipe now \
because it has nothing to do with being a kernel provider: whether the kernel \
comes from the vendor or is built from source, the device needs these same \
blobs. Keeping them here means AZ0X_KERNEL_PROVIDER only ever changes who \
supplies zImage/dtb/modules, and a file conflict between the two providers is \
structurally impossible rather than something to remember not to do."

# Redistributed unmodified vendor binaries. No source, no upstream package.
LICENSE = "Proprietary"
LIC_FILES_CHKSUM:az01 = "file://lib/firmware/LICENCE.broadcom_bcm43xx;md5=3160c14df7228891b868060e1951dfbc"
LIC_FILES_CHKSUM:az04 = "file://lib/firmware/LICENCE.mali_csffw;md5=e064aaec4d21ef856e1b76a6f5dc435f"

# The signed-boot group ships no LICENCE file at all - no LICENCE.broadcom_bcm43xx
# and no cypress/ directory, unlike the other az01 boards. The blobs are still
# proprietary; with no licence text to point at, this anchors on a Broadcom
# firmware descriptor purely so the checksum pins provenance and a silent
# content change is caught.
LIC_FILES_CHKSUM:az0x-signed = "file://lib/firmware/brcm/brcmfmac43456-sdio.txt;md5=5951b99b32067c4ff43d22af705fde16"

# Same vendor-staging idiom as az0x-prebuilt-kernel.inc: resolve file:// against
# the per-board vendor dir rather than the usual recipe files/ dirs.
FILESPATH = "${AZ0X_VENDOR_DIR}/${AZ0X_BOARD_DIR}"
SRC_URI = "file://lib/firmware/"
S = "${WORKDIR}"

PACKAGE_ARCH = "${MACHINE_ARCH}"

# Foreign binaries: never strip, never split debug, never run ELF QA over them.
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_SYSROOT_STRIP = "1"
EXCLUDE_FROM_SHLIBS = "1"

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
    install -d ${D}${nonarch_base_libdir}/firmware
    cp -a ${S}/lib/firmware/. ${D}${nonarch_base_libdir}/firmware/

    # cp -a preserves the staged files' ownership, and the vendor tree came out
    # of a debugfs rdump which cannot chown - so everything carries uid 1000.
    # Packaging refuses any path whose uid has no target user ("uid not found:
    # 1000 ... may be due to host contamination"). Everything here is root:root.
    chown -R root:root ${D}
}

FILES:${PN} = "${nonarch_base_libdir}/firmware"

INSANE_SKIP:${PN} += "arch ldflags already-stripped buildpaths"
