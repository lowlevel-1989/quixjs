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

// create*, delete* e is* de buffers, framebuffers, programas,
// renderbuffers, shaders y texturas.
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "args.h"

#define CREATE(js, K, gen) \
  static JSValue js_gl_##js(ARGS) { CTX(); GLuint id = 0; gen; return new_obj(ctx, K, id); }
CREATE(createBuffer,       OBJ_BUFFER,       qgl.GenBuffers(1, &id))
CREATE(createFramebuffer,  OBJ_FRAMEBUFFER,  qgl.GenFramebuffers(1, &id))
CREATE(createRenderbuffer, OBJ_RENDERBUFFER, qgl.GenRenderbuffers(1, &id))
CREATE(createTexture,      OBJ_TEXTURE,      qgl.GenTextures(1, &id))
CREATE(createProgram,      OBJ_PROGRAM,      id = qgl.CreateProgram())
CREATE(createShader,       OBJ_SHADER,       id = qgl.CreateShader(u(0)))

static void forget(JSContext *ctx, JSValue *slot, JSValueConst obj) {
  if (JS_IsObject(*slot) && JS_VALUE_GET_PTR(*slot) == JS_VALUE_GET_PTR(obj)) {
    JS_FreeValue(ctx, *slot);
    *slot = JS_NULL;
  }
}

static JSValue js_gl_delete(ARGS, ObjKind kind) {
  CTX();
  if (argc < 1) return JS_UNDEFINED;
  GLObj *g = obj_get(argv[0]);
  if (!g || g->kind != kind || g->deleted) return JS_UNDEFINED;
  GLuint id = g->name;
  switch (kind) {
    case OBJ_BUFFER:
      qgl.DeleteBuffers(1, &id);
      forget(ctx, &w->array_buffer, argv[0]);
      forget(ctx, &w->element_buffer, argv[0]);
      break;
    case OBJ_FRAMEBUFFER:
      if (w->current_fb == id) {
        w->current_fb = w->fbo;
        qgl.BindFramebuffer(GL_FRAMEBUFFER, w->fbo);
      }
      qgl.DeleteFramebuffers(1, &id);
      forget(ctx, &w->framebuffer, argv[0]);
      break;
    case OBJ_RENDERBUFFER:
      qgl.DeleteRenderbuffers(1, &id);
      forget(ctx, &w->renderbuffer, argv[0]);
      break;
    case OBJ_TEXTURE:
      qgl.DeleteTextures(1, &id);
      if (g->size) memstats_texture(-(long)g->size, -1);
      g->size = 0;
      for (int i = 0; i < MAX_UNITS; i++) {
        forget(ctx, &w->textures[i][0], argv[0]);
        forget(ctx, &w->textures[i][1], argv[0]);
      }
      break;
    case OBJ_PROGRAM:
      qgl.DeleteProgram(id);
      shader_cache_forget(id);
      forget(ctx, &w->program, argv[0]);
      break;
    case OBJ_SHADER:
      qgl.DeleteShader(id);
      break;
    default: break;
  }
  g->deleted = 1;
  return JS_UNDEFINED;
}
static JSValue js_gl_deleteBuffer(ARGS)       { return js_gl_delete(ctx, this_val, argc, argv, OBJ_BUFFER); }
static JSValue js_gl_deleteFramebuffer(ARGS)  { return js_gl_delete(ctx, this_val, argc, argv, OBJ_FRAMEBUFFER); }
static JSValue js_gl_deleteRenderbuffer(ARGS) { return js_gl_delete(ctx, this_val, argc, argv, OBJ_RENDERBUFFER); }
static void copy_cache_forget(GLuint tex);
static JSValue js_gl_deleteTexture(ARGS) {
  if (argc > 0) {
    GLuint t = obj_name(argv[0], OBJ_TEXTURE);
    copy_cache_forget(t);
    gpu2d_forget_texture(t);   // un canvas de GPU que dibujaba en ella
  }
  return js_gl_delete(ctx, this_val, argc, argv, OBJ_TEXTURE);
}
static JSValue js_gl_deleteProgram(ARGS)      { return js_gl_delete(ctx, this_val, argc, argv, OBJ_PROGRAM); }
static JSValue js_gl_deleteShader(ARGS)       { return js_gl_delete(ctx, this_val, argc, argv, OBJ_SHADER); }

#define IS(js, K) static JSValue js_gl_##js(ARGS) { CTX(); \
  GLObj *g = argc > 0 ? obj_get(argv[0]) : NULL; return JS_NewBool(ctx, g && g->kind == K && !g->deleted); }
IS(isBuffer, OBJ_BUFFER)
IS(isFramebuffer, OBJ_FRAMEBUFFER)
IS(isRenderbuffer, OBJ_RENDERBUFFER)
IS(isTexture, OBJ_TEXTURE)
IS(isProgram, OBJ_PROGRAM)
IS(isShader, OBJ_SHADER)

static void set_slot(JSContext *ctx, JSValue *slot, JSValueConst v) {
  JS_FreeValue(ctx, *slot);
  *slot = JS_IsObject(v) ? JS_DupValue(ctx, v) : JS_NULL;
}

static JSValue js_gl_bindBuffer(ARGS) {
  CTX();
  GLenum target = u(0);
  JSValueConst obj = argc > 1 ? argv[1] : JS_NULL;
  qgl.BindBuffer(target, obj_name(obj, OBJ_BUFFER));
  if (target == GL_ARRAY_BUFFER) set_slot(ctx, &w->array_buffer, obj);
  else if (target == GL_ELEMENT_ARRAY_BUFFER) set_slot(ctx, &w->element_buffer, obj);
  return JS_UNDEFINED;
}

static JSValue js_gl_bindFramebuffer(ARGS) {
  CTX();
  JSValueConst obj = argc > 1 ? argv[1] : JS_NULL;
  GLuint id = obj_name(obj, OBJ_FRAMEBUFFER);
  // null es el framebuffer del canvas
  w->current_fb = id ? id : w->fbo;
  qgl.BindFramebuffer(GL_FRAMEBUFFER, w->current_fb);
  set_slot(ctx, &w->framebuffer, obj);
  return JS_UNDEFINED;
}

static JSValue js_gl_bindRenderbuffer(ARGS) {
  CTX();
  JSValueConst obj = argc > 1 ? argv[1] : JS_NULL;
  qgl.BindRenderbuffer(GL_RENDERBUFFER, obj_name(obj, OBJ_RENDERBUFFER));
  set_slot(ctx, &w->renderbuffer, obj);
  return JS_UNDEFINED;
}

static JSValue js_gl_bindTexture(ARGS) {
  CTX();
  GLenum target = u(0);
  JSValueConst obj = argc > 1 ? argv[1] : JS_NULL;
  qgl.BindTexture(target, obj_name(obj, OBJ_TEXTURE));
  set_slot(ctx, &w->textures[w->unit][target == GL_TEXTURE_CUBE_MAP ? 1 : 0], obj);
  return JS_UNDEFINED;
}

static JSValue js_gl_framebufferTexture2D(ARGS) {
  CTX();
  qgl.FramebufferTexture2D(u(0), u(1), u(2), argc > 3 ? obj_name(argv[3], OBJ_TEXTURE) : 0, n(4));
  return JS_UNDEFINED;
}

static JSValue js_gl_framebufferRenderbuffer(ARGS) {
  CTX();
  GLuint rb = argc > 3 ? obj_name(argv[3], OBJ_RENDERBUFFER) : 0;
  if (u(1) == WEBGL_DEPTH_STENCIL_ATTACHMENT) {
    qgl.FramebufferRenderbuffer(u(0), GL_DEPTH_ATTACHMENT, u(2), rb);
    qgl.FramebufferRenderbuffer(u(0), GL_STENCIL_ATTACHMENT, u(2), rb);
  } else {
    qgl.FramebufferRenderbuffer(u(0), u(1), u(2), rb);
  }
  return JS_UNDEFINED;
}

static JSValue js_gl_renderbufferStorage(ARGS) {
  CTX();
  GLenum fmt = u(1);
  if (fmt == WEBGL_DEPTH_STENCIL) {
    fmt = gl_has_extension("GL_OES_packed_depth_stencil") ? GL_DEPTH24_STENCIL8_OES : GL_STENCIL_INDEX8;
  }
  qgl.RenderbufferStorage(u(0), fmt, n(2), n(3));
  return JS_UNDEFINED;
}
