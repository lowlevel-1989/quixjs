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

// runtime/audio/mixer/common.h: lo que comparten las partes de
// runtime/audio/mixer.c: los clips, las voces y el flujo de PCM, y su estado.

#pragma once

#include <SDL2/SDL.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "core/timer.h"
#include "core/url/url.h"
#include "../mixer.h"

#include "../decoder.h"

#define MAX_VOICES   32
#define DEC_FRAMES   2048   // frames decodificados de una vez por voz

// Los sonidos cortos (efectos) se decodifican enteros al cargarlos: abrir el
// decodificador de un ogg (sus cabeceras y tablas) eran ~4 ms en cada play,
// en el thread principal (los aleteos de los cuervos, los pasos...)
#ifndef PCM_MAX_FRAMES
#define PCM_MAX_FRAMES (44100 * 3)
#endif

struct AudioClip {
  int      refs;          // se toca solo desde el thread principal
  uint8_t *data;          // el archivo comprimido (ogg, mp3) o el wav (NULL si ya esta en pcm)
  size_t   len;
  int      mapped;        // data esta mapeada (mmap), no copiada: 1 + lo que empieza
                          // antes el mapeo (url_map: el archivo dentro de la rom)
  int16_t *pcm;           // el sonido entero decodificado (interleaved) o NULL
  int      channels;
  int      rate;
  long     length;
  long     loop_start;    // los comentarios LOOPSTART / LOOPLENGTH (0 si no hay)
  long     loop_length;
};

typedef struct {
  int         id;
  int         active;     // la usa el thread de audio
  int         ended;      // la marca el thread de audio, la libera el principal
  AudioClip  *clip;
  Decoder    *dec;
  float       volume, pan, rate;
  int         loop;
  long        loop_start, loop_end;   // en frames del clip
  // decodificado pendiente (interleaved, siempre 2 canales)
  float       buf[(DEC_FRAMES + 1) * 2];
  int         buf_len;    // frames en buf
  double      pos;        // posicion fraccionaria dentro de buf
  long        buf_start;  // frame del clip que corresponde a buf[0]
  // modo sin dispositivo
  double      start_ms;
  double      offset;
} Voice;

static Voice              voices[MAX_VOICES];

// el flujo de PCM (video): cola circular de frames estereo
#define STREAM_FRAMES (48000 * 2)   // 2 s a 48 kHz
static struct {
  int     open, paused;
  int     rate;
  float   volume;
  float   buf[STREAM_FRAMES * 2];
  long    head, tail;               // en frames; tail - head = en cola
  double  pos;                      // posicion fraccionaria (remuestreo)
  long    played;                   // frames de origen ya sonados
  double  start_ms, paused_ms;      // sin dispositivo
} stream_mix;
static int                next_id = 1;
static SDL_AudioDeviceID  device = 0;
static int                out_rate = 44100;
static int                ready = 0;
