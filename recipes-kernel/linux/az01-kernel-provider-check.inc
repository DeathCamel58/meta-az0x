# Checks on AZ0X_KERNEL_PROVIDER, shared by every AZ01 kernel recipe
# (linux-az01-prebuilt, and linux-yocto / linux-yocto-rt through
# az01-kernel.inc). A kernel that would not boot is skipped with a reason, so
# building any image for the board stops with that reason in the
# "Nothing PROVIDES virtual/kernel" error rather than with an unbootable rootfs.
python () {
    if 'az01' not in (d.getVar('MACHINEOVERRIDES') or '').split(':'):
        return

    prov = d.getVar('AZ0X_KERNEL_PROVIDER') or 'yocto'
    valid = ('prebuilt', 'yocto', 'yocto-rt')
    if prov not in valid:
        raise bb.parse.SkipRecipe(
            "AZ0X_KERNEL_PROVIDER is '%s'; expected one of %s"
            % (prov, ', '.join(valid)))

    if prov == 'prebuilt':
        return

    # az0x-signed verifies a signed FIT from its kernel partition before
    # starting it. A from-source kernel is emitted as an unsigned FIT in the
    # vendor shape (az0x-signed-fit.inc) for an external tool to sign and
    # install; stock u-boot refuses it, and the failure looks like a dead
    # device with no console output. So it takes an explicit opt-in.
    if (d.getVar('MACHINE') == 'az0x-signed'
            and d.getVar('AZ0X_KERNEL_ALLOW_UNSIGNED') != '1'):
        raise bb.parse.SkipRecipe(
            "az0x-signed devices verify their kernel FIT (required = \"conf\", key "
            "denon-1), so AZ0X_KERNEL_PROVIDER='%s' produces a kernel stock "
            "u-boot refuses to start. Set AZ0X_KERNEL_ALLOW_UNSIGNED = \"1\" "
            "if you have a way to sign it or a bootloader that does not "
            "enforce the signature." % prov)
}
