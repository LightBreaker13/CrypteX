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

