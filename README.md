# Charybdis 4x6 ZMK configuration

Wireless Charybdis with nice!nano v2 controllers and a right-hand PMW3610
trackball. The right half is the central device.

## Keymap

| Position | Function |
| --- | --- |
| Left letter area, Ctrl/Esc key | Tap Esc; Ctrl is active immediately on press for shortcuts |
| Left thumb, upper left | Command |
| Left thumb, upper middle | Tap Space; hold for the editing/scroll layer |
| Left thumb, upper right | F20, for an assistant shortcut configured on the computer |
| Left thumb, lower left | Option |
| Left thumb, lower right | Hold for the Bluetooth layer |
| Right thumb, upper left | Right Command + Left Option |
| Right thumb, upper right | Tap Enter; hold for the editing/scroll layer |
| Right thumb, lower | F19, for an additional shortcut configured on the computer |

On the editing layer, the two outer keys in the right-hand home row are
Backspace (delete left) and Delete (delete right). Trackball scrolling remains
on this layer. There is no precision mouse layer.

Hold the Bluetooth layer key and press left-hand number keys 1-5 to select
Bluetooth profiles 0-4.

## Firmware builds

Pushes, pull requests, and manual runs trigger **Build Charybdis firmware** in
GitHub Actions. Only download artifacts from a successful run for the intended
commit. The combined artifact is named `firmware` and contains:

- `charybdis_left-nice_nano_v2-zmk.uf2`: flash to the left half.
- `charybdis_right-nice_nano_v2-zmk.uf2`: flash to the right half.
- `settings_reset-nice_nano_v2-zmk.uf2`: erases stored settings; not part of a
  normal firmware update.

Connect each half by USB, double-tap its reset button to mount the bootloader
drive, and copy that half's UF2 file onto the drive. Use both halves from the
same build.

The ZMK core and reusable CI workflow are pinned to ZMK v0.3.0
(`edf5c0814fd3ea202e43aad2d68fd32e882a518c`). The legacy PMW3610 driver is
pinned to `1d9c2c68ca76012e1b1e5f6ef02fa5eadc4ca399`. Keep these versions
compatible with the `nice_nano_v2` board definition; upgrading to newer ZMK
requires migrating the board target and sensor driver together.
