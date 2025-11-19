#pragma once

typedef enum {
    EASE_LINEAR,
    EASE_OUT,
    EASE_INOUT
} ease_t;

typedef struct {
    float t;
    float d;
    float from;
    float to;
    ease_t ease;
    int active;
} tween_t;

void anim_init(void);
float anim_eval(ease_t ease, float x);
float anim_step(tween_t *tw, float dt);
float anim_time_delta(void);

