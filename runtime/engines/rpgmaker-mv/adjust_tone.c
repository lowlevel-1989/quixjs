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

// adjust_tone.c: la parte nativa del hook adjust-tone.js. Ver ese archivo.

#include <math.h>
#include "binding/canvas.h"
#include "binding/canvas/2d/context2d.h"
#include "natives.h"

// adjustTone(context2d, r, g, b): Bitmap.prototype.adjustTone sin pasar por
// getImageData / putImageData ni recorrer los pixeles en JS (adjust-tone.js).
// Suma r, g, b a cada pixel con el redondeo y el tope de Uint8ClampedArray,
// como hace la original. false si no se puede (un canvas en la GPU...).
JSValue rpgmaker_mv_adjust_tone(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 4) return JS_FALSE;
  double r, g, b;
  if (JS_ToFloat64(ctx, &r, argv[1]) || JS_ToFloat64(ctx, &g, argv[2]) || JS_ToFloat64(ctx, &b, argv[3])) {
    return JS_EXCEPTION;
  }
  Context2D *c = context2d_get(ctx, argv[0]);
  if (!c) {
    JS_FreeValue(ctx, JS_GetException(ctx));
    return JS_FALSE;
  }
  CanvasData *cd = js_canvas_get(ctx, c->canvas);
  if (!cd || cd->gpu || cd->gpu_wanted) return JS_FALSE;
  Surface *s = context2d_surface(ctx, c);
  if (!s || !s->px) return JS_FALSE;
  // la suma de cada valor posible (0..255), una vez: como pixels[i] += r en
  // un Uint8ClampedArray (redondeo al par mas cercano y tope 0..255)
  uint8_t lut[3][256];
  const double add[3] = { r, g, b };
  for (int k = 0; k < 3; k++) {
    for (int v = 0; v < 256; v++) {
      double x = v + add[k];
      if (x <= 0) lut[k][v] = 0;
      else if (x >= 255) lut[k][v] = 255;
      else lut[k][v] = (uint8_t)nearbyint(x);
    }
  }
  size_t n = (size_t)s->w * s->h;
  uint8_t *p = s->px;
  for (size_t i = 0; i < n; i++, p += 4) {
    p[0] = lut[0][p[0]];
    p[1] = lut[1][p[1]];
    p[2] = lut[2][p[2]];
  }
  return JS_TRUE;
}
