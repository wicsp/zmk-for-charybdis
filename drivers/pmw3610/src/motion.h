/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <stdint.h>

enum pixart_input_mode { MOVE = 0, SCROLL, SNIPE };
enum pmw3610_motion_event { PMW3610_X, PMW3610_Y, PMW3610_WHEEL, PMW3610_HWHEEL };

struct pmw3610_motion {
    enum pixart_input_mode mode;
    int32_t remainder_x[3];
    int32_t remainder_y[3];
    int32_t pending_x;
    int32_t pending_y;
    int32_t scroll_delta_x;
    int32_t scroll_delta_y;
    int64_t last_report_ms;
    bool reported;
    bool sync_pending;
};

struct pmw3610_scroll_config {
    int32_t tick;
    int32_t positive_x;
    int32_t positive_y;
};

typedef int (*pmw3610_emit_fn)(void *context, enum pmw3610_motion_event event,
                               int32_t value, bool sync);

int32_t pmw3610_scale_axis(int32_t delta, int32_t divisor, int32_t *remainder);
bool pmw3610_motion_pending(const struct pmw3610_motion *motion);
bool pmw3610_motion_set_mode(struct pmw3610_motion *motion, enum pixart_input_mode mode);
/* Returns -1 when idle, 0 when due, otherwise milliseconds until the deadline. */
int32_t pmw3610_motion_delay(const struct pmw3610_motion *motion, int64_t now_ms,
                             int32_t interval_ms);
int pmw3610_motion_flush(struct pmw3610_motion *motion, int64_t now_ms,
                         const struct pmw3610_scroll_config *scroll,
                         pmw3610_emit_fn emit, void *context);
