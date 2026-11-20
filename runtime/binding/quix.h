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
//   quix.pressKey(code, t)   mete una tecla en la cola de entrada
//                            (t: "down", "up" o "press" = down + up)
//   quix.frame               frames mostrados
//   quix.backend             "sdl" o "dummy"
//   quix.fixedStep           true si el reloj es fijo
//   quix.registerFont(f, u)  registra una fuente (lo usan FontFace y @font-face)
//   quix.hasFont(f)          true si la familia esta registrada
//   quix.version, quix.frameTime, quix.achievements(), quix.achievementUnlocked(id),
//   quix.achievementsIcon(), quix.isGpuCanvas(c), quix.collectGarbage(),
//   quix.gcPressure(): ver quix.c y la tabla de docs/usage.md
//   quix.lan: la red local para juegos, en binding/lan.c

#include "js/js.h"

// la version de quixjs: QUIX_VERSION, del archivo VERSION de la raiz
// (el Makefile genera obj/gen/version.h)
#include "version.h"

void js_api_add_quix(JSContext *ctx);

// [game] render_scale ya resuelto (1: a la resolucion del juego); lo lee
// quix.renderScale()
void quix_set_render_scale(double scale);
