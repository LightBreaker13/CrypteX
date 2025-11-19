#include "storage_detect.h"
#include "common.h"

int storage_detect(storage_device_t *out, int max) {
    if (!out || max <= 0) {
        return 0;
    }
    static const storage_device_t seed[] = {
        {"NVMe0n1", 512000, 1},
        {"SATA0", 256000, 0},
        {"USB-Installer", 16000, 0}
    };
    int count = (int)ARRAY_SIZE(seed);
    if (count > max) {
        count = max;
    }
    for (int i = 0; i < count; ++i) {
        out[i] = seed[i];
    }
    return count;
}

