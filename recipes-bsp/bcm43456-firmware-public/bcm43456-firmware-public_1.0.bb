SUMMARY = "Publicly distributed BCM43456 Wi-Fi and BCM4345C5 Bluetooth firmware"
DESCRIPTION = "Pinned Linux-distribution firmware for the BCM43456/BCM4345C5 chip \
family, with the AP6256 module NVRAM in a separate package. Boards select this \
set instead of the vendor blobs with AZ0X_WIFI_FIRMWARE = \"public\"; it also \
carries the CLM blob the vendor set lacks. Board-qualified NVRAM links are \
created for the machines in BCM43456_AP6256_BOARDS."

# Reused distribution notices, kept next to the payloads they cover.
LICENSE = "Firmware-synaptics & Firmware-cypress & Firmware-broadcom_bcm43xx"
LIC_FILES_CHKSUM = " \
    file://${BPN}-LICENSE.Wifi-distribution;md5=ef71bd8fbfc3d820a46061421760e8e9 \
    file://${BPN}-LICENSE.Bluetooth-distribution;md5=72df60cd2ebffa2d1fe68ecefaea1edc \
    file://${BPN}-LICENSE.Broadcom;md5=93ed41f137a046c1aeee98f3f60017d2 \
"
NO_GENERIC_LICENSE[Firmware-synaptics] = "${BPN}-LICENSE.Wifi-distribution"
NO_GENERIC_LICENSE[Firmware-cypress] = "${BPN}-LICENSE.Bluetooth-distribution"
NO_GENERIC_LICENSE[Firmware-broadcom_bcm43xx] = "${BPN}-LICENSE.Broadcom"

WIFI_RELEASE = "3bab0f823f5b53150b76aab77093adef6655b920"
BT_RELEASE = "a6d4105d0989103ea6384b65940c76add1914127"
BT_LICENSE_RELEASE = "cdf61dc691a49ff01a124752bd04194907f0f9cd"
NVRAM_RELEASE = "cd0502653db3739843b0a41a560b66dcf245567b"
BRCM_LICENSE_RELEASE = "ed63c1b592979a469c94fbe8f67a16c8d7f9c67d"

SRC_URI = " \
    https://raw.githubusercontent.com/RPi-Distro/firmware-nonfree/${WIFI_RELEASE}/debian/added-firmware/brcm/brcmfmac43456-sdio.bin;name=wifi;downloadfilename=${BPN}-wifi.bin \
    https://raw.githubusercontent.com/RPi-Distro/firmware-nonfree/${WIFI_RELEASE}/debian/added-firmware/brcm/brcmfmac43456-sdio.clm_blob;name=clm;downloadfilename=${BPN}-wifi.clm_blob \
    https://raw.githubusercontent.com/LibreELEC/brcmfmac_sdio-firmware/${NVRAM_RELEASE}/brcmfmac43456-sdio.txt;name=nvram;downloadfilename=${BPN}-module-nvram.txt \
    https://raw.githubusercontent.com/RPi-Distro/bluez-firmware/${BT_RELEASE}/broadcom/BCM4345C5.hcd;name=bluetooth;downloadfilename=${BPN}-bluetooth.hcd \
    https://raw.githubusercontent.com/RPi-Distro/firmware-nonfree/${WIFI_RELEASE}/debian/copyright;name=license_wifi;downloadfilename=${BPN}-LICENSE.Wifi-distribution \
    https://raw.githubusercontent.com/RPi-Distro/bluez-firmware/${BT_LICENSE_RELEASE}/debian/copyright;name=license_bluetooth;downloadfilename=${BPN}-LICENSE.Bluetooth-distribution \
    https://raw.githubusercontent.com/LibreELEC/LibreELEC.tv/${BRCM_LICENSE_RELEASE}/licenses/LicenseRef-firmware-Broadcom.txt;name=license_broadcom;downloadfilename=${BPN}-LICENSE.Broadcom \
"
SRC_URI[wifi.sha256sum] = "ddf83f2100885b166be52d21c8966db164fdd4e1d816aca2acc67ee9cc28d726"
SRC_URI[clm.sha256sum] = "2dbd7d22fc9af0eb560ceab45b19646d211bc7b34a1dd00c6bfac5dd6ba25e8a"
SRC_URI[nvram.sha256sum] = "66c71eb53b47c49d42386b66735134578640b0946f3f46e44f384fef5aacfd9e"
SRC_URI[bluetooth.sha256sum] = "fb9f4ec2df5301bd35f416384e103c012c5983024c49aa72fbbaf90177512caa"
SRC_URI[license_wifi.sha256sum] = "07082fd0bb65e32c73af39a2292dd6c83f169be7427d338ba1fe34f9b86c0e30"
SRC_URI[license_bluetooth.sha256sum] = "5f6acad870790f7e23150a30cab85c643f926266bf16e6a63acbde4257b957d4"
SRC_URI[license_broadcom.sha256sum] = "5881e9e347d00304b9042efe2f5d48099500a0ddbd352dd808a2ff4108f8fbbe"

S = "${WORKDIR}"
# The board-qualified NVRAM links depend on the machine's BCM43456_AP6256_BOARDS,
# so these packages are not architecture-neutral.
PACKAGE_ARCH = "${MACHINE_ARCH}"

# Root compatible strings of boards fitted with an AP6256 module. This is a
# hardware fact and is set in conf/machine/<machine>.conf, not here. Each board
# gets brcmfmac43456-sdio.<compatible>.txt, which brcmfmac asks for before the
# generic name.
BCM43456_AP6256_BOARDS ?= ""

PACKAGES =+ "${PN}-nvram-ap6256"
SUMMARY:${PN}-nvram-ap6256 = "AP6256 module NVRAM for BCM43456 boards"
DESCRIPTION:${PN}-nvram-ap6256 = "Unmodified AP6256 37.4 MHz module data, plus a \
board-qualified link for each machine listed in BCM43456_AP6256_BOARDS."
LICENSE:${PN}-nvram-ap6256 = "Firmware-broadcom_bcm43xx"
FILES:${PN}-nvram-ap6256 = " \
    ${nonarch_base_libdir}/firmware/brcm/brcmfmac43456-sdio.ap6256.txt \
    ${nonarch_base_libdir}/firmware/LICENSE.bcm43456-public.Nvram \
    ${@''.join('${nonarch_base_libdir}/firmware/brcm/brcmfmac43456-sdio.%s.txt ' % b for b in (d.getVar('BCM43456_AP6256_BOARDS') or '').split())} \
"

do_compile[noexec] = "1"

do_install() {
    install -d ${D}${nonarch_base_libdir}/firmware/brcm
    install -m 0644 ${S}/${BPN}-wifi.bin ${D}${nonarch_base_libdir}/firmware/brcm/brcmfmac43456-sdio.bin
    install -m 0644 ${S}/${BPN}-wifi.clm_blob ${D}${nonarch_base_libdir}/firmware/brcm/brcmfmac43456-sdio.clm_blob
    install -m 0644 ${S}/${BPN}-bluetooth.hcd ${D}${nonarch_base_libdir}/firmware/brcm/BCM4345C5.hcd

    # Module data is not a universal fallback: the board must select it.
    install -m 0644 ${S}/${BPN}-module-nvram.txt ${D}${nonarch_base_libdir}/firmware/brcm/brcmfmac43456-sdio.ap6256.txt
    for board in ${BCM43456_AP6256_BOARDS}; do
        ln -s brcmfmac43456-sdio.ap6256.txt ${D}${nonarch_base_libdir}/firmware/brcm/brcmfmac43456-sdio.$board.txt
    done

    # Keep the notices in the firmware package itself, not an optional -doc split.
    install -m 0644 ${S}/${BPN}-LICENSE.Wifi-distribution ${D}${nonarch_base_libdir}/firmware/LICENSE.bcm43456-public.Wifi
    install -m 0644 ${S}/${BPN}-LICENSE.Bluetooth-distribution ${D}${nonarch_base_libdir}/firmware/LICENSE.bcm43456-public.Bluetooth
    install -m 0644 ${S}/${BPN}-LICENSE.Broadcom ${D}${nonarch_base_libdir}/firmware/LICENSE.bcm43456-public.Nvram
}

FILES:${PN} += "${nonarch_base_libdir}/firmware/brcm/brcmfmac43456-sdio.bin \
    ${nonarch_base_libdir}/firmware/brcm/brcmfmac43456-sdio.clm_blob \
    ${nonarch_base_libdir}/firmware/brcm/BCM4345C5.hcd \
    ${nonarch_base_libdir}/firmware/LICENSE.bcm43456-public.Wifi \
    ${nonarch_base_libdir}/firmware/LICENSE.bcm43456-public.Bluetooth"
