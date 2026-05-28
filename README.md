# Kinesis Advantage 360 Pro ZMK Module

## Modifying the keymap

[The ZMK documentation](https://zmk.dev/docs) covers both basic and advanced functionality and has a table of OS compatibility for keycodes. This repository is an external ZMK module for the Advantage 360 Pro. It builds against upstream ZMK using [config/west.yml](config/west.yml) and exposes the Adv360 board definitions through [zephyr/module.yml](zephyr/module.yml). The status LEDs are handled by this module's Adv360 status LED driver, not by ZMK RGB underglow.

* If you would like to continue using GitHub we recommend using Nick Coutsos’s keymap editor: https://nickcoutsos.github.io/keymap-editor/.
* If you would prefer to leave GitHub and firmware flashing behind you can perform a one-time firmware update to gain access to Clique. Get started here: https://kinesis-ergo.com/360p-clique-upgrade/.

Certain ZMK features (e.g. combos) require knowing the exact key positions in the matrix. They can be found in both image and text format [here](assets/key-positions.md)

## Building the Firmware with GitHub Actions

### Setup

1. Fork this module repository.
2. Enable GitHub Actions on your fork.

### Build firmware

1. Push a commit to trigger the build.
2. Download the artifact.

The workflow builds this module against upstream ZMK from [config/west.yml](config/west.yml). You do not need a ZMK fork.

## Building the Firmware Locally

This repository is a ZMK module. Build it from the module checkout and let west fetch upstream ZMK from [config/west.yml](config/west.yml).

### Setup

Install the normal ZMK/Zephyr build dependencies, including `west` and an Arm embedded toolchain. Then run the west setup once from the root of this repository:

```shell
west init -l config
west update
west zephyr-export
```

### Building both halves

Generate the version macro, build the left half with ZMK Studio/Clique support, and build the right half as a normal split peripheral:

```shell
bin/get_version_local.sh clique

west build -s zmk/app -p -d build/left \
  -b adv360_left/nrf52840/zmk \
  -S studio-rpc-usb-uart \
  -- \
  -DZMK_CONFIG="${PWD}/config" \
  -DZMK_EXTRA_MODULES="${PWD}" \
  -DCONFIG_ZMK_STUDIO=y

west build -s zmk/app -p -d build/right \
  -b adv360_right/nrf52840/zmk \
  -- \
  -DZMK_CONFIG="${PWD}/config" \
  -DZMK_EXTRA_MODULES="${PWD}"
```

The UF2 files are generated at:

* `build/left/zephyr/zmk.uf2`
* `build/right/zephyr/zmk.uf2`

If you used `bin/get_version_local.sh`, restore `config/version.dtsi` before committing unrelated keymap changes:

```shell
git checkout -- config/version.dtsi
```

### Building from an existing ZMK checkout

If you already have an upstream ZMK checkout, build from the ZMK repository directory and point `ZMK_EXTRA_MODULES` at this module checkout:

```shell
west build -s app -p -d build/adv360_left \
  -b adv360_left/nrf52840/zmk \
  -S studio-rpc-usb-uart \
  -- \
  -DZMK_CONFIG="/path/to/Adv360-Pro-ZMK/config" \
  -DZMK_EXTRA_MODULES="/path/to/Adv360-Pro-ZMK" \
  -DCONFIG_ZMK_STUDIO=y

west build -s app -p -d build/adv360_right \
  -b adv360_right/nrf52840/zmk \
  -- \
  -DZMK_CONFIG="/path/to/Adv360-Pro-ZMK/config" \
  -DZMK_EXTRA_MODULES="/path/to/Adv360-Pro-ZMK"
```

## Building the Firmware in a local container

The container build is a convenience wrapper around the same module workflow above. It does not use a ZMK fork.

### Setup

#### Software

* Either Podman or Docker is required, Podman is chosen if both are installed.
* Make is also required

#### Windows specific

* If compiling on Windows use WSL2 and Docker [Docker Setup Guide](https://docs.docker.com/desktop/windows/wsl/).
* Install make using `sudo apt-get install make` inside the WSL2 instance.
* The repository can be cloned directly into the WSL2 instance or accessed through the C: mount point WSL provides by default (`/mnt/c/path-to-repo`).

#### macOS specific

On macOS [brew](https://brew.sh) can be used to install the required components.

* docker
* [colima](https://github.com/abiosoft/colima) can be used as the docker engine

```shell
brew install docker colima
colima start
```
> Note: On Apple Silicon (ARM based) systems you need to make sure to start colima with the correct architecture for the container being used.
> ```
> colima start --arch x86_64
> ```

#### Ubuntu/Debian specific

```shell
sudo apt-get install docker make
```

### Building the firmware

1. Execute `make` to build firmware for both halves or `make left` to only build firmware for the left hand side.
2. Check the `firmware` directory for the latest firmware build. The first part of the filename is the timestamp when the firmware was built.

### Cleanup

The built docker container and compiled firmware files can be deleted with `make clean`. This might be necessary if you updated this module and are encountering build failures.

Creating the docker container takes some time. Therefore `make clean_firmware` can be used to only clean firmware without removing the docker container. Similarly `make clean_image` can be used to remove the docker container without removing compiled firmware files.

## Flashing firmware

Follow the programming instruction on page 8 of the [Quick Start Guide](https://kinesis-ergo.com/wp-content/uploads/Advantage360-Professional-QSG-v8-25-22.pdf) to flash the firmware.

### Overview

1. Extract the firmwares from the archive downloaded from the GitHub build job (If using the cloud builder) or the firmware folder (If building locally).
1. Connect the left side keyboard to USB.
1. Press Mod+macro1 to put the left side into bootloader mode; it should attach to your computer as a USB drive.
1. Copy `left.uf2` to the USB drive and it will disconnect.
1. Power off both keyboards (by unplugging them and making sure the switches are off).
1. Turn on the left side keyboard with the switch.
1. Connect the right side keyboard to USB to power it on.
1. Press Mod+macro3 to put the right side into bootloader mode to attach it as a USB drive.
1. Copy `right.uf2` to the mounted drive.
1. Unplug the right side keyboard and turn it back on.
1. Enjoy!

> Note: There are also physical reset buttons on both keyboards which can be used to enter and exit the bootloader mode. Their location is described in section 2.7 on page 9 in the [User Manual](https://kinesis-ergo.com/wp-content/uploads/Advantage360-ZMK-KB360-PRO-Users-Manual-v3-10-23.pdf) and use is described in section 5.9 on page 14. 

> Note: Some operating systems wont always treat the drive as ejected after the settings-reset file is flashed or may throw a spurious error, this doesn't mean that the flashing process has failed.

### Upgrading from V2 to V3

If you encounter a git conflict when updating your repository to V3.0 please follow the instructions on how to resolve it [here](UPGRADE.md).

Updating from V2.0 based firmwares to V3.0 based firmwares can be a rather complex process. There are reset files for every major firmware revision as well as documentation on the update process available [here](https://kinesis-ergo.com/support/kb360pro/#firmware-updates).

## Versioning

Starting on 11/15/2023 the Advantage 360 Pro will now automatically record the compilation date, branch and Git commit hash in a macro that can be accessed with Mod+V. This will type out the following string: YYYYMMDD-XXXX-YYYYYY, where XXXX is the first 4 characters of the Git branch and YYYYYY is the Git commit hash. In addition to this the builds compiled by GitHub actions are now timestamped and also record the commit hash in the filename. 

## N-Key Rollover

By default this keyboard has NKRO enabled, however for compatibility reasons the higher ranges are not enabled. If you want to use F13-F24 or the INTL1-9 keys with NKRO enabled you can set `CONFIG_ZMK_HID_KEYBOARD_EXTENDED_REPORT=y` in [adv360_left_nrf52840_zmk_defconfig](boards/kinesis/adv360/adv360_left_nrf52840_zmk_defconfig).

## Battery reporting

By default reporting the battery level over BLE is disabled as this can cause some computers to spontaneously wake up repeatedly. If you'd like to enable this functionality change `CONFIG_BT_BAS=n` to `CONFIG_BT_BAS=y` in [adv360_left_nrf52840_zmk_defconfig](boards/kinesis/adv360/adv360_left_nrf52840_zmk_defconfig).

## Status LEDs

The CAPS/NUM/SCROLL LOCK and layer LEDs are controlled by the module-local Adv360 status LED driver. Brightness defaults to `CONFIG_ZMK_ADV360_STATUS_LEDS_BRIGHTNESS=20` in [Kconfig](Kconfig) and can be overridden in the board defconfigs if needed. The `&stp STP_BAT` binding previews local battery level, and `&stp STP_TOG` toggles status LEDs on or off.

## Layer colors

A total of 32 layers are supported by ZMK, with the highest currently active layer displayed using the layer LEDs on each of the left and right modules. All possible colors are listed below; for the first 8 layers the same color is displayed on both modules. After that, only the right module color will cycle through until "rolling over", which will cause the left module color to change as well (and this then repeats). To avoid confusion, the black/off LED color is only used for layer 0.

| Layer # | L/R | Layer # | L/R | Layer # | L/R | Layer # | L/R |
| ---: | :---: | ---: | :---: | ---: | :---: | ---: | :---: |
| 0 | <img valign='middle' src='assets/swatches/000000.svg'/> <img valign='middle' src='assets/swatches/000000.svg'/> | 8 | <img valign='middle' src='assets/swatches/FFFFFF.svg'/> <img valign='middle' src='assets/swatches/0000FF.svg'/> | 16 | <img valign='middle' src='assets/swatches/0000FF.svg'/> <img valign='middle' src='assets/swatches/FF0000.svg'/> | 24 | <img valign='middle' src='assets/swatches/00FF00.svg'/> <img valign='middle' src='assets/swatches/00FFFF.svg'/> |
| 1 | <img valign='middle' src='assets/swatches/FFFFFF.svg'/> <img valign='middle' src='assets/swatches/FFFFFF.svg'/> | 9 | <img valign='middle' src='assets/swatches/FFFFFF.svg'/> <img valign='middle' src='assets/swatches/00FF00.svg'/> | 17 | <img valign='middle' src='assets/swatches/0000FF.svg'/> <img valign='middle' src='assets/swatches/FF00FF.svg'/> | 25 | <img valign='middle' src='assets/swatches/00FF00.svg'/> <img valign='middle' src='assets/swatches/FFFF00.svg'/> |
| 2 | <img valign='middle' src='assets/swatches/0000FF.svg'/> <img valign='middle' src='assets/swatches/0000FF.svg'/> | 10 | <img valign='middle' src='assets/swatches/FFFFFF.svg'/> <img valign='middle' src='assets/swatches/FF0000.svg'/> | 18 | <img valign='middle' src='assets/swatches/0000FF.svg'/> <img valign='middle' src='assets/swatches/00FFFF.svg'/> | 26 | <img valign='middle' src='assets/swatches/FF0000.svg'/> <img valign='middle' src='assets/swatches/FFFFFF.svg'/> |
| 3 | <img valign='middle' src='assets/swatches/00FF00.svg'/> <img valign='middle' src='assets/swatches/00FF00.svg'/> | 11 | <img valign='middle' src='assets/swatches/FFFFFF.svg'/> <img valign='middle' src='assets/swatches/FF00FF.svg'/> | 19 | <img valign='middle' src='assets/swatches/0000FF.svg'/> <img valign='middle' src='assets/swatches/FFFF00.svg'/> | 27 | <img valign='middle' src='assets/swatches/FF0000.svg'/> <img valign='middle' src='assets/swatches/0000FF.svg'/> |
| 4 | <img valign='middle' src='assets/swatches/FF0000.svg'/> <img valign='middle' src='assets/swatches/FF0000.svg'/> | 12 | <img valign='middle' src='assets/swatches/FFFFFF.svg'/> <img valign='middle' src='assets/swatches/00FFFF.svg'/> | 20 | <img valign='middle' src='assets/swatches/00FF00.svg'/> <img valign='middle' src='assets/swatches/FFFFFF.svg'/> | 28 | <img valign='middle' src='assets/swatches/FF0000.svg'/> <img valign='middle' src='assets/swatches/00FF00.svg'/> |
| 5 | <img valign='middle' src='assets/swatches/FF00FF.svg'/> <img valign='middle' src='assets/swatches/FF00FF.svg'/> | 13 | <img valign='middle' src='assets/swatches/FFFFFF.svg'/> <img valign='middle' src='assets/swatches/FFFF00.svg'/> | 21 | <img valign='middle' src='assets/swatches/00FF00.svg'/> <img valign='middle' src='assets/swatches/0000FF.svg'/> | 29 | <img valign='middle' src='assets/swatches/FF0000.svg'/> <img valign='middle' src='assets/swatches/FF00FF.svg'/> |
| 6 | <img valign='middle' src='assets/swatches/00FFFF.svg'/> <img valign='middle' src='assets/swatches/00FFFF.svg'/> | 14 | <img valign='middle' src='assets/swatches/0000FF.svg'/> <img valign='middle' src='assets/swatches/FFFFFF.svg'/> | 22 | <img valign='middle' src='assets/swatches/00FF00.svg'/> <img valign='middle' src='assets/swatches/FF0000.svg'/> | 30 | <img valign='middle' src='assets/swatches/FF0000.svg'/> <img valign='middle' src='assets/swatches/00FFFF.svg'/> |
| 7 | <img valign='middle' src='assets/swatches/FFFF00.svg'/> <img valign='middle' src='assets/swatches/FFFF00.svg'/> | 15 | <img valign='middle' src='assets/swatches/0000FF.svg'/> <img valign='middle' src='assets/swatches/00FF00.svg'/> | 23 | <img valign='middle' src='assets/swatches/00FF00.svg'/> <img valign='middle' src='assets/swatches/FF00FF.svg'/> | 31 | <img valign='middle' src='assets/swatches/FF0000.svg'/> <img valign='middle' src='assets/swatches/FFFF00.svg'/> |

## Changelog

The changelog for this module and the ZMK revisions it has built against can be found [here](CHANGELOG.md).

## Beta testing

The Advantage 360 Pro is always getting updates and refinements. If you are willing to beta test you can follow [this guide from ZMK](https://zmk.dev/docs/features/beta-testing#testing-features) on how to change which ZMK revision this module builds against. The `west.yml` file that is mentioned is located in config/. [This link](config/west.yml) can take you to the file. Typically you will only need to change the `revision: ` to match the beta branch. There is currently no beta branch available for testing.

Feedback on upstream ZMK beta branches should be submitted as a GitHub issue on the upstream ZMK repository as opposed to this module repository.

In the event of a major update the beta branch may not be compatible with the current mainline version of this module. If this is the case it will be detailed here along with instructions on how to update.

## Note

Older versions of this repository referenced a customised ZMK fork. This module targets upstream ZMK and keeps the Advantage 360 Pro board definitions and board-specific status LED support out of tree. Local and CI builds pass `-DZMK_EXTRA_MODULES=<repo root>` to `west build` or include this module from a west manifest.

# DISCLAIMER

This repo packages Advantage 360 Pro support as an external ZMK module to track upstream ZMK without carrying a ZMK fork. Please do not seek official support for this firmware!
