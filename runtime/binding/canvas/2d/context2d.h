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

// CanvasRenderingContext2D. El codigo esta repartido por temas:
//   context2d.c  la clase y la lista de metodos/propiedades
//   state.c      save / restore (pila de estados)
//   style.c      fillStyle, strokeStyle, globalAlpha, lineWidth, gradientes, patrones...
//   transform.c  translate, rotate, scale, transform, setTransform
//   path.c       beginPath, moveTo, lineTo, arc, rect, bezier...
//   draw.c       fillRect, clearRect, strokeRect, fill, stroke, clip
//   image.c      drawImage, getImageData, putImageData, createImageData
//   text.c       font, textAlign, measureText, fillText

#include "js/js.h"
#include "gfx/surface.h"
#include "gfx/paint.h"
#include "gfx/raster.h"
#include "binding/canvas.h"

// Profundidad maxima de save(). Los save() de mas se cuentan para que los
// restore() queden equilibrados, pero no guardan estado.
#define CONTEXT2D_STATE_MAX 32

typedef enum { TEXT_ALIGN_START, TEXT_ALIGN_END, TEXT_ALIGN_LEFT, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER } TextAlign;
typedef enum { BASELINE_ALPHABETIC, BASELINE_TOP, BASELINE_HANGING, BASELINE_MIDDLE, BASELINE_IDEOGRAPHIC, BASELINE_BOTTOM } TextBaseline;

typedef struct {
  Matrix       transform;
  Paint        fill;
  Paint        stroke;
  JSValue      fill_obj;      // CanvasGradient / CanvasPattern (o JS_UNDEFINED)
  JSValue      stroke_obj;
  double       global_alpha;
  CompositeOp  op;
  double       line_width;
  LineCap      line_cap;
  LineJoin     line_join;
  double       miter_limit;
  Mask        *clip;          // NULL = sin clip
  int          image_smoothing;
  char         font[96];
  double       font_size;     // px, sacado de font
  TextAlign    text_align;
  TextBaseline text_baseline;
  double       shadow_blur;
  double       shadow_offset_x;
  double       shadow_offset_y;
  Color        shadow_color;
} Context2DState;

typedef struct {
  JSValue        canvas;
  Context2DState stack[CONTEXT2D_STATE_MAX];
  int            depth;       // indice del estado actual
  int            overflow;    // save() que no caben en la pila
  Path           path;
} Context2D;

#define CTX2D_STATE(c) (&(c)->stack[(c)->depth])

Context2D *context2d_get(JSContext *ctx, JSValueConst this_val);   // lanza TypeError si no es un contexto
Surface   *context2d_surface(JSContext *ctx, Context2D *c);
void       context2d_draw_params(Context2D *c, DrawParams *dp);

// state.c
void    context2d_state_default(Context2DState *s);
void    context2d_state_release(JSRuntime *rt, Context2DState *s);
JSValue js_ctx2d_save(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_restore(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_reset(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// style.c
void    js_ctx2d_style_init(JSContext *ctx);
JSValue js_ctx2d_get_style(JSContext *ctx, JSValueConst this_val, int magic);
JSValue js_ctx2d_set_style(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic);
JSValue js_ctx2d_get_number(JSContext *ctx, JSValueConst this_val, int magic);
JSValue js_ctx2d_set_number(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic);
JSValue js_ctx2d_get_enum(JSContext *ctx, JSValueConst this_val, int magic);
JSValue js_ctx2d_set_enum(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic);
JSValue js_ctx2d_createLinearGradient(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_createRadialGradient(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_createPattern(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_setLineDash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_getLineDash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// transform.c
JSValue js_ctx2d_translate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_rotate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_scale(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_transform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_setTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_resetTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_getTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// path.c
JSValue js_ctx2d_beginPath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_closePath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_moveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_lineTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_rect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_arc(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_arcTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_ellipse(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_quadraticCurveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_bezierCurveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_isPointInPath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// draw.c
JSValue js_ctx2d_fillRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_strokeRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_clearRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_fill(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_stroke(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_clip(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// image.c
void    js_ctx2d_image_init(JSContext *ctx);
JSValue js_ctx2d_drawImage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_getImageData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_putImageData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_createImageData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// text.c
JSValue js_ctx2d_get_font(JSContext *ctx, JSValueConst this_val);
JSValue js_ctx2d_set_font(JSContext *ctx, JSValueConst this_val, JSValueConst val);
JSValue js_ctx2d_measureText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_fillText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
JSValue js_ctx2d_strokeText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
double  context2d_parse_font_size(const char *font);
