#pragma once

#include <stdint.h>

void sha256(const uint8_t *data, uint32_t len, uint8_t out[32]);
uint32_t crc32c(const uint8_t *data, uint32_t len);

