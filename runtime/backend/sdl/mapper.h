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

// Asignar el mando: el lado SDL de backend_capture_input y
// backend_save_controller (ver backend/backend.h y mapper.c)

#include <SDL2/SDL.h>
#include <stddef.h>

void mapper_load(const char *path);         // controller.map, antes de SDL_Init
// un boton del mapeo de fabrica (sin lo de Controls), del mando crudo: 1, 0 o
// -1 si no se sabe. Para los combos del sistema (input.c)
int  mapper_base_pressed(SDL_GameController *gc, const char *role);
void mapper_reload(const char *path);       // despues de guardar (CAPTURE_STOP)
int  mapper_from_file(SDL_JoystickGUID guid); // ese mando tiene linea en controller.map?
void mapper_event(const SDL_Event *e);      // cada evento, mientras se asigna
int  mapper_capture(int step, char *out, size_t room);
// pairs: "a:b0,dpup:h0.1,..." (controller.map) y, con @ delante, las
// acciones de [controls.keys] ("@Torch:b11"), que van a controller.keys
int  mapper_save(const char *path, const char *pairs);
// [controls.keys]: con cada evento de un mando, las acciones de
// controller.keys que pulsa (down 1) o suelta (down 0)
void mapper_key_event(const SDL_Event *e, void (*cb)(void *ctx, const char *name, int down), void *ctx);
