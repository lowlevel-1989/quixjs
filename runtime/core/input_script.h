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

// --script <archivo>: guion de entrada para automatizar pruebas de un juego.
// Una accion por linea, en el frame indicado (# comentarios):
//
//   120  press Enter        pulsa y suelta (keydown en este frame, keyup en el siguiente)
//   200  down  ArrowDown    mantiene pulsada
//   230  up    ArrowDown    suelta
//   300  shot  title.png    captura del frame (PNG)
//   400  log   texto        escribe el texto en la salida
//   900  exit  0            termina con ese codigo
//
// Los nombres de tecla son KeyboardEvent.code (Enter, KeyZ, ArrowDown...).
// Las teclas llegan igual que las del teclado real.

#include "backend/backend.h"
#include "gfx/surface.h"

int  input_script_load(const char *path);
// Ejecuta las acciones de este frame. shot_screen devuelve la pantalla actual
// (puede ser NULL). Devuelve 1 si el guion pide salir (*exit_code).
int  input_script_run(long frame, Backend *backend,
                      const Surface *(*shot_screen)(void *user), void *user, int *exit_code);
int  input_script_active(void);
void input_script_free(void);
