SUMMARY = "Vendor-prebuilt AZ01 kernel, device trees, modules and firmware"
DESCRIPTION = "Stages inMusic's already-compiled 6.6.x-az01 PREEMPT_RT kernel \
image, its RK3288 device trees, the u-boot boot script, the full /lib/modules \
tree and the /lib/firmware blobs as opaque binary artifacts. PROVIDES \
virtual/kernel so that nothing in the build ever compiles a kernel: the device \
keeps running the vendor's own kernel and we replace only userspace."

# The kernel image and in-tree modules are GPL-2.0-only. modules/updates/
# additionally contains Paragon UFSD (ufsd.ko, jnl.ko), and lib/firmware holds
# Broadcom/Cypress/Ilitek blobs (see LICENCE.broadcom_bcm43xx) - all
# proprietary, redistributed here only as unmodified vendor binaries.
LICENSE = "GPL-2.0-only & Proprietary"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

# One recipe serves every az01-family board - premium, player, console and mpc.
# Which board is being built is a machine fact, so both of these come straight
# from conf/machine/<board>.conf rather than being listed here.
AZ0X_MACHINE_DIR = "${AZ0X_BOARD_DIR}"
AZ0X_KVER = "${AZ0X_BOARD_KVER}"

require az0x-prebuilt-kernel.inc

# Matches the shared "az01" MACHINEOVERRIDE set by az01-common.inc.
COMPATIBLE_MACHINE = "az01"

KERNEL_IMAGETYPE ?= "zImage"

# az01 boots the kernel and dtb OUT OF THE ROOTFS: the vendor boot.scr.uimg
# does "load ${devtype} ${devnum}:${distro_bootpart} ... ${prefix}zImage"
# against the rootfs partition. So these must land in the image.
az0x_install_boot() {
    install -d ${D}/boot
    install -m 0644 ${S}/boot/zImage-${KERNEL_VERSION} ${D}/boot/
    ln -sf zImage-${KERNEL_VERSION} ${D}/boot/${KERNEL_IMAGETYPE}
    install -m 0644 ${S}/boot/*.dtb ${D}/boot/
    # boot.scr.uimg now comes from the az01-bootscript recipe.
}

FILES:kernel-image = "/boot/zImage*"
RPROVIDES:kernel-image += "kernel-image-zimage"

# Make the kernel image and dtbs available in tmp/deploy/images/az01/ as well,
# so the AZ01 repack step can pick them up without digging into the rootfs.
do_deploy() {
    install -d ${DEPLOYDIR}
    install -m 0644 ${S}/boot/zImage-${KERNEL_VERSION} ${DEPLOYDIR}/
    ln -sf zImage-${KERNEL_VERSION} ${DEPLOYDIR}/${KERNEL_IMAGETYPE}
    install -m 0644 ${S}/boot/*.dtb ${DEPLOYDIR}/
    # Not every az01 board ships one: JC11S boots a signed FIT straight from
    # the kernel partition and has no boot script in the rootfs.
    if [ -f ${S}/boot/boot.scr.uimg ]; then
        install -m 0644 ${S}/boot/boot.scr.uimg ${DEPLOYDIR}/
    fi
}
addtask deploy after do_install before do_build
