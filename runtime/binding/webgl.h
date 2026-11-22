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

// WebGL 1 sobre OpenGL ES 2.0 (runtime/gfx/gl.c).
//
// Cada canvas con contexto WebGL dibuja en su propio framebuffer (textura RGBA
// + depth/stencil). bindFramebuffer(null) es ese framebuffer. Cuando hace
// falta ver el resultado (pantalla, drawImage, capturas) se lee con
// glReadPixels a la superficie del canvas (webgl_canvas_sync).
//
// Todos los contextos WebGL comparten el mismo contexto GL: en la practica un
// juego usa uno (PIXI crea otro de prueba y lo descarta).

#include "js/js.h"
#include "binding/canvas.h"

int     webgl_enabled(void);
void    webgl_set_enabled(int enabled, int headless);
void    js_api_add_webgl(JSContext *ctx);
// version2: getContext("webgl2"); NULL si el driver no es OpenGL ES 3.x
JSValue js_create_webgl_context(JSContext *ctx, JSValueConst canvas, JSValueConst attrs, int version2);
void    webgl_canvas_sync(CanvasData *cd);
void    webgl_canvas_resized(CanvasData *cd);
// Textura del canvas para presentarla sin leerla (0 si no hay)
unsigned webgl_canvas_texture(CanvasData *cd, int *w, int *h, int *premultiplied);
// Como webgl_canvas_texture, pero la textura se entrega (se muestra mas tarde)
// y el canvas sigue dibujando en otra. buffers: 2 (la muestra el thread que
// presenta) o 3 (sin el thread: el driver puede mostrarla en la siguiente
// entrega, gfx/drivers/). 0 si no se puede (preserveDrawingBuffer):
// entonces se usa webgl_canvas_texture.
unsigned webgl_canvas_handoff(CanvasData *cd, int buffers, int *w, int *h, int *premultiplied);
