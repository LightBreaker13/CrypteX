#include "sysmon.h"
#include "fb.h"
#include "process.h"
#include "console.h"
#include "common.h"

static int sysmon_open_flag;
static int focus_index;

void sysmon_open(void) {
    sysmon_open_flag = 1;
    log_event(LOG_SUCCESS, "SysMon opened");
}

void sysmon_close(void) {
    sysmon_open_flag = 0;
}

int sysmon_is_open(void) {
    return sysmon_open_flag;
}

void sysmon_handle_input(char c) {
    if (!sysmon_open_flag) {
        return;
    }
    if (c == 'q') {
        sysmon_close();
    }
}

static void render_table(int x, int y) {
    proc_t rows[16];
    int count = proc_enumerate(rows, ARRAY_SIZE(rows));
    fb_draw_text(x, y, "PID   CPU  MEM  TASK", 0x00FFAA00, 0x00101010);
    for (int i = 0; i < count; ++i) {
        char line[96];
        kmemset(line, 0, sizeof(line));
        kstrncpy(line, (i == focus_index) ? ">" : " ", sizeof(line));
        char num[16];
        kitoa(rows[i].pid, num, sizeof(num));
        kstrcat(line, num, sizeof(line));
        kstrcat(line, "   ", sizeof(line));
        kitoa(rows[i].cpu_pct, num, sizeof(num));
        kstrcat(line, num, sizeof(line));
        kstrcat(line, "%  ", sizeof(line));
        kitoa(rows[i].mem_kb, num, sizeof(num));
        kstrcat(line, num, sizeof(line));
        kstrcat(line, "KB  ", sizeof(line));
        kstrcat(line, rows[i].name, sizeof(line));
        fb_draw_text(x, y + 16 * (i + 1), line, 0x00FFFFFF, 0x00000000);
    }
}

void sysmon_render(void) {
    if (!sysmon_open_flag) {
        return;
    }
    int w = fb_width();
    fb_fillrect(8, 48, w / 2 - 16, 180, 0x00202040);
    fb_draw_text(16, 56, "SYSTEM MONITOR", 0x00FFFFFF, 0x00000000);
    render_table(16, 72);
}

