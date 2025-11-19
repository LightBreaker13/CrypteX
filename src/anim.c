#include "anim.h"

static float last_time;

void anim_init(void) {
    last_time = 0.0f;
}

float anim_eval(ease_t ease, float x) {
    if (x < 0.0f) {
        x = 0.0f;
    }
    if (x > 1.0f) {
        x = 1.0f;
    }
    switch (ease) {
        case EASE_OUT:
            return 1.0f - (1.0f - x) * (1.0f - x);
        case EASE_INOUT:
            if (x < 0.5f) {
                return 2.0f * x * x;
            }
            return 1.0f - (-2.0f * x + 2.0f) * (-2.0f * x + 2.0f) / 2.0f;
        case EASE_LINEAR:
        default:
            return x;
    }
}

float anim_step(tween_t *tw, float dt) {
    if (!tw || !tw->active || tw->d <= 0.0f) {
        return tw ? tw->to : 0.0f;
    }
    tw->t += dt;
    float phase = tw->t / tw->d;
    float value = tw->from + (tw->to - tw->from) * anim_eval(tw->ease, phase);
    if (tw->t >= tw->d) {
        tw->active = 0;
        value = tw->to;
    }
    return value;
}

float anim_time_delta(void) {
    last_time += 0.016f;
    if (last_time > 1.0f) {
        last_time -= 1.0f;
    }
    return 0.016f;
}

