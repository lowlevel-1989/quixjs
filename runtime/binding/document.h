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

// document y los elementos del DOM minimo (ver document.c).

#include "js/js.h"
#include "canvas.h"

JSValue js_create_document(JSContext *ctx);
JSValue js_create_element(JSContext *ctx, const char *tag);

// Prototipo comun de los elementos (appendChild, setAttribute, eventos...)
JSValue js_element_proto(JSContext *ctx);
// Propiedades de instancia (tagName, style, childNodes...)
void element_setup(JSContext *ctx, JSValueConst elem, const char *tag);

void register_element_with_id(JSContext *ctx, JSValue obj, const char *id);
void free_element_registry(JSContext *ctx);

// Canvas visibles anadidos al documento, en orden (el primero va debajo).
int document_screen_canvases(JSContext *ctx, CanvasData **out, int max);
