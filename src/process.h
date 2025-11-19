#pragma once

#include <stdint.h>

typedef struct {
    int pid;
    char name[64];
    int cpu_pct;
    int mem_kb;
} proc_t;

int proc_enumerate(proc_t *out, int max);
int proc_kill_tree(int pid);

