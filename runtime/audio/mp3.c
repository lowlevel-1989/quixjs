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

// MP3 para el mezclador (decoder.h), con minimp3 (runtime/thirdparty/minimp3,
// CC0): https://github.com/lieff/minimp3/tree/142823796ab8efa55e1bf11406028f3be77ecceb
// Al abrir recorre el archivo una vez (sus frames, para saber el largo y
// poder saltar a un punto); despues decodifica mientras suena.

#include <stdlib.h>
#include <string.h>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_NO_STDIO
#include "thirdparty/minimp3/minimp3_ex.h"
#include "decoder.h"

// ID3v2 delante, o un frame de MPEG audio capa 3 (11 bits de sincronia)
static int m_is(const uint8_t *data, size_t len) {
  if (len >= 3 && memcmp(data, "ID3", 3) == 0) return 1;
  return len >= 2 && data[0] == 0xff && (data[1] & 0xe6) == 0xe2;
}

static void *m_open(const uint8_t *data, size_t len, DecoderInfo *info) {
  mp3dec_ex_t *d = calloc(1, sizeof *d);
  if (!d) return NULL;
  if (mp3dec_ex_open_buf(d, data, len, MP3D_SEEK_TO_SAMPLE) != 0 || d->info.channels < 1) {
    mp3dec_ex_close(d);
    free(d);
    return NULL;
  }
  info->channels = d->info.channels;
  info->rate = d->info.hz;
  info->length = (long)(d->samples / (uint64_t)d->info.channels);
  return d;
}

static int m_read(void *s, float *out, int frames) {
  mp3dec_ex_t *d = s;
  size_t got = mp3dec_ex_read(d, out, (size_t)frames * (size_t)d->info.channels);
  return (int)(got / (size_t)d->info.channels);
}

static int m_seek(void *s, long frame) {
  mp3dec_ex_t *d = s;
  return mp3dec_ex_seek(d, (uint64_t)frame * (uint64_t)d->info.channels);
}

static void m_close(void *s) {
  mp3dec_ex_close(s);
  free(s);
}

const DecoderKind mp3_decoder = { "mp3", m_is, m_open, m_read, m_seek, m_close };
