#pragma once

int fs_init(void);
int fs_create_file(const char* path, const uint8_t* data, uint32_t size);
int fs_modify_file(const char* path, const uint8_t* data, uint32_t size);
int fs_verify_file(const char* path);
int fs_recover_block(const char* path, uint32_t block_idx, uint32_t complete_idx, uint32_t partial_idx, uint32_t partial_shard);

