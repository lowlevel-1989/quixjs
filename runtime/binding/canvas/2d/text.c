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

// font, textAlign, textBaseline, measureText, fillText, strokeText
//
// El texto se dibuja con SDL_ttf (gfx/font.c). De momento con el color de
// fillStyle/strokeStyle: los gradientes y patrones usan el color del primer
// stop o negro (TODO).

#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gfx/font.h"
#include "context2d.h"

double context2d_parse_font_size(const char *font) {
  // busca el primer "<numero>px" o "<numero>pt"
  for (const char *p = font; *p; p++) {
    if (!isdigit((unsigned char)*p) && *p != '.') continue;
    if (p > font && (isalnum((unsigned char)p[-1]) || p[-1] == '.')) continue;
    char *end;
    double v = strtod(p, &end);
    if (end != p) {
      if (strncmp(end, "px", 2) == 0) return v;
      if (strncmp(end, "pt", 2) == 0) return v * 4.0 / 3.0;
      if (strncmp(end, "em", 2) == 0) return v * 16.0;
    }
    p = end > p ? end - 1 : p;
  }
  return -1;
}

JSValue js_ctx2d_get_font(JSContext *ctx, JSValueConst this_val) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  return JS_NewString(ctx, CTX2D_STATE(c)->font);
}

JSValue js_ctx2d_set_font(JSContext *ctx, JSValueConst this_val, JSValueConst val) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  const char *f = JS_ToCString(ctx, val);
  if (!f) return JS_EXCEPTION;
  double size = context2d_parse_font_size(f);
  if (size > 0) {
    Context2DState *s = CTX2D_STATE(c);
    snprintf(s->font, sizeof(s->font), "%s", f);
    s->font_size = size;
  }
  JS_FreeCString(ctx, f);
  return JS_UNDEFINED;
}

static size_t utf8_length(const char *s) {
  size_t n = 0;
  for (; *s; s++) if (((unsigned char)*s & 0xc0) != 0x80) n++;
  return n;
}

// Saltos de linea, tabuladores... se dibujan como espacios (como el navegador)
static char *clean_text(const char *s) {
  char *t = strdup(s);
  if (!t) return NULL;
  for (char *p = t; *p; p++) {
    if (*p == '\n' || *p == '\r' || *p == '\t' || *p == '\f') *p = ' ';
  }
  return t;
}

JSValue js_ctx2d_measureText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  Context2DState *s = CTX2D_STATE(c);

  const char *raw = argc > 0 ? JS_ToCString(ctx, argv[0]) : NULL;
  char *text = raw ? clean_text(raw) : NULL;
  if (raw) JS_FreeCString(ctx, raw);

  double width, ascent, descent;
  FontHandle *f = font_from_css(s->font);
  if (f) {
    FontMetrics m;
    font_metrics(f, &m);
    width = text ? font_text_width(f, text) : 0;
    ascent = m.ascent;
    descent = m.descent;
  } else {
    // sin fuentes: aproximacion
    width = text ? utf8_length(text) * s->font_size * 0.6 : 0;
    ascent = s->font_size * 0.8;
    descent = s->font_size * 0.2;
  }
  free(text);

  JSValue m = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, m, "width", JS_NewFloat64(ctx, width));
  JS_SetPropertyStr(ctx, m, "actualBoundingBoxLeft", JS_NewFloat64(ctx, 0));
  JS_SetPropertyStr(ctx, m, "actualBoundingBoxRight", JS_NewFloat64(ctx, width));
  JS_SetPropertyStr(ctx, m, "actualBoundingBoxAscent", JS_NewFloat64(ctx, ascent));
  JS_SetPropertyStr(ctx, m, "actualBoundingBoxDescent", JS_NewFloat64(ctx, descent));
  JS_SetPropertyStr(ctx, m, "fontBoundingBoxAscent", JS_NewFloat64(ctx, ascent));
  JS_SetPropertyStr(ctx, m, "fontBoundingBoxDescent", JS_NewFloat64(ctx, descent));
  return m;
}

static Color paint_color(const Paint *p) {
  if (p->type == PAINT_COLOR) return p->color;
  if (p->type == PAINT_GRADIENT && p->gradient && p->gradient->stop_count > 0) return p->gradient->stops[0].color;
  Color black = { 0, 0, 0, 255 };
  return black;
}

// magic 0: fillText, 1: strokeText
static JSValue draw_text(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int stroke) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  if (argc < 3) return JS_ThrowTypeError(ctx, "fillText: 3 arguments required");

  double x, y, max_width = -1;
  if (JS_ToFloat64(ctx, &x, argv[1]) || JS_ToFloat64(ctx, &y, argv[2])) return JS_EXCEPTION;
  if (!isfinite(x) || !isfinite(y)) return JS_UNDEFINED;
  if (argc > 3 && !JS_IsUndefined(argv[3])) {
    if (JS_ToFloat64(ctx, &max_width, argv[3])) return JS_EXCEPTION;
    if (!(max_width > 0)) return JS_UNDEFINED;
  }

  Surface *dst = context2d_surface(ctx, c);
  Context2DState *s = CTX2D_STATE(c);
  FontHandle *f = font_from_css(s->font);
  if (!dst || !f) return JS_UNDEFINED;

  const char *raw = JS_ToCString(ctx, argv[0]);
  if (!raw) return JS_EXCEPTION;
  char *text = clean_text(raw);
  JS_FreeCString(ctx, raw);
  if (!text) return JS_ThrowOutOfMemory(ctx);

  Color col = paint_color(stroke ? &s->stroke : &s->fill);
  int outline = stroke ? (int)ceil(s->line_width / 2) : 0;
  if (stroke && outline < 1) outline = 1;

  // las medidas antes de dibujar: al dibujar, la cache de fuentes puede
  // cambiar (otra fuente para letras que faltan, el contorno) y f ya no ser
  // la misma
  FontMetrics m;
  font_metrics(f, &m);
  Surface glyphs;
  if (font_render(f, text, col.r, col.g, col.b, col.a, outline, &glyphs) != 0) {
    free(text);
    return JS_UNDEFINED;
  }
  free(text);
  double w = glyphs.w - 2 * outline;
  double dw = glyphs.w;
  if (max_width > 0 && w > max_width) {
    // se estrecha para caber en maxWidth
    dw = glyphs.w * max_width / w;
    w = max_width;
  }

  double left = x;
  switch (s->text_align) {
    case TEXT_ALIGN_CENTER: left = x - w / 2; break;
    case TEXT_ALIGN_RIGHT:
    case TEXT_ALIGN_END:    left = x - w; break;
    default: break;
  }
  double top;
  switch (s->text_baseline) {
    case BASELINE_TOP:
    case BASELINE_HANGING:    top = y; break;
    case BASELINE_MIDDLE:     top = y - (m.ascent + m.descent) / 2.0; break;
    case BASELINE_BOTTOM:
    case BASELINE_IDEOGRAPHIC: top = y - m.ascent - m.descent; break;
    default:                  top = y - m.ascent; break;
  }

  DrawParams dp;
  context2d_draw_params(c, &dp);
  raster_draw_image(dst, &glyphs, 0, 0, glyphs.w, glyphs.h,
                    left - outline, top - outline, dw, glyphs.h, &s->transform, &dp);
  surface_free(&glyphs);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_fillText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return draw_text(ctx, this_val, argc, argv, 0);
}

JSValue js_ctx2d_strokeText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return draw_text(ctx, this_val, argc, argv, 1);
}
