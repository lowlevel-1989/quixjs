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

// runtime/audio/mixer/voice.c: las voces: play, set (volumen, pan,
// velocidad, loop), stop y el tiempo de cada una.
// Parte de runtime/audio/mixer.c (archivo indice).

#include "common.h"

// el thread de audio de SDL no mezcla mientras se tocan las voces (y el
// flujo de PCM, en stream.c)
static void lock(void) { if (device) SDL_LockAudioDevice(device); }
static void unlock(void) { if (device) SDL_UnlockAudioDevice(device); }

static Voice *find(int id) {
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices[i].id == id && (voices[i].active || voices[i].ended)) return &voices[i];
  }
  return NULL;
}

static void voice_free(Voice *v) {
  if (v->dec) decoder_close(v->dec);
  clip_release(v->clip);
  memset(v, 0, sizeof(*v));
}

int voice_play(AudioClip *c, double offset, int loop, double loop_start, double loop_end, float rate) {
  if (!ready || !c) return -1;
  mixer_update();

  Voice *slot = NULL;
  for (int i = 0; i < MAX_VOICES; i++) {
    if (!voices[i].active && !voices[i].ended && !voices[i].clip) { slot = &voices[i]; break; }
  }
  if (!slot) return -1;   // demasiados sonidos a la vez: este no suena

  Voice v;
  memset(&v, 0, sizeof(v));
  v.clip = c;
  clip_retain(c);
  v.volume = 1;
  v.rate = rate > 0 ? rate : 1;
  v.loop = loop;
  v.loop_start = (long)(loop_start * c->rate);
  v.loop_end = loop_end > 0 ? (long)(loop_end * c->rate) : 0;
  if (v.loop_end > c->length || v.loop_end <= v.loop_start) v.loop_end = 0;
  v.offset = offset > 0 ? offset : 0;
  v.start_ms = clock_now_ms();

  if (device) {
    long start = (long)(v.offset * c->rate);
    if (!c->pcm) {
      v.dec = decoder_open(c->data, c->len);
      if (!v.dec) {
        clip_release(c);
        return -1;
      }
      if (start > 0 && start < c->length) decoder_seek(v.dec, start);
    }
    v.buf_start = start > 0 && start < c->length ? start : 0;
  }
  v.id = next_id++;
  v.active = 1;

  lock();
  *slot = v;
  unlock();
  return slot->id;
}

void voice_set(int id, float volume, float pan, float rate, int loop) {
  lock();
  Voice *v = find(id);
  if (v) {
    v->volume = volume < 0 ? 0 : volume;
    v->pan = pan < -1 ? -1 : (pan > 1 ? 1 : pan);
    if (rate > 0) v->rate = rate;
    v->loop = loop;
  }
  unlock();
}

void voice_stop(int id) {
  lock();
  Voice *v = find(id);
  if (v) {
    v->active = 0;
    v->ended = 1;
  }
  unlock();
}

// Segundos dentro del clip (modo sin dispositivo: por el reloj)
static double virtual_time(Voice *v) {
  double t = v->offset + (clock_now_ms() - v->start_ms) / 1000.0 * v->rate;
  double dur = (double)v->clip->length / v->clip->rate;
  if (v->loop && dur > 0) {
    double ls = (double)v->loop_start / v->clip->rate;
    double le = v->loop_end > 0 ? (double)v->loop_end / v->clip->rate : dur;
    if (t > le && le > ls) t = ls + fmod(t - ls, le - ls);
  }
  return t;
}

int voice_playing(int id) {
  Voice *v = find(id);
  if (!v) return 0;
  if (!device && v->active && !v->loop) {
    double dur = (double)v->clip->length / v->clip->rate;
    if (virtual_time(v) >= dur) {
      v->active = 0;
      v->ended = 1;
    }
  }
  return v->active;
}

double voice_time(int id) {
  Voice *v = find(id);
  if (!v) return 0;
  if (!device) return virtual_time(v);
  return (v->buf_start + v->pos) / (double)v->clip->rate;
}
