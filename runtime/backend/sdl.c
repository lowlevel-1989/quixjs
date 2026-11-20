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

// El backend sdl: la ventana (SDL2), mostrar cada imagen con OpenGL ES 2
// (las capas de los canvas, el contador de --fps y los avisos), el thread que
// presenta ([engine] present_ahead), el teclado y los mandos.
//
// El codigo esta en sdl/, partido por temas para que se pueda leer; este
// archivo los une en el orden de siempre y se compila como uno solo. Asi el
// compilador ve lo mismo que cuando era un solo archivo y genera el mismo
// codigo (aqui esta el camino de cada imagen hasta la pantalla). Una parte
// nueva se agrega aqui, en su sitio (despues de lo que usa). Ver
// sdl/README.md.

#include "sdl/common.h"
#include "sdl/window.c"
#include "sdl/input.c"
#include "sdl/overlay.c"
#include "sdl/draw.c"
#include "sdl/thread.c"
#include "sdl/present.c"
#include "sdl/gamepads.c"
#include "sdl/api.c"
