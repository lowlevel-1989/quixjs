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

// WebAudio sobre el mezclador de runtime/audio: AudioContext,
// decodeAudioData (ogg vorbis), AudioBufferSourceNode, GainNode, PannerNode,
// StereoPannerNode... El grafo (fuente -> ganancia -> panner -> ...) vive en
// JS; en cada frame se calcula el volumen, el paneo y la velocidad de cada
// voz y se pasan al mezclador. Tambien new Audio() / <audio> para musica.

#include "js/js.h"

int  js_api_add_audio(JSContext *ctx, int device);
void audio_tick(JSContext *ctx);
void audio_free(JSContext *ctx);
