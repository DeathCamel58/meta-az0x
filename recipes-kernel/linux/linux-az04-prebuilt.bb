SUMMARY = "Vendor-prebuilt AZ04 kernel, device tree, modules and firmware"
DESCRIPTION = "Stages inMusic's already-compiled 6.18.x-imb PREEMPT_RT aarch64 \
kernel, its RK3588 device tree and the full /lib/modules and /lib/firmware \
trees as opaque binary artifacts. PROVIDES virtual/kernel so nothing in the \
build ever compiles a kernel. \
\
Unlike az01, an az04 device does NOT boot out of the rootfs: the kernel, the \
dtb, the cmdline overlay and an initramfs live in a signed FIT image \
(kernel.fit) on a separate 'boot' partition. The Image and dtb packaged here \
are therefore reference/repack inputs, not something the device loads from \
the filesystem - which is why the vendor's own rootfs ships an empty /boot."

# GPL-2.0-only kernel and in-tree modules; lib/firmware carries the ARM Mali
# CSF firmware (see LICENCE.mali_csffw) and Broadcom blobs - proprietary,
# redistributed here only as unmodified vendor binaries.
LICENSE = "GPL-2.0-only & Proprietary"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

AZ0X_MACHINE_DIR = "${AZ0X_BOARD_DIR}"
AZ0X_KVER = "${AZ0X_BOARD_KVER}"

require az0x-prebuilt-kernel.inc

COMPATIBLE_MACHINE = "az04"

# aarch64: a flat "Image", not a zImage. (meta-rockchip's rk3588s.inc would
# default this to fitImage, which only means anything to a real kernel build.)
KERNEL_IMAGETYPE ?= "Image"

# Staged into /boot for reference and so the repack step has them in one
# place. The device ignores /boot entirely - see the DESCRIPTION above.
az0x_install_boot() {
    install -d ${D}/boot
    install -m 0644 ${S}/boot/Image-${KERNEL_VERSION} ${D}/boot/
    ln -sf Image-${KERNEL_VERSION} ${D}/boot/${KERNEL_IMAGETYPE}
    install -m 0644 ${S}/boot/rk3588-az04-*.dtb ${D}/boot/
}

FILES:kernel-image = "/boot/Image*"
RPROVIDES:kernel-image += "kernel-image-image"

# The signed FIT and the extracted cmdline overlay go to the deploy dir only,
# never into the rootfs: kernel.fit is 26 MiB and belongs to the boot
# partition, and shipping it inside the filesystem would just waste space.
do_deploy() {
    install -d ${DEPLOYDIR}
    install -m 0644 ${S}/boot/Image-${KERNEL_VERSION} ${DEPLOYDIR}/
    ln -sf Image-${KERNEL_VERSION} ${DEPLOYDIR}/${KERNEL_IMAGETYPE}
    install -m 0644 ${S}/boot/rk3588-az04-*.dtb ${DEPLOYDIR}/
    install -m 0644 ${S}/boot/kernel.fit ${DEPLOYDIR}/
    install -m 0644 ${S}/boot/fdt-cmdline.dtb ${DEPLOYDIR}/
}
addtask deploy after do_install before do_build
