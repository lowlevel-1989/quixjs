/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// Image / HTMLImageElement: al asignar src, una imagen de archivo o de Blob se
// decodifica en el hilo de trabajo (core/worker.h) y load/error llegan cuando
// esta; un archivo con transformacion (core/url.h: uno cifrado) se lee y se
// transforma alli tambien;
// una data: URI se decodifica al momento y load llega en el siguiente ciclo
// del bucle, como en el navegador.

#include "js/js.h"
#include "gfx/surface.h"

void js_api_add_image(JSContext *ctx);
JSValue js_create_image(JSContext *ctx);

// NULL si val no es una imagen o aun no ha cargado
const Surface *js_image_get_surface(JSContext *ctx, JSValueConst val);

