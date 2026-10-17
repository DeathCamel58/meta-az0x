# Build linux-yocto for the AZ01 boards. Inert unless AZ0X_KERNEL_PROVIDER
# selects it (the recipe SkipRecipes unless PREFERRED_PROVIDER names it).
#
# Note this bbappend's prefix "linux-yocto_" does NOT match linux-yocto-rt_6.18.bb
# - bitbake requires the bbappend prefix to match, and
# "linux-yocto_%.bb".startswith("linux-yocto-r") is False. The RT recipe gets
# its own bbappend; see linux-yocto-rt_%.bbappend.
require az01-kernel.inc
