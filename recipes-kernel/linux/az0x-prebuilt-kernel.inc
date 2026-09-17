# Shared machinery for the "reuse the vendor's compiled kernel" recipes.
#
#   linux-az01-prebuilt.bb   armv7hf  RK3288   zImage + dtbs in the rootfs
#   linux-az04-prebuilt.bb   aarch64  RK3588   Image + dtb in a signed FIT
#
# Both PROVIDE virtual/kernel, compile nothing, and package the vendor's
# artifacts verbatim. Each recipe sets AZ0X_MACHINE_DIR and AZ0X_KVER, then
# defines az0x_install_boot to place whatever that family keeps in /boot.

PROVIDES = "virtual/kernel"
PV = "${AZ0X_KVER}"

inherit deploy

# Pull the artifacts straight out of ../vendor/<machine>/. FILESPATH is
# overridden so the file:// fetcher resolves against the vendor staging dir
# rather than the usual recipe files/ dirs. lib/ carries modules and firmware.
FILESPATH = "${AZ0X_VENDOR_DIR}/${AZ0X_MACHINE_DIR}"
SRC_URI = "file://boot/ file://lib/"

S = "${WORKDIR}"

KERNEL_VERSION = "${AZ0X_KVER}"

# These are foreign binaries: never strip, never split debug, never run the
# usual ELF QA over them.
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"
INHIBIT_SYSROOT_STRIP = "1"
EXCLUDE_FROM_SHLIBS = "1"

# Nothing to configure, compile or run on the host.
do_configure[noexec] = "1"
do_compile[noexec] = "1"

PACKAGE_ARCH = "${MACHINE_ARCH}"

az0x_install_modules() {
    # Full module tree, verbatim. depmod has already been run by the vendor
    # and modules.dep uses relative paths, so it stays valid as-is. Do NOT
    # regenerate it - we have no matching System.map.
    install -d ${D}${nonarch_base_libdir}/modules
    cp -a ${S}/lib/modules/${KERNEL_VERSION} ${D}${nonarch_base_libdir}/modules/
}

# NOTE: /lib/firmware and /boot/boot.scr.uimg used to be installed here. They
# moved to the standalone az0x-firmware and az01-bootscript recipes, because
# neither is a kernel artifact and the device needs both regardless of which
# kernel provider is selected. See AZ0X_KERNEL_PROVIDER.

do_install() {
    az0x_install_boot
    az0x_install_modules

    # cp -a above preserves the OWNERSHIP of the staged vendor files. Those
    # come out of a debugfs rdump, which cannot chown, so they carry the
    # extracting user's uid (1000 here) rather than root. Yocto's packaging
    # then refuses every such path - "uid not found: 1000 ... may be due to
    # host contamination" - and do_package fails.
    #
    # Everything in a kernel package belongs to root:root; pseudo records this
    # faithfully without needing real privileges. Do it for the whole of ${D}
    # so it holds no matter how a board's az0x_install_boot copied things in.
    chown -R root:root ${D}
}

# --- kernel-module-* RPROVIDES ---------------------------------------------
# A real kernel recipe gets split by kernel-module-split.bbclass into one
# kernel-module-<name> package per .ko, and unrelated recipes RDEPEND on those
# names (systemd wants kernel-module-dm-mod, packagegroup-base wants various
# kernel-module-ieee80211-*, and so on). We never build a kernel, so none of
# those names would otherwise exist.
#
# The vendor module tree tells us exactly which modules this kernel has, both
# loadable (*.ko under kernel/ and updates/) and built-in (modules.builtin).
# Built-in counts: the functionality is present, just not as a separate object.
# So synthesise the same package names here and let them resolve honestly.
def az0x_module_rprovides(d):
    import os, glob
    mdir = os.path.join(d.getVar('AZ0X_VENDOR_DIR') or '',
                        d.getVar('AZ0X_MACHINE_DIR') or '',
                        'lib', 'modules', d.getVar('AZ0X_KVER') or '')
    names = set()

    builtin = os.path.join(mdir, 'modules.builtin')
    if os.path.exists(builtin):
        with open(builtin) as f:
            for line in f:
                line = line.strip()
                if line:
                    names.add(os.path.basename(line).split('.')[0])

    for ko in glob.glob(os.path.join(mdir, '**', '*.ko*'), recursive=True):
        names.add(os.path.basename(ko).split('.')[0])

    # kernel-module-split.bbclass normalises underscores to dashes.
    return ' '.join(sorted('kernel-module-' + n.replace('_', '-')
                           for n in names if n))

AZ0X_MODULE_RPROVIDES = "${@az0x_module_rprovides(d)}"

# The other half of what kernel.bbclass would have declared. Without this,
# any RRECOMMENDS on a kernel-module-* name that this kernel genuinely does
# not have is a hard "Nothing RPROVIDES" failure instead of a soft miss.
PACKAGES_DYNAMIC += "^kernel-module-.*"

PACKAGES = "kernel-image kernel-devicetree kernel-modules ${PN}-dbg ${PN}"

FILES:kernel-devicetree = "/boot/*.dtb /boot/*.dtbo /boot/*.fit"
FILES:kernel-modules = "${nonarch_base_libdir}/modules"

# Satisfy anything that runtime-depends on the names a real kernel recipe
# would have produced.
RPROVIDES:kernel-image = "kernel"
RPROVIDES:kernel-modules = "kernel-modules-${KERNEL_VERSION} ${AZ0X_MODULE_RPROVIDES}"
RDEPENDS:kernel-modules = "kmod"

ALLOW_EMPTY:${PN} = "1"

AZ0X_INSANE_SKIP = "arch ldflags already-stripped buildpaths"
INSANE_SKIP:kernel-image += "${AZ0X_INSANE_SKIP}"
INSANE_SKIP:kernel-devicetree += "${AZ0X_INSANE_SKIP}"
INSANE_SKIP:kernel-modules += "${AZ0X_INSANE_SKIP} debug-files staticdev"
