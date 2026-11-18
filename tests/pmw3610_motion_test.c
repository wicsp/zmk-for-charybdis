/* SPDX-License-Identifier: MIT */
#include "motion.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

struct output {
    int32_t total[4];
    int attempts;
    int accepted;
    int fail_at;
    bool last_sync;
};

static const struct pmw3610_scroll_config scroll = {
    .tick = 70, .positive_x = 1, .positive_y = -1,
};

static int emit(void *context, enum pmw3610_motion_event event, int32_t value, bool sync) {
    struct output *out = context;
    out->attempts++;
    if (out->attempts == out->fail_at) {
        return -ENOMSG;
    }
    out->total[event] += value;
    out->accepted++;
    out->last_sync = sync;
    return 0;
}

static void flush(struct pmw3610_motion *motion, int64_t now, struct output *out) {
    assert(pmw3610_motion_flush(motion, now, &scroll, emit, out) == 0);
}

static void test_scaling(void) {
    for (int32_t divisor = 1; divisor <= 100; divisor++) {
        for (int sign = -1; sign <= 1; sign += 2) {
            int32_t remainder = 0;
            int32_t emitted = 0;
            for (int i = 1; i <= 1000; i++) {
                emitted += pmw3610_scale_axis(sign, divisor, &remainder);
                assert(emitted == sign * (i / divisor));
                assert(emitted * divisor + remainder == sign * i);
                assert(abs(remainder) < divisor);
            }
        }
        /* Mixed directions and all 12-bit sensor values preserve signed displacement. */
        int32_t remainder = 0;
        int64_t raw_total = 0;
        int64_t output_total = 0;
        for (int32_t raw = -2048; raw <= 2047; raw++) {
            raw_total += raw;
            output_total += pmw3610_scale_axis(raw, divisor, &remainder);
            assert(output_total * divisor + remainder == raw_total);
            assert(abs(remainder) < divisor);
        }
    }
    int32_t x = 0, y = 0;
    assert(pmw3610_scale_axis(3, 4, &x) == 0);
    assert(pmw3610_scale_axis(-3, 4, &x) == 0 && x == 0);
    assert(pmw3610_scale_axis(1, 4, &x) == 0);
    assert(pmw3610_scale_axis(3, 4, &y) == 0);
    assert(pmw3610_scale_axis(1, 4, &y) == 1);
    assert(x == 1 && y == 0); /* Independent axes. */
}

static void test_deadline_and_idle(void) {
    struct pmw3610_motion motion = {0};
    struct output out = {0};
    assert(pmw3610_motion_delay(&motion, 0, 8) == -1);
    motion.pending_x = 1;
    assert(pmw3610_motion_delay(&motion, 0, 8) == 0); /* First motion is immediate. */
    flush(&motion, 0, &out);
    assert(out.total[PMW3610_X] == 1 && out.last_sync);

    /* A single final sample must be flushed by the deadline without another sample. */
    motion.pending_x = 2;
    assert(pmw3610_motion_delay(&motion, 1, 8) == 7);
    assert(pmw3610_motion_delay(&motion, 4, 8) == 4);
    motion.pending_x += 3; /* More samples cannot push the deadline back. */
    assert(pmw3610_motion_delay(&motion, 7, 8) == 1);
    assert(pmw3610_motion_delay(&motion, 8, 8) == 0);
    flush(&motion, 8, &out);
    assert(out.total[PMW3610_X] == 6);
    assert(!pmw3610_motion_pending(&motion));
    assert(pmw3610_motion_delay(&motion, 9, 8) == -1);
    assert(pmw3610_motion_delay(&motion, 10000, 8) == -1);
    int accepted = out.accepted;
    flush(&motion, 10000, &out);
    assert(out.accepted == accepted); /* No stale output or recurring idle reports. */

    motion.pending_y = -1;
    assert(pmw3610_motion_delay(&motion, 10000, 8) == 0);
    flush(&motion, 10000, &out);
    assert(out.total[PMW3610_Y] == -1);
}

static void test_backpressure(void) {
    struct pmw3610_motion motion = {.pending_x = 5, .pending_y = -3};
    struct output out = {.fail_at = 1};
    assert(pmw3610_motion_flush(&motion, 0, &scroll, emit, &out) == -ENOMSG);
    assert(motion.pending_x == 5 && motion.pending_y == -3 && !motion.reported);
    flush(&motion, 8, &out);
    assert(out.total[PMW3610_X] == 5 && out.total[PMW3610_Y] == -3);
    assert(out.last_sync && !pmw3610_motion_pending(&motion));

    motion = (struct pmw3610_motion){.pending_x = 5, .pending_y = -3};
    out = (struct output){.fail_at = 2};
    assert(pmw3610_motion_flush(&motion, 0, &scroll, emit, &out) == -ENOMSG);
    assert(motion.pending_x == 0 && motion.pending_y == -3 && motion.sync_pending);
    assert(out.total[PMW3610_X] == 5 && !out.last_sync);
    flush(&motion, 8, &out);
    assert(out.total[PMW3610_X] == 5); /* Already accepted X must not be duplicated. */
    assert(out.total[PMW3610_Y] == -3 && out.last_sync);
    assert(!pmw3610_motion_pending(&motion));

    /* If Y reverses and cancels during backpressure, the accepted X still needs sync. */
    motion = (struct pmw3610_motion){.pending_x = 5, .pending_y = -3};
    out = (struct output){.fail_at = 2};
    assert(pmw3610_motion_flush(&motion, 0, &scroll, emit, &out) == -ENOMSG);
    motion.pending_y += 3;
    assert(pmw3610_motion_pending(&motion));
    flush(&motion, 8, &out);
    assert(out.total[PMW3610_X] == 5 && out.total[PMW3610_Y] == 0 && out.last_sync);
    assert(!pmw3610_motion_pending(&motion));
}

static void test_modes_and_scroll(void) {
    struct pmw3610_motion motion = {0};
    struct output out = {0};
    assert(pmw3610_scale_axis(3, 4, &motion.remainder_x[MOVE]) == 0);
    motion.pending_x = 2;
    assert(!pmw3610_motion_set_mode(&motion, SCROLL));
    assert(motion.mode == MOVE);
    flush(&motion, 0, &out);
    assert(out.total[PMW3610_X] == 2 && out.total[PMW3610_HWHEEL] == 0);
    assert(pmw3610_motion_set_mode(&motion, SCROLL));
    assert(motion.remainder_x[MOVE] == 3 && motion.remainder_x[SCROLL] == 0);

    motion.pending_y = 70;
    flush(&motion, 8, &out);
    assert(out.total[PMW3610_WHEEL] == 0 && motion.scroll_delta_y == 70);
    assert(!pmw3610_motion_pending(&motion)); /* Sub-tick scroll never spins a timer. */
    motion.pending_y = 1;
    flush(&motion, 16, &out);
    assert(out.total[PMW3610_WHEEL] == -1 && motion.scroll_delta_y == 0);
    motion.pending_x = -71;
    flush(&motion, 24, &out);
    assert(out.total[PMW3610_HWHEEL] == -1);

    motion.pending_y = 140;
    out.fail_at = out.attempts + 1;
    assert(pmw3610_motion_flush(&motion, 32, &scroll, emit, &out) == -ENOMSG);
    assert(motion.pending_y == 140);
    flush(&motion, 40, &out);
    assert(out.total[PMW3610_WHEEL] == -2); /* Existing one-tick behavior preserved. */
    assert(pmw3610_motion_set_mode(&motion, MOVE));
    assert(pmw3610_scale_axis(1, 4, &motion.remainder_x[MOVE]) == 1);
    assert(motion.remainder_x[MOVE] == 0);
}

static void test_large_backlog(void) {
    struct pmw3610_motion motion = {.pending_x = 100000, .pending_y = -100000};
    struct output out = {0};
    for (int i = 0; i < 4; i++) {
        flush(&motion, i * 8, &out);
    }
    assert(out.total[PMW3610_X] == 100000 && out.total[PMW3610_Y] == -100000);
    assert(!pmw3610_motion_pending(&motion));
}

static void test_stream_with_intermittent_backpressure(void) {
    struct pmw3610_motion motion = {0};
    struct output out = {0};
    int32_t total_x = 0, total_y = 0;
    for (int t = 0; t < 10000; t++) {
        int32_t x = (t * 17 % 101) - 50;
        int32_t y = (t * 31 % 103) - 51;
        total_x += x;
        total_y += y;
        motion.pending_x += x;
        motion.pending_y += y;
        if (pmw3610_motion_delay(&motion, t, 8) == 0) {
            out.fail_at = t % 3 == 0 ? out.attempts + (t % 2) + 1 : 0;
            int err = pmw3610_motion_flush(&motion, t, &scroll, emit, &out);
            assert(err == 0 || err == -ENOMSG);
        }
        assert(out.total[PMW3610_X] + motion.pending_x == total_x);
        assert(out.total[PMW3610_Y] + motion.pending_y == total_y);
    }
    out.fail_at = 0;
    flush(&motion, 10008, &out); /* Timer, with no new sample. */
    assert(out.total[PMW3610_X] == total_x && out.total[PMW3610_Y] == total_y);
    assert(!pmw3610_motion_pending(&motion));
}

int main(void) {
    test_scaling();
    test_deadline_and_idle();
    test_backpressure();
    test_modes_and_scroll();
    test_large_backlog();
    test_stream_with_intermittent_backpressure();
    puts("PASS: signed scaling, deadlines, final flush, idle, backpressure, mode changes, scroll, backlog");
    return 0;
}
