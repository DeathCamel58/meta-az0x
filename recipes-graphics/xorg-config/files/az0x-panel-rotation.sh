#!/bin/sh
# Generate the Xorg rotation snippet from the device tree, at boot.
#
# The rotation is a board fact the kernel already has: every AZ0x dtb carries
# inmusic,panel-rotation at the root (and the same value as `rotation` in the
# panel node). Reading it here rather than baking it in at build time means one
# rootfs serves every board - which is what the vendor does, and is required for
# the signed-boot devices where a single image covers six products.
#
# Rotation MUST be applied at X startup, not at runtime: xrandr --rotate on a
# live connector wedges the RK3288 pipeline (MIPI-DSI command FIFO fails
# mid-modeset, VOP never recovers). Hence a config file, written before X runs.
set -u

DT=/sys/firmware/devicetree/base
OUT=/run/az0x/10-panel-rotation.conf
OUTPUT="${AZ0X_XORG_OUTPUT:-DSI-1}"

read_rot() {
    for f in "$DT/inmusic,panel-rotation" "$DT"/mipi@*/panel@0/rotation; do
        [ -f "$f" ] || continue
        # busybox od only does single-letter flags, so match the octal bytes
        # rather than trying to parse a big-endian u32 in shell arithmetic.
        set -- $(od -b "$f" 2>/dev/null | head -n1)
        [ $# -ge 5 ] || continue
        echo "$2 $3 $4 $5"
        return 0
    done
    return 1
}

BYTES="$(read_rot || true)"
case "$BYTES" in
    "000 000 000 000") ROTATE=normal   ; MATRIX="1 0 0 0 1 0 0 0 1"   ; DEG=0   ;;
    "000 000 000 132") ROTATE=right    ; MATRIX="0 1 0 -1 0 1 0 0 1"  ; DEG=90  ;;
    "000 000 000 264") ROTATE=inverted ; MATRIX="-1 0 1 0 -1 1 0 0 1" ; DEG=180 ;;
    "000 000 001 016") ROTATE=left     ; MATRIX="0 -1 1 1 0 0 0 0 1"  ; DEG=270 ;;
    *)                 ROTATE=normal   ; MATRIX="1 0 0 0 1 0 0 0 1"   ; DEG=unknown ;;
esac

mkdir -p "$(dirname "$OUT")"
cat > "$OUT" <<XORGEOF
# Generated at boot by az0x-panel-rotation from the device tree.
# inmusic,panel-rotation = $DEG degrees (raw bytes: ${BYTES:-none})
#
# The kernel turns the dtb rotation into a DRM panel-orientation property,
# which Wayland compositors honour on their own. Xorg's modesetting driver
# ignores it, so X has to be told explicitly.

Section "Device"
    Identifier "RK3288 VOP"
    Driver "modesetting"
    Option "monitor-$OUTPUT" "$OUTPUT"
EndSection

Section "Monitor"
    Identifier "$OUTPUT"
    Option "Rotate" "$ROTATE"
EndSection

Section "InputClass"
    Identifier "AZ0x touchscreen rotation"
    MatchIsTouchscreen "on"
    Driver "libinput"
    Option "TransformationMatrix" "$MATRIX"
EndSection
XORGEOF

echo "az0x-panel-rotation: ${DEG} degrees -> Rotate \"$ROTATE\""
