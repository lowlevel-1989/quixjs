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

// Extensiones: getExtension y getSupportedExtensions.
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "args.h"

static JSValue js_lose_context(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv,
                               int magic, JSValue *data) {
  WebGLCtx *w = JS_GetOpaque(data[0], js_webgl_class_id);
  if (!w) return JS_UNDEFINED;
  if (magic == 0) {
    // loseContext: se liberan sus recursos (PIXI lo hace con el contexto de prueba)
    gl_make_current();
    fb_free(w);
    w->lost = 1;
    if (active == w) active = NULL;
  }
  return JS_UNDEFINED;
}

static const char *supported_ext[] = {
  "WEBGL_lose_context", "WEBGL_debug_renderer_info", "OES_element_index_uint", "OES_standard_derivatives", NULL
};

static int ext_supported(const char *name) {
  if (strcmp(name, "WEBGL_lose_context") == 0 || strcmp(name, "WEBGL_debug_renderer_info") == 0) return 1;
  if (strcmp(name, "OES_element_index_uint") == 0) return gl_has_extension("GL_OES_element_index_uint");
  if (strcmp(name, "OES_standard_derivatives") == 0) return gl_has_extension("GL_OES_standard_derivatives");
  return 0;
}

static JSValue js_gl_getExtension(ARGS) {
  CTX();
  const char *name = argc > 0 ? JS_ToCString(ctx, argv[0]) : NULL;
  if (!name) return JS_NULL;
  JSValue r = JS_NULL;
  if (ext_supported(name)) {
    r = JS_NewObject(ctx);
    if (strcmp(name, "WEBGL_lose_context") == 0) {
      JSValue data[1] = { JS_DupValue(ctx, this_val) };
      JS_SetPropertyStr(ctx, r, "loseContext", JS_NewCFunctionData(ctx, js_lose_context, 0, 0, 1, data));
      JS_SetPropertyStr(ctx, r, "restoreContext", JS_NewCFunctionData(ctx, js_lose_context, 0, 1, 1, data));
      JS_FreeValue(ctx, data[0]);
    } else if (strcmp(name, "WEBGL_debug_renderer_info") == 0) {
      JS_SetPropertyStr(ctx, r, "UNMASKED_VENDOR_WEBGL", JS_NewInt32(ctx, UNMASKED_VENDOR_WEBGL));
      JS_SetPropertyStr(ctx, r, "UNMASKED_RENDERER_WEBGL", JS_NewInt32(ctx, UNMASKED_RENDERER_WEBGL));
    } else if (strcmp(name, "OES_standard_derivatives") == 0) {
      JS_SetPropertyStr(ctx, r, "FRAGMENT_SHADER_DERIVATIVE_HINT_OES", JS_NewInt32(ctx, 0x8B8B));
    }
  }
  JS_FreeCString(ctx, name);
  return r;
}

static JSValue js_gl_getSupportedExtensions(ARGS) {
  CTX();
  JSValue a = JS_NewArray(ctx);
  uint32_t n = 0;
  for (int i = 0; supported_ext[i]; i++) {
    if (ext_supported(supported_ext[i])) JS_SetPropertyUint32(ctx, a, n++, JS_NewString(ctx, supported_ext[i]));
  }
  return a;
}

static JSValue js_gl_get_canvas(JSContext *ctx, JSValueConst this_val) {
  WebGLCtx *w = JS_GetOpaque2(ctx, this_val, js_webgl_class_id);
  if (!w) return JS_EXCEPTION;
  return JS_DupValue(ctx, w->canvas);
}

static JSValue js_gl_get_dbw(JSContext *ctx, JSValueConst this_val, int magic) {
  WebGLCtx *w = JS_GetOpaque2(ctx, this_val, js_webgl_class_id);
  if (!w) return JS_EXCEPTION;
  return JS_NewInt32(ctx, magic ? w->height : w->width);
}
