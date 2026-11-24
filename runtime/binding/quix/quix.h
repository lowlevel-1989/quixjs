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

// Objeto global "quix": utilidades del runtime para tests y juegos.
//   quix.exit(code)          termina el loop con ese codigo de salida
//   quix.input.pressKey(code, t)   mete una tecla en la cola de entrada
//                            (t: "down", "up" o "press" = down + up)
//   quix.frame               frames mostrados
//   quix.backend             "sdl" o "dummy"
//   quix.fixedStep           true si el reloj es fijo
//   quix.fonts.register(f, u)  registra una fuente (lo usan FontFace y @font-face)
//   quix.fonts.has(f)          true si la familia esta registrada
//   quix.version, quix.frameTime, quix.achievements.list(), quix.achievements.unlocked(id),
//   quix.achievements.icon, quix.gpu.isCanvas(c), quix.memory.collect(),
//   quix.memory.pressure(): ver quix.c y la tabla de docs/usage.md
//   quix.lan: la red local para juegos, en binding/quix/lan.c
//   quix.state: los savestates, en binding/quix/state.c
// Cada grupo esta en su parte (ver common.h).

#include "js/js.h"

// la version de quixjs: QUIX_VERSION, del archivo VERSION de la raiz
// (cmake/generated.cmake genera version.h)
#include "version.h"

void js_api_add_quix(JSContext *ctx);

// [game] render_scale ya resuelto (1: a la resolucion del juego); lo lee
// quix.gpu.renderScale
void quix_set_render_scale(double scale);

// quix.state, los savestates (binding/quix/state.c): las ranuras y lo que
// piden los combos del mando (core/main/state.c), entre dos frames. save y
// load dan "saved"/"loaded", "busy" (el motor dice que ahora no), "empty",
// "none" (sin motor), "off" (hardcore) o "error"
#define QUIX_STATE_SLOTS 4
int         quix_state_registered(void);
int         quix_state_slot(void);
void        quix_state_set_slot(int slot);
int         quix_state_can(JSContext *ctx, int load);   // can(load) del motor
long long   quix_state_time(JSContext *ctx, int slot);
const char *quix_state_save(JSContext *ctx, int slot);   // o "saving": una promesa del motor
const char *quix_state_load(JSContext *ctx, int slot);   // o "loading"
// los avisos: el de guardar (load 0) o cargar (load 1) con su resultado, y
// uno libre (el de la ranura elegida)
void        quix_state_notice(int slot, int load, const char *result);
// el aviso: el numero de la ranura en el distintivo, del color de lo que pasa
enum { QUIX_STATE_SLOT, QUIX_STATE_SAVE, QUIX_STATE_LOAD };
void        quix_state_toast(int kind, int slot, const char *title, const char *text);
// el mando y la consola: pedir guardar o cargar; si ahora no se puede, la
// peticion espera (solo una; otra se descarta) y quix_state_tick, en cada
// frame, la hace cuando se pueda. Da "saved", "loaded", "saving", "loading",
// "waiting", "discarded" o por que no
const char *quix_state_request(JSContext *ctx, int load, int slot);
void        quix_state_tick(JSContext *ctx);
int         quix_state_waiting(int *load);   // la ranura que espera (0: ninguna)

// --trace-saves: ejecuta runtime/binding/quix/trace-saves.js antes de los
// scripts del juego (runtime/core/main/start.c); trace_saves.c, la guia en
// docs/trace-saves.md
void trace_saves_start(JSContext *ctx);
