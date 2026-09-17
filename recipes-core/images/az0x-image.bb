SUMMARY = "Minimal de-branded AZ0x root filesystem"
DESCRIPTION = "A clean scarthgap userspace for the inMusic AZ0x families with \
no DJ application stack and no vendor telemetry. Boots to a systemd system on \
the vendor's own prebuilt kernel. This is the base you graft things onto - \
see az01-image-engine.bb for the feature-parity flavor. \
\
Builds for whichever MACHINE is selected: az01 (RK3288, armv7hf) or az04 \
(RK3588, aarch64). Read the az04 status section of the README before \
assuming an az04 build is flashable."

LICENSE = "MIT"

require az0x-image-base.inc

# Empty root password plus a serial and ssh login. This is a bring-up and
# modding image, not a production one - see the security note in
# ../../../README.md before you ship one of these anywhere.
IMAGE_FEATURES += "debug-tweaks ssh-server-openssh"

# Set, not appended: see the note on CORE_IMAGE_BASE_INSTALL in the .inc.
IMAGE_INSTALL = " \
    ${AZ0X_KERNEL_INSTALL} \
    ${AZ0X_BASE_INSTALL} \
    ${AZ0X_FIRMWARE_INSTALL} \
    ${AZ0X_DEBUG_INSTALL} \
    ${CORE_IMAGE_EXTRA_INSTALL} \
"

# Handy when you are poking at a device you just reflashed. Set
# AZ0X_DEBUG_INSTALL = "" in local.conf for a leaner image.
AZ0X_DEBUG_INSTALL ?= " \
    strace \
    file \
    less \
    nano \
    usbutils \
    i2c-tools \
    devmem2 \
"
