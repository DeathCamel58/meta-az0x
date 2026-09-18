SUMMARY = "Xorg panel rotation and matching touch transform for AZ0x boards"
DESCRIPTION = "Generates an xorg.conf.d snippet at boot that rotates the \
display and applies the matching touchscreen transformation matrix, reading \
the rotation from the device tree instead of baking it in per machine. \
\
Every AZ0x dtb carries inmusic,panel-rotation at the root, and the same value \
as `rotation` in the panel node. Deriving it at runtime means one rootfs \
serves every board - which is what the vendor does, and what the signed-boot \
devices require, since a single firmware image covers six products from one \
filesystem. It also removes the need for a machine per panel orientation. \
\
Rotation MUST be set at server startup, not at runtime. Applying \
`xrandr --rotate` to a live connector wedges the RK3288 display pipeline: the \
MIPI-DSI command FIFO fails mid-modeset and the VOP never recovers \
(flip_done timed out / commit wait timed out), needing a reboot. Observed on \
jp11. Hence a config file written before X starts, not a session script. \
\
The rootfs is read-only and /etc cannot be written at boot, so the snippet \
lives in /run and xorg.conf.d holds a symlink to it. A tmpfiles entry creates \
the target first, so a failed generator leaves X an empty config rather than a \
dangling symlink. \
\
The touch matrix must match the rotation or taps land in the wrong place. \
libinput is used rather than evdev because evdev has no TransformationMatrix \
option."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit features_check systemd
REQUIRED_DISTRO_FEATURES = "x11"

SRC_URI = "file://az0x-panel-rotation.sh \
           file://az0x-panel-rotation.service \
           file://az0x-panel-rotation.tmpfiles"
S = "${WORKDIR}"

# Which connector to rotate. Every AZ0x panel is on DSI-1; overridable per
# machine if that ever stops being true.
AZ0X_XORG_OUTPUT ?= "DSI-1"

SYSTEMD_SERVICE:${PN} = "az0x-panel-rotation.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install() {
    install -d ${D}${libexecdir}
    install -m 0755 ${S}/az0x-panel-rotation.sh ${D}${libexecdir}/az0x-panel-rotation

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/az0x-panel-rotation.service ${D}${systemd_system_unitdir}/

    install -d ${D}${nonarch_libdir}/tmpfiles.d
    install -m 0644 ${S}/az0x-panel-rotation.tmpfiles \
        ${D}${nonarch_libdir}/tmpfiles.d/az0x-panel-rotation.conf

    # xorg.conf.d is read-only at runtime; point it at the generated file.
    install -d ${D}${sysconfdir}/X11/xorg.conf.d
    ln -sf /run/az0x/10-panel-rotation.conf \
        ${D}${sysconfdir}/X11/xorg.conf.d/10-panel-rotation.conf

    if [ -n "${AZ0X_XORG_OUTPUT}" ]; then
        install -d ${D}${sysconfdir}/default
        echo "AZ0X_XORG_OUTPUT=${AZ0X_XORG_OUTPUT}" \
            > ${D}${sysconfdir}/default/az0x-panel-rotation
    fi
}

FILES:${PN} = "${libexecdir}/az0x-panel-rotation \
               ${systemd_system_unitdir} \
               ${nonarch_libdir}/tmpfiles.d \
               ${sysconfdir}/X11/xorg.conf.d \
               ${sysconfdir}/default/az0x-panel-rotation"

RDEPENDS:${PN} = "xf86-input-libinput"
