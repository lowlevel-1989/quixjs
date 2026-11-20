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

// La capa de motor (js.h) sobre la API en C de JavaScriptCore.
//
// El codigo esta en jsc/, partido por temas para que se pueda leer; este
// archivo los une y se compila como uno solo. Asi el compilador ve lo mismo
// que cuando era un solo archivo: mete dentro (inline) las funciones pequenas
// de una parte en otra, y por aqui pasa cada llamada de JS a C. Compiladas
// aparte, esas llamadas eran ~4 % mas lentas. Una parte nueva se agrega aqui.
// Ver js/README.md.

#include "jsc/jsc.h"
#include "jsc/context.c"
#include "jsc/value.c"
#include "jsc/property.c"
#include "jsc/error.c"
#include "jsc/function.c"
#include "jsc/class.c"
#include "jsc/eval.c"
#include "jsc/typedarray.c"
#include "jsc/profiler.c"
