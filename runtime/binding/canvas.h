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

#include "js/js.h"
#include "gfx/surface.h"

// HTMLCanvasElement: cada canvas tiene su framebuffer RGBA en memoria.
struct WebGLCtx;
struct Gpu2D;

typedef struct {
  Surface surface;
  JSValue context2d;   // JS_UNDEFINED hasta getContext("2d")
  JSValue contextgl;   // JS_UNDEFINED hasta getContext("webgl")
  struct WebGLCtx *webgl;   // con WebGL: su framebuffer se lee a surface
  int     dirty;       // el contexto 2d dibujo desde que se mostro (subir a la GPU)
  // hook gpu-canvas: el contexto 2d dibuja en la GPU (gfx/gpu2d.c)
  int     gpu_wanted;  // se creo dentro de una funcion con el hook
  struct Gpu2D *gpu;   // su textura; NULL hasta el primer dibujo (o si no se puede)
  int     cpu_stale;   // surface no tiene lo ultimo dibujado en la GPU
} CanvasData;

void js_api_add_canvas(JSContext *ctx);

JSValue     js_create_canvas(JSContext *ctx);
CanvasData *js_canvas_get(JSContext *ctx, JSValueConst val);   // NULL si no es un canvas

void    js_api_add_context2d(JSContext *ctx);   // CanvasRenderingContext2D, ImageData...
JSValue js_create_canvas_context(JSContext *ctx, JSValueConst canvas);
void    context2d_reset(JSContext *ctx, JSValueConst context2d);

// Superficie de un origen de drawImage/createPattern (canvas o imagen).
const Surface *js_get_image_source(JSContext *ctx, JSValueConst val);

// gpu-canvas. Los canvas creados mientras canvas_gpu_scope > 0 dibujan en la
// GPU (lo usa el hook gpu-canvas de [hooks]).
void canvas_gpu_scope(int delta);
// El Gpu2D del canvas si debe dibujar en la GPU (lo crea la primera vez); NULL
// si va por software
struct Gpu2D *canvas_gpu(CanvasData *cd);
// Deja surface con lo ultimo dibujado (lectura); sigue en la GPU
void canvas_sync_cpu(CanvasData *cd);
// Pasa el canvas a software para siempre (una operacion que la GPU no hace)
void canvas_leave_gpu(CanvasData *cd, const char *why);
