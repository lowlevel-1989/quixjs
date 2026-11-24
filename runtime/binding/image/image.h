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

// Image / HTMLImageElement: al asignar src, una imagen de archivo o de Blob se
// decodifica en el thread de trabajo (core/worker.h) y load/error llegan cuando
// esta; un archivo con transformacion (core/url/url.h: uno cifrado) se lee y se
// transforma alli tambien;
// una data: URI se decodifica al momento y load llega en el siguiente ciclo
// del loop, como en el navegador.

#include "js/js.h"
#include "gfx/soft/surface.h"

void js_api_add_image(JSContext *ctx);
JSValue js_create_image(JSContext *ctx);

// Los pixeles de una imagen, NULL si val no es una imagen o aun no ha cargado.
// Para dibujarla en un canvas 2d: desde entonces sus pixeles se quedan
const Surface *js_image_get_surface(JSContext *ctx, JSValueConst val);
// Para subirla a WebGL (no la marca como usada en 2d)
const Surface *js_image_get_surface_gpu(JSContext *ctx, JSValueConst val);
// El tamano de la imagen para el juego (naturalWidth/naturalHeight): con
// [game] image_scale sus pixeles pueden ser menos (gfx/soft/image_scale.h). 0 si
// val es una imagen cargada
int js_image_natural_size(JSContext *ctx, JSValueConst val, int *w, int *h);
// Ya esta en una textura de WebGL
void js_image_uploaded(JSContext *ctx, JSValueConst val);
// Con poca RAM: suelta los pixeles de la CPU de las imagenes que ya estan en
// una textura y nunca se dibujaron en 2d (se decodifican otra vez si hacen
// falta). Devuelve los bytes soltados y en count cuantas
size_t js_image_release_uploaded(int *count);
// Los pixeles de una imagen soltada, decodificados en segundo plano y
// entregados a deliver (thread principal). -1: no hace falta (tiene pixeles)
// o no se puede
int js_image_decode_pixels(JSContext *ctx, JSValueConst val, int *w, int *h,
                           void (*deliver)(void *user, uint8_t *px, int w, int h), void *user);

