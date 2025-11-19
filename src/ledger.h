#pragma once

#include <stdint.h>

typedef struct {
    uint32_t crc;
    uint8_t digest[32];
    char note[96];
} ledger_entry_t;

int ledger_init(void);
int jnl_recover(void);
int jnl_checkpoint(const char *note);
int ledger_entries(ledger_entry_t *out, int max_entries);

