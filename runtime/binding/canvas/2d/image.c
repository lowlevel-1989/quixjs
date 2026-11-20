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

// drawImage, getImageData, putImageData, createImageData

#include <math.h>
#include <string.h>
#include "context2d.h"
#include "backend/backend.h"
#include "binding/image.h"

static JSValue imagedata_proto = { 0 };
static int     imagedata_ready = 0;

// Crea un ImageData { width, height, data: Uint8ClampedArray } y devuelve
// el puntero a los bytes en *pixels.
static JSValue imagedata_new(JSContext *ctx, int w, int h, uint8_t **pixels) {
  if (w <= 0 || h <= 0) return JS_ThrowRangeError(ctx, "The source width/height is 0");
  if ((double)w * h * 4 > 512.0 * 1024 * 1024) return JS_ThrowRangeError(ctx, "ImageData too large");

  JSValue len = JS_NewInt64(ctx, (int64_t)w * h * 4);
  JSValue data = JS_NewTypedArray(ctx, 1, &len, JS_TYPED_ARRAY_UINT8C);
  JS_FreeValue(ctx, len);
  if (JS_IsException(data)) return data;

  size_t off = 0, blen = 0, bpe = 0, size = 0;
  JSValue buf = JS_GetTypedArrayBuffer(ctx, data, &off, &blen, &bpe);
  uint8_t *p = JS_GetArrayBuffer(ctx, &size, buf);
  JS_FreeValue(ctx, buf);
  if (!p) {
    JS_FreeValue(ctx, data);
    return JS_EXCEPTION;
  }
  if (pixels) *pixels = p + off;

  JSValue obj = JS_NewObjectProto(ctx, imagedata_proto);
  JS_SetPropertyStr(ctx, obj, "width", JS_NewInt32(ctx, w));
  JS_SetPropertyStr(ctx, obj, "height", JS_NewInt32(ctx, h));
  JS_SetPropertyStr(ctx, obj, "data", data);
  return obj;
}

// Lee un ImageData (o cualquier objeto con width, height y data)
static int imagedata_read(JSContext *ctx, JSValueConst obj, int *w, int *h, uint8_t **pixels) {
  JSValue vw = JS_GetPropertyStr(ctx, obj, "width");
  JSValue vh = JS_GetPropertyStr(ctx, obj, "height");
  JSValue data = JS_GetPropertyStr(ctx, obj, "data");
  int ok = JS_ToInt32(ctx, w, vw) == 0 && JS_ToInt32(ctx, h, vh) == 0;
  JS_FreeValue(ctx, vw);
  JS_FreeValue(ctx, vh);

  *pixels = NULL;
  if (ok) {
    size_t off = 0, blen = 0, bpe = 0, size = 0;
    JSValue buf = JS_GetTypedArrayBuffer(ctx, data, &off, &blen, &bpe);
    if (!JS_IsException(buf)) {
      uint8_t *p = JS_GetArrayBuffer(ctx, &size, buf);
      if (p && blen >= (size_t)(*w) * (*h) * 4) *pixels = p + off;
    }
    JS_FreeValue(ctx, buf);
  }
  JS_FreeValue(ctx, data);
  return *pixels ? 0 : -1;
}

// new ImageData(width, height) / new ImageData(data, width, height)
static JSValue js_imagedata_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  if (argc >= 2 && JS_IsObject(argv[0])) {
    int w, h = 0;
    if (JS_ToInt32(ctx, &w, argv[1])) return JS_EXCEPTION;
    size_t off = 0, blen = 0, bpe = 0;
    JSValue buf = JS_GetTypedArrayBuffer(ctx, argv[0], &off, &blen, &bpe);
    if (JS_IsException(buf)) return buf;
    JS_FreeValue(ctx, buf);
    if (w <= 0 || blen % (size_t)(w * 4) != 0) return JS_ThrowRangeError(ctx, "ImageData: invalid data length");
    h = (int)(blen / (w * 4));
    if (argc > 2 && JS_ToInt32(ctx, &h, argv[2])) return JS_EXCEPTION;
    JSValue obj = JS_NewObjectProto(ctx, imagedata_proto);
    JS_SetPropertyStr(ctx, obj, "width", JS_NewInt32(ctx, w));
    JS_SetPropertyStr(ctx, obj, "height", JS_NewInt32(ctx, h));
    JS_SetPropertyStr(ctx, obj, "data", JS_DupValue(ctx, argv[0]));
    return obj;
  }
  int w = 0, h = 0;
  if (argc < 2 || JS_ToInt32(ctx, &w, argv[0]) || JS_ToInt32(ctx, &h, argv[1]))
    return JS_ThrowTypeError(ctx, "ImageData: width and height required");
  return imagedata_new(ctx, w, h, NULL);
}

void js_ctx2d_image_init(JSContext *ctx) {
  if (imagedata_ready) return;
  imagedata_proto = JS_NewObject(ctx);
  JSValue ctor = JS_NewCFunction2(ctx, js_imagedata_ctor, "ImageData", 2, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, ctor, imagedata_proto);
  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, "ImageData", ctor);
  JS_FreeValue(ctx, global);
  // imagedata_proto queda vivo mientras viva el contexto (lo referencia ImageData)
  JS_FreeValue(ctx, imagedata_proto);
  imagedata_ready = 1;
}

static int read_numbers(JSContext *ctx, JSValueConst *argv, double *out, int n) {
  for (int i = 0; i < n; i++) {
    if (JS_ToFloat64(ctx, &out[i], argv[i])) return -1;
    if (!isfinite(out[i])) return 1;
  }
  return 0;
}

JSValue js_ctx2d_drawImage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  if (argc != 3 && argc != 5 && argc != 9) {
    return JS_ThrowTypeError(ctx, "drawImage: 3, 5 or 9 arguments required");
  }

  const Surface *src = js_get_image_source(ctx, argv[0]);
  if (!src || src->w == 0 || src->h == 0) return JS_UNDEFINED;   // sin cargar: no dibuja
  Surface *dst = context2d_surface(ctx, c);
  if (!dst) return JS_UNDEFINED;

  double v[8];
  int r = read_numbers(ctx, argv + 1, v, argc - 1);
  if (r < 0) return JS_EXCEPTION;
  if (r > 0) return JS_UNDEFINED;

  // el tamano para el juego: el de una imagen reducida ([game] image_scale)
  // es el de siempre, y sus pixeles, menos
  int lw = src->w, lh = src->h;
  js_image_natural_size(ctx, argv[0], &lw, &lh);
  double sx = 0, sy = 0, sw = lw, sh = lh;
  double dx, dy, dw = lw, dh = lh;
  if (argc == 3) {
    dx = v[0]; dy = v[1];
  } else if (argc == 5) {
    dx = v[0]; dy = v[1]; dw = v[2]; dh = v[3];
  } else {
    sx = v[0]; sy = v[1]; sw = v[2]; sh = v[3];
    dx = v[4]; dy = v[5]; dw = v[6]; dh = v[7];
  }

  if (lw != src->w || lh != src->h) {
    // el rectangulo de origen, a los pixeles de la imagen reducida
    double kx = (double)src->w / lw, ky = (double)src->h / lh;
    sx *= kx;
    sw *= kx;
    sy *= ky;
    sh *= ky;
  }

  DrawParams dp;
  context2d_draw_params(c, &dp);

  if (src == dst) {
    // dibujar un canvas sobre si mismo: se usa una copia
    Surface copy;
    if (surface_copy(&copy, src) != 0) return JS_ThrowOutOfMemory(ctx);
    raster_draw_image(dst, &copy, sx, sy, sw, sh, dx, dy, dw, dh, &CTX2D_STATE(c)->transform, &dp);
    surface_free(&copy);
  } else {
    raster_draw_image(dst, src, sx, sy, sw, sh, dx, dy, dw, dh, &CTX2D_STATE(c)->transform, &dp);
  }
  return JS_UNDEFINED;
}

JSValue js_ctx2d_getImageData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  if (argc < 4) return JS_ThrowTypeError(ctx, "getImageData: 4 arguments required");

  int x, y, w, h;
  if (JS_ToInt32(ctx, &x, argv[0]) || JS_ToInt32(ctx, &y, argv[1]) ||
      JS_ToInt32(ctx, &w, argv[2]) || JS_ToInt32(ctx, &h, argv[3])) return JS_EXCEPTION;
  if (w < 0) { x += w; w = -w; }
  if (h < 0) { y += h; h = -h; }

  uint8_t *out;
  JSValue obj = imagedata_new(ctx, w, h, &out);
  if (JS_IsException(obj)) return obj;

  Surface *s = context2d_surface(ctx, c);
  if (!s || !s->px) return obj;
  for (int j = 0; j < h; j++) {
    int sy = y + j;
    if (sy < 0 || sy >= s->h) continue;
    for (int i = 0; i < w; i++) {
      int sx = x + i;
      if (sx < 0 || sx >= s->w) continue;
      memcpy(out + ((size_t)j * w + i) * 4, s->px + ((size_t)sy * s->w + sx) * 4, 4);
    }
  }
  return obj;
}

JSValue js_ctx2d_putImageData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  if (argc < 3) return JS_ThrowTypeError(ctx, "putImageData: 3 arguments required");

  int w, h;
  uint8_t *in;
  if (imagedata_read(ctx, argv[0], &w, &h, &in) != 0) return JS_ThrowTypeError(ctx, "putImageData: invalid ImageData");

  int dx, dy;
  if (JS_ToInt32(ctx, &dx, argv[1]) || JS_ToInt32(ctx, &dy, argv[2])) return JS_EXCEPTION;

  // rectangulo sucio opcional
  int rx = 0, ry = 0, rw = w, rh = h;
  if (argc >= 7) {
    if (JS_ToInt32(ctx, &rx, argv[3]) || JS_ToInt32(ctx, &ry, argv[4]) ||
        JS_ToInt32(ctx, &rw, argv[5]) || JS_ToInt32(ctx, &rh, argv[6])) return JS_EXCEPTION;
    if (rw < 0) { rx += rw; rw = -rw; }
    if (rh < 0) { ry += rh; rh = -rh; }
    if (rx < 0) { rw += rx; rx = 0; }
    if (ry < 0) { rh += ry; ry = 0; }
    if (rx + rw > w) rw = w - rx;
    if (ry + rh > h) rh = h - ry;
  }

  Surface *s = context2d_surface(ctx, c);
  if (!s || !s->px) return JS_UNDEFINED;
  // sin transformacion, sin globalAlpha y sin composicion: copia directa
  for (int j = ry; j < ry + rh; j++) {
    int ty = dy + j;
    if (ty < 0 || ty >= s->h) continue;
    for (int i = rx; i < rx + rw; i++) {
      int tx = dx + i;
      if (tx < 0 || tx >= s->w) continue;
      memcpy(s->px + ((size_t)ty * s->w + tx) * 4, in + ((size_t)j * w + i) * 4, 4);
    }
  }
  return JS_UNDEFINED;
}

JSValue js_ctx2d_createImageData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (!context2d_get(ctx, this_val)) return JS_EXCEPTION;
  int w = 0, h = 0;
  if (argc == 1 && JS_IsObject(argv[0])) {
    JSValue vw = JS_GetPropertyStr(ctx, argv[0], "width");
    JSValue vh = JS_GetPropertyStr(ctx, argv[0], "height");
    JS_ToInt32(ctx, &w, vw);
    JS_ToInt32(ctx, &h, vh);
    JS_FreeValue(ctx, vw);
    JS_FreeValue(ctx, vh);
  } else if (argc >= 2) {
    if (JS_ToInt32(ctx, &w, argv[0]) || JS_ToInt32(ctx, &h, argv[1])) return JS_EXCEPTION;
  }
  if (w < 0) w = -w;
  if (h < 0) h = -h;
  return imagedata_new(ctx, w, h, NULL);
}
