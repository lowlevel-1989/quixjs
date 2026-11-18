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

// Consola de debug (--debug): quixjs escucha en un socket Unix (quixjs.sock)
// y atiende comandos de texto entre frames, cuando no corre JS (el punto
// seguro). tools/console.py se conecta; tambien acepta un comando suelto:
//
//   python3 tools/console.py press Enter
//
// Cada respuesta son lineas de texto y termina con una linea "ok" o
// "err MENSAJE". Comandos: help, info, press/down/up KEY, shot FILE,
// eval JS, pause, resume, run N, quit, y los de medir: fps, gc, pacing, mem,
// types, prof y ach. La lista completa con su ayuda: help.

#include "js/js.h"
#include "backend/backend.h"
#include "gfx/surface.h"

typedef struct {
  JSContext *ctx;
  Backend   *backend;
  long      *frame;
  const Surface *(*screen)(void *user);
  void      *screen_user;
  int       *fps;          // el tope de fps ([engine] fps): el comando fps lo cambia
} DebugEnv;

int  debug_init(const char *path);
// Atiende los comandos pendientes. Devuelve 1 si piden salir.
int  debug_poll(DebugEnv *env);
int  debug_paused(void);           // el loop no avanza el juego
// El momento (ms) en que termino de presentarse una imagen: el comando pacing
// reparte los intervalos (el ritmo real en pantalla, no el del reloj del juego)
void debug_note_present(double ms);
void debug_frame_done(void);       // un frame avanzado (para "run N")
void debug_shutdown(void);
