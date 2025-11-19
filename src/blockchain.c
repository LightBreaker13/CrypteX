#include "blockchain.h"
#include "crypto.h"
#include "common.h"
#include "console.h"
#include <stddef.h>

static blockchain_manager_t bcm;

static void compute_block_hash(file_block_t* block, uint8_t out[32]) {
    uint8_t temp[256];
    uint32_t offset = 0;
    
    kmemcpy(temp + offset, &block->block_index, 4);
    offset += 4;
    kmemcpy(temp + offset, block->prev_hash, 32);
    offset += 32;
    kmemcpy(temp + offset, block->file_hash, 32);
    offset += 32;
    kmemcpy(temp + offset, &block->timestamp, 8);
    offset += 8;
    kmemcpy(temp + offset, &block->file_size, 4);
    offset += 4;
    kmemcpy(temp + offset, &block->operation, 4);
    offset += 4;
    kmemcpy(temp + offset, block->metadata_hash, 32);
    offset += 32;
    
    sha256(temp, offset, out);
}

static void compute_chain_hash(file_blockchain_t* chain, uint8_t out[32]) {
    if (chain->block_count == 0) {
        kmemset(out, 0, 32);
        return;
    }
    
    file_block_t* last_block = &chain->blocks[chain->block_count - 1];
    kmemcpy(out, last_block->block_hash, 32);
}

int blockchain_init(void) {
    kmemset(&bcm, 0, sizeof(bcm));
    
    kstrncpy(bcm.system_chain.file_path, "/system", sizeof(bcm.system_chain.file_path) - 1);
    bcm.system_chain.file_type = FILE_TYPE_SYSTEM;
    bcm.system_chain.block_count = 0;
    
    bcm.user_file_count = 0;
    
    log_event(LOG_SUCCESS, "Blockchain system initialized");
    return 0;
}

int blockchain_is_system_file(const char* path) {
    if (!path) return 0;
    
    if (path[0] == '/' && (
        kstrncmp(path, "/system", 7) == 0 ||
        kstrncmp(path, "/boot", 5) == 0 ||
        kstrncmp(path, "/kernel", 7) == 0 ||
        kstrncmp(path, "/lib", 4) == 0 ||
        kstrncmp(path, "/bin", 4) == 0 ||
        kstrncmp(path, "/etc", 4) == 0
    )) {
        return 1;
    }
    return 0;
}

file_blockchain_t* blockchain_get_file(const char* path, file_type_t type) {
    if (!path) return NULL;
    
    int is_system = blockchain_is_system_file(path);
    
    if (is_system || type == FILE_TYPE_SYSTEM) {
        return &bcm.system_chain;
    }
    
    for (uint32_t i = 0; i < bcm.user_file_count; ++i) {
        if (kstrcmp(bcm.user_files[i].file_path, path) == 0) {
            return &bcm.user_files[i];
        }
    }
    
    if (bcm.user_file_count >= BLOCKCHAIN_MAX_FILES) {
        log_event(LOG_ERROR, "Blockchain: Max user files reached");
        return NULL;
    }
    
    file_blockchain_t* new_chain = &bcm.user_files[bcm.user_file_count++];
    kmemset(new_chain, 0, sizeof(*new_chain));
    kstrncpy(new_chain->file_path, path, sizeof(new_chain->file_path) - 1);
    new_chain->file_type = FILE_TYPE_USER;
    new_chain->block_count = 0;
    
    log_event(LOG_SUCCESS, "Created new blockchain for file");
    return new_chain;
}

int blockchain_add_block(file_blockchain_t* chain, const uint8_t* file_data, 
                        uint32_t file_size, uint32_t operation) {
    if (!chain) {
        return -1;
    }
    
    if (chain->block_count >= BLOCKCHAIN_MAX_BLOCKS) {
        log_event(LOG_ERROR, "Blockchain: Max blocks reached for file");
        return -1;
    }
    
    file_block_t* block = &chain->blocks[chain->block_count++];
    kmemset(block, 0, sizeof(*block));
    
    block->block_index = chain->block_count - 1;
    block->file_size = file_size;
    block->operation = operation;
    block->timestamp = 0;
    
    if (file_data && file_size > 0) {
        sha256(file_data, file_size, block->file_hash);
    } else {
        kmemset(block->file_hash, 0, 32);
    }
    
    if (block->block_index == 0) {
        kmemset(block->prev_hash, 0, 32);
    } else {
        file_block_t* prev_block = &chain->blocks[block->block_index - 1];
        kmemcpy(block->prev_hash, prev_block->block_hash, 32);
    }
    
    uint8_t metadata[64];
    kmemset(metadata, 0, sizeof(metadata));
    kmemcpy(metadata, chain->file_path, kstrlen(chain->file_path));
    kmemcpy(metadata + 32, &file_size, 4);
    sha256(metadata, 36, block->metadata_hash);
    
    compute_block_hash(block, block->block_hash);
    compute_chain_hash(chain, chain->chain_hash);
    
    return 0;
}

int blockchain_verify(file_blockchain_t* chain) {
    if (!chain) return -1;
    
    if (chain->block_count == 0) {
        return 0;
    }
    
    for (uint32_t i = 0; i < chain->block_count; ++i) {
        file_block_t* block = &chain->blocks[i];
        uint8_t computed_hash[32];
        compute_block_hash(block, computed_hash);
        
        if (kmemcmp(computed_hash, block->block_hash, 32) != 0) {
            log_event(LOG_ERROR, "Blockchain verification failed: block hash mismatch");
            return -1;
        }
        
        if (i > 0) {
            file_block_t* prev_block = &chain->blocks[i - 1];
            if (kmemcmp(block->prev_hash, prev_block->block_hash, 32) != 0) {
                log_event(LOG_ERROR, "Blockchain verification failed: chain broken");
                return -1;
            }
        }
    }
    
    uint8_t computed_chain_hash[32];
    compute_chain_hash(chain, computed_chain_hash);
    if (kmemcmp(computed_chain_hash, chain->chain_hash, 32) != 0) {
        log_event(LOG_ERROR, "Blockchain verification failed: chain hash mismatch");
        return -1;
    }
    
    return 0;
}

file_block_t* blockchain_get_latest(file_blockchain_t* chain) {
    if (!chain || chain->block_count == 0) {
        return NULL;
    }
    return &chain->blocks[chain->block_count - 1];
}

int blockchain_recover_file(file_blockchain_t* chain, uint8_t* out_data, uint32_t* out_size) {
    if (!chain || !out_data || !out_size) {
        return -1;
    }
    
    if (chain->block_count == 0) {
        return -1;
    }
    
    file_block_t* latest = blockchain_get_latest(chain);
    if (!latest) {
        return -1;
    }
    
    if (latest->operation == 2) {
        log_event(LOG_WARN, "File was deleted, cannot recover");
        return -1;
    }
    
    *out_size = latest->file_size;
    
    log_event(LOG_SUCCESS, "File recovery data available from blockchain");
    return 0;
}

static void generate_shard_parity(const uint8_t* data, uint32_t size, uint8_t* parity) {
    kmemset(parity, 0, BLOCK_SHARD_SIZE);
    for (uint32_t i = 0; i < size && i < BLOCK_SHARD_SIZE; ++i) {
        parity[i] = data[i];
    }
    for (uint32_t i = size; i < BLOCK_SHARD_SIZE; ++i) {
        parity[i] = 0;
    }
}

static void split_into_shards(const uint8_t* data, uint32_t size, block_shard_t* shards) {
    uint32_t shard_size = size / BLOCK_SHARDS_PER_BLOCK;
    if (shard_size == 0) shard_size = 1;
    
    for (int i = 0; i < BLOCK_SHARDS_PER_BLOCK; ++i) {
        kmemset(&shards[i], 0, sizeof(block_shard_t));
        
        uint32_t offset = i * shard_size;
        uint32_t copy_size = (offset + BLOCK_SHARD_SIZE <= size) ? BLOCK_SHARD_SIZE : (size - offset);
        if (copy_size > BLOCK_SHARD_SIZE) copy_size = BLOCK_SHARD_SIZE;
        
        if (copy_size > 0 && offset < size) {
            kmemcpy(shards[i].data, data + offset, copy_size);
        }
        
        sha256(shards[i].data, BLOCK_SHARD_SIZE, shards[i].shard_hash);
        generate_shard_parity(shards[i].data, BLOCK_SHARD_SIZE, shards[i].parity);
    }
}

int blockchain_add_redundancy(file_blockchain_t* chain, uint32_t block_idx, const uint8_t* file_data, uint32_t file_size) {
    if (!chain || block_idx >= chain->block_count) {
        return -1;
    }
    
    if (chain->file_type != FILE_TYPE_SYSTEM) {
        return 0;
    }
    
    file_block_t* block = &chain->blocks[block_idx];
    split_into_shards(file_data, file_size, block->shards);
    block->has_redundancy = 1;
    
    log_event(LOG_SUCCESS, "Redundancy data added to system block");
    return 0;
}

static void xor_blocks(const uint8_t* a, const uint8_t* b, uint8_t* out, uint32_t size) {
    for (uint32_t i = 0; i < size; ++i) {
        out[i] = a[i] ^ b[i];
    }
}

static void compute_inter_block_parity(file_blockchain_t* chain, uint32_t block_idx, uint8_t* parity_out) {
    kmemset(parity_out, 0, BLOCK_SHARD_SIZE);
    
    for (uint32_t i = 0; i < chain->block_count; ++i) {
        if (i == block_idx) continue;
        file_block_t* block = &chain->blocks[i];
        if (block->has_redundancy) {
            for (int s = 0; s < BLOCK_SHARDS_PER_BLOCK; ++s) {
                xor_blocks(parity_out, block->shards[s].data, parity_out, BLOCK_SHARD_SIZE);
            }
        }
    }
}

int blockchain_recover_block_from_redundancy(file_blockchain_t* chain, uint32_t block_idx,
                                            uint32_t complete_block_idx, uint32_t partial_block_idx, uint32_t partial_shard_idx) {
    if (!chain || block_idx >= chain->block_count || 
        complete_block_idx >= chain->block_count || partial_block_idx >= chain->block_count) {
        return -1;
    }
    
    if (chain->file_type != FILE_TYPE_SYSTEM) {
        log_event(LOG_WARN, "Recovery only available for system files");
        return -1;
    }
    
    if (block_idx == complete_block_idx || block_idx == partial_block_idx || complete_block_idx == partial_block_idx) {
        log_event(LOG_WARN, "Invalid block indices for recovery");
        return -1;
    }
    
    file_block_t* target_block = &chain->blocks[block_idx];
    file_block_t* complete_block = &chain->blocks[complete_block_idx];
    file_block_t* partial_block = &chain->blocks[partial_block_idx];
    
    if (!complete_block->has_redundancy || !partial_block->has_redundancy) {
        log_event(LOG_WARN, "Source blocks missing redundancy data");
        return -1;
    }
    
    if (partial_shard_idx >= BLOCK_SHARDS_PER_BLOCK) {
        log_event(LOG_WARN, "Invalid shard index");
        return -1;
    }
    
    uint8_t inter_parity[BLOCK_SHARD_SIZE];
    compute_inter_block_parity(chain, block_idx, inter_parity);
    
    block_shard_t* partial_shard = &partial_block->shards[partial_shard_idx];
    block_shard_t* target_shard0 = &target_block->shards[0];
    block_shard_t* target_shard1 = &target_block->shards[1];
    
    for (int s = 0; s < BLOCK_SHARDS_PER_BLOCK; ++s) {
        uint8_t* target_shard_data = (s == 0) ? target_shard0->data : target_shard1->data;
        block_shard_t* complete_shard = &complete_block->shards[s];
        
        xor_blocks(inter_parity, complete_shard->data, target_shard_data, BLOCK_SHARD_SIZE);
        xor_blocks(target_shard_data, partial_shard->data, target_shard_data, BLOCK_SHARD_SIZE);
        xor_blocks(target_shard_data, partial_shard->parity, target_shard_data, BLOCK_SHARD_SIZE);
        
        block_shard_t* target_shard = (s == 0) ? target_shard0 : target_shard1;
        sha256(target_shard_data, BLOCK_SHARD_SIZE, target_shard->shard_hash);
        generate_shard_parity(target_shard_data, BLOCK_SHARD_SIZE, target_shard->parity);
    }
    
    target_block->has_redundancy = 1;
    compute_block_hash(target_block, target_block->block_hash);
    
    log_event(LOG_SUCCESS, "Block recovered from one complete block and half of another");
    return 0;
}

int blockchain_verify_redundancy(file_blockchain_t* chain, uint32_t block_idx) {
    if (!chain || block_idx >= chain->block_count) {
        return -1;
    }
    
    file_block_t* block = &chain->blocks[block_idx];
    if (!block->has_redundancy) {
        return 0;
    }
    
    for (int i = 0; i < BLOCK_SHARDS_PER_BLOCK; ++i) {
        uint8_t computed_hash[32];
        sha256(block->shards[i].data, BLOCK_SHARD_SIZE, computed_hash);
        
        if (kmemcmp(computed_hash, block->shards[i].shard_hash, 32) != 0) {
            log_event(LOG_ERROR, "Shard hash mismatch in block");
            return -1;
        }
    }
    
    return 0;
}

