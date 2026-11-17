/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// Mezclador de audio para portatiles: ogg vorbis decodificado en streaming
// (stb_vorbis) mientras suena, y mezclado en el hilo de audio de SDL.
//
// - Un AudioClip es el ogg comprimido (lo que da decodeAudioData). No se
//   decodifica entero: una musica de 3 min ocupa sus ~3 MB, no ~30 MB de PCM.
// - Cada voz que suena tiene su decodificador y su posicion.
// - Sin dispositivo de audio (backend dummy, tests) las voces avanzan con el
//   reloj del runtime, sin decodificar: onended y los bucles funcionan igual.

#include <stddef.h>
#include <stdint.h>

typedef struct AudioClip AudioClip;

// Copia los datos. NULL si no es un ogg vorbis valido.
AudioClip *clip_from_ogg(const uint8_t *data, size_t len);
// Igual, pero se queda con data (malloc) sin copiarlo; vale desde el hilo de
// trabajo (core/worker.h). Si falla, libera data
AudioClip *clip_from_ogg_owned(uint8_t *data, size_t len);
void       clip_retain(AudioClip *c);
void       clip_release(AudioClip *c);
int        clip_channels(const AudioClip *c);
int        clip_rate(const AudioClip *c);
long       clip_length(const AudioClip *c);   // en frames (muestras por canal)
// Los comentarios LOOPSTART y LOOPLENGTH del ogg, en frames (0 si no estan)
long       clip_loop_start(const AudioClip *c);
long       clip_loop_length(const AudioClip *c);

// device = 1 abre el audio de SDL; 0 = modo sin dispositivo.
int  mixer_init(int device);
void mixer_shutdown(void);
int  mixer_has_device(void);

// Devuelve el id de la voz (> 0) o -1.
int  voice_play(AudioClip *c, double offset, int loop, double loop_start, double loop_end, float rate);
void voice_set(int id, float volume, float pan, float rate, int loop);
void voice_stop(int id);
int  voice_playing(int id);   // 0 cuando termino (o no existe)
double voice_time(int id);    // segundos reproducidos dentro del clip

// Voces sonando: el bucle principal avanza frame a frame mientras haya.
int  mixer_active(void);

// Libera las voces que terminaron (hilo principal, cada frame).
void mixer_update(void);

// Un flujo de PCM ya decodificado (el audio de un video): el hilo principal
// lo va llenando y el de audio lo mezcla. Uno a la vez.
int    mixer_stream_open(int rate);
// frames estereo intercalados (float). Devuelve cuantos caben (los de mas se pierden)
int    mixer_stream_push(const float *stereo, int frames);
// segundos ya sonados (el reloj del video); sin dispositivo, el del runtime
double mixer_stream_time(void);
// frames en la cola, aun sin sonar
int    mixer_stream_queued(void);
void   mixer_stream_volume(float volume);
void   mixer_stream_pause(int paused);
void   mixer_stream_close(void);
