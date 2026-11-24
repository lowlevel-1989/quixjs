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

// runtime/audio/mixer/stream.c: el flujo de PCM (el audio del video): una cola que
// mezcla el callback.
// Parte de runtime/audio/mixer.c (archivo indice).

#include "common.h"

int mixer_stream_open(int rate) {
  if (!ready || rate <= 0) return -1;
  lock();
  memset(&stream_mix, 0, sizeof(stream_mix));
  stream_mix.rate = rate;
  stream_mix.volume = 1;
  stream_mix.open = 1;
  stream_mix.start_ms = clock_now_ms();
  unlock();
  return 0;
}

int mixer_stream_push(const float *stereo, int frames) {
  if (!stream_mix.open) return 0;
  lock();
  long room = STREAM_FRAMES - (stream_mix.tail - stream_mix.head) - 1;
  if (frames > room) frames = (int)room;
  for (int i = 0; i < frames; i++) {
    long k = (stream_mix.tail + i) % STREAM_FRAMES;
    stream_mix.buf[k * 2] = stereo[i * 2];
    stream_mix.buf[k * 2 + 1] = stereo[i * 2 + 1];
  }
  stream_mix.tail += frames;
  unlock();
  return frames;
}

int mixer_stream_queued(void) {
  if (!stream_mix.open) return 0;
  if (!device) {
    // sin dispositivo la cola se "oye" al ritmo del reloj
    long played = (long)(mixer_stream_time() * stream_mix.rate);
    if (played > stream_mix.tail) played = stream_mix.tail;
    stream_mix.head = played;
  }
  return (int)(stream_mix.tail - stream_mix.head);
}

double mixer_output_time(void) {
  return device ? (double)output_frames / out_rate : 0;
}

double mixer_stream_time(void) {
  if (!stream_mix.open) return 0;
  if (!device) {
    double now = stream_mix.paused ? stream_mix.paused_ms : clock_now_ms();
    return (now - stream_mix.start_ms) / 1000.0;
  }
  return (double)stream_mix.played / stream_mix.rate;
}

void mixer_stream_volume(float volume) {
  stream_mix.volume = volume < 0 ? 0 : volume;
}

void mixer_stream_pause(int paused) {
  lock();
  if (!device) {
    if (paused && !stream_mix.paused) stream_mix.paused_ms = clock_now_ms();
    if (!paused && stream_mix.paused) stream_mix.start_ms += clock_now_ms() - stream_mix.paused_ms;
  }
  stream_mix.paused = paused;
  unlock();
}

void mixer_stream_close(void) {
  lock();
  stream_mix.open = 0;
  unlock();
}
