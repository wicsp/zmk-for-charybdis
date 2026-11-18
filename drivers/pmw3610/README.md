# Local PMW3610 driver

Vendored from [DoctorWangWang/zmk-pmw3610-driver](https://github.com/DoctorWangWang/zmk-pmw3610-driver)
at `1d9c2c68ca76012e1b1e5f6ef02fa5eadc4ca399`, based on inorichi/ufan's
driver. Original copyright and license notices are retained in the source files.

The repository's `zephyr/module.yml` loads this driver through
`ZMK_EXTRA_MODULES`. Do not also add the external driver to `config/west.yml`:
that would load duplicate Kconfig symbols and devicetree bindings.

Local changes:

- Signed per-axis, per-mode scaling remainders preserve small pointer movements.
  The existing CPI and divisor still determine the nominal pointer speed.
- Software 125 Hz reporting uses an 8 ms deadline after the last report. The
  first report after idle is immediate. Subsequent pending motion is sent by
  delayable work, even if no more sensor interrupts arrive. Work-queue load can
  delay execution; this is not a hard real-time guarantee.
- Input submission is non-blocking. Only accepted axes are removed from the
  pending motion; failed submissions are retried. A partially accepted XY pair
  retains its synchronization state, including when later Y motion cancels out.
- Pending events finish in their original mode before a mode switch. Cursor
  fractions are stored separately from scroll/snipe fractions.
- Scroll direction, threshold, axis priority, and the previous one-tick behavior
  are retained. Scroll remainder handling is outside this change.

The sensor initialization, SPI timing, sampling-rate register settings, REST
settings, and Bluetooth settings have not been changed. No report timer is
rearmed when there is no pending whole-count motion or incomplete input pair.
Power and physical cursor feel must still be measured on the keyboard.

Run the motion regression tests from the repository root:

```sh
cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -Idrivers/pmw3610/src tests/pmw3610_motion_test.c \
  drivers/pmw3610/src/motion.c -o /tmp/pmw3610_motion_test
/tmp/pmw3610_motion_test
```

These tests compile the same motion implementation used in the firmware. They
cover signed scaling across all supported divisors and 12-bit sensor values,
independent axes, direction reversals, report deadlines and final samples,
idle behavior, input backpressure, mode transitions, scroll behavior, and large
pending reports. CI runs them before building the three firmware targets.
