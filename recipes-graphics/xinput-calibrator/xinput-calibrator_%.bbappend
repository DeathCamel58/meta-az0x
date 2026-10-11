# Stop xinput_calibrator from running itself at every X startup.
#
# az0x-panel-rotation already derives the touch transform from the device tree
# (inmusic,panel-rotation) and writes it as a libinput TransformationMatrix in
# an xorg.conf.d InputClass. That is the correct and complete answer for these
# boards: the panels are capacitive, report absolute coordinates over their full
# native range, and need no per-unit calibration - only the rotation, which the
# dtb states exactly.
#
# The stock recipe installs two autostart hooks:
#
#   ${sysconfdir}/X11/Xsession.d/30xinput_calibrate.sh
#   ${sysconfdir}/xdg/autostart/xinput_calibrator.desktop
#
# Both fire because /etc/formfactor/config sets HAVE_TOUCHSCREEN=1 and no
# /etc/pointercal.xinput is shipped (pointercal-xinput installs nothing unless a
# board provides calibration data). The calibrator then samples touches in the
# space xorg.conf has *already* rotated, and writes the result into the device's
# "libinput Calibration Matrix" property - which is a second, independent
# transform. The two compose, and the product is garbage: measured on jp11 over
# two boots as (-39.86 -4.24 10.72 / 8.54 -41.95 4.82) and
# (7.17 33.08 -6.57 / -36.13 -0.20 11.22), different every time because it
# depends on where the user tapped. Symptom is touch that works during the
# calibration screen (run before the matrix is applied) and is dead everywhere
# after it. Seen identically on jp11 and jp13.
#
# The package itself stays - it is a hard RDEPENDS of
# packagegroup-core-x11-utils, and the binary is still useful by hand. Only the
# two autostart hooks go.
# Only on this layer's machines: a BSP layer must not change other machines'
# builds just by being added.
az0x_drop_calibrator_autostart() {
    rm -f ${D}${sysconfdir}/X11/Xsession.d/30xinput_calibrate.sh
    rm -f ${D}${sysconfdir}/xdg/autostart/xinput_calibrator.desktop
}
do_install:append:az01() {
    az0x_drop_calibrator_autostart
}
do_install:append:az04() {
    az0x_drop_calibrator_autostart
}
