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

// save, restore

#include <stdlib.h>
#include <string.h>
#include "context2d.h"

void ctx2d_state_default(Context2DState *s) {
  memset(s, 0, sizeof(*s));
  matrix_identity(&s->transform);
  s->fill.type = PAINT_COLOR;
  s->fill.color = (Color){ 0, 0, 0, 255 };
  s->stroke.type = PAINT_COLOR;
  s->stroke.color = (Color){ 0, 0, 0, 255 };
  s->fill_obj = JS_UNDEFINED;
  s->stroke_obj = JS_UNDEFINED;
  s->global_alpha = 1.0;
  s->op = OP_SOURCE_OVER;
  s->line_width = 1.0;
  s->line_cap = CAP_BUTT;
  s->line_join = JOIN_MITER;
  s->miter_limit = 10.0;
  s->clip = NULL;
  s->image_smoothing = 1;
  strcpy(s->font, "10px sans-serif");
  s->font_size = 10;
  s->text_align = TEXT_ALIGN_START;
  s->text_baseline = BASELINE_ALPHABETIC;
  s->shadow_color = (Color){ 0, 0, 0, 0 };
}

void ctx2d_state_release(JSRuntime *rt, Context2DState *s) {
  JS_FreeValueRT(rt, s->fill_obj);
  JS_FreeValueRT(rt, s->stroke_obj);
  s->fill_obj = JS_UNDEFINED;
  s->stroke_obj = JS_UNDEFINED;
  if (s->clip) {
    mask_free(s->clip);
    free(s->clip);
    s->clip = NULL;
  }
}

JSValue js_ctx2d_save(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;

  if (c->depth + 1 >= CONTEXT2D_STATE_MAX) {
    c->overflow++;
    return JS_UNDEFINED;
  }

  Context2DState *cur = &c->stack[c->depth];
  Context2DState *next = &c->stack[c->depth + 1];
  *next = *cur;
  next->fill_obj = JS_DupValue(ctx, cur->fill_obj);
  next->stroke_obj = JS_DupValue(ctx, cur->stroke_obj);
  if (cur->clip) {
    next->clip = malloc(sizeof(Mask));
    if (!next->clip || mask_copy(next->clip, cur->clip) != 0) {
      free(next->clip);
      next->clip = NULL;
    }
  }
  c->depth++;
  return JS_UNDEFINED;
}

JSValue js_ctx2d_restore(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;

  if (c->overflow > 0) {
    c->overflow--;
    return JS_UNDEFINED;
  }
  if (c->depth == 0) return JS_UNDEFINED;   // restore sin save: no hace nada

  ctx2d_state_release(JS_GetRuntime(ctx), &c->stack[c->depth]);
  c->depth--;
  return JS_UNDEFINED;
}

JSValue js_ctx2d_reset(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  context2d_reset(ctx, this_val);
  Surface *s = context2d_surface(ctx, c);
  if (s) surface_clear(s);
  return JS_UNDEFINED;
}
