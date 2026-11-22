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

// Buffers: bufferData y bufferSubData.
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "args.h"

// bytes de un ArrayBuffer o una vista
static uint8_t *bytes_of(JSContext *ctx, JSValueConst v, size_t *len) {
  if (JS_BufferKind(ctx, v) != JS_BUFFER_NONE) return JS_GetBufferBytes(ctx, v, len);
  // DataView
  size_t size = 0;
  JSValue bv = JS_GetPropertyStr(ctx, v, "buffer");
  JSValue bo = JS_GetPropertyStr(ctx, v, "byteOffset");
  JSValue bl = JS_GetPropertyStr(ctx, v, "byteLength");
  uint32_t o = 0, l = 0;
  JS_ToUint32(ctx, &o, bo);
  JS_ToUint32(ctx, &l, bl);
  uint8_t *p = JS_BufferKind(ctx, bv) == JS_BUFFER_ARRAY_BUFFER ? JS_GetArrayBuffer(ctx, &size, bv) : NULL;
  JS_FreeValue(ctx, bv);
  JS_FreeValue(ctx, bo);
  JS_FreeValue(ctx, bl);
  *len = p ? l : 0;
  return p ? p + o : NULL;
}

// El buffer enlazado a target (su GLObj), o NULL
static GLObj *bound_buffer(WebGLCtx *w, GLenum target) {
  if (target == GL_ARRAY_BUFFER) return obj_get(w->array_buffer);
  if (target == GL_ELEMENT_ARRAY_BUFFER) return obj_get(w->element_buffer);
  return NULL;
}

static JSValue js_gl_bufferData(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  if (argc < 3) return JS_UNDEFINED;
  GLsizeiptr size;
  if (JS_IsNumber(argv[1])) {
    size = (GLsizeiptr)n(1);
    qgl.BufferData(u(0), size, NULL, u(2));
  } else {
    size_t len = 0;
    uint8_t *p = bytes_of(ctx, argv[1], &len);
    size = (GLsizeiptr)len;
    qgl.BufferData(u(0), size, p, u(2));
  }
  GLObj *b = bound_buffer(w, u(0));
  if (b) {
    b->size = size;
    b->usage = u(2);
  }
  return JS_UNDEFINED;
}

static JSValue js_gl_bufferSubData(ARGS) {
  CTX();
  if (argc < 3) return JS_UNDEFINED;
  size_t len = 0;
  uint8_t *p = bytes_of(ctx, argv[2], &len);
  if (!p) return JS_UNDEFINED;
  // Un buffer de STREAM/DYNAMIC que se reescribe desde el principio (los
  // lotes de sprites de PIXI): se le pide memoria nueva antes (orphaning).
  // Si no, el driver espera a que la GPU acabe de usarlo (en panfrost, ~1.6
  // ms cada vez). Lo que hubiera detras de lo escrito se pierde: con esos
  // usos se vuelve a escribir antes de dibujar.
  //
  // (Probado y descartado en la RG CubeXX: orphaning tambien de los buffers
  // que se reescriben enteros, como el quad de 64 bytes de los filtros de
  // PIXI, que espera a la GPU una mediana de 4.4 ms, y con un tamano distinto
  // cada vez para que mesa no reutilice la memoria. bufferSubData bajaba de
  // ~500 a ~110 us por llamada, pero la batalla iba peor: sin esas esperas el
  // juego manda a la GPU mas de lo que acaba y el ritmo de las imagenes se
  // vuelve irregular, de ~100 a 300-600 imagenes tarde.)
  //
  // (Tambien descartado: en WebGL 2, los pequenos que three.js reescribe
  // enteros en cada imagen aunque digan STATIC. Tumble Party en la RG CubeXX
  // bajaba de 25 a 18 fps: la espera era la GPU, que no acababa antes.)
  GLObj *b = bound_buffer(w, u(0));
  if (b && n(1) == 0 && b->size >= (GLsizeiptr)len && (b->usage == GL_STREAM_DRAW || b->usage == GL_DYNAMIC_DRAW)) {
    qgl.BufferData(u(0), b->size, NULL, b->usage);
  }
  TRACE_START(tb);
  qgl.BufferSubData(u(0), (GLintptr)n(1), (GLsizeiptr)len, p);
  TRACE_END(tb, 1.0, "bufferSubData %zu bytes", len);
  return JS_UNDEFINED;
}
