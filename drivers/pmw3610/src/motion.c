/* SPDX-License-Identifier: MIT */
#include "motion.h"

#include <limits.h>

int32_t pmw3610_scale_axis(int32_t delta, int32_t divisor, int32_t *remainder) {
    int32_t total = delta + *remainder;
    int32_t scaled = total / divisor;
    *remainder = total - scaled * divisor;
    return scaled;
}

bool pmw3610_motion_pending(const struct pmw3610_motion *motion) {
    return motion->pending_x != 0 || motion->pending_y != 0 || motion->sync_pending;
}

bool pmw3610_motion_set_mode(struct pmw3610_motion *motion, enum pixart_input_mode mode) {
    if (mode == motion->mode) {
        return true;
    }
    if (pmw3610_motion_pending(motion)) {
        return false;
    }
    if (mode == SCROLL) {
        motion->scroll_delta_x = 0;
        motion->scroll_delta_y = 0;
    }
    motion->mode = mode;
    return true;
}

int32_t pmw3610_motion_delay(const struct pmw3610_motion *motion, int64_t now_ms,
                             int32_t interval_ms) {
    if (!pmw3610_motion_pending(motion)) {
        return -1;
    }
    if (!motion->reported || now_ms - motion->last_report_ms >= interval_ms) {
        return 0;
    }
    return (int32_t)(interval_ms - (now_ms - motion->last_report_ms));
}

static int32_t report_chunk(int32_t value) {
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return value;
}

static void report_accepted(struct pmw3610_motion *motion, int64_t now_ms, bool sync) {
    motion->reported = true;
    motion->last_report_ms = now_ms;
    motion->sync_pending = !sync;
}

int pmw3610_motion_flush(struct pmw3610_motion *motion, int64_t now_ms,
                         const struct pmw3610_scroll_config *scroll,
                         pmw3610_emit_fn emit, void *context) {
    if (!pmw3610_motion_pending(motion)) {
        return 0;
    }

    if (motion->mode == SCROLL) {
        int32_t x = motion->scroll_delta_x + motion->pending_x;
        int32_t y = motion->scroll_delta_y + motion->pending_y;
        int err = 0;
        /* Preserve the existing scroll threshold, axis priority and one-tick behavior. */
        if (y > scroll->tick || y < -scroll->tick) {
            err = emit(context, PMW3610_WHEEL,
                       y > 0 ? scroll->positive_y : -scroll->positive_y, true);
            if (err) {
                return err;
            }
            x = y = 0;
        } else if (x > scroll->tick || x < -scroll->tick) {
            err = emit(context, PMW3610_HWHEEL,
                       x > 0 ? scroll->positive_x : -scroll->positive_x, true);
            if (err) {
                return err;
            }
            x = y = 0;
        }
        motion->scroll_delta_x = x;
        motion->scroll_delta_y = y;
        motion->pending_x = 0;
        motion->pending_y = 0;
        report_accepted(motion, now_ms, true);
        return 0;
    }

    if (motion->pending_x != 0) {
        int32_t x = report_chunk(motion->pending_x);
        bool sync = motion->pending_y == 0;
        int err = emit(context, PMW3610_X, x, sync);
        if (err) {
            return err;
        }
        motion->pending_x -= x;
        report_accepted(motion, now_ms, sync);
    }
    if (motion->pending_y != 0 || motion->sync_pending) {
        int32_t y = report_chunk(motion->pending_y);
        int err = emit(context, PMW3610_Y, y, true);
        if (err) {
            return err;
        }
        motion->pending_y -= y;
        report_accepted(motion, now_ms, true);
    }
    return 0;
}
