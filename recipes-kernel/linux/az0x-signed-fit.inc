# From-source kernel FIT for the signed-boot machine (az0x-signed).
#
# The signed-boot ARM controllers (Prime 4+, JC11S and siblings) do not boot
# zImage out of the rootfs. Their u-boot loads a FIT image called kernel.fit
# from the dedicated ext2 `kernel` partition and verifies it against key
# denon-1 before starting. On the vendor path that FIT is the vendor's own
# signed blob and there is nothing to build. On the from-source path this
# include assembles an equivalent FIT from our kernel, our board device tree
# and a one-node command-line overlay.
#
# Shape. The vendor FIT has three image nodes, and both the vendor u-boot
# (bootm 0x800800#conf-${board}#cmdline) and any tooling that repacks the FIT
# address them by name:
#
#   /images/kernel              the kernel, load/entry 0x02000000
#   /images/fdt-<board>         the board device tree, load 0x01f00000
#   /images/fdt-cmdline         a /chosen bootargs overlay, load 0x01fc2000
#
# and two configurations:
#
#   /configurations/conf-<board>   default: kernel + fdt-<board>
#   /configurations/cmdline        fdt only: fdt-cmdline
#
# so the command line (root=PARTLABEL=denonlinux) can be replaced without
# disturbing the kernel/device-tree nodes. This include reproduces exactly
# that layout; the node names are deliberate and must not be "tidied" into
# the kernel-fitimage.bbclass naming (fdt-<file>.dtb / conf-<file>.dtb),
# which u-boot's bootcmd would not find.
#
# Not signed here. Signing the FIT and writing it to the `kernel` partition
# is left to an external boot-activation tool. This layer only emits the
# shaped, unsigned FIT plus its inputs so that step has something to work
# from.
#
# Costs nothing on the prebuilt path or on any other machine: the containing
# recipes are only built when AZ0X_KERNEL_PROVIDER selects a from-source
# kernel, and everything below is gated on the az0x-signed override.
#
# See docs/az0x-signed-fit.md for the design note and how to validate a full
# build.

FILESEXTRAPATHS:prepend:az0x-signed := "${THISDIR}/az0x-signed-fit-files:"

# Off unless the build is both az0x-signed and a from-source kernel. The
# recipes that require this include (linux-yocto, linux-yocto-rt) are only
# ever built when AZ0X_KERNEL_PROVIDER names them, so the provider half is
# already implied by being here at all.
AZ0X_FIT_ENABLE ?= "0"
AZ0X_FIT_ENABLE:az0x-signed = "1"

# Base name of the emitted FIT, in DEPLOY_DIR_IMAGE. u-boot loads kernel.fit
# from the `kernel` partition.
AZ0X_FIT_NAME ?= "kernel.fit"

# The single line that redirects the system onto our USB rootfs. The GPT
# partition name and the ext4 label must both be `denonlinux` for
# root=PARTLABEL to resolve. Overridable so the label can be retargeted
# without editing this file.
AZ0X_FIT_BOOTARGS ?= "root=PARTLABEL=denonlinux rootwait rw"

# Load addresses, copied from the vendor FIT so the boot path is unchanged.
AZ0X_FIT_KERNEL_LOADADDRESS ?= "0x02000000"
AZ0X_FIT_KERNEL_ENTRYPOINT ?= "0x02000000"
AZ0X_FIT_FDT_LOADADDRESS ?= "0x01f00000"
AZ0X_FIT_CMDLINE_LOADADDRESS ?= "0x01fc2000"

# The checked-in templates this task fills in.
SRC_URI:append:az0x-signed = " \
    file://kernel.its.in \
    file://fdt-cmdline.dts.in \
"

# mkimage builds the FIT; dtc compiles the command-line overlay.
DEPENDS:append:az0x-signed = " u-boot-tools-native dtc-native"

# Assemble and deploy in do_deploy: kernel.bbclass and kernel-devicetree
# have, by the time do_deploy:append runs, put the kernel image (as the
# ${KERNEL_IMAGETYPE} symlink) and the board dtb (as <AZ0X_BOARD_DTB>.dtb)
# into DEPLOYDIR. No separate task and no guessing at paths in ${B}.
do_deploy:append:az0x-signed() {
    if [ "${AZ0X_FIT_ENABLE}" != "1" ]; then
        bbnote "az0x from-source FIT disabled (AZ0X_FIT_ENABLE=${AZ0X_FIT_ENABLE}); skipping"
        return 0
    fi

    kimage="${DEPLOYDIR}/${KERNEL_IMAGETYPE}"
    dtb="${DEPLOYDIR}/${AZ0X_BOARD_DTB}.dtb"

    [ -e "$kimage" ] || bbfatal "cannot build az0x FIT: kernel image $kimage is missing"
    [ -e "$dtb" ]    || bbfatal "cannot build az0x FIT: device tree $dtb is missing"

    fitwork="${WORKDIR}/az0x-fit"
    rm -rf "$fitwork"
    install -d "$fitwork"

    # 1. The command-line overlay: a one-fragment /chosen/bootargs dtbo.
    sed -e "s|@@BOOTARGS@@|${AZ0X_FIT_BOOTARGS}|g" \
        "${WORKDIR}/fdt-cmdline.dts.in" > "$fitwork/fdt-cmdline.dts"
    dtc -@ -I dts -O dtb "$fitwork/fdt-cmdline.dts" -o "$fitwork/fdt-cmdline.dtbo"

    # 2. The image description, with the boot-tool-visible node names above.
    sed -e "s|@@BOARD_DTB@@|${AZ0X_BOARD_DTB}|g" \
        -e "s|@@KERNEL_IMAGE@@|$kimage|g" \
        -e "s|@@BOARD_DTB_FILE@@|$dtb|g" \
        -e "s|@@CMDLINE_FILE@@|$fitwork/fdt-cmdline.dtbo|g" \
        -e "s|@@KERNEL_LOADADDRESS@@|${AZ0X_FIT_KERNEL_LOADADDRESS}|g" \
        -e "s|@@KERNEL_ENTRYPOINT@@|${AZ0X_FIT_KERNEL_ENTRYPOINT}|g" \
        -e "s|@@FDT_LOADADDRESS@@|${AZ0X_FIT_FDT_LOADADDRESS}|g" \
        -e "s|@@CMDLINE_LOADADDRESS@@|${AZ0X_FIT_CMDLINE_LOADADDRESS}|g" \
        "${WORKDIR}/kernel.its.in" > "$fitwork/kernel.its"

    mkimage -f "$fitwork/kernel.its" "${DEPLOYDIR}/${AZ0X_FIT_NAME}"

    # 3. Ship the inputs too: a signing tool may want the raw overlay rather
    #    than a whole FIT, and the .its is useful evidence of what was built.
    install -m 0644 "$fitwork/fdt-cmdline.dtbo" "${DEPLOYDIR}/fdt-cmdline.dtbo"
    install -m 0644 "$fitwork/kernel.its" "${DEPLOYDIR}/kernel.its"

    bbnote "az0x signed FIT: ${DEPLOYDIR}/${AZ0X_FIT_NAME}"
}
