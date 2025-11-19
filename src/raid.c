#include "raid.h"
#include "console.h"

int raid_init(void) {
    log_event(LOG_SUCCESS, "RAID mapper prepared");
    return 0;
}

