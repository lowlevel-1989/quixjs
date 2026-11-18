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

// tint.c: la parte nativa del hook tint-native.js. Ver ese archivo.

#include <math.h>
#include "binding/canvas.h"
#include "binding/canvas/2d/context2d.h"
#include "natives.h"

static int clamp255(double v) {
  if (!(v > 0)) return 0;
  if (v >= 255) return 255;
  return (int)(v + 0.5);
}

static uint8_t to_u8(double v) {
  if (v <= 0.0) return 0;
  if (v >= 1.0) return 255;
  return (uint8_t)(v * 255.0 + 0.5);
}

// tintSprite(context2d, srcCanvas, x, y, w, h, r1, g1, b1, r2, g2, b2, diff,
//            r3, g3, b3, a3)
// Lo que hace Sprite.prototype._executeTint de RPG Maker MV con las
// operaciones del canvas 2d de quixjs (sin 'saturation'), en una pasada:
//   copy        el trozo (x, y, w, h) del bitmap
//   lighter     + (r1, g1, b1)                  (queda opaco)
//   difference  blanco, lighter + (r2, g2, b2), difference blanco: - (r2, g2, b2)
//               (solo con diff, si el canvas sabe 'difference')
//   source-atop (r3, g3, b3) con alpha a3 / 255
//   destination-in el bitmap: vuelve el alpha original
// Con alpha opaco tras lighter, cada canal solo depende de su valor
// premultiplicado (color * alpha): una tabla de 256 por canal. false si no
// se puede (un canvas en la GPU, tamanos que no cuadran).
JSValue rpgmaker_mv_tint_sprite(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 17) return JS_FALSE;
  double v[15];
  for (int i = 0; i < 15; i++) {
    if (JS_ToFloat64(ctx, &v[i], argv[2 + i])) return JS_EXCEPTION;
  }
  int x = (int)v[0], y = (int)v[1], w = (int)v[2], h = (int)v[3];
  if (w <= 0 || h <= 0 || x < 0 || y < 0) return JS_FALSE;

  Context2D *c = context2d_get(ctx, argv[0]);
  if (!c) {
    JS_FreeValue(ctx, JS_GetException(ctx));
    return JS_FALSE;
  }
  CanvasData *dcd = js_canvas_get(ctx, c->canvas);
  CanvasData *scd = js_canvas_get(ctx, argv[1]);
  if (!dcd || !scd || dcd->gpu || dcd->gpu_wanted || scd->gpu || scd->gpu_wanted) return JS_FALSE;
  Surface *d = context2d_surface(ctx, c);
  Surface *s = &scd->surface;
  if (!d || !d->px || !s->px) return JS_FALSE;
  if (d->w < w || d->h < h || x + w > s->w || y + h > s->h) return JS_FALSE;

  int add[3] = { clamp255(v[4]), clamp255(v[5]), clamp255(v[6]) };
  int sub[3] = { clamp255(v[7]), clamp255(v[8]), clamp255(v[9]) };
  int diff = v[10] != 0;
  int col[3] = { clamp255(v[11]), clamp255(v[12]), clamp255(v[13]) };
  double k = v[14] / 255.0;
  if (!(k > 0)) k = 0;
  if (k > 1) k = 1;

  // la tabla: valor premultiplicado -> color final (como cada paso del
  // canvas, que redondea a 8 bits entre uno y otro)
  uint8_t lut[3][256];
  for (int ch = 0; ch < 3; ch++) {
    for (int p = 0; p < 256; p++) {
      int t = p + add[ch];                    // lighter
      if (t > 255) t = 255;
      if (diff) {
        t = 255 - t;                          // difference blanco
        t += sub[ch];                         // lighter
        if (t > 255) t = 255;
        t = 255 - t;                          // difference blanco
      }
      double co = col[ch] / 255.0 * k + t / 255.0 * (1.0 - k);   // source-atop
      lut[ch][p] = to_u8(co);
    }
  }

  for (int j = 0; j < h; j++) {
    const uint8_t *sp = s->px + ((size_t)(y + j) * s->w + x) * 4;
    uint8_t *dp = d->px + (size_t)j * d->w * 4;
    for (int i = 0; i < w; i++, sp += 4, dp += 4) {
      int a = sp[3];
      if (a == 0) {
        dp[0] = dp[1] = dp[2] = dp[3] = 0;
        continue;
      }
      if (a == 255) {
        dp[0] = lut[0][sp[0]];
        dp[1] = lut[1][sp[1]];
        dp[2] = lut[2][sp[2]];
      } else {
        dp[0] = lut[0][(sp[0] * a + 127) / 255];
        dp[1] = lut[1][(sp[1] * a + 127) / 255];
        dp[2] = lut[2][(sp[2] * a + 127) / 255];
      }
      dp[3] = (uint8_t)a;
    }
  }
  return JS_TRUE;
}
