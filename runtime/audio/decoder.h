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
#pragma once

// Un sonido comprimido (o PCM en un .wav), leido desde memoria: lo que usa el
// mezclador sin saber el formato. El formato sale de los primeros bytes: ogg
// vorbis (vorbis.c), mp3 (mp3.c) o wav (wav.c). Ver README.md.

#include <stddef.h>
#include <stdint.h>

typedef struct Decoder Decoder;

// data sigue siendo de quien llama (y tiene que vivir mientras el Decoder).
// NULL si no es ninguno de los formatos o no se puede leer
Decoder *decoder_open(const uint8_t *data, size_t len);
void     decoder_close(Decoder *d);
int      decoder_channels(const Decoder *d);   // los del archivo
int      decoder_rate(const Decoder *d);
long     decoder_length(const Decoder *d);     // en frames (0 si no se sabe)
// LOOPSTART y LOOPLENGTH de los comentarios del ogg (como RPG Maker), en
// frames; -1 si no hay
void     decoder_loop(const Decoder *d, long *start, long *length);
// Los siguientes frames, interleaved con ch canales (1 o 2: se mezclan o se
// duplican los del archivo). Los frames que dio (0 al final)
int      decoder_read(Decoder *d, int ch, float *out, int frames);
int      decoder_seek(Decoder *d, long frame);

// Lo que hace cada formato (vorbis.c, mp3.c, wav.c): leen con sus canales
typedef struct {
  int  channels, rate;
  long length, loop_start, loop_length;
} DecoderInfo;
typedef struct {
  const char *name;
  int   (*is)(const uint8_t *data, size_t len);
  void *(*open)(const uint8_t *data, size_t len, DecoderInfo *info);
  int   (*read)(void *s, float *out, int frames);   // interleaved, info.channels
  int   (*seek)(void *s, long frame);
  void  (*close)(void *s);
} DecoderKind;

extern const DecoderKind vorbis_decoder, mp3_decoder, wav_decoder;
