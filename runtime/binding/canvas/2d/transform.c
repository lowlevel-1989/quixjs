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

// translate, rotate, scale, transform, setTransform

#include <math.h>
#include "context2d.h"

static int read_numbers(JSContext *ctx, int argc, JSValueConst *argv, double *out, int n) {
  if (argc < n) return -1;
  for (int i = 0; i < n; i++) {
    if (JS_ToFloat64(ctx, &out[i], argv[i])) return -1;
    if (!isfinite(out[i])) return -1;   // el navegador ignora la llamada
  }
  return 0;
}

static void apply(Context2D *c, const Matrix *m) {
  Context2DState *s = CTX2D_STATE(c);
  matrix_multiply(&s->transform, &s->transform, m);
}

JSValue js_ctx2d_translate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[2];
  if (read_numbers(ctx, argc, argv, v, 2) == 0) {
    Matrix m = { 1, 0, 0, 1, v[0], v[1] };
    apply(c, &m);
  }
  return JS_UNDEFINED;
}

JSValue js_ctx2d_rotate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[1];
  if (read_numbers(ctx, argc, argv, v, 1) == 0) {
    double cs = cos(v[0]), sn = sin(v[0]);
    Matrix m = { cs, sn, -sn, cs, 0, 0 };
    apply(c, &m);
  }
  return JS_UNDEFINED;
}

JSValue js_ctx2d_scale(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[2];
  if (read_numbers(ctx, argc, argv, v, 2) == 0) {
    Matrix m = { v[0], 0, 0, v[1], 0, 0 };
    apply(c, &m);
  }
  return JS_UNDEFINED;
}

JSValue js_ctx2d_transform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[6];
  if (read_numbers(ctx, argc, argv, v, 6) == 0) {
    Matrix m = { v[0], v[1], v[2], v[3], v[4], v[5] };
    apply(c, &m);
  }
  return JS_UNDEFINED;
}

static int matrix_from_object(JSContext *ctx, JSValueConst obj, Matrix *m) {
  static const char *names[6] = { "a", "b", "c", "d", "e", "f" };
  double v[6];
  for (int i = 0; i < 6; i++) {
    JSValue x = JS_GetPropertyStr(ctx, obj, names[i]);
    if (JS_IsUndefined(x)) {
      v[i] = (i == 0 || i == 3) ? 1 : 0;
    } else if (JS_ToFloat64(ctx, &v[i], x)) {
      JS_FreeValue(ctx, x);
      return -1;
    }
    JS_FreeValue(ctx, x);
  }
  *m = (Matrix){ v[0], v[1], v[2], v[3], v[4], v[5] };
  return 0;
}

JSValue js_ctx2d_setTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  Context2DState *s = CTX2D_STATE(c);

  if (argc == 0) {
    matrix_identity(&s->transform);
  } else if (argc == 1 && JS_IsObject(argv[0])) {
    // setTransform(DOMMatrix)
    Matrix m;
    if (matrix_from_object(ctx, argv[0], &m) != 0) return JS_EXCEPTION;
    s->transform = m;
  } else {
    double v[6];
    if (read_numbers(ctx, argc, argv, v, 6) == 0) {
      s->transform = (Matrix){ v[0], v[1], v[2], v[3], v[4], v[5] };
    }
  }
  return JS_UNDEFINED;
}

JSValue js_ctx2d_resetTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  matrix_identity(&CTX2D_STATE(c)->transform);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_getTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  const Matrix *m = &CTX2D_STATE(c)->transform;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "a", JS_NewFloat64(ctx, m->a));
  JS_SetPropertyStr(ctx, o, "b", JS_NewFloat64(ctx, m->b));
  JS_SetPropertyStr(ctx, o, "c", JS_NewFloat64(ctx, m->c));
  JS_SetPropertyStr(ctx, o, "d", JS_NewFloat64(ctx, m->d));
  JS_SetPropertyStr(ctx, o, "e", JS_NewFloat64(ctx, m->e));
  JS_SetPropertyStr(ctx, o, "f", JS_NewFloat64(ctx, m->f));
  return o;
}
