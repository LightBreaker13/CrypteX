#include "fs.h"
#include "console.h"

int fs_init(void) {
    log_event(LOG_WARN, "Filesystem driver is in stub mode");
    return 0;
}

