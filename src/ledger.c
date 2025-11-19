#include "ledger.h"
#include "common.h"
#include "crypto.h"
#include "console.h"

#define LEDGER_MAX 32

static ledger_entry_t ledger[LEDGER_MAX];
static int ledger_count;

int ledger_init(void) {
    ledger_count = 0;
    log_event(LOG_SUCCESS, "Ledger online");
    return 0;
}

int jnl_recover(void) {
    if (ledger_count == 0) {
        log_event(LOG_SUCCESS, "Journal clean");
        return 0;
    }
    log_event(LOG_WARN, "Journal replay complete");
    return ledger_count;
}

int jnl_checkpoint(const char *note) {
    if (ledger_count >= LEDGER_MAX) {
        ledger_count = 0;
    }
    ledger_entry_t *entry = &ledger[ledger_count++];
    kmemset(entry, 0, sizeof(*entry));
    kstrncpy(entry->note, note ? note : "checkpoint", sizeof(entry->note) - 1);
    entry->crc = crc32c((const uint8_t *)entry->note, (uint32_t)kstrlen(entry->note));
    sha256((const uint8_t *)entry->note, (uint32_t)kstrlen(entry->note), entry->digest);
    log_event(LOG_SUCCESS, "Ledger checkpoint stored");
    return 0;
}

int ledger_entries(ledger_entry_t *out, int max_entries) {
    if (!out || max_entries <= 0) {
        return 0;
    }
    int to_copy = ledger_count < max_entries ? ledger_count : max_entries;
    for (int i = 0; i < to_copy; ++i) {
        out[i] = ledger[i];
    }
    return to_copy;
}

