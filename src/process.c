#include "process.h"
#include "common.h"
#include "console.h"

static const proc_t demo_table[] = {
    {1, "init", 4, 512},
    {4, "sysmon", 2, 256},
    {7, "console", 1, 192},
    {12, "shell", 3, 320},
    {20, "installer", 0, 0}
};

int proc_enumerate(proc_t *out, int max) {
    if (!out || max <= 0) {
        return 0;
    }
    int count = (int)ARRAY_SIZE(demo_table);
    if (count > max) {
        count = max;
    }
    for (int i = 0; i < count; ++i) {
        out[i] = demo_table[i];
    }
    return count;
}

int proc_kill_tree(int pid) {
    char msg[64];
    kstrncpy(msg, "Kill request for PID ", sizeof(msg));
    kitoa(pid, msg + kstrlen(msg), sizeof(msg) - kstrlen(msg));
    log_event(LOG_WARN, msg);
    return 0;
}

