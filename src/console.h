#pragma once

typedef enum {
    LOG_SUCCESS = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
} log_level_t;

void console_init(void);
void console_open(void);
void console_close(void);
void console_toggle(void);
int console_is_open(void);
void console_render(void);
void console_handle_input(char c);
void log_event(log_level_t level, const char *message);

