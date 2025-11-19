#include "audio.h"
#include "console.h"

static int audio_volume = 80;
static int audio_muted;

void audio_init(void) {
    audio_volume = 80;
    audio_muted = 0;
    log_event(LOG_SUCCESS, "Audio cues armed");
}

void audio_play(snd_t sound) {
    if (audio_muted) {
        return;
    }
    (void)sound;
}

void audio_set_volume(int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    audio_volume = pct;
    (void)audio_volume;
}

void audio_set_mute(int mute) {
    audio_muted = mute ? 1 : 0;
}

