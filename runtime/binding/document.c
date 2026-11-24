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

// document: un DOM minimo, lo justo para que las librerias encuentren body,
// puedan crear elementos, agregar el canvas y registrar eventos. Solo los
// canvas tienen comportamiento real. Ver document.h.
//
// El codigo esta en document/, partido por temas para que se pueda leer; este
// archivo los une y se compila como uno solo. Una parte nueva se agrega aqui,
// en su sitio (despues de lo que usa). Ver document/README.md.

#include "document/common.h"
#include "document/screen.c"
#include "document/element.c"
#include "document/query.c"
#include "document/dom.c"
