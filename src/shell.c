#include "shell.h"
#include "fb.h"
#include "console.h"
#include "common.h"
#include "sysmon.h"
#include "installer.h"
#include "ledger.h"
#include "profiles.h"
#include "blockchain.h"
#include "fs.h"

#define SHELL_LINES 8
#define SHELL_WIDTH 64

static char history[SHELL_LINES][SHELL_WIDTH];
static int history_count;
static char input_buffer[SHELL_WIDTH];
static int shell_visible;

void shell_init(void) {
    shell_visible = 1;
    history_count = 0;
    kmemset(input_buffer, 0, sizeof(input_buffer));
}

void shell_open(void) {
    shell_visible = 1;
    log_event(LOG_SUCCESS, "Shell ready");
}

void shell_close(void) {
    shell_visible = 0;
}

void shell_toggle(void) {
    shell_visible = !shell_visible;
}

int shell_is_open(void) {
    return shell_visible;
}

static void push_history(const char *line) {
    if (history_count >= SHELL_LINES) {
        for (int i = 1; i < SHELL_LINES; ++i) {
            kstrncpy(history[i - 1], history[i], SHELL_WIDTH - 1);
        }
        history_count = SHELL_LINES - 1;
    }
    kstrncpy(history[history_count++], line, SHELL_WIDTH - 1);
}

static void cmd_echo(const char *args) {
    log_event(LOG_SUCCESS, args && *args ? args : "(empty)");
}

static void cmd_help(void) {
    log_event(LOG_SUCCESS, "Commands: HELP ECHO SYSMON CONSOLE INSTALL JOURNAL CHECKPOINT VERIFY CHAIN BCSTATUS");
}

static void cmd_sysmon(void) {
    sysmon_open();
}

static void cmd_console(void) {
    console_open();
}

static void cmd_install(void) {
    installer_open();
}

static void cmd_journal(void) {
    ledger_entry_t entries[8];
    int count = ledger_entries(entries, ARRAY_SIZE(entries));
    for (int i = 0; i < count; ++i) {
        log_event(LOG_SUCCESS, entries[i].note);
    }
}

static void cmd_checkpoint(const char *note) {
    if (!note || !*note) {
        note = "manual checkpoint";
    }
    jnl_checkpoint(note);
}

static void cmd_verify(const char *path) {
    if (!path || !*path) {
        log_event(LOG_WARN, "Usage: VERIFY <filepath>");
        return;
    }
    int result = fs_verify_file(path);
    if (result == 0) {
        log_event(LOG_SUCCESS, "File blockchain verified");
    } else {
        log_event(LOG_ERROR, "File blockchain verification failed");
    }
}

static void cmd_chain(const char *path) {
    if (!path || !*path) {
        log_event(LOG_WARN, "Usage: CHAIN <filepath>");
        return;
    }
    file_type_t type = blockchain_is_system_file(path) ? FILE_TYPE_SYSTEM : FILE_TYPE_USER;
    file_blockchain_t* chain = blockchain_get_file(path, type);
    if (!chain) {
        log_event(LOG_WARN, "File not found in blockchain");
        return;
    }
    file_block_t* latest = blockchain_get_latest(chain);
    if (latest) {
        char msg[128];
        kstrncpy(msg, "Blocks: ", sizeof(msg) - 1);
        char num[32];
        kitoa((int)chain->block_count, num, sizeof(num));
        kstrcat(msg, num, sizeof(msg));
        log_event(LOG_SUCCESS, msg);
    } else {
        log_event(LOG_WARN, "No blocks in chain");
    }
}

static void cmd_bcstatus(void) {
    log_event(LOG_SUCCESS, "Blockchain system: Active");
    log_event(LOG_SUCCESS, "System files: Shared blockchain");
    log_event(LOG_SUCCESS, "User files: Individual blockchains");
}

static void execute_command(const char *line) {
    if (!kstrlen(line)) {
        return;
    }
    push_history(line);
    if (!kstrcmp(line, "HELP")) {
        cmd_help();
    } else if (!kstrncmp(line, "ECHO ", 5)) {
        cmd_echo(line + 5);
    } else if (!kstrcmp(line, "SYSMON")) {
        cmd_sysmon();
    } else if (!kstrcmp(line, "CONSOLE")) {
        cmd_console();
    } else if (!kstrcmp(line, "INSTALL")) {
        cmd_install();
    } else if (!kstrcmp(line, "JOURNAL")) {
        cmd_journal();
    } else if (!kstrncmp(line, "CHECKPOINT", 10)) {
        const char *note = line + 10;
        while (*note == ' ') note++;
        cmd_checkpoint(note);
    } else if (!kstrncmp(line, "VERIFY ", 7)) {
        cmd_verify(line + 7);
    } else if (!kstrncmp(line, "CHAIN ", 6)) {
        cmd_chain(line + 6);
    } else if (!kstrcmp(line, "BCSTATUS")) {
        cmd_bcstatus();
    } else {
        log_event(LOG_WARN, "Unknown command");
    }
}

void shell_handle_char(char c) {
    if (!shell_visible) {
        return;
    }
    if (c == '\b') {
        size_t len = kstrlen(input_buffer);
        if (len) {
            input_buffer[len - 1] = '\0';
        }
        return;
    }
    if (c == '\n') {
        char upper[SHELL_WIDTH];
        kstrncpy(upper, input_buffer, sizeof(upper) - 1);
        for (size_t i = 0; i < kstrlen(upper); ++i) {
            upper[i] = kupper(upper[i]);
        }
        execute_command(upper);
        kmemset(input_buffer, 0, sizeof(input_buffer));
        return;
    }
    size_t len = kstrlen(input_buffer);
    if (len + 1 < sizeof(input_buffer)) {
        input_buffer[len] = c;
        input_buffer[len + 1] = '\0';
    }
}

void shell_run(void) {
    if (!shell_visible) {
        return;
    }
    fb_fillrect(8, fb_height() - 150, fb_width() - 16, 142, 0x00202020);
    fb_draw_text(16, fb_height() - 142, "SHELL >", 0x00FFFFFF, 0);
    int y = fb_height() - 124;
    for (int i = 0; i < history_count; ++i) {
        fb_draw_text(16, y + i * 16, history[i], 0x00A0FF70, 0);
    }
    char prompt[96];
    kstrncpy(prompt, "> ", sizeof(prompt));
    kstrcat(prompt, input_buffer, sizeof(prompt));
    fb_draw_text(16, fb_height() - 32, prompt, 0x00FFFFFF, 0);
}

