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

// WAV (RIFF) para el mezclador (decoder.h): PCM de 8, 16, 24 o 32 bits, o
// float de 32 bits, ya sin comprimir: se lee de donde esta y se pasa a float.

#include <stdlib.h>
#include <string.h>
#include "decoder.h"

typedef struct {
  const uint8_t *data;   // las muestras (el bloque "data")
  long  frames, pos;
  int   channels, bits, is_float;
} Wav;

static uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }

static int w_is(const uint8_t *data, size_t len) {
  return len >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WAVE", 4) == 0;
}

static void *w_open(const uint8_t *data, size_t len, DecoderInfo *info) {
  Wav w;
  memset(&w, 0, sizeof w);
  int fmt = 0;
  size_t at = 12;
  while (at + 8 <= len) {
    uint32_t size = le32(data + at + 4);
    const uint8_t *body = data + at + 8;
    size_t room = len - at - 8;
    if (memcmp(data + at, "fmt ", 4) == 0 && size >= 16 && room >= 16) {
      fmt = le16(body);
      w.channels = le16(body + 2);
      info->rate = (int)le32(body + 4);
      w.bits = le16(body + 14);
      // WAVE_FORMAT_EXTENSIBLE: el formato de verdad, en el subformato
      if (fmt == 0xfffe && size >= 26 && room >= 26) fmt = le16(body + 24);
    } else if (memcmp(data + at, "data", 4) == 0) {
      w.data = body;
      size_t bytes = size < room ? size : room;
      int frame = w.channels * (w.bits / 8);
      w.frames = frame > 0 ? (long)(bytes / (size_t)frame) : 0;
      break;
    }
    at += 8 + size + (size & 1);
  }
  w.is_float = fmt == 3 && w.bits == 32;
  if (!w.data || w.channels < 1 || !(fmt == 1 || w.is_float) ||
      !(w.bits == 8 || w.bits == 16 || w.bits == 24 || w.bits == 32)) return NULL;
  Wav *p = malloc(sizeof *p);
  if (!p) return NULL;
  *p = w;
  info->channels = w.channels;
  info->length = w.frames;
  return p;
}

static float sample(const Wav *w, const uint8_t *p) {
  switch (w->bits) {
    case 8:  return (p[0] - 128) * (1.0f / 128.0f);
    case 16: return (int16_t)le16(p) * (1.0f / 32768.0f);
    case 24: return (int32_t)((uint32_t)p[0] << 8 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 24) * (1.0f / 2147483648.0f);
    default: {
      uint32_t u = le32(p);
      if (w->is_float) {
        float f;
        memcpy(&f, &u, 4);
        return f;
      }
      return (int32_t)u * (1.0f / 2147483648.0f);
    }
  }
}

static int w_read(void *s, float *out, int frames) {
  Wav *w = s;
  long n = w->frames - w->pos < frames ? w->frames - w->pos : frames;
  int bytes = w->bits / 8;
  const uint8_t *p = w->data + (size_t)w->pos * (size_t)(w->channels * bytes);
  for (long i = 0; i < n * w->channels; i++, p += bytes) out[i] = sample(w, p);
  w->pos += n;
  return (int)n;
}

static int w_seek(void *s, long frame) {
  Wav *w = s;
  w->pos = frame < w->frames ? frame : w->frames;
  return 0;
}

static void w_close(void *s) { free(s); }

const DecoderKind wav_decoder = { "wav", w_is, w_open, w_read, w_seek, w_close };
