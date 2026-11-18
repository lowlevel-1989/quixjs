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

// Video WebM (VP8 / VP9; audio Opus: una pista Vorbis no suena): lo que
// reproduce el elemento <video> (RPG Maker MV: los "Play Movie" de los
// eventos).
//
// Con librerias, no desde cero:
//   nestegg  (runtime/thirdparty/nestegg) lee el contenedor WebM
//   libvpx   decodifica VP8 / VP9
//   libopus  decodifica Opus (el audio de los WebM de RPG Maker)
// El audio va al mezclador como un flujo (mixer_stream_*) y la imagen sigue
// su reloj. Se decodifica en un thread propio, que deja unas pocas imagenes
// listas por delante; el thread principal muestra la que toca.
//
// Sin libvpx y libopus al compilar (make VIDEO=0) no hay video: video_open
// devuelve NULL y el <video> dice que no lo puede reproducir.

#include "gfx/surface.h"

typedef struct Video Video;

// 1 si quixjs se compilo con video
int    video_supported(void);
// Abre el archivo. NULL si no es un WebM que se pueda reproducir.
Video *video_open(const char *path);
void   video_close(Video *v);
int    video_width(const Video *v);
int    video_height(const Video *v);
double video_duration(const Video *v);   // segundos (0 si no se sabe)

void   video_play(Video *v);
void   video_pause(Video *v);
void   video_set_volume(Video *v, float volume);
// Avanza hasta su reloj. Si hay una imagen nueva la deja en frame (RGBA) y
// devuelve 1.
int    video_update(Video *v, Surface *frame);
int    video_ended(const Video *v);
double video_time(const Video *v);       // segundos reproducidos
long   video_decoded_frames(const Video *v);
