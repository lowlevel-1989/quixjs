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

// Elige el formato por sus primeros bytes y pasa lo que da a los canales que
// pide el mezclador. Ver decoder.h.

#include <stdlib.h>
#include "decoder.h"

#define CHUNK 1024   // frames que se leen de una vez al cambiar de canales

struct Decoder {
  const DecoderKind *kind;
  void              *s;
  DecoderInfo        info;
  float             *tmp;   // CHUNK frames con los canales del archivo
};

static const DecoderKind *const kinds[] = { &vorbis_decoder, &mp3_decoder, &wav_decoder };

Decoder *decoder_open(const uint8_t *data, size_t len) {
  if (!data || !len) return NULL;
  for (size_t i = 0; i < sizeof kinds / sizeof kinds[0]; i++) {
    if (!kinds[i]->is(data, len)) continue;
    Decoder *d = calloc(1, sizeof *d);
    if (!d) return NULL;
    d->kind = kinds[i];
    d->info.loop_start = d->info.loop_length = -1;
    d->s = d->kind->open(data, len, &d->info);
    if (!d->s || d->info.channels < 1 || d->info.rate <= 0) {
      if (d->s) d->kind->close(d->s);
      free(d);
      return NULL;
    }
    return d;
  }
  return NULL;
}

void decoder_close(Decoder *d) {
  if (!d) return;
  d->kind->close(d->s);
  free(d->tmp);
  free(d);
}

int decoder_channels(const Decoder *d) { return d->info.channels; }
int decoder_rate(const Decoder *d) { return d->info.rate; }
long decoder_length(const Decoder *d) { return d->info.length; }

void decoder_loop(const Decoder *d, long *start, long *length) {
  *start = d->info.loop_start;
  *length = d->info.loop_length;
}

int decoder_seek(Decoder *d, long frame) {
  return d->kind->seek(d->s, frame < 0 ? 0 : frame);
}

int decoder_read(Decoder *d, int ch, float *out, int frames) {
  int src = d->info.channels;
  if (src == ch) return d->kind->read(d->s, out, frames);
  // otros canales: por partes, mezclando (a mono) o duplicando (a estereo)
  if (!d->tmp && !(d->tmp = malloc(sizeof(float) * CHUNK * (size_t)src))) return 0;
  int done = 0;
  while (done < frames) {
    int want = frames - done < CHUNK ? frames - done : CHUNK;
    int got = d->kind->read(d->s, d->tmp, want);
    if (got <= 0) break;
    for (int i = 0; i < got; i++) {
      const float *f = d->tmp + (size_t)i * src;
      float *o = out + (size_t)(done + i) * ch;
      if (ch == 1) {
        float sum = 0;
        for (int k = 0; k < src; k++) sum += f[k];
        o[0] = sum / src;
      } else {
        o[0] = f[0];
        o[1] = src >= 2 ? f[1] : f[0];
      }
    }
    done += got;
    if (got < want) break;
  }
  return done;
}
