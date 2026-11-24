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

// runtime/audio/mixer/clip.c: los clips: los sonidos cargados (de un archivo, de
// memoria o de PCM) y cuantos los usan.
// Parte de runtime/audio/mixer.c (archivo indice).

#include "common.h"

// Se queda con data (malloc) en vez de copiarlo. Vale desde el thread de
// trabajo: no toca nada compartido. Si falla, libera data.
static AudioClip *clip_from_data_owned_flag(uint8_t *data, size_t len, int mapped);

// suelta el archivo: mapeado o la copia
static void drop(uint8_t *data, size_t len, int mapped) {
  if (mapped) munmap(data - (mapped - 1), len + (size_t)(mapped - 1));
  else free(data);
}

AudioClip *clip_from_data_owned(uint8_t *data, size_t len) {
  return clip_from_data_owned_flag(data, len, 0);
}

static AudioClip *clip_from_data_owned_flag(uint8_t *data, size_t len, int mapped) {
  Decoder *dec = decoder_open(data, len);
  if (!dec) {
    drop(data, len, mapped);
    return NULL;
  }
  int channels = decoder_channels(dec), rate = decoder_rate(dec);
  long length = decoder_length(dec), loop_start, loop_length;
  decoder_loop(dec, &loop_start, &loop_length);
  // corto: entero a pcm ahora (en el thread de trabajo), y fuera el archivo
  int16_t *pcm = NULL;
  int ch = channels >= 2 ? 2 : 1;
  if (length > 0 && length <= PCM_MAX_FRAMES) {
    pcm = malloc(sizeof(int16_t) * (size_t)length * ch);
    float *tmp = malloc(sizeof(float) * 1024 * 2);
    long got = 0;
    while (pcm && tmp && got < length) {
      int want = length - got < 1024 ? (int)(length - got) : 1024;
      int n = decoder_read(dec, ch, tmp, want);
      if (n <= 0) break;
      for (int i = 0; i < n * ch; i++) {
        float f = tmp[i] > 1.0f ? 1.0f : tmp[i] < -1.0f ? -1.0f : tmp[i];
        pcm[got * ch + i] = (int16_t)(f * 32767.0f);
      }
      got += n;
    }
    free(tmp);
    if (pcm && got <= 0) {
      free(pcm);
      pcm = NULL;
    } else if (pcm) {
      length = got;
    }
  }
  decoder_close(dec);

  AudioClip *c = calloc(1, sizeof(AudioClip));
  if (!c) {
    drop(data, len, mapped);
    return NULL;
  }
  if (pcm) {
    drop(data, len, mapped);
    c->pcm = pcm;
  } else {
    c->data = data;
    c->len = len;
    c->mapped = mapped;
  }
  c->loop_start = loop_start > 0 ? loop_start : 0;
  c->loop_length = loop_length > 0 ? loop_length : 0;
  c->channels = pcm ? ch : channels;
  c->rate = rate;
  c->length = length;
  c->refs = 1;
  return c;
}

// Un sonido largo (musica) se queda mapeado (mmap), no copiado: sus paginas son
// del archivo, y el kernel las suelta si falta memoria y las vuelve a leer al
// sonar. Copiado, cada musica que habia sonado seguia en la RAM del programa
// (Gunfield: 26 musicas, 91 MB). Uno corto se decodifica entero, como siempre.
AudioClip *clip_from_file(const char *path) {
  size_t len = 0, delta = 0;
  uint8_t *map = url_map(path, &len, &delta);
  if (!map) {
    // comprimido en la rom: no se puede mapear, se lee entero
    uint8_t *data = url_read_file(path, &len);
    return data ? clip_from_data_owned(data, len) : NULL;
  }
  Decoder *d = decoder_open(map, len);
  long length = d ? decoder_length(d) : 0;
  if (d) decoder_close(d);
  if (!d || length <= PCM_MAX_FRAMES) {
    // corto (o no es un sonido): como siempre, desde una copia
    uint8_t *copy = d ? malloc(len) : NULL;
    if (copy) memcpy(copy, map, len);
    munmap(map - delta, len + delta);
    return copy ? clip_from_data_owned(copy, len) : NULL;
  }
  AudioClip *c = clip_from_data_owned_flag(map, len, 1 + (int)delta);
  return c;
}

AudioClip *clip_from_data(const uint8_t *data, size_t len) {
  uint8_t *copy = malloc(len ? len : 1);
  if (!copy) return NULL;
  memcpy(copy, data, len);
  return clip_from_data_owned(copy, len);
}

AudioClip *clip_from_pcm(const float *left, const float *right, long frames, int rate) {
  if (!left || frames <= 0 || rate <= 0) return NULL;
  int ch = right ? 2 : 1;
  AudioClip *c = calloc(1, sizeof(*c));
  int16_t *pcm = malloc((size_t)frames * ch * sizeof(int16_t));
  if (!c || !pcm) {
    free(c);
    free(pcm);
    return NULL;
  }
  for (long i = 0; i < frames; i++) {
    for (int k = 0; k < ch; k++) {
      float v = (k ? right : left)[i];
      v = v > 1.0f ? 1.0f : v < -1.0f ? -1.0f : v;
      pcm[i * ch + k] = (int16_t)(v * 32767.0f);
    }
  }
  c->refs = 1;
  c->pcm = pcm;
  c->channels = ch;
  c->rate = rate;
  c->length = frames;
  return c;
}

void clip_retain(AudioClip *c) {
  if (c) c->refs++;
}

void clip_release(AudioClip *c) {
  if (c && --c->refs == 0) {
    drop(c->data, c->len, c->mapped);
    free(c->pcm);
    free(c);
  }
}

int clip_channels(const AudioClip *c) { return c->channels; }
int clip_rate(const AudioClip *c) { return c->rate; }
long clip_length(const AudioClip *c) { return c->length; }
long clip_loop_start(const AudioClip *c) { return c->loop_start; }
long clip_loop_length(const AudioClip *c) { return c->loop_length; }
