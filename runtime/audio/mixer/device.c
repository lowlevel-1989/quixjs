/*
 * quixjs - A lightweight JS runtime using SDL2 and JavaScriptCore
 * Copyright (C) 2025  Vinicio Valbuena (lowlevel.1989)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// runtime/audio/mixer/device.c: el dispositivo de audio: abrirlo (o seguir
// sin el, por el reloj), cada frame (mixer_update) y cerrarlo.
// Parte de runtime/audio/mixer.c (archivo indice).

#include "common.h"

int mixer_init(int with_device) {
  if (ready) return 0;
  memset(voices, 0, sizeof(voices));
  ready = 1;
  if (!with_device) return 0;

  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
    fprintf(stderr, "[audio] SDL audio: %s (sin sonido)\n", SDL_GetError());
    return -1;
  }
  SDL_AudioSpec want, have;
  SDL_zero(want);
  want.freq = 44100;
  want.format = AUDIO_S16SYS;
  want.channels = 2;
  // en una portatil, mas margen: con la CPU llena (un video) la salida
  // pedia audio tarde y se oian cortes
#ifdef QUIX_HANDHELD
  want.samples = 2048;
#else
  want.samples = 1024;
#endif
  want.callback = audio_callback;
  device = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  if (!device) {
    fprintf(stderr, "[audio] no audio device: %s (sin sonido)\n", SDL_GetError());
    return -1;
  }
  out_rate = have.freq;
  SDL_PauseAudioDevice(device, 0);
  printf("Audio: %d Hz, stereo\n", out_rate);
  return 0;
}

int mixer_has_device(void) {
  return device != 0;
}

int mixer_active(void) {
  int n = 0;
  for (int i = 0; i < MAX_VOICES; i++) if (voices[i].active || voices[i].ended) n++;
  return n;
}

void mixer_update(void) {
  for (int i = 0; i < MAX_VOICES; i++) {
    Voice *v = &voices[i];
    if (v->clip && !device && v->active) voice_playing(v->id);
    if (v->clip && !v->active && v->ended) {
      lock();
      AudioClip *c = v->clip;
      Decoder *d = v->dec;
      memset(v, 0, sizeof(*v));
      unlock();
      if (d) decoder_close(d);
      clip_release(c);
    }
  }
}

void mixer_shutdown(void) {
  if (!ready) return;
  if (device) {
    SDL_CloseAudioDevice(device);
    device = 0;
  }
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices[i].clip) voice_free(&voices[i]);
  }
  ready = 0;
}
