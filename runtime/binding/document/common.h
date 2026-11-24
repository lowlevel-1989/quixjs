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

// runtime/binding/document/common.h: lo que comparten las partes de
// runtime/binding/document.c: los elementos con id y el estado del DOM.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../canvas.h"
#include "../image/image.h"
#include "../events.h"
#include "../document.h"
#include "../script.h"
#include "../webgl.h"
#include "core/app_context.h"

// Un DOM minimo: lo justo para que las librerias encuentren body, puedan
// crear elementos, agregar el canvas y registrar eventos. Solo los canvas
// tienen comportamiento real; el resto son objetos con las propiedades
// habituales.

typedef struct {
  char     *id;
  JSValue  obj;
} ElementEntry;

static ElementEntry *element_registry       = NULL;
static int           element_registry_count = 0;

static JSValue element_proto = { 0 };
static JSValue screen_list   = { 0 };   // canvas agregados al documento (en orden)
static int     document_ready = 0;
