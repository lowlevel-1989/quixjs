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

// HTMLCanvasElement: crear el canvas, width / height y getContext ("2d" o
// "webgl"). Ver canvas.h.

#include <stdio.h>
#include <string.h>
#include "core/app_context.h"
#include "document.h"
#include "image/image.h"
#include "canvas.h"
#include "webgl.h"
#include "gfx/gl/gpu2d.h"

static JSClassID js_canvas_class_id;

static void js_canvas_finalizer(JSRuntime *rt, JSValue val) {
  CanvasData *cd = JS_GetOpaque(val, js_canvas_class_id);
  if (!cd) return;
  gpu2d_free(cd->gpu);
  surface_free(&cd->surface);
  // context2d y contextgl son prestados (los mantienen vivos sus propiedades
  // ocultas)
  js_free_rt(rt, cd);
}

static void js_canvas_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func) {
  CanvasData *cd = JS_GetOpaque(val, js_canvas_class_id);
  if (cd) {
    JS_MarkValue(rt, cd->context2d, mark_func);
    JS_MarkValue(rt, cd->contextgl, mark_func);
  }
}

static JSClassDef js_canvas_class = {
  "HTMLCanvasElement",
  .finalizer = js_canvas_finalizer,
  .gc_mark = js_canvas_mark,
};

CanvasData *js_canvas_get(JSContext *ctx, JSValueConst val) {
  return JS_GetOpaque(val, js_canvas_class_id);
}

static int gpu_scope = 0;

void canvas_gpu_scope(int delta) {
  gpu_scope += delta;
  if (gpu_scope < 0) gpu_scope = 0;
}

struct Gpu2D *canvas_gpu(CanvasData *cd) {
  if (!cd->gpu_wanted) return NULL;
  if (!cd->gpu) {
    cd->gpu = gpu2d_new(&cd->surface);
    if (!cd->gpu) {
      cd->gpu_wanted = 0;   // sin GPU: por software, como cualquiera
      return NULL;
    }
    printf("[gpu-canvas] %dx%d canvas on the GPU\n", cd->surface.w, cd->surface.h);
  }
  return cd->gpu;
}

void canvas_sync_cpu(CanvasData *cd) {
  if (cd->gpu && cd->cpu_stale && gpu2d_read(cd->gpu, &cd->surface) == 0) cd->cpu_stale = 0;
}

void canvas_leave_gpu(CanvasData *cd, const char *why) {
  if (!cd->gpu) {
    cd->gpu_wanted = 0;
    return;
  }
  canvas_sync_cpu(cd);
  gpu2d_free(cd->gpu);
  cd->gpu = NULL;
  cd->gpu_wanted = 0;
  printf("[gpu-canvas] %dx%d canvas back to the CPU (%s)\n", cd->surface.w, cd->surface.h, why);
}

const Surface *js_canvas_image_source(JSContext *ctx, JSValueConst val) {
  CanvasData *cd = js_canvas_get(ctx, val);
  if (cd) {
    canvas_sync_cpu(cd);                    // lo dibujado en la GPU (gpu-canvas)
    if (cd->webgl) webgl_canvas_sync(cd);   // lo dibujado con WebGL
    return &cd->surface;
  }
  return js_image_get_surface(ctx, val);
}

// magic 0: width, 1: height
static JSValue js_canvas_get_size(JSContext *ctx, JSValueConst this_val, int magic) {
  CanvasData *cd = JS_GetOpaque2(ctx, this_val, js_canvas_class_id);
  if (!cd) return JS_EXCEPTION;
  return JS_NewInt32(ctx, magic == 0 ? cd->surface.w : cd->surface.h);
}

static JSValue js_canvas_set_size(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic) {
  CanvasData *cd = JS_GetOpaque2(ctx, this_val, js_canvas_class_id);
  if (!cd) return JS_EXCEPTION;

  double v = 0;
  if (JS_ToFloat64(ctx, &v, val)) return JS_EXCEPTION;
  int n = (v >= 0 && v < 16384) ? (int)v : (magic == 0 ? 300 : 150);

  // Como en el navegador: cambiar el tamano (aunque sea el mismo) borra el
  // canvas y reinicia el estado del contexto
  int w = magic == 0 ? n : cd->surface.w;
  int h = magic == 1 ? n : cd->surface.h;
  size_t before = (size_t)cd->surface.w * (size_t)cd->surface.h * 4;
  if (surface_resize(&cd->surface, w, h) != 0) return JS_ThrowOutOfMemory(ctx);
  size_t after = (size_t)w * (size_t)h * 4;
  if (after > before) JS_ReportExtraMemory(ctx, after - before);
  if (!JS_IsUndefined(cd->context2d)) context2d_reset(ctx, cd->context2d);
  if (cd->webgl) webgl_canvas_resized(cd);
  if (cd->gpu && gpu2d_resize(cd->gpu, w, h) != 0) canvas_leave_gpu(cd, "resize");
  cd->cpu_stale = 0;
  cd->dirty = 1;
  return JS_UNDEFINED;
}

static JSValue js_canvas_getContext(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  CanvasData *cd = JS_GetOpaque2(ctx, this_val, js_canvas_class_id);
  if (!cd) return JS_EXCEPTION;

  const char *type = argc > 0 ? JS_ToCString(ctx, argv[0]) : NULL;
  if (!type) return JS_NULL;
  int is_2d = strcmp(type, "2d") == 0;
  int is_gl2 = strcmp(type, "webgl2") == 0;
  int is_gl = is_gl2 || strcmp(type, "webgl") == 0 || strcmp(type, "experimental-webgl") == 0;
  JS_FreeCString(ctx, type);

  // un canvas tiene un solo tipo de contexto (como el navegador)
  if (is_2d) {
    if (!JS_IsUndefined(cd->contextgl)) return JS_NULL;
    if (JS_IsUndefined(cd->context2d)) {
      JSValue c2d = js_create_canvas_context(ctx, this_val);
      if (JS_IsException(c2d)) return JS_EXCEPTION;
      // prestado en C, vivo por la propiedad oculta (sin ciclo protegido)
      cd->context2d = c2d;
      JS_SetHidden(ctx, this_val, "context2d", c2d);
    }
    return JS_DupValue(ctx, cd->context2d);
  }
  if (is_gl) {
    if (!JS_IsUndefined(cd->context2d)) return JS_NULL;
    if (JS_IsUndefined(cd->contextgl)) {
      JSValue gl = js_create_webgl_context(ctx, this_val, argc > 1 ? argv[1] : JS_UNDEFINED, is_gl2);
      if (JS_IsException(gl)) return gl;
      if (JS_IsNull(gl)) return JS_NULL;   // sin WebGL (o sin WebGL 2): el juego usa otra cosa
      cd->contextgl = gl;
      JS_SetHidden(ctx, this_val, "contextgl", gl);
    }
    return JS_DupValue(ctx, cd->contextgl);
  }
  return JS_NULL;
}

static JSValue js_canvas_toDataURL(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  // TODO: codificar a PNG en base64 cuando un juego lo necesite
  return JS_NewString(ctx, "data:,");
}

static const JSCFunctionListEntry js_canvas_proto_funcs[] = {
  JS_CGETSET_MAGIC_DEF("width",  js_canvas_get_size, js_canvas_set_size, 0),
  JS_CGETSET_MAGIC_DEF("height", js_canvas_get_size, js_canvas_set_size, 1),
  JS_CFUNC_DEF("getContext", 1, js_canvas_getContext),
  JS_CFUNC_DEF("toDataURL",  0, js_canvas_toDataURL),
};

JSValue js_create_canvas(JSContext *ctx) {
  AppContext *app = JS_GetContextOpaque(ctx);

  JSValue canvas = JS_NewObjectClass(ctx, js_canvas_class_id);
  if (JS_IsException(canvas)) return canvas;

  CanvasData *cd = js_mallocz(ctx, sizeof(CanvasData));
  if (!cd) {
    JS_FreeValue(ctx, canvas);
    return JS_EXCEPTION;
  }
  cd->context2d = JS_UNDEFINED;
  cd->contextgl = JS_UNDEFINED;
  cd->webgl = NULL;
  cd->gpu_wanted = gpu_scope > 0 && gpu2d_available();
  // Por defecto el tamano del juego (el navegador usa 300x150)
  if (surface_init(&cd->surface, app->cfg->game.width, app->cfg->game.height) != 0) {
    js_free(ctx, cd);
    JS_FreeValue(ctx, canvas);
    return JS_ThrowOutOfMemory(ctx);
  }
  JS_SetOpaque(canvas, cd);

  element_setup(ctx, canvas, "canvas");
  return canvas;
}

static JSValue js_canvas_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  return JS_ThrowTypeError(ctx, "Illegal constructor");
}

void js_api_add_canvas(JSContext *ctx) {
  JS_NewClassID(&js_canvas_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_canvas_class_id, &js_canvas_class);

  JSValue element_proto = js_element_proto(ctx);
  JSValue proto = JS_NewObjectProto(ctx, element_proto);
  JS_FreeValue(ctx, element_proto);
  JS_SetPropertyFunctionList(ctx, proto, js_canvas_proto_funcs, countof(js_canvas_proto_funcs));

  JSValue ctor = JS_NewCFunction2(ctx, js_canvas_ctor, "HTMLCanvasElement", 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, ctor, proto);
  JS_SetClassProto(ctx, js_canvas_class_id, proto);

  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, "HTMLCanvasElement", ctor);
  JS_FreeValue(ctx, global);
}
