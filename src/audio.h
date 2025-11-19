#pragma once

typedef enum {
    SND_OK = 1,
    SND_WARN = 2,
    SND_FAIL = 3,
    SND_CLICK = 4,
    SND_OPEN = 5,
    SND_CLOSE = 6
} snd_t;

void audio_init(void);
void audio_play(snd_t sound);
void audio_set_volume(int pct);
void audio_set_mute(int mute);

