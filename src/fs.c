#include "fs.h"
#include "console.h"
#include "blockchain.h"

int fs_init(void) {
    blockchain_init();
    log_event(LOG_SUCCESS, "Filesystem initialized with blockchain support");
    return 0;
}

int fs_create_file(const char* path, const uint8_t* data, uint32_t size) {
    if (!path) return -1;
    
    file_type_t type = blockchain_is_system_file(path) ? FILE_TYPE_SYSTEM : FILE_TYPE_USER;
    file_blockchain_t* chain = blockchain_get_file(path, type);
    
    if (!chain) {
        log_event(LOG_ERROR, "Failed to get blockchain for file");
        return -1;
    }
    
    if (blockchain_add_block(chain, data, size, 0) != 0) {
        log_event(LOG_ERROR, "Failed to add create block");
        return -1;
    }
    
    log_event(LOG_SUCCESS, "File created with blockchain entry");
    return 0;
}

int fs_modify_file(const char* path, const uint8_t* data, uint32_t size) {
    if (!path) return -1;
    
    file_type_t type = blockchain_is_system_file(path) ? FILE_TYPE_SYSTEM : FILE_TYPE_USER;
    file_blockchain_t* chain = blockchain_get_file(path, type);
    
    if (!chain) {
        log_event(LOG_ERROR, "File not found in blockchain");
        return -1;
    }
    
    if (blockchain_add_block(chain, data, size, 1) != 0) {
        log_event(LOG_ERROR, "Failed to add modify block");
        return -1;
    }
    
    log_event(LOG_SUCCESS, "File modified, blockchain updated");
    return 0;
}

int fs_verify_file(const char* path) {
    if (!path) return -1;
    
    file_type_t type = blockchain_is_system_file(path) ? FILE_TYPE_SYSTEM : FILE_TYPE_USER;
    file_blockchain_t* chain = blockchain_get_file(path, type);
    
    if (!chain) {
        return -1;
    }
    
    return blockchain_verify(chain);
}

