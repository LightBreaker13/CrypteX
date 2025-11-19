#pragma once

typedef struct {
    int dx;
    int dy;
    int lbtn;
    int rbtn;
} mouse_state_t;

void input_init(void);
int kbd_read_char(void);
int mouse_poll(mouse_state_t *state);

