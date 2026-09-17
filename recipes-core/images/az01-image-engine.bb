SUMMARY = "AZ01 root filesystem with the vendor Engine application stack"
DESCRIPTION = "Feature-parity flavor (b): the minimal az01 base plus the \
vendor /usr/Engine tree, its systemd units, the LD_PRELOAD shims and the \
Qt/EGLFS runtime. NOT WIRED UP YET - the graft recipe it depends on is still \
a template. See meta-az0x/recipes-az0x/engine/README.md."

LICENSE = "MIT"

require az0x-image-base.inc

IMAGE_FEATURES += "debug-tweaks ssh-server-openssh"

IMAGE_INSTALL = " \
    ${AZ0X_KERNEL_INSTALL} \
    ${AZ0X_BASE_INSTALL} \
    ${AZ0X_FIRMWARE_INSTALL} \
    ${AZ0X_BOOTSCRIPT_INSTALL} \
    ${AZ0X_DTB_INSTALL} \
    ${AZ01_ENGINE_INSTALL} \
    ${CORE_IMAGE_EXTRA_INSTALL} \
"

# The one line that turns flavor (a) into flavor (b).
AZ01_ENGINE_INSTALL ?= "az01-engine-graft"


# Guard rail: this recipe parses fine but is skipped until the graft recipe
# exists, so you get an explanation instead of a missing-provider error.
python () {
    import os
    graft = os.path.join(d.getVar('AZ0X_LAYERDIR') or '', 'recipes-az0x',
                         'engine', 'az01-engine-graft.bb')
    if not os.path.exists(graft):
        raise bb.parse.SkipRecipe(
            "image flavor (b) is not enabled: copy meta-az0x/recipes-az0x/"
            "engine/az01-engine-graft.bb.sample to az01-engine-graft.bb and "
            "fill it in first")
}

# The grafted Engine tree is armv7hf JP13 material, so this flavor is az01-only.
# An az04 equivalent would need its own graft from an aarch64 vendor rootfs.
COMPATIBLE_MACHINE = "az01-premium"
