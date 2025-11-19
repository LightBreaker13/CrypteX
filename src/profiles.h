#pragma once

#include <stdint.h>

typedef struct {
    uint8_t id[32];
    char name[64];
    int requires_pass;
} profile_desc_t;

int profiles_init(void);
int profiles_count(void);
const profile_desc_t *profiles_get(int index);
void profile_select(const profile_desc_t *profile);
void profile_restore_last_state(const profile_desc_t *profile);

