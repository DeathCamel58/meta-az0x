# Build linux-yocto-rt for the AZ01 boards. Inert unless AZ0X_KERNEL_PROVIDER
# selects it (the recipe SkipRecipes unless PREFERRED_PROVIDER names it).
#
# The RT recipe has its own filename, so the plain linux-yocto_%.bbappend does
# not apply to it - bitbake requires the bbappend prefix to match, and
# "linux-yocto_%.bb".startswith("linux-yocto-r") is False. Without this file a
# AZ0X_KERNEL_PROVIDER="yocto-rt" build would get neither the AZ01 defconfig,
# config fragment and device-tree/driver patches from az01-kernel.inc, nor the
# COMPATIBLE_MACHINE entries that let it build for our boards.
require az01-kernel.inc
