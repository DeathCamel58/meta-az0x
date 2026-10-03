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
# Building our own kernel is possible on the az01 boards because u-boot loads
# zImage and the dtb FROM THE ROOTFS and calls bootz on a raw image - no FIT,
# no signature. The az0x-signed board (JC11S and siblings) is the exception:
# it boots a signed FIT, so a from-source kernel is emitted as an unsigned FIT
# in the vendor shape (see az0x-signed-fit.inc) for an external tool to sign
# and install, and still needs AZ0X_KERNEL_ALLOW_UNSIGNED="1". (az04 cannot:
# the same signed-FIT enforcement, and this layer emits no FIT for it.)

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

# Modules are pulled in as a machine-essential RDEPENDS.
#
# The from-source path used to drop them, because inheriting
# multi_v7_defconfig produced ~595 modules against a fixed 492 MiB rootfs.
# That worked, but it meant any symbol left at =m was silently absent, which
# caused six separate bugs - DRM_ROCKCHIP, PWM_ROCKCHIP, CFG80211/BRCMFMAC, the
# whole sound stack, USB_EHCI_HCD_PLATFORM and the USB control surface.
#
# We now carry our own defconfig instead of the generic multi-SoC one, so the
# module set is ours to control, and =m means what it says again.
AZ0X_ESSENTIAL_MODULES ?= "kernel-modules"

# D1 kept the VENDOR device trees, to isolate "does mainline 6.6 boot this SoC"
# from "is our reconstructed DTS right". That worked, so D2 now builds the dtb
# from source: KERNEL_DEVICETREE names it, the kernel's own kernel-devicetree
# package ships it, and az01-vendor-devicetree steps aside.
#
# The name is not a guess - the unit's u-boot reports fdtfile=rk3288-az01-jp11.dtb,
# and boot.scr does `load ... ${fdtfile} || exit`, so a wrong name is a silent
# non-boot. Set AZ0X_DTB_INSTALL back to "az01-vendor-devicetree" (and
# KERNEL_DEVICETREE to "") to fall back to the vendor blobs.
AZ0X_DTB_INSTALL:az01-ksrc = ""
KERNEL_DEVICETREE:az01-ksrc = "rockchip/${AZ0X_BOARD_DTB}.dtb"
