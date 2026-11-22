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

// Ogg vorbis para el mezclador (decoder.h), con stb_vorbis compilado aqui
// (runtime/thirdparty/stb/stb_vorbis.c, dominio publico o MIT): solo lectura
// desde memoria.

#include <stdlib.h>
#include <string.h>
#define STB_VORBIS_NO_PUSHDATA_API
#define STB_VORBIS_NO_STDIO
#include "thirdparty/stb/stb_vorbis.c"
#include "decoder.h"

// El numero que sigue a key ("LOOPSTART=") en los comentarios, como lo busca
// RPG Maker (WebAudio._readMetaData); -1 si no esta
static long comment_number(stb_vorbis_comment *cm, const char *key) {
  size_t n = strlen(key);
  for (int i = 0; i < cm->comment_list_length; i++) {
    const char *p = strstr(cm->comment_list[i], key);
    if (p && p[n] >= '0' && p[n] <= '9') return strtol(p + n, NULL, 10);
  }
  return -1;
}

static int v_is(const uint8_t *data, size_t len) {
  return len >= 4 && memcmp(data, "OggS", 4) == 0;
}

static void *v_open(const uint8_t *data, size_t len, DecoderInfo *info) {
  int err = 0;
  stb_vorbis *v = stb_vorbis_open_memory(data, (int)len, &err, NULL);
  if (!v) return NULL;
  stb_vorbis_info vi = stb_vorbis_get_info(v);
  stb_vorbis_comment cm = stb_vorbis_get_comment(v);
  info->channels = vi.channels;
  info->rate = (int)vi.sample_rate;
  info->length = (long)stb_vorbis_stream_length_in_samples(v);
  info->loop_start = comment_number(&cm, "LOOPSTART=");
  info->loop_length = comment_number(&cm, "LOOPLENGTH=");
  return v;
}

static int v_read(void *s, float *out, int frames) {
  stb_vorbis *v = s;
  int ch = stb_vorbis_get_info(v).channels;
  return stb_vorbis_get_samples_float_interleaved(v, ch, out, frames * ch);
}

static int v_seek(void *s, long frame) {
  return stb_vorbis_seek(s, (unsigned int)frame) ? 0 : -1;
}

static void v_close(void *s) { stb_vorbis_close(s); }

const DecoderKind vorbis_decoder = { "ogg", v_is, v_open, v_read, v_seek, v_close };
