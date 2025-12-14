#pragma once
#include "GameConfig.h"

void make_rainbow_sound(SynthSound& snd);
void make_stellar_explosion_sound(SynthSound& snd);
void make_blue_sound(SynthSound& snd);
void request_play(SynthSound& snd);
void audio_callback(void* userdata, Uint8* stream, int len);
