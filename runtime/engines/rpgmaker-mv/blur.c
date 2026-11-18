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

// blur.c: la parte nativa del hook blur-native.js. Ver ese archivo.

#include <stdlib.h>
#include "binding/canvas.h"
#include "binding/canvas/2d/context2d.h"
#include "natives.h"
#include "core/trace.h"

// Una pasada de Bitmap.prototype.blur, con la misma cuenta que la original:
// una copia de w+2 x h+2 con los bordes repetidos (las esquinas vacias), y
// sobre negro, la suma ('lighter') de esa copia movida a las 9 posiciones de
// 3x3 con globalAlpha 1/9. Cada pixel queda opaco con la media de los 9 de
// alrededor (sus colores por su alfa).
static int blur_pass(uint8_t *px, int w, int h) {
  int tw = w + 2, th = h + 2;
  // la copia con bordes, premultiplicada (color * alfa / 255), 3 canales
  uint16_t *t = calloc((size_t)tw * th * 3, sizeof *t);
  // la suma de 3 en horizontal de cada fila de la copia
  uint16_t *hs = malloc((size_t)w * th * 3 * sizeof *hs);
  if (!t || !hs) {
    free(t);
    free(hs);
    return -1;
  }
#define PUT(tx, ty, sx, sy) do { \
    const uint8_t *s_ = px + ((size_t)(sy) * w + (sx)) * 4; \
    uint16_t *d_ = t + ((size_t)(ty) * tw + (tx)) * 3; \
    unsigned a_ = s_[3]; \
    if (a_ == 255) { d_[0] = s_[0]; d_[1] = s_[1]; d_[2] = s_[2]; } \
    else if (a_) { \
      d_[0] = (uint16_t)((s_[0] * a_ + 127) / 255); \
      d_[1] = (uint16_t)((s_[1] * a_ + 127) / 255); \
      d_[2] = (uint16_t)((s_[2] * a_ + 127) / 255); \
    } \
  } while (0)
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) PUT(x + 1, y + 1, x, y);
    PUT(0, y + 1, 0, y);           // columna izquierda
    PUT(w + 1, y + 1, w - 1, y);   // columna derecha
  }
  for (int x = 0; x < w; x++) {
    PUT(x + 1, 0, x, 0);           // fila de arriba
    PUT(x + 1, h + 1, x, h - 1);   // fila de abajo
  }
#undef PUT
  for (int y = 0; y < th; y++) {
    const uint16_t *r = t + (size_t)y * tw * 3;
    uint16_t *o = hs + (size_t)y * w * 3;
    for (int x = 0; x < w; x++) {
      for (int c = 0; c < 3; c++) o[x * 3 + c] = r[x * 3 + c] + r[(x + 1) * 3 + c] + r[(x + 2) * 3 + c];
    }
  }
  for (int y = 0; y < h; y++) {
    const uint16_t *r0 = hs + (size_t)y * w * 3, *r1 = r0 + (size_t)w * 3, *r2 = r1 + (size_t)w * 3;
    uint8_t *d = px + (size_t)y * w * 4;
    for (int x = 0; x < w; x++) {
      for (int c = 0; c < 3; c++) {
        unsigned s = (unsigned)r0[x * 3 + c] + r1[x * 3 + c] + r2[x * 3 + c];   // hasta 9 * 255
        d[x * 4 + c] = (uint8_t)((s * 7282 + 32768) >> 16);                    // s / 9 redondeado (65536 / 9)
      }
      d[x * 4 + 3] = 255;
    }
  }
  free(t);
  free(hs);
  return 0;
}

// Amplia src (cualquier tamano) a dst con interpolacion bilineal (los
// centros de los pixeles alineados, como drawImage con imageSmoothing)
static void upscale_bilinear(uint8_t *dst, int dw, int dh, const uint8_t *src, int sw, int sh) {
  for (int y = 0; y < dh; y++) {
    double fy = (y + 0.5) * sh / dh - 0.5;
    if (fy < 0) fy = 0;
    int y0 = (int)fy, y1 = y0 + 1 < sh ? y0 + 1 : y0;
    int wy = (int)((fy - y0) * 256);
    const uint8_t *r0 = src + (size_t)y0 * sw * 4, *r1 = src + (size_t)y1 * sw * 4;
    uint8_t *d = dst + (size_t)y * dw * 4;
    for (int x = 0; x < dw; x++) {
      double fx = (x + 0.5) * sw / dw - 0.5;
      if (fx < 0) fx = 0;
      int x0 = (int)fx, x1 = x0 + 1 < sw ? x0 + 1 : x0;
      int wx = (int)((fx - x0) * 256);
      for (int c = 0; c < 4; c++) {
        int top = r0[x0 * 4 + c] * (256 - wx) + r0[x1 * 4 + c] * wx;
        int bot = r1[x0 * 4 + c] * (256 - wx) + r1[x1 * 4 + c] * wx;
        d[x * 4 + c] = (uint8_t)((top * (256 - wy) + bot * wy + 32768) >> 16);
      }
    }
  }
}

// blur(context2d[, src]): Bitmap.prototype.blur (dos pasadas) sobre los pixeles del
// canvas, sin los 18 drawImage de la original. false si no se puede (un
// canvas en la GPU...): el hook usa entonces la original.
JSValue rpgmaker_mv_blur(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_FALSE;
  Context2D *c = context2d_get(ctx, argv[0]);
  if (!c) {
    JS_FreeValue(ctx, JS_GetException(ctx));
    return JS_FALSE;
  }
  CanvasData *cd = js_canvas_get(ctx, c->canvas);
  if (!cd || cd->gpu || cd->gpu_wanted) return JS_FALSE;
  if (cd->surface.w <= 0 || cd->surface.h <= 0) return JS_FALSE;
  Surface *s = context2d_surface(ctx, c);
  if (!s || !s->px) return JS_FALSE;
  // con src (snap-half: la foto de la pantalla a media resolucion), primero
  // se amplia src al tamano del canvas
  if (argc > 1 && JS_IsObject(argv[1])) {
    const Surface *src = js_get_image_source(ctx, argv[1]);
    if (!src || !src->px || src->w <= 0 || src->h <= 0) return JS_FALSE;
    upscale_bilinear(s->px, s->w, s->h, src->px, src->w, src->h);
  }
  TRACE_START(tb);
  for (int i = 0; i < 2; i++) {
    if (blur_pass(s->px, s->w, s->h) != 0) return i ? JS_TRUE : JS_FALSE;   // sin memoria
  }
  TRACE_END(tb, 1.0, "blur %dx%d", s->w, s->h);
  return JS_TRUE;
}
