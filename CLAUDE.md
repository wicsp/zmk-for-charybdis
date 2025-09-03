# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a ZMK (Zephyr Mechanical Keyboard) firmware configuration for the Charybdis 4x6 split keyboard with a trackball on the right side. The keyboard uses Nice!Nano v2 controllers and includes RGB underglow and PMW3610 trackball sensor support.

## Build Configuration

The firmware is built using GitHub Actions configured in `build.yaml`. The build matrix includes:
- `charybdis_left` shield on `nice_nano_v2` board
- `charybdis_right` shield on `nice_nano_v2` board  
- `settings_reset` shield for factory reset

ZMK builds are triggered automatically on push/PR to generate firmware files.

## Architecture & Key Components

### Hardware Definition Structure
- `config/boards/shields/charybdis/` - Shield definitions for both halves
  - `charybdis.dtsi` - Common device tree definitions (matrix transform, kscan)
  - `charybdis_left.overlay` - Left half GPIO mappings and RGB underglow config
  - `charybdis_right.overlay` - Right half GPIO mappings, trackball (PMW3610), and RGB config
  - `Kconfig.shield` - Build-time shield configuration
  - `charybdis.zmk.yml` - Shield metadata and features

### Keymap & Configuration
- `config/charybdis.keymap` - Main keymap with 5 layers:
  - Layer 0: QWERTY base layer with Colemak-DH layout
  - Layer 1: Function keys and navigation (F_layers)
  - Layer 2: Bluetooth controls (BT_layers) 
  - Layer 3: Scroll mode for trackball (scroll-layers)
  - Layer 4: Precision/sniper mode for trackball (snipe-layers)
- `config/charybdis.conf` - Main configuration file with battery reporting and RGB settings
- `config/charybdis.json` - Physical layout definition for keymap editors

### Dependencies
- `config/west.yml` defines the manifest:
  - ZMK firmware from zmkfirmware/zmk (main branch)
  - PMW3610 trackball driver from DoctorWangWang/zmk-pmw3610-driver (main branch)

### Special Features
- **Trackball Integration**: PMW3610 sensor on right half with scroll/snipe layer functionality
- **RGB Underglow**: WS2812 LED strips (29 LEDs on left, 27 LEDs on right)  
- **Layer-based Macros**: RGB effects tied to layer switches (To1_macro, To0_macro, To2_macro)
- **Bluetooth**: Multi-device pairing with BT controls on Layer 2

## Common Tasks

### Modifying Keymap
Edit `config/charybdis.keymap` - keymap uses standard ZMK syntax with layer definitions in the `keymap` node.

### Adjusting Trackball Settings
Trackball configuration is in `config/boards/shields/charybdis/charybdis_right.overlay`:
- `scroll-layers = <3>` - Layer 3 enables scroll mode
- `snipe-layers = <4>` - Layer 4 enables precision mode
- `spi-max-frequency = <2000000>` - SPI communication frequency

### RGB Underglow Configuration  
RGB settings in shield overlay files:
- Chain length: 29 LEDs (left), 27 LEDs (right)
- Color mapping: GRB format for WS2812
- Enable/disable in `config/charybdis.conf` with `CONFIG_ZMK_RGB_UNDERGLOW=y`

### Bluetooth Configuration
BT settings in `config/charybdis.conf`:
- `CONFIG_BT_CTLR_TX_PWR_PLUS_8=y` - Increased transmission power
- `CONFIG_ZMK_BATTERY_REPORT_INTERVAL=60` - Battery reporting interval