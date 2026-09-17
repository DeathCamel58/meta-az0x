# Kernel provider selection for the AZ01 family.
#
# The device can run either the vendor's compiled kernel staged as an opaque
# prebuilt, or a kernel we build from source. This is a switch rather than a
# replacement: the prebuilt path is the one proven to boot a Prime Go, so it
# stays the default and stays bisectable against anything new.
#
#   AZ0X_KERNEL_PROVIDER = "prebuilt"   vendor binaries (default, proven)
#                        = "yocto"      linux-yocto 6.6, LINUX_KERNEL_TYPE=standard
#                        = "yocto-rt"   linux-yocto-rt 6.6, preempt-rt
#
# Building our own kernel is only possible at all because az01 u-boot loads
# zImage and the dtb FROM THE ROOTFS and calls bootz on a raw image - no FIT,
# no signature. (az04 cannot: signed FIT with required = "conf".)

AZ0X_KERNEL_PROVIDER ?= "prebuilt"

AZ0X_KERNEL_PN = "${@ {'prebuilt': 'linux-az01-prebuilt', \
                       'yocto':    'linux-yocto', \
                       'yocto-rt': 'linux-yocto-rt'}.get( \
                       d.getVar('AZ0X_KERNEL_PROVIDER') or 'prebuilt', '') }"

PREFERRED_PROVIDER_virtual/kernel = "${AZ0X_KERNEL_PN}"
KERNEL_IMAGETYPE = "zImage"

# An override token so recipes and the image base can say ":az01-ksrc" instead
# of threading inline python everywhere. OVERRIDES is in BB_BASEHASH_IGNORE_VARS
# so this costs nothing in sstate.
#
# ORDERING: this APPENDS (.=), it does not prepend. In BitBake the LAST matching
# override wins, and az01-common.inc already did `MACHINEOVERRIDES =. "az01:"`.
# Prepending here would put az01-ksrc *before* az01 and so make it lose, which
# is exactly what happened first time round: AZ0X_KERNEL_INSTALL:az01 kept
# beating :az01-ksrc and the image still asked for kernel-devicetree.
MACHINEOVERRIDES .= "${@':az01-kprebuilt' if (d.getVar('AZ0X_KERNEL_PROVIDER') or 'prebuilt') == 'prebuilt' else ':az01-ksrc'}"

# Device trees come prebuilt on the default path; the source path overrides this.
KERNEL_DEVICETREE = ""

# Modules are pulled in as a machine-essential RDEPENDS. Routed through a
# variable so the source-kernel milestones can drop them: a from-source
# multi_v7_defconfig builds far more modules than the vendor's 76, and the
# rootfs partition is a fixed 492 MiB. Serial console, eMMC and ext4 are all
# built in, so a modules-free rootfs boots fine.
AZ0X_ESSENTIAL_MODULES ?= "kernel-modules"
AZ0X_ESSENTIAL_MODULES:az01-ksrc = ""

# Milestone D1: source kernel, but keep the VENDOR device trees. This isolates
# "does mainline 6.6 boot this SoC" from "is our reconstructed DTS right" -
# two independent failure modes that must not be coupled on a board whose only
# feedback channel is a UART. Shipping all 8 vendor dtbs also means ${fdtfile}
# cannot miss. D2 flips these two lines to build the dtb from source.
AZ0X_DTB_INSTALL:az01-ksrc = "az01-vendor-devicetree"
