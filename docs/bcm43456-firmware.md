# BCM43456 Wi-Fi and BCM4345C5 Bluetooth firmware

This layer supplies the radio payloads for the BCM43456 SDIO Wi-Fi and
BCM4345C5 Bluetooth parts. It does not decide how the radio is managed: the
`wlan0` network configuration, the supplicant/ConnMan choice and the
radio-enable policy belong to meta-openaz0x.

## Two sources, one owner per path

`az0x-firmware` copies the vendor firmware tree, including
`brcm/brcmfmac43456-sdio.bin` and `brcm/BCM4345C5.hcd`. The public set
(`bcm43456-firmware-public`) installs the same two names, so a machine must pick
one source:

```bitbake
AZ0X_WIFI_FIRMWARE ?= "vendor"   # default, in az01-common.inc
```

- `vendor` - keep the vendor blobs; the public package is not installed.
- `public` - install `bcm43456-firmware-public` (and the AP6256 NVRAM package
  when the board lists it), and `az0x-firmware` omits the overlapping
  `brcmfmac43456-sdio.bin` and `BCM4345C5.hcd`.

`az0x-signed` selects `public`: the public set carries the CLM blob the vendor
files lack, which is what brcmfmac reports as `no clm_blob available` on the
vendor set.

Because both variables are read at parse time, a machine that selects `public`
gets the package in `MACHINE_ESSENTIAL_EXTRA_RDEPENDS`; nothing has to be added
to an image recipe.

## Board NVRAM

Which boards carry an AP6256 module is a hardware fact, so each machine lists
its root compatible strings:

```bitbake
BCM43456_AP6256_BOARDS = "inmusic,jc11s"   # conf/machine/az0x-signed.conf
```

The firmware recipe installs the unmodified module data as
`brcm/brcmfmac43456-sdio.ap6256.txt` and, for each listed board, the
board-qualified link brcmfmac asks for first:

```text
brcm/brcmfmac43456-sdio.<root-compatible>.txt -> brcmfmac43456-sdio.ap6256.txt
```

There is deliberately no generic `brcmfmac43456-sdio.txt` in the public package:
the module data is not a universal fallback. Boards not listed keep whatever
their own firmware source provides.

## Sources and licences

| Payload | Source |
| --- | --- |
| Wi-Fi `.bin`, `.clm_blob` | RPi-Distro/firmware-nonfree |
| Bluetooth `BCM4345C5.hcd` | RPi-Distro/bluez-firmware |
| AP6256 NVRAM | LibreELEC/brcmfmac_sdio-firmware |

Commits and SHA-256 checksums are pinned in the recipe. The Synaptics, Cypress
and Broadcom distribution notices are installed next to the payloads, and the
recipe `LICENSE` names all three.

## Verifying a build

Build the same image for a `public` machine and a `vendor` machine; the
`do_rootfs` clash check is the test that exactly one package owns each file:

```sh
bitbake az0x-image-desktop     # for az0x-signed and for an az01 board
```

`az0x-firmware` must not ship `brcm/brcmfmac43456-sdio.bin` or
`brcm/BCM4345C5.hcd` when `AZ0X_WIFI_FIRMWARE` is `public`, and must ship them
otherwise.
