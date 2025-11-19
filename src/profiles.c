#include "profiles.h"
#include "common.h"
#include "console.h"

typedef struct {
    profile_desc_t desc;
    int last_session_ok;
} profile_state_t;

static profile_state_t profile_store[3];
static const profile_desc_t *active_profile;

static void seed_profile(profile_state_t *slot, const char *name, int requires_pass) {
    kmemset(slot, 0, sizeof(*slot));
    kstrncpy(slot->desc.name, name, sizeof(slot->desc.name) - 1);
    slot->desc.requires_pass = requires_pass;
    for (size_t i = 0; i < sizeof(slot->desc.id); ++i) {
        slot->desc.id[i] = (uint8_t)(name[i % kstrlen(name)] + (int)i);
    }
}

int profiles_init(void) {
    seed_profile(&profile_store[0], "Recovery Console", 0);
    seed_profile(&profile_store[1], "Operator", 1);
    seed_profile(&profile_store[2], "Observer", 0);
    active_profile = &profile_store[1].desc;
    log_event(LOG_SUCCESS, "Profiles ready");
    return 0;
}

int profiles_count(void) {
    return (int)ARRAY_SIZE(profile_store);
}

const profile_desc_t *profiles_get(int index) {
    if (index < 0 || index >= profiles_count()) {
        return NULL;
    }
    return &profile_store[index].desc;
}

void profile_select(const profile_desc_t *profile) {
    if (!profile) {
        return;
    }
    active_profile = profile;
    char msg[96];
    kstrncpy(msg, "Profile: ", sizeof(msg));
    kstrcat(msg, profile->name, sizeof(msg));
    log_event(LOG_SUCCESS, msg);
}

void profile_restore_last_state(const profile_desc_t *profile) {
    (void)profile;
    log_event(LOG_WARN, "Restoring last workspace (simulated)");
}

