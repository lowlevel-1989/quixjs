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

#include <stddef.h>

typedef struct {
  unsigned char r, g, b, a;
} Color;

// Interpreta un color CSS: #rgb, #rgba, #rrggbb, #rrggbbaa, rgb(), rgba(),
// hsl(), hsla(), "transparent" y los nombres de color habituales.
// Devuelve 0 si lo entiende, -1 si no (el canvas ignora valores invalidos).
int color_parse(const char *str, Color *out);

// Serializa como lo hace el navegador: "#rrggbb" si es opaco, si no
// "rgba(r, g, b, a)".
void color_to_string(const Color *c, char *out, size_t size);
