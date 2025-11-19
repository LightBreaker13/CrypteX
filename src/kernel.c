#include "fb.h"
#include "input.h"
#include "gui.h"
#include "profiles.h"
#include "ledger.h"
#include "fs.h"
#include "raid.h"
#include "compat_win.h"
#include "console.h"
#include "anim.h"
#include "audio.h"
#include "shell.h"

void kernel_main(void *mb2) {
    fb_init(mb2);
    console_init();
    audio_init();
    anim_init();
    input_init();

    profiles_init();
    ledger_init();
    jnl_recover();
    fs_init();
    raid_init();
    win_compat_init();

    const profile_desc_t *default_profile = profiles_get(1);
    profile_select(default_profile);
    profile_restore_last_state(default_profile);

    shell_open();
    gui_init();
    gui_loop();
}

