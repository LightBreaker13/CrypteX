#pragma once

#include <stdint.h>

#define BLOCKCHAIN_MAX_BLOCKS 1024
#define BLOCKCHAIN_MAX_FILES 256
#define FILE_PATH_MAX 256
#define BLOCK_SHARD_SIZE 64
#define BLOCK_SHARDS_PER_BLOCK 2

typedef enum {
    FILE_TYPE_SYSTEM = 0,
    FILE_TYPE_USER = 1
} file_type_t;

typedef struct {
    uint8_t data[BLOCK_SHARD_SIZE];
    uint8_t shard_hash[32];
    uint8_t parity[BLOCK_SHARD_SIZE];
} block_shard_t;

typedef struct {
    uint32_t block_index;
    uint8_t prev_hash[32];
    uint8_t file_hash[32];
    uint64_t timestamp;
    uint32_t file_size;
    uint32_t operation;  // 0=create, 1=modify, 2=delete, 3=metadata
    uint8_t metadata_hash[32];
    uint8_t block_hash[32];
    // Redundancy data for system files
    block_shard_t shards[BLOCK_SHARDS_PER_BLOCK];
    uint8_t has_redundancy;
} file_block_t;

typedef struct {
    char file_path[FILE_PATH_MAX];
    file_type_t file_type;
    uint32_t block_count;
    file_block_t blocks[BLOCKCHAIN_MAX_BLOCKS];
    uint8_t chain_hash[32];
} file_blockchain_t;

typedef struct {
    file_blockchain_t system_chain;
    file_blockchain_t user_files[BLOCKCHAIN_MAX_FILES];
    uint32_t user_file_count;
} blockchain_manager_t;

// Initialize blockchain system
int blockchain_init(void);

// Create or get blockchain for a file
file_blockchain_t* blockchain_get_file(const char* path, file_type_t type);

// Add a block to a file's blockchain
int blockchain_add_block(file_blockchain_t* chain, const uint8_t* file_data, 
                        uint32_t file_size, uint32_t operation);

// Verify a file's blockchain integrity
int blockchain_verify(file_blockchain_t* chain);

// Get the latest block for a file
file_block_t* blockchain_get_latest(file_blockchain_t* chain);

// Check if file is a system file
int blockchain_is_system_file(const char* path);

// Recover file from blockchain
int blockchain_recover_file(file_blockchain_t* chain, uint8_t* out_data, uint32_t* out_size);

// Redundancy and recovery functions (for system files)
int blockchain_add_redundancy(file_blockchain_t* chain, uint32_t block_idx, const uint8_t* file_data, uint32_t file_size);
int blockchain_recover_block_from_redundancy(file_blockchain_t* chain, uint32_t block_idx,
                                            uint32_t complete_block_idx, uint32_t partial_block_idx, uint32_t partial_shard_idx);
int blockchain_verify_redundancy(file_blockchain_t* chain, uint32_t block_idx);

