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

// fillRect, strokeRect, clearRect, fill, stroke, clip

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "context2d.h"
#include "gfx/gpu2d.h"

// gpu-canvas: el Gpu2D si esta operacion puede ir a la GPU (sin clip ni
// sombra); NULL para hacerla por software
static Gpu2D *gpu_target(JSContext *ctx, Context2D *c, CanvasData **out) {
  CanvasData *cd = js_canvas_get(ctx, c->canvas);
  if (!cd || !cd->gpu_wanted) return NULL;
  const Context2DState *s = CTX2D_STATE(c);
  if (s->clip) return NULL;
  if (s->shadow_color.a && (s->shadow_blur > 0 || s->shadow_offset_x != 0 || s->shadow_offset_y != 0)) return NULL;
  *out = cd;
  return canvas_gpu(cd);
}

static void gpu_drawn(CanvasData *cd) {
  cd->cpu_stale = 1;
  cd->dirty = 1;
}

static int read_rect(JSContext *ctx, int argc, JSValueConst *argv, double *v) {
  if (argc < 4) return -1;
  for (int i = 0; i < 4; i++) {
    if (JS_ToFloat64(ctx, &v[i], argv[i])) return -1;
    if (!isfinite(v[i])) return -1;
  }
  return 0;
}

static FillRule read_fill_rule(JSContext *ctx, int argc, JSValueConst *argv) {
  for (int i = 0; i < argc; i++) {
    if (!JS_IsString(argv[i])) continue;
    const char *r = JS_ToCString(ctx, argv[i]);
    FillRule rule = (r && strcmp(r, "evenodd") == 0) ? FILL_EVENODD : FILL_NONZERO;
    if (r) JS_FreeCString(ctx, r);
    return rule;
  }
  return FILL_NONZERO;
}

static double stroke_width_device(const Context2DState *s) {
  const Matrix *m = &s->transform;
  double scale = sqrt(fabs(m->a * m->d - m->b * m->c));
  double w = s->line_width * scale;
  // una linea de 1px tiene que verse aunque no tenga antialiasing
  return w < 1 ? 1 : w;
}

JSValue js_ctx2d_fillRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[4];
  if (read_rect(ctx, argc, argv, v) != 0) return JS_UNDEFINED;
  Context2DState *s = CTX2D_STATE(c);
  CanvasData *cd;
  Gpu2D *g = gpu_target(ctx, c, &cd);
  if (g && gpu2d_fill_rect(g, v[0], v[1], v[2], v[3], &s->transform, &s->fill, s->global_alpha, s->op) == 0) {
    gpu_drawn(cd);
    return JS_UNDEFINED;
  }
  Surface *surf = context2d_surface(ctx, c);
  if (!surf) return JS_UNDEFINED;

  PaintSampler ps;
  paint_sampler_init(&ps, &s->fill, &s->transform);
  DrawParams dp;
  context2d_draw_params(c, &dp);
  raster_fill_rect(surf, v[0], v[1], v[2], v[3], &s->transform, &ps, &dp);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_clearRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[4];
  if (read_rect(ctx, argc, argv, v) != 0) return JS_UNDEFINED;
  Context2DState *s = CTX2D_STATE(c);
  CanvasData *cd;
  Gpu2D *g = gpu_target(ctx, c, &cd);
  if (g && gpu2d_clear_rect(g, v[0], v[1], v[2], v[3], &s->transform) == 0) {
    gpu_drawn(cd);
    return JS_UNDEFINED;
  }
  Surface *surf = context2d_surface(ctx, c);
  if (!surf) return JS_UNDEFINED;

  raster_clear_rect(surf, v[0], v[1], v[2], v[3], &s->transform, s->clip);
  return JS_UNDEFINED;
}

static void stroke_path(JSContext *ctx, Context2D *c, const Path *path) {
  CanvasData *cd;
  Gpu2D *g = gpu_target(ctx, c, &cd);
  if (g) {
    const Context2DState *gs = CTX2D_STATE(c);
    Mask gm;
    if (mask_init(&gm, cd->surface.w, cd->surface.h) == 0) {
      mask_stroke_path(&gm, path, stroke_width_device(gs), gs->line_cap, gs->line_join);
      int ok = gpu2d_fill_mask(g, &gm, &gs->transform, &gs->stroke, gs->global_alpha, gs->op) == 0;
      mask_free(&gm);
      if (ok) {
        gpu_drawn(cd);
        return;
      }
    }
  }
  Surface *surf = context2d_surface(ctx, c);
  if (!surf) return;
  Context2DState *s = CTX2D_STATE(c);

  Mask m;
  if (mask_init(&m, surf->w, surf->h) != 0) return;
  mask_stroke_path(&m, path, stroke_width_device(s), s->line_cap, s->line_join);

  PaintSampler ps;
  paint_sampler_init(&ps, &s->stroke, &s->transform);
  DrawParams dp;
  context2d_draw_params(c, &dp);
  raster_fill_mask(surf, &m, &ps, &dp);
  mask_free(&m);
}

JSValue js_ctx2d_strokeRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[4];
  if (read_rect(ctx, argc, argv, v) != 0) return JS_UNDEFINED;

  const Matrix *t = &CTX2D_STATE(c)->transform;
  Path p;
  path_init(&p);
  double x, y;
  matrix_apply(t, v[0], v[1], &x, &y);               path_move_to(&p, x, y);
  matrix_apply(t, v[0] + v[2], v[1], &x, &y);        path_line_to(&p, x, y);
  matrix_apply(t, v[0] + v[2], v[1] + v[3], &x, &y); path_line_to(&p, x, y);
  matrix_apply(t, v[0], v[1] + v[3], &x, &y);        path_line_to(&p, x, y);
  path_close(&p);
  stroke_path(ctx, c, &p);
  path_free(&p);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_fill(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  CanvasData *cd;
  Gpu2D *g = gpu_target(ctx, c, &cd);
  if (g) {
    const Context2DState *gs = CTX2D_STATE(c);
    Mask gm;
    if (mask_init(&gm, cd->surface.w, cd->surface.h) == 0) {
      mask_fill_path(&gm, &c->path, read_fill_rule(ctx, argc, argv));
      int ok = gpu2d_fill_mask(g, &gm, &gs->transform, &gs->fill, gs->global_alpha, gs->op) == 0;
      mask_free(&gm);
      if (ok) {
        gpu_drawn(cd);
        return JS_UNDEFINED;
      }
    }
  }
  Surface *surf = context2d_surface(ctx, c);
  if (!surf) return JS_UNDEFINED;
  Context2DState *s = CTX2D_STATE(c);

  Mask m;
  if (mask_init(&m, surf->w, surf->h) != 0) return JS_ThrowOutOfMemory(ctx);
  mask_fill_path(&m, &c->path, read_fill_rule(ctx, argc, argv));

  PaintSampler ps;
  paint_sampler_init(&ps, &s->fill, &s->transform);
  DrawParams dp;
  context2d_draw_params(c, &dp);
  raster_fill_mask(surf, &m, &ps, &dp);
  mask_free(&m);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_stroke(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  stroke_path(ctx, c, &c->path);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_clip(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  Surface *surf = context2d_surface(ctx, c);
  if (!surf) return JS_UNDEFINED;
  Context2DState *s = CTX2D_STATE(c);

  Mask *m = malloc(sizeof(Mask));
  if (!m || mask_init(m, surf->w, surf->h) != 0) {
    free(m);
    return JS_ThrowOutOfMemory(ctx);
  }
  mask_fill_path(m, &c->path, read_fill_rule(ctx, argc, argv));

  // el nuevo clip es la interseccion con el anterior
  if (s->clip) {
    mask_intersect(m, s->clip);
    mask_free(s->clip);
    free(s->clip);
  }
  s->clip = m;
  return JS_UNDEFINED;
}
