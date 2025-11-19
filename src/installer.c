#include "installer.h"
#include "fb.h"
#include "console.h"
#include "storage_detect.h"
#include "common.h"
#include "audio.h"

typedef enum {
    STEP_WELCOME,
    STEP_SCAN,
    STEP_CONFIRM,
    STEP_PROGRESS,
    STEP_DONE
} install_step_t;

static int installer_open_flag;
static install_step_t step;
static storage_device_t devices[8];
static int device_count;
static int selected_index;
static char confirm_buffer[8];

void installer_open(void) {
    installer_open_flag = 1;
    step = STEP_WELCOME;
    selected_index = 0;
    kmemset(confirm_buffer, 0, sizeof(confirm_buffer));
    log_event(LOG_WARN, "Installer launched (simulation)");
    audio_play(SND_OPEN);
}

void installer_close(void) {
    installer_open_flag = 0;
    audio_play(SND_CLOSE);
}

int installer_is_open(void) {
    return installer_open_flag;
}

static void advance_step(void) {
    if (step == STEP_WELCOME) {
        step = STEP_SCAN;
        device_count = storage_detect(devices, ARRAY_SIZE(devices));
    } else if (step == STEP_SCAN) {
        step = STEP_CONFIRM;
    } else if (step == STEP_CONFIRM) {
        step = STEP_PROGRESS;
    } else if (step == STEP_PROGRESS) {
        step = STEP_DONE;
    } else {
        installer_close();
    }
}

void installer_handle_key(char c) {
    if (!installer_open_flag) {
        return;
    }
    if (c == 'q') {
        installer_close();
        return;
    }
    switch (step) {
        case STEP_WELCOME:
            if (c == '\n') {
                advance_step();
            }
            break;
        case STEP_SCAN:
            if (c == '\n') {
                advance_step();
            }
            break;
        case STEP_CONFIRM:
            if (c == '\n') {
                if (!kstrcmp(confirm_buffer, "INSTALL")) {
                    advance_step();
                } else {
                    log_event(LOG_ERROR, "Type INSTALL to continue");
                }
                kmemset(confirm_buffer, 0, sizeof(confirm_buffer));
            } else if (c == '\b') {
                size_t len = kstrlen(confirm_buffer);
                if (len) confirm_buffer[len - 1] = '\0';
            } else {
                size_t len = kstrlen(confirm_buffer);
                if (len + 1 < sizeof(confirm_buffer)) {
                    confirm_buffer[len] = kupper(c);
                    confirm_buffer[len + 1] = '\0';
                }
            }
            break;
        case STEP_PROGRESS:
            advance_step();
            break;
        case STEP_DONE:
            if (c == '\n') {
                installer_close();
            }
            break;
    }
}

static void render_devices(int x, int y) {
    for (int i = 0; i < device_count; ++i) {
        char line[96];
        kmemset(line, 0, sizeof(line));
        kstrncpy(line, (i == selected_index) ? ">" : " ", sizeof(line));
        kstrcat(line, devices[i].name, sizeof(line));
        kstrcat(line, "  ", sizeof(line));
        char sizebuf[32];
        kitoa((int)devices[i].size_mb, sizebuf, sizeof(sizebuf));
        kstrcat(line, sizebuf, sizeof(line));
        kstrcat(line, "MB", sizeof(line));
        fb_draw_text(x, y + i * 16, line, 0x00FFFFFF, 0);
    }
}

void installer_render(void) {
    if (!installer_open_flag) {
        return;
    }
    int w = fb_width();
    fb_fillrect(32, 200, w - 64, 200, 0x00222222);
    fb_draw_text(40, 208, "Installer", 0x00FFFFFF, 0);
    switch (step) {
        case STEP_WELCOME:
            fb_draw_text(40, 232, "Welcome to the guided installer. Press Enter to scan storage.", 0x00FFFFFF, 0);
            break;
        case STEP_SCAN:
            fb_draw_text(40, 232, "Detecting storage devices...", 0x00FFFFFF, 0);
            render_devices(40, 248);
            break;
        case STEP_CONFIRM:
            fb_draw_text(40, 232, "Type INSTALL to confirm non-destructive install:", 0x00FFFF00, 0);
            fb_draw_text(40, 248, confirm_buffer, 0x00FFFFFF, 0);
            break;
        case STEP_PROGRESS:
            fb_draw_text(40, 232, "Installing... (simulated)", 0x0000FF00, 0);
            break;
        case STEP_DONE:
            fb_draw_text(40, 232, "Install complete. Press Enter to close.", 0x0000FF00, 0);
            break;
    }
}

