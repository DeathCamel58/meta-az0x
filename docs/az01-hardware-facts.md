# AZ01 (JP11 / Denon DJ Prime GO) observed hardware facts

Measured on the device itself, running the **vendor kernel 6.6.119** under our
own Yocto userspace (`AZ0X_KERNEL_PROVIDER = "prebuilt"` plus `libdrm-tests
evtest i2c-tools`). Nothing here comes from disassembly - it is all observed
behaviour of hardware, which is why it is the regression oracle for every
driver we write.

Flashing stock vendor firmware turned out to be unnecessary: the prebuilt
kernel already runs every proprietary driver, and our userspace gives a shell.

## u-boot patches the device tree -- IMPORTANT

The DTB in the rootfs is a template. u-boot edits it before handing it to the
kernel. Comparing all 59 `status` properties, live vs shipped, it changes
exactly four:

| node | shipped DTB | live |
|---|---|---|
| `/i2c@ff160000/ili2116@41` | `okay` | **`disabled`** |
| `/i2c@ff160000/ili2117@26` | `disabled` | **`okay`** |
| `/reserved-memory/framebuffer` | `disabled` | **`okay`** |
| `/usb@ff580000` | `disabled` | **`okay`** |

It also rewrites exactly one `compatible`, which matters more than all four:

| node | shipped DTB | live |
|---|---|---|
| `/mipi@ff960000/panel@0` | `urt,umo-p076md-t` | **`urt,umoh-9522md-t`** |

Verified by diffing all 137 compatible properties of the live tree against the
template; that is the only one that differs. No vendor DTB in the family
declares `umoh-9522md-t` at all - 22 boards say `p076md-t` and 9 say
`p080md-t` - so the template records design intent and u-boot records what is
physically fitted. Writing a panel driver against the template compatible
would have produced a driver for hardware this unit does not have.

So a device tree we write ourselves must apply these too, or touch and one USB
controller will be dead and the bootloader framebuffer handover will not
happen. The touch swap is presumably u-boot probing which controller is fitted;
this unit has the **ILI2117**, not the ILI2116 the shipped DTB enables.

**These values are JP11's.** The fixups are a family-wide mechanism but the
nodes differ per board: JP13 swaps `ili2110@41` for `ili2118@26` rather than
2116/2117, and also flips `/i2c@ff140000/usb5537@2d` to `okay` — the USB hub
that carries its user ports, internal drive bay and control surface. Read the
live tree on the board in front of you rather than assuming this table.

The `/reserved-memory/framebuffer` row is load-bearing and currently does not
take effect: u-boot fills in its `reg` and enables it, but `/reserved-memory`
declares `#address-cells`/`#size-cells` of 1 against a root of 2, so the kernel
rejects the whole node (`OF: fdt: Reserved memory: unsupported node format`)
and CMA is then placed over the region u-boot reserved. Fixing the cells would
break u-boot's fixup, which writes an 8-byte `reg` assuming 1/1.

## Display -- the complete mode (this is most of the panel work)

    modetest -c  ->  DSI-1 connected, 94x151 mm
    #0 800x1280 60.06  800 877 901 925  1280 1292 1296 1314  73000  preferred

    hdisplay 800   hsync_start 877   hsync_end 901   htotal 925
    vdisplay 1280  vsync_start 1292  vsync_end 1296  vtotal 1314
    clock 73000 kHz   refresh 60.06 Hz

`panel orientation` property reads `Left Side Up` (2), matching the DTS
`rotation = <0x10e>` (270) and our `AZ0X_XORG_ROTATE = "left"`.

The vendor driver that binds is `panel-urt-umoh-9522md-t`, against the DTS
compatible `urt,umo-p076md-t` - so one vendor driver covers several URT panels.
The VOP is already running at handover: `rockchip-vop ff940000.vop: enabled by
bootloader`.

Still missing for a mainline panel driver: the DSI init payload sequence. The
timings above no longer need to be recovered.

## Touch -- solved by mainline, no driver work needed

    ili2117 4-0026: ILI2117 vendor 0x00 firmware version 5.0
    input: ILI2117 Touchscreen as .../ff160000.i2c/i2c-4/4-0026/input/input0

Mainline `drivers/input/touchscreen/ili210x.c` already matches
`ilitek,ili2117` (`ili211x_chip`, with its own read/parse/polling ops). So this
needs `CONFIG_TOUCHSCREEN_ILI210X=y` and the DT node at 0x26 enabled - not a
reimplementation.

Note this corrects the earlier plan, which expected `ilitek_ts_i2c.c`; that
driver's match table has no 2117 (nor 2116).

## Power -- fully characterised

    DC jack IN :  online=1  voltage_now=18640000  in_voltage0_raw=663
    DC jack OUT:  online=0  voltage_now=15824000  in_voltage0_raw=561 (10 samples: 560-562)
                  battery status flips to Discharging

Channel 0 is the DC-jack sense (`io-channels = <&saradc 0>`). The conversion
is a 10-bit SARADC on a 1.8 V reference behind a 16:1 divider:

    V_mV = raw * 1800 * 16 / 1024 = raw * 28.125

    raw 663 -> 18647 mV   (driver reports 18640, 0.04% off)
    raw 561 -> 15778 mV   (driver reports 15824)

**The unplugged reading is not a constant** - it is the battery terminal
voltage, so it drifts with charge state. Do not take the midpoint of one
in/out pair as the threshold without checking it against a *full* battery.

The pack is 4S Li-ion (14.4 V nominal, so 16.8 V fully charged); the adapter
sits at ~18.6 V. The threshold therefore has to sit between 16800 mV and
18647 mV. Use **17500 mV** (raw 622): 700 mV of headroom over a full pack and
1150 mV below the adapter. The measured midpoint (raw 612, 17212 mV) also
falls in the valid band but leaves only 400 mV over a full battery.

Write this into our DTS explicitly as `inmusic,online-threshold-mv = <17500>`
rather than depending on whatever constant the vendor binary compiled in.

## Battery -- also solved by mainline

    /sys/class/power_supply/sbs-3-000b: JMS ACVB-BF, Li-ion, 14.4 V design,
    2600 mAh design / 2390 mAh full, 29 cycles

A standard SBS battery on i2c3 at 0x0b, handled by mainline `sbs-battery`.
Needs `CONFIG_BATTERY_SBS=y`. On our from-source kernel this currently shows as
`i2c 3-000b: deferred probe pending`.

## Audio -- one fixed rate on this board

    card 1: JP11, device 0: JP11 PCM inmusic,jp11-audio-codec-0
    aplay --dump-hw-params:  RATE: 44100   (single value)
                             FORMAT: S16_LE S24_LE S32_LE   CHANNELS: [2 4]

Playback at 48000, 88200 and 96000 is rejected outright. During 44100 playback
both clock-select lines stay low:

    gpio-59 (GPIO2_A3 | clksel0) out lo
    gpio-60 (GPIO2_A4 | clksel1) out lo

One rate, one clksel state - **on this board**. An earlier version of this
section concluded from that the clksel encoding table "does not exist", which is
wrong: it exists and is shared by every AZ0x board, and decompiling the vendor's
machine driver recovered it in full.

    rate    code   clksel1  clksel0
    44100    0        0        0
    48000    1        0        1
    88200    2        1        0
    96000    3        1        1

What is board-specific is the *rate mask*, which the vendor keeps in a table
indexed by compatible. JP11's mask contains only 44100, which is why measuring
this board alone made it look as though no table existed - a reminder that a
measurement on one board does not license a claim about the platform.

Other GPIOs on the codec path:

    gpio-56 (nCODEC_RESET | reset) out hi ACTIVE LOW
    gpio-58 (MUTE        | mute ) out hi

The codec driver is `jp07-codec` - shared with the JP07 board - and its only
visible behaviour is bias-level transitions with a mute flag, consistent with
the analysis that it is a GPIO+clock stub rather than a register-programmed
codec.

DT topology, for reference:

    /sound        inmusic,jp11-audio        clksel0-gpios=<&gpio2 3 0> clksel1-gpios=<&gpio2 4 0>
    /sound-codec  inmusic,jp11-audio-codec  enable-gpios=<..1 0> mute-gpios=<..2 0> reset-gpios=<..0 1>

## Boot counter -- warm reboots end in fastboot unless the OS clears it

The vendor u-boot keeps a boot counter in the PMU syscon (`0xff730000`,
offset `0xa0`) in u-boot's `bootcount_syscon` format: magic `0xb001` in the
upper 16 bits, the count in the lower 16. It increments the count on every
boot. A power cycle clears the register; a warm reset does not. After a few
consecutive warm reboots the bootloader decides boots are failing and enters
recovery (fastboot, `15e4:d007`) instead of booting the kernel FIT.

Seen on JC11S: right after a cold boot the register reads `b0010001`, and the
first automatic warm reboot booted normally while a later one stopped in
fastboot. The reboot-mode register (offset `0x94`) read `5242c300` (normal)
after recovery, so the boot-mode flag was not the cause. The exact limit has
not been measured.

The OS has to clear the count after a successful boot. `az01-bootcount`
(`recipes-bsp/az01-bootcount`) does that from a boot-time unit: it writes
`0xb0010000` through `/dev/mem`, and only on an `inmusic,az01` +
`rockchip,rk3288` board whose register already holds the magic. The u-boot
environment is not where this counter lives, so `fw_setenv bootcount 0` has
no effect. `az01-bootcount show` prints the current value.

## Loose end

`brcmfmac43456-sdio.inmusic,jp11.bin` is not found at boot, so Wi-Fi firmware
is missing from `az0x-firmware` under that board-specific name.
