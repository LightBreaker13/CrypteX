#include "gui.h"
#include "fb.h"
#include "input.h"
#include "shell.h"
#include "sysmon.h"
#include "console.h"
#include "installer.h"
#include "profiles.h"
#include "anim.h"
#include "common.h"

static uint32_t bar_color = 0x00282840;

static void draw_bar(void) {
    fb_fillrect(0, 0, fb_width(), 32, bar_color);
    fb_draw_text(8, 8, "MyOS Desktop", 0x00FFFFFF, 0);
    fb_draw_text(180, 8, "[S]hell  [C]onsole  [M]onitor  [I]nstall", 0x00A0FFFF, 0);
}

static void handle_shortcuts(char c) {
    switch (c) {
        case 's':
        case 'S':
            shell_toggle();
            break;
        case 'c':
        case 'C':
            console_toggle();
            break;
        case 'm':
        case 'M':
            if (sysmon_is_open()) {
                sysmon_close();
            } else {
                sysmon_open();
            }
            break;
        case 'i':
        case 'I':
            if (installer_is_open()) {
                installer_close();
            } else {
                installer_open();
            }
            break;
        default:
            break;
    }
}

void gui_init(void) {
    shell_init();
    sysmon_open();
}

void gui_loop(void) {
    mouse_state_t ms;
    for (;;) {
        fb_clear(0x00081018);
        draw_bar();

        int ch;
        while ((ch = kbd_read_char()) >= 0) {
            handle_shortcuts((char)ch);
            shell_handle_char((char)ch);
            console_handle_input((char)ch);
            sysmon_handle_input((char)ch);
            installer_handle_key((char)ch);
        }

        while (mouse_poll(&ms)) {
        }

        sysmon_render();
        console_render();
        shell_run();
        installer_render();
    }
}

