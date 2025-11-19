#pragma once

#include <stdint.h>

typedef struct {
    char name[32];
    uint64_t size_mb;
    int bootable;
} storage_device_t;

int storage_detect(storage_device_t *out, int max);

