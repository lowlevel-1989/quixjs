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

#include <string.h>
#include <stdio.h>
#include "context2d.h"

// El id de la clase CanvasRenderingContext2D: con el, JS_GetOpaque sabe si un
// objeto es un contexto 2d y el finalizador encuentra sus datos.
static JSClassID js_context2d_class_id;

static void js_context2d_finalizer(JSRuntime *rt, JSValue val) {
  Context2D *c = JS_GetOpaque(val, js_context2d_class_id);
  if (!c) return;
  for (int i = 0; i <= c->depth; i++) context2d_state_release(rt, &c->stack[i]);
  path_free(&c->path);
  // c->canvas es prestado (lo mantiene vivo la propiedad oculta "canvas")
  js_free_rt(rt, c);
}

static void js_context2d_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func) {
  Context2D *c = JS_GetOpaque(val, js_context2d_class_id);
  if (!c) return;
  JS_MarkValue(rt, c->canvas, mark_func);
  for (int i = 0; i <= c->depth; i++) {
    JS_MarkValue(rt, c->stack[i].fill_obj, mark_func);
    JS_MarkValue(rt, c->stack[i].stroke_obj, mark_func);
  }
}

static JSClassDef js_context2d_class = {
  "CanvasRenderingContext2D",
  .finalizer = js_context2d_finalizer,
  .gc_mark = js_context2d_mark,
};

Context2D *context2d_get(JSContext *ctx, JSValueConst this_val) {
  return JS_GetOpaque2(ctx, this_val, js_context2d_class_id);
}

Surface *context2d_surface(JSContext *ctx, Context2D *c) {
  CanvasData *cd = js_canvas_get(ctx, c->canvas);
  if (!cd) return NULL;
  cd->dirty = 1;   // todo dibujo pasa por aqui (tambien alguna lectura: no importa)
  // lo que llega aqui se hace por software: un canvas en la GPU deja de estarlo
  if (cd->gpu || cd->gpu_wanted) canvas_leave_gpu(cd, "an operation the GPU does not do");
  return &cd->surface;
}

void context2d_draw_params(Context2D *c, DrawParams *dp) {
  Context2DState *s = CTX2D_STATE(c);
  dp->alpha = s->global_alpha;
  dp->op = s->op;
  dp->clip = s->clip;
}

static JSValue js_ctx2d_get_canvas(JSContext *ctx, JSValueConst this_val) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  return JS_DupValue(ctx, c->canvas);
}

// Numeros (magic de js_ctx2d_get_number)
enum { NUM_GLOBAL_ALPHA, NUM_LINE_WIDTH, NUM_MITER_LIMIT, NUM_SHADOW_BLUR,
       NUM_SHADOW_OFFSET_X, NUM_SHADOW_OFFSET_Y, NUM_LINE_DASH_OFFSET };
// Enumerados como texto (magic de js_ctx2d_get_enum)
enum { ENUM_COMPOSITE, ENUM_LINE_CAP, ENUM_LINE_JOIN, ENUM_TEXT_ALIGN, ENUM_TEXT_BASELINE,
       ENUM_SMOOTHING, ENUM_SMOOTHING_QUALITY };
// Estilos (magic de js_ctx2d_get_style)
enum { STYLE_FILL, STYLE_STROKE, STYLE_SHADOW };

static const JSCFunctionListEntry js_ctx2d_proto[] = {
  JS_CGETSET_DEF("canvas", js_ctx2d_get_canvas, NULL),

  // style.c
  JS_CGETSET_MAGIC_DEF("fillStyle",   js_ctx2d_get_style, js_ctx2d_set_style, STYLE_FILL),
  JS_CGETSET_MAGIC_DEF("strokeStyle", js_ctx2d_get_style, js_ctx2d_set_style, STYLE_STROKE),
  JS_CGETSET_MAGIC_DEF("shadowColor", js_ctx2d_get_style, js_ctx2d_set_style, STYLE_SHADOW),
  JS_CGETSET_MAGIC_DEF("globalAlpha",    js_ctx2d_get_number, js_ctx2d_set_number, NUM_GLOBAL_ALPHA),
  JS_CGETSET_MAGIC_DEF("lineWidth",      js_ctx2d_get_number, js_ctx2d_set_number, NUM_LINE_WIDTH),
  JS_CGETSET_MAGIC_DEF("miterLimit",     js_ctx2d_get_number, js_ctx2d_set_number, NUM_MITER_LIMIT),
  JS_CGETSET_MAGIC_DEF("shadowBlur",     js_ctx2d_get_number, js_ctx2d_set_number, NUM_SHADOW_BLUR),
  JS_CGETSET_MAGIC_DEF("shadowOffsetX",  js_ctx2d_get_number, js_ctx2d_set_number, NUM_SHADOW_OFFSET_X),
  JS_CGETSET_MAGIC_DEF("shadowOffsetY",  js_ctx2d_get_number, js_ctx2d_set_number, NUM_SHADOW_OFFSET_Y),
  JS_CGETSET_MAGIC_DEF("lineDashOffset", js_ctx2d_get_number, js_ctx2d_set_number, NUM_LINE_DASH_OFFSET),
  JS_CGETSET_MAGIC_DEF("globalCompositeOperation", js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_COMPOSITE),
  JS_CGETSET_MAGIC_DEF("lineCap",      js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_LINE_CAP),
  JS_CGETSET_MAGIC_DEF("lineJoin",     js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_LINE_JOIN),
  JS_CGETSET_MAGIC_DEF("textAlign",    js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_TEXT_ALIGN),
  JS_CGETSET_MAGIC_DEF("textBaseline", js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_TEXT_BASELINE),
  JS_CGETSET_MAGIC_DEF("imageSmoothingEnabled",       js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_SMOOTHING),
  JS_CGETSET_MAGIC_DEF("webkitImageSmoothingEnabled", js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_SMOOTHING),
  JS_CGETSET_MAGIC_DEF("mozImageSmoothingEnabled",    js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_SMOOTHING),
  JS_CGETSET_MAGIC_DEF("msImageSmoothingEnabled",     js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_SMOOTHING),
  JS_CGETSET_MAGIC_DEF("imageSmoothingQuality",       js_ctx2d_get_enum, js_ctx2d_set_enum, ENUM_SMOOTHING_QUALITY),
  JS_CFUNC_DEF("createLinearGradient", 4, js_ctx2d_createLinearGradient),
  JS_CFUNC_DEF("createRadialGradient", 6, js_ctx2d_createRadialGradient),
  JS_CFUNC_DEF("createPattern",        2, js_ctx2d_createPattern),
  JS_CFUNC_DEF("setLineDash",          1, js_ctx2d_setLineDash),
  JS_CFUNC_DEF("getLineDash",          0, js_ctx2d_getLineDash),

  // state.c
  JS_CFUNC_DEF("save",    0, js_ctx2d_save),
  JS_CFUNC_DEF("restore", 0, js_ctx2d_restore),
  JS_CFUNC_DEF("reset",   0, js_ctx2d_reset),

  // transform.c
  JS_CFUNC_DEF("translate",      2, js_ctx2d_translate),
  JS_CFUNC_DEF("rotate",         1, js_ctx2d_rotate),
  JS_CFUNC_DEF("scale",          2, js_ctx2d_scale),
  JS_CFUNC_DEF("transform",      6, js_ctx2d_transform),
  JS_CFUNC_DEF("setTransform",   6, js_ctx2d_setTransform),
  JS_CFUNC_DEF("resetTransform", 0, js_ctx2d_resetTransform),
  JS_CFUNC_DEF("getTransform",   0, js_ctx2d_getTransform),

  // path.c
  JS_CFUNC_DEF("beginPath",        0, js_ctx2d_beginPath),
  JS_CFUNC_DEF("closePath",        0, js_ctx2d_closePath),
  JS_CFUNC_DEF("moveTo",           2, js_ctx2d_moveTo),
  JS_CFUNC_DEF("lineTo",           2, js_ctx2d_lineTo),
  JS_CFUNC_DEF("rect",             4, js_ctx2d_rect),
  JS_CFUNC_DEF("arc",              5, js_ctx2d_arc),
  JS_CFUNC_DEF("arcTo",            5, js_ctx2d_arcTo),
  JS_CFUNC_DEF("ellipse",          7, js_ctx2d_ellipse),
  JS_CFUNC_DEF("quadraticCurveTo", 4, js_ctx2d_quadraticCurveTo),
  JS_CFUNC_DEF("bezierCurveTo",    6, js_ctx2d_bezierCurveTo),
  JS_CFUNC_DEF("isPointInPath",    2, js_ctx2d_isPointInPath),

  // draw.c
  JS_CFUNC_DEF("fillRect",   4, js_ctx2d_fillRect),
  JS_CFUNC_DEF("strokeRect", 4, js_ctx2d_strokeRect),
  JS_CFUNC_DEF("clearRect",  4, js_ctx2d_clearRect),
  JS_CFUNC_DEF("fill",       0, js_ctx2d_fill),
  JS_CFUNC_DEF("stroke",     0, js_ctx2d_stroke),
  JS_CFUNC_DEF("clip",       0, js_ctx2d_clip),

  // image.c
  JS_CFUNC_DEF("drawImage",       3, js_ctx2d_drawImage),
  JS_CFUNC_DEF("getImageData",    4, js_ctx2d_getImageData),
  JS_CFUNC_DEF("putImageData",    3, js_ctx2d_putImageData),
  JS_CFUNC_DEF("createImageData", 2, js_ctx2d_createImageData),

  // text.c
  JS_CGETSET_DEF("font", js_ctx2d_get_font, js_ctx2d_set_font),
  JS_CFUNC_DEF("measureText", 1, js_ctx2d_measureText),
  JS_CFUNC_DEF("fillText",    3, js_ctx2d_fillText),
  JS_CFUNC_DEF("strokeText",  3, js_ctx2d_strokeText),
};

static JSValue js_ctx2d_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  return JS_ThrowTypeError(ctx, "Illegal constructor");
}

// Registramos la nueva clase en JS
static int init_done = 0;
static int js_context2d_init(JSContext *ctx) {
  if (init_done) return 1;

  JS_NewClassID(&js_context2d_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_context2d_class_id, &js_context2d_class);

  JSValue proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, proto, js_ctx2d_proto, countof(js_ctx2d_proto));
  JS_SetClassProto(ctx, js_context2d_class_id, JS_DupValue(ctx, proto));

  // CanvasRenderingContext2D global (para instanceof)
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue ctor = JS_NewCFunction2(ctx, js_ctx2d_ctor, "CanvasRenderingContext2D", 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, ctor, proto);
  JS_SetPropertyStr(ctx, global, "CanvasRenderingContext2D", ctor);
  JS_FreeValue(ctx, proto);
  JS_FreeValue(ctx, global);

  js_ctx2d_style_init(ctx);
  js_ctx2d_image_init(ctx);

  init_done = 1;
  return 0;
}

void js_api_add_context2d(JSContext *ctx) {
  js_context2d_init(ctx);
}

JSValue js_create_canvas_context(JSContext *ctx, JSValueConst canvas) {
  js_context2d_init(ctx);

  JSValue obj = JS_NewObjectClass(ctx, js_context2d_class_id);
  if (JS_IsException(obj)) return obj;

  Context2D *c = js_mallocz(ctx, sizeof(Context2D));
  if (!c) {
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
  }
  // el canvas: prestado en C, vivo por una propiedad oculta del contexto
  // (el canvas guarda el contexto igual: el recolector ve el ciclo y puede
  // liberar los dos)
  c->canvas = canvas;
  JS_SetHidden(ctx, obj, "canvas", JS_DupValue(ctx, canvas));
  c->depth = 0;
  context2d_state_default(&c->stack[0]);
  path_init(&c->path);
  JS_SetOpaque(obj, c);
  return obj;
}

void context2d_reset(JSContext *ctx, JSValueConst context2d) {
  Context2D *c = JS_GetOpaque(context2d, js_context2d_class_id);
  if (!c) return;
  for (int i = 0; i <= c->depth; i++) context2d_state_release(JS_GetRuntime(ctx), &c->stack[i]);
  c->depth = 0;
  c->overflow = 0;
  context2d_state_default(&c->stack[0]);
  path_clear(&c->path);
}
