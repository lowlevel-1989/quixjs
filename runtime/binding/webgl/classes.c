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

// Las clases de JS: WebGLRenderingContext, WebGLObject y
// WebGLUniformLocation (finalizadores y marcado para el GC).
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

static void js_webgl_finalizer(JSRuntime *rt, JSValue val) {
  WebGLCtx *w = JS_GetOpaque(val, js_webgl_class_id);
  if (!w) return;
  if (gl_available()) {
    gl_make_current();
    fb_free(w);
  }
  if (active == w) active = NULL;
  // w->canvas es prestado (lo mantiene viva la propiedad oculta "canvas")
  JS_FreeValueRT(rt, w->program);
  JS_FreeValueRT(rt, w->array_buffer);
  JS_FreeValueRT(rt, w->element_buffer);
  JS_FreeValueRT(rt, w->framebuffer);
  JS_FreeValueRT(rt, w->renderbuffer);
  for (int i = 0; i < MAX_UNITS; i++) {
    JS_FreeValueRT(rt, w->textures[i][0]);
    JS_FreeValueRT(rt, w->textures[i][1]);
  }
  js_free_rt(rt, w);
}

static void js_webgl_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark) {
  WebGLCtx *w = JS_GetOpaque(val, js_webgl_class_id);
  if (!w) return;
  JS_MarkValue(rt, w->canvas, mark);
  JS_MarkValue(rt, w->program, mark);
  JS_MarkValue(rt, w->array_buffer, mark);
  JS_MarkValue(rt, w->element_buffer, mark);
  JS_MarkValue(rt, w->framebuffer, mark);
  JS_MarkValue(rt, w->renderbuffer, mark);
  for (int i = 0; i < MAX_UNITS; i++) {
    JS_MarkValue(rt, w->textures[i][0], mark);
    JS_MarkValue(rt, w->textures[i][1], mark);
  }
}

static JSClassDef js_webgl_class = { "WebGLRenderingContext", .finalizer = js_webgl_finalizer, .gc_mark = js_webgl_mark };

static void js_globj_finalizer(JSRuntime *rt, JSValue val) {
  // los nombres GL no se borran aqui: el juego llama a deleteX (PIXI lo hace)
  js_free_rt(rt, JS_GetOpaque(val, js_globj_class_id));
}
static JSClassDef js_globj_class = { "WebGLObject", .finalizer = js_globj_finalizer };

static void js_glloc_finalizer(JSRuntime *rt, JSValue val) {
  js_free_rt(rt, JS_GetOpaque(val, js_glloc_class_id));
}
static JSClassDef js_glloc_class = { "WebGLUniformLocation", .finalizer = js_glloc_finalizer };

static JSValue new_obj(JSContext *ctx, ObjKind kind, GLuint name) {
  JSValue o = JS_NewObjectProtoClass(ctx, obj_protos[kind], js_globj_class_id);
  if (JS_IsException(o)) return o;
  GLObj *g = js_mallocz(ctx, sizeof(GLObj));
  if (!g) {
    JS_FreeValue(ctx, o);
    return JS_EXCEPTION;
  }
  g->name = name;
  g->kind = kind;
  JS_SetOpaque(o, g);
  return o;
}

static GLuint obj_name(JSValueConst v, ObjKind kind) {
  GLObj *g = JS_GetOpaque(v, js_globj_class_id);
  return (g && g->kind == kind && !g->deleted) ? g->name : 0;
}

static GLObj *obj_get(JSValueConst v) {
  return JS_GetOpaque(v, js_globj_class_id);
}
