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

// Decodificacion de imagenes (PNG, JPG...) con SDL_image a una Surface, y
// guardado en PNG para las capturas. No necesita ventana.

#include <stddef.h>
#include <stdint.h>
#include "surface.h"

int image_load_file(const char *path, Surface *out);
int image_load_memory(const uint8_t *data, size_t len, Surface *out);
int image_save_png(const uint8_t *rgba, int w, int h, const char *path);
// Decodifica a pixeles RGBA sueltos (malloc; el que llama los libera): de un
// archivo (path) o de memoria (data, len). No toca las estadisticas ni nada
// compartido, asi que vale desde otro thread (la decodificacion en segundo plano)
int image_decode_pixels(const uint8_t *data, size_t len, const char *path, uint8_t **px, int *w, int *h);
