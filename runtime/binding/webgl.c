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

// WebGL 1 sobre OpenGL ES 2.0. Ver webgl.h.
//
// El codigo esta en webgl/, partido por temas para que se pueda leer; este
// archivo los une en el orden de siempre y se compila como uno solo. Asi el
// compilador ve lo mismo que cuando era un solo archivo y genera el mismo
// codigo: mete dentro (inline) las funciones pequenas de una parte en otra,
// y los metodos de WebGL se llaman cientos de veces por imagen. Una parte
// nueva se agrega aqui, en su sitio (despues de lo que usa). Ver
// webgl/README.md.

#include "webgl/common.h"
#include "webgl/canvas.c"
#include "webgl/classes.c"
#include "webgl/args.h"
#include "webgl/state.c"
#include "webgl/objects.c"
#include "webgl/shaders.c"
#include "webgl/uniforms.c"
#include "webgl/buffers.c"
#include "webgl/textures.c"
#include "webgl/evict.c"
#include "webgl/queries.c"
#include "webgl/extensions.c"
#include "webgl/webgl2.c"
#include "webgl/api.c"
