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

// Canvas 2d en la GPU. Ver gpu2d.h.
//
// El codigo esta en gpu2d/, partido por temas para que se pueda leer; este
// archivo los une y se compila como uno solo. Asi el compilador ve todo junto
// y mete dentro (inline) las funciones pequenas de una parte en otra (aqui
// esta cada draw call del canvas en la GPU). Una parte nueva se agrega aqui,
// en su sitio (despues de lo que usa). Ver gpu2d/README.md.

#include "gpu2d/common.h"
#include "gpu2d/program.c"
#include "gpu2d/target.c"
#include "gpu2d/batch.c"
#include "gpu2d/draw.c"
#include "gpu2d/texture.c"
