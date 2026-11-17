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

// fillStyle, strokeStyle, lineWidth, globalAlpha, gradientes y patrones

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "context2d.h"

// Mismo orden que en context2d.c
enum { NUM_GLOBAL_ALPHA, NUM_LINE_WIDTH, NUM_MITER_LIMIT, NUM_SHADOW_BLUR,
       NUM_SHADOW_OFFSET_X, NUM_SHADOW_OFFSET_Y, NUM_LINE_DASH_OFFSET };
enum { ENUM_COMPOSITE, ENUM_LINE_CAP, ENUM_LINE_JOIN, ENUM_TEXT_ALIGN, ENUM_TEXT_BASELINE,
       ENUM_SMOOTHING, ENUM_SMOOTHING_QUALITY };
enum { STYLE_FILL, STYLE_STROKE, STYLE_SHADOW };

static JSClassID js_gradient_class_id;
static JSClassID js_pattern_class_id;

// ------------------------------------------------------------ CanvasGradient

static void js_gradient_finalizer(JSRuntime *rt, JSValue val) {
  Gradient *g = JS_GetOpaque(val, js_gradient_class_id);
  js_free_rt(rt, g);
}

static JSClassDef js_gradient_class = {
  "CanvasGradient",
  .finalizer = js_gradient_finalizer,
};

static JSValue js_gradient_addColorStop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Gradient *g = JS_GetOpaque2(ctx, this_val, js_gradient_class_id);
  if (!g) return JS_EXCEPTION;
  if (argc < 2) return JS_ThrowTypeError(ctx, "addColorStop: 2 arguments required");

  double offset;
  if (JS_ToFloat64(ctx, &offset, argv[0])) return JS_EXCEPTION;
  if (!(offset >= 0 && offset <= 1)) return JS_ThrowRangeError(ctx, "The provided value is outside the range [0, 1]");

  const char *str = JS_ToCString(ctx, argv[1]);
  if (!str) return JS_EXCEPTION;
  Color col;
  int ok = color_parse(str, &col) == 0;
  JS_FreeCString(ctx, str);
  if (!ok) return JS_ThrowSyntaxError(ctx, "addColorStop: invalid color");

  gradient_add_stop(g, offset, col);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_gradient_proto_funcs[] = {
  JS_CFUNC_DEF("addColorStop", 2, js_gradient_addColorStop),
};

static JSValue new_gradient(JSContext *ctx, GradientType type, const double *v) {
  JSValue obj = JS_NewObjectClass(ctx, js_gradient_class_id);
  if (JS_IsException(obj)) return obj;
  Gradient *g = js_mallocz(ctx, sizeof(Gradient));
  if (!g) {
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
  }
  g->type = type;
  if (type == GRADIENT_LINEAR) {
    g->x0 = v[0]; g->y0 = v[1]; g->x1 = v[2]; g->y1 = v[3];
  } else {
    g->x0 = v[0]; g->y0 = v[1]; g->r0 = v[2];
    g->x1 = v[3]; g->y1 = v[4]; g->r1 = v[5];
  }
  JS_SetOpaque(obj, g);
  return obj;
}

static int read_finite(JSContext *ctx, int argc, JSValueConst *argv, double *out, int n) {
  if (argc < n) {
    JS_ThrowTypeError(ctx, "%d arguments required", n);
    return -1;
  }
  for (int i = 0; i < n; i++) {
    if (JS_ToFloat64(ctx, &out[i], argv[i])) return -1;
    if (!isfinite(out[i])) {
      JS_ThrowTypeError(ctx, "The provided value is non-finite");
      return -1;
    }
  }
  return 0;
}

JSValue js_ctx2d_createLinearGradient(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (!context2d_get(ctx, this_val)) return JS_EXCEPTION;
  double v[4];
  if (read_finite(ctx, argc, argv, v, 4) != 0) return JS_EXCEPTION;
  return new_gradient(ctx, GRADIENT_LINEAR, v);
}

JSValue js_ctx2d_createRadialGradient(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (!context2d_get(ctx, this_val)) return JS_EXCEPTION;
  double v[6];
  if (read_finite(ctx, argc, argv, v, 6) != 0) return JS_EXCEPTION;
  if (v[2] < 0 || v[5] < 0) return JS_ThrowRangeError(ctx, "The radius provided is negative");
  return new_gradient(ctx, GRADIENT_RADIAL, v);
}

// ------------------------------------------------------------- CanvasPattern

static void js_pattern_finalizer(JSRuntime *rt, JSValue val) {
  Pattern *p = JS_GetOpaque(val, js_pattern_class_id);
  if (!p) return;
  surface_free(&p->image);
  js_free_rt(rt, p);
}

static JSClassDef js_pattern_class = {
  "CanvasPattern",
  .finalizer = js_pattern_finalizer,
};

static JSValue js_pattern_setTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Pattern *p = JS_GetOpaque2(ctx, this_val, js_pattern_class_id);
  if (!p) return JS_EXCEPTION;
  if (argc == 0) {
    matrix_identity(&p->transform);
    return JS_UNDEFINED;
  }
  static const char *names[6] = { "a", "b", "c", "d", "e", "f" };
  double v[6] = { 1, 0, 0, 1, 0, 0 };
  for (int i = 0; i < 6; i++) {
    JSValue x = JS_GetPropertyStr(ctx, argv[0], names[i]);
    if (!JS_IsUndefined(x) && JS_ToFloat64(ctx, &v[i], x)) {
      JS_FreeValue(ctx, x);
      return JS_EXCEPTION;
    }
    JS_FreeValue(ctx, x);
  }
  p->transform = (Matrix){ v[0], v[1], v[2], v[3], v[4], v[5] };
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_pattern_proto_funcs[] = {
  JS_CFUNC_DEF("setTransform", 1, js_pattern_setTransform),
};

JSValue js_ctx2d_createPattern(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (!context2d_get(ctx, this_val)) return JS_EXCEPTION;
  if (argc < 1) return JS_ThrowTypeError(ctx, "createPattern: image required");

  const Surface *src = js_get_image_source(ctx, argv[0]);
  if (!src) return JS_NULL;   // imagen sin cargar

  PatternRepeat repeat = PATTERN_REPEAT;
  if (argc > 1 && !JS_IsNull(argv[1]) && !JS_IsUndefined(argv[1])) {
    const char *r = JS_ToCString(ctx, argv[1]);
    if (!r) return JS_EXCEPTION;
    if (strcmp(r, "repeat-x") == 0) repeat = PATTERN_REPEAT_X;
    else if (strcmp(r, "repeat-y") == 0) repeat = PATTERN_REPEAT_Y;
    else if (strcmp(r, "no-repeat") == 0) repeat = PATTERN_NO_REPEAT;
    else if (strcmp(r, "repeat") != 0 && *r) {
      JS_FreeCString(ctx, r);
      return JS_ThrowSyntaxError(ctx, "createPattern: invalid repetition");
    }
    JS_FreeCString(ctx, r);
  }

  JSValue obj = JS_NewObjectClass(ctx, js_pattern_class_id);
  if (JS_IsException(obj)) return obj;
  Pattern *p = js_mallocz(ctx, sizeof(Pattern));
  if (!p || surface_copy(&p->image, src) != 0) {
    js_free(ctx, p);
    JS_FreeValue(ctx, obj);
    return JS_ThrowOutOfMemory(ctx);
  }
  p->repeat = repeat;
  matrix_identity(&p->transform);
  JS_SetOpaque(obj, p);
  return obj;
}

void js_ctx2d_style_init(JSContext *ctx) {
  JSRuntime *rt = JS_GetRuntime(ctx);
  JSValue global = JS_GetGlobalObject(ctx);

  JS_NewClassID(&js_gradient_class_id);
  JS_NewClass(rt, js_gradient_class_id, &js_gradient_class);
  JSValue gproto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, gproto, js_gradient_proto_funcs, countof(js_gradient_proto_funcs));
  JS_SetClassProto(ctx, js_gradient_class_id, gproto);

  JS_NewClassID(&js_pattern_class_id);
  JS_NewClass(rt, js_pattern_class_id, &js_pattern_class);
  JSValue pproto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, pproto, js_pattern_proto_funcs, countof(js_pattern_proto_funcs));
  JS_SetClassProto(ctx, js_pattern_class_id, pproto);

  JS_FreeValue(ctx, global);
}

// ---------------------------------------------------------------- estilos

JSValue js_ctx2d_get_style(JSContext *ctx, JSValueConst this_val, int magic) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  Context2DState *s = CTX2D_STATE(c);

  char buf[64];
  if (magic == STYLE_SHADOW) {
    color_to_string(&s->shadow_color, buf, sizeof(buf));
    return JS_NewString(ctx, buf);
  }

  const Paint *paint = magic == STYLE_FILL ? &s->fill : &s->stroke;
  JSValue obj = magic == STYLE_FILL ? s->fill_obj : s->stroke_obj;
  if (paint->type != PAINT_COLOR) return JS_DupValue(ctx, obj);

  color_to_string(&paint->color, buf, sizeof(buf));
  return JS_NewString(ctx, buf);
}

JSValue js_ctx2d_set_style(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  Context2DState *s = CTX2D_STATE(c);

  if (magic == STYLE_SHADOW) {
    const char *str = JS_ToCString(ctx, val);
    if (!str) return JS_EXCEPTION;
    Color col;
    if (color_parse(str, &col) == 0) s->shadow_color = col;
    JS_FreeCString(ctx, str);
    return JS_UNDEFINED;
  }

  Paint *paint = magic == STYLE_FILL ? &s->fill : &s->stroke;
  JSValue *obj = magic == STYLE_FILL ? &s->fill_obj : &s->stroke_obj;

  Gradient *g = JS_GetOpaque(val, js_gradient_class_id);
  Pattern *p = JS_GetOpaque(val, js_pattern_class_id);
  if (g || p) {
    JS_FreeValue(ctx, *obj);
    *obj = JS_DupValue(ctx, val);
    paint->type = g ? PAINT_GRADIENT : PAINT_PATTERN;
    paint->gradient = g;
    paint->pattern = p;
    return JS_UNDEFINED;
  }

  // Los valores que no son un color valido se ignoran (como el navegador)
  if (!JS_IsString(val)) return JS_UNDEFINED;
  const char *str = JS_ToCString(ctx, val);
  if (!str) return JS_EXCEPTION;
  Color col;
  if (color_parse(str, &col) == 0) {
    JS_FreeValue(ctx, *obj);
    *obj = JS_UNDEFINED;
    paint->type = PAINT_COLOR;
    paint->color = col;
    paint->gradient = NULL;
    paint->pattern = NULL;
  }
  JS_FreeCString(ctx, str);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_get_number(JSContext *ctx, JSValueConst this_val, int magic) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  Context2DState *s = CTX2D_STATE(c);
  switch (magic) {
    case NUM_GLOBAL_ALPHA:    return JS_NewFloat64(ctx, s->global_alpha);
    case NUM_LINE_WIDTH:      return JS_NewFloat64(ctx, s->line_width);
    case NUM_MITER_LIMIT:     return JS_NewFloat64(ctx, s->miter_limit);
    case NUM_SHADOW_BLUR:     return JS_NewFloat64(ctx, s->shadow_blur);
    case NUM_SHADOW_OFFSET_X: return JS_NewFloat64(ctx, s->shadow_offset_x);
    case NUM_SHADOW_OFFSET_Y: return JS_NewFloat64(ctx, s->shadow_offset_y);
    default:                  return JS_NewFloat64(ctx, 0);
  }
}

JSValue js_ctx2d_set_number(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  Context2DState *s = CTX2D_STATE(c);

  double v;
  if (JS_ToFloat64(ctx, &v, val)) return JS_EXCEPTION;
  if (!isfinite(v)) return JS_UNDEFINED;

  switch (magic) {
    case NUM_GLOBAL_ALPHA:    if (v >= 0 && v <= 1) s->global_alpha = v; break;
    case NUM_LINE_WIDTH:      if (v > 0) s->line_width = v; break;
    case NUM_MITER_LIMIT:     if (v > 0) s->miter_limit = v; break;
    case NUM_SHADOW_BLUR:     if (v >= 0) s->shadow_blur = v; break;
    case NUM_SHADOW_OFFSET_X: s->shadow_offset_x = v; break;
    case NUM_SHADOW_OFFSET_Y: s->shadow_offset_y = v; break;
    default: break;
  }
  return JS_UNDEFINED;
}

static const char *cap_names[]      = { "butt", "round", "square", NULL };
static const char *join_names[]     = { "miter", "round", "bevel", NULL };
static const char *align_names[]    = { "start", "end", "left", "right", "center", NULL };
static const char *baseline_names[] = { "alphabetic", "top", "hanging", "middle", "ideographic", "bottom", NULL };

static int find_name(const char **names, const char *v) {
  for (int i = 0; names[i]; i++) if (strcmp(names[i], v) == 0) return i;
  return -1;
}

JSValue js_ctx2d_get_enum(JSContext *ctx, JSValueConst this_val, int magic) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  Context2DState *s = CTX2D_STATE(c);
  switch (magic) {
    case ENUM_COMPOSITE:         return JS_NewString(ctx, composite_op_name(s->op));
    case ENUM_LINE_CAP:          return JS_NewString(ctx, cap_names[s->line_cap]);
    case ENUM_LINE_JOIN:         return JS_NewString(ctx, join_names[s->line_join]);
    case ENUM_TEXT_ALIGN:        return JS_NewString(ctx, align_names[s->text_align]);
    case ENUM_TEXT_BASELINE:     return JS_NewString(ctx, baseline_names[s->text_baseline]);
    case ENUM_SMOOTHING:         return JS_NewBool(ctx, s->image_smoothing);
    case ENUM_SMOOTHING_QUALITY: return JS_NewString(ctx, "low");
    default:                     return JS_UNDEFINED;
  }
}

JSValue js_ctx2d_set_enum(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  Context2DState *s = CTX2D_STATE(c);

  if (magic == ENUM_SMOOTHING) {
    s->image_smoothing = JS_ToBool(ctx, val);
    return JS_UNDEFINED;
  }
  if (magic == ENUM_SMOOTHING_QUALITY) return JS_UNDEFINED;

  const char *v = JS_ToCString(ctx, val);
  if (!v) return JS_EXCEPTION;
  int idx;
  switch (magic) {
    case ENUM_COMPOSITE: {
      CompositeOp op;
      if (composite_op_parse(v, &op) == 0) s->op = op;
      break;
    }
    case ENUM_LINE_CAP:
      if ((idx = find_name(cap_names, v)) >= 0) s->line_cap = (LineCap)idx;
      break;
    case ENUM_LINE_JOIN:
      if ((idx = find_name(join_names, v)) >= 0) s->line_join = (LineJoin)idx;
      break;
    case ENUM_TEXT_ALIGN:
      if ((idx = find_name(align_names, v)) >= 0) s->text_align = (TextAlign)idx;
      break;
    case ENUM_TEXT_BASELINE:
      if ((idx = find_name(baseline_names, v)) >= 0) s->text_baseline = (TextBaseline)idx;
      break;
  }
  JS_FreeCString(ctx, v);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_setLineDash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  // TODO: lineas discontinuas. De momento se dibujan continuas.
  return JS_UNDEFINED;
}

JSValue js_ctx2d_getLineDash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_NewArray(ctx);
}
