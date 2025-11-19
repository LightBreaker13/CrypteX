#include "console.h"
#include "fb.h"
#include "audio.h"
#include "common.h"

#define LOG_CAP 48

typedef struct {
    log_level_t level;
    char text[96];
} log_entry_t;

static log_entry_t log_buffer[LOG_CAP];
static int log_count;
static int console_open_flag;

void console_init(void) {
    log_count = 0;
    console_open_flag = 1;
    log_event(LOG_SUCCESS, "Console ready");
}

void console_open(void) {
    console_open_flag = 1;
}

void console_close(void) {
    console_open_flag = 0;
}

void console_toggle(void) {
    console_open_flag = !console_open_flag;
}

int console_is_open(void) {
    return console_open_flag;
}

static uint32_t level_color(log_level_t lvl) {
    switch (lvl) {
        case LOG_SUCCESS: return 0x0000FF00;
        case LOG_WARN: return 0x00FFFF00;
        case LOG_ERROR: return 0x00FF0000;
        default: return 0x00FFFFFF;
    }
}

void log_event(log_level_t level, const char *message) {
    if (!message) {
        return;
    }
    if (log_count >= LOG_CAP) {
        for (int i = 1; i < LOG_CAP; ++i) {
            log_buffer[i - 1] = log_buffer[i];
        }
        log_count = LOG_CAP - 1;
    }
    log_buffer[log_count].level = level;
    kstrncpy(log_buffer[log_count].text, message, sizeof(log_buffer[log_count].text) - 1);
    log_count++;

    switch (level) {
        case LOG_SUCCESS: audio_play(SND_OK); break;
        case LOG_WARN: audio_play(SND_WARN); break;
        case LOG_ERROR: audio_play(SND_FAIL); break;
        default: break;
    }
}

void console_handle_input(char c) {
    if (!console_open_flag) {
        return;
    }
    if (c == 'q') {
        console_close();
    }
}

void console_render(void) {
    if (!console_open_flag) {
        return;
    }
    int h = fb_height();
    int w = fb_width();
    fb_fillrect(w / 2, 48, w / 2 - 16, h - 200, 0x00121212);
    fb_draw_text(w / 2 + 8, 56, "CONSOLE LOG", 0x00FFFFFF, 0);
    int start = log_count > 12 ? log_count - 12 : 0;
    int y = 72;
    for (int i = start; i < log_count; ++i) {
        uint32_t color = level_color(log_buffer[i].level);
        fb_draw_text(w / 2 + 8, y, log_buffer[i].text, color, 0);
        y += 16;
    }
}

