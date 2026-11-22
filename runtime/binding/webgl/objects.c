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
      if (w->current_fb == id || (g->alias && w->current_fb == g->alias)) {
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
static void pending_forget(GLuint tex);
static void fb_alias_forget(WebGLCtx *w, GLuint tex);
static void evict_forget(JSContext *ctx, GLuint tex);
static void evict_touch(JSContext *ctx, GLuint tex);
static JSValue js_gl_deleteTexture(ARGS) {
  if (argc > 0) {
    GLuint t = obj_name(argv[0], OBJ_TEXTURE);
    copy_cache_forget(t);
    evict_forget(ctx, t);      // la que se podia soltar (evict.c)
    pending_forget(t);         // la que se iba a rellenar al decodificarse
    gpu2d_forget_texture(t);   // un canvas de GPU que dibujaba en ella
    WebGLCtx *w = webgl_get(ctx, this_val);
    if (w) fb_alias_forget(w, t);
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

// ---- un framebuffer por textura de destino
// Construct 2 (y otros) dibuja sus efectos en texturas con un solo
// framebuffer al que le cambia la textura: framebufferTexture2D en cada
// cambio. Con libmali eso cuesta ~0,3 ms (vuelve a validar el framebuffer);
// cambiar de framebuffer, no. Mientras el del juego tiene una textura de
// color enganchada (y nada mas), quixjs usa en su lugar uno propio con esa
// textura, creado una vez. Para el juego no cambia nada.
#define FB_ALIAS_MAX 64
static struct { GLuint tex, fb; } fb_alias[FB_ALIAS_MAX];

static GLuint fb_alias_for(GLuint tex) {
  int free_slot = -1;
  for (int i = 0; i < FB_ALIAS_MAX; i++) {
    if (fb_alias[i].tex == tex) return fb_alias[i].fb;
    if (!fb_alias[i].tex && free_slot < 0) free_slot = i;
  }
  if (free_slot < 0) return 0;
  GLuint fb = 0;
  qgl.GenFramebuffers(1, &fb);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, fb);
  qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  fb_alias[free_slot].tex = tex;
  fb_alias[free_slot].fb = fb;
  return fb;
}

static int fb_alias_alive(GLuint fb) {
  for (int i = 0; i < FB_ALIAS_MAX; i++)
    if (fb && fb_alias[i].fb == fb) return 1;
  return 0;
}

// la textura se borra: su framebuffer tambien
static void fb_alias_forget(WebGLCtx *w, GLuint tex) {
  for (int i = 0; tex && i < FB_ALIAS_MAX; i++) {
    if (fb_alias[i].tex != tex) continue;
    if (w->current_fb == fb_alias[i].fb) {
      w->current_fb = w->fbo;
      qgl.BindFramebuffer(GL_FRAMEBUFFER, w->fbo);
    }
    qgl.DeleteFramebuffers(1, &fb_alias[i].fb);
    fb_alias[i].tex = fb_alias[i].fb = 0;
  }
}

static JSValue js_gl_bindFramebuffer(ARGS) {
  CTX();
  JSValueConst obj = argc > 1 ? argv[1] : JS_NULL;
  GLuint id = obj_name(obj, OBJ_FRAMEBUFFER);
  GLObj *g = id ? obj_get(obj) : NULL;
  if (g && g->alias) {
    if (fb_alias_alive(g->alias)) id = g->alias;
    else g->alias = 0;
  }
  // null es el framebuffer del canvas
  GLuint fb = id ? id : w->fbo;
  // WebGL 2: READ_FRAMEBUFFER solo para leer (blitFramebuffer, readPixels);
  // ata solo ese y no cambia donde se dibuja. FRAMEBUFFER y DRAW_FRAMEBUFFER
  // cambian donde se dibuja (FRAMEBUFFER tambien de donde se lee)
  GLenum target = u(0);
  if (target == GL_READ_FRAMEBUFFER) {
    qgl.BindFramebuffer(GL_READ_FRAMEBUFFER, fb);
    return JS_UNDEFINED;
  }
  w->current_fb = fb;
  qgl.BindFramebuffer(target == GL_DRAW_FRAMEBUFFER ? GL_DRAW_FRAMEBUFFER : GL_FRAMEBUFFER, fb);
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
  GLuint name = obj_name(obj, OBJ_TEXTURE);
  qgl.BindTexture(target, name);
  set_slot(ctx, &w->textures[w->unit][target == GL_TEXTURE_CUBE_MAP ? 1 : 0], obj);
  if (target == GL_TEXTURE_2D) evict_touch(ctx, name);   // una soltada vuelve (evict.c)
  return JS_UNDEFINED;
}

// el framebuffer del juego que esta ligado (NULL: el del canvas)
static GLObj *bound_fb(WebGLCtx *w) {
  GLObj *g = obj_get(w->framebuffer);
  return (g && g->kind == OBJ_FRAMEBUFFER && !g->deleted) ? g : NULL;
}

// deja de hacer de alias: el framebuffer del juego, el suyo, con la textura
// de color que tenia el alias (three.js engancha el color y despues la
// profundidad: sin esto, su framebuffer se quedaba sin color y no se veia)
static void fb_unalias(WebGLCtx *w, GLObj *g) {
  if (!g || !g->alias) return;
  GLuint tex = 0;
  for (int i = 0; i < FB_ALIAS_MAX; i++)
    if (fb_alias[i].fb == g->alias) tex = fb_alias[i].tex;
  g->alias = 0;
  w->current_fb = g->name;
  qgl.BindFramebuffer(GL_FRAMEBUFFER, g->name);
  if (tex) qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
}

static JSValue js_gl_framebufferTexture2D(ARGS) {
  CTX();
  GLuint tex = argc > 3 ? obj_name(argv[3], OBJ_TEXTURE) : 0;
  GLObj *g = bound_fb(w);
  if (g && !g->no_alias && u(0) == GL_FRAMEBUFFER && u(1) == GL_COLOR_ATTACHMENT0 &&
      u(2) == GL_TEXTURE_2D && n(4) == 0) {
    GLuint fb = tex ? fb_alias_for(tex) : 0;
    if (fb || !tex) {
      g->alias = fb;
      w->current_fb = fb ? fb : g->name;
      qgl.BindFramebuffer(GL_FRAMEBUFFER, w->current_fb);
      return JS_UNDEFINED;
    }
  }
  if (g) {
    fb_unalias(w, g);
    g->no_alias = 1;
  }
  qgl.FramebufferTexture2D(u(0), u(1), u(2), tex, n(4));
  return JS_UNDEFINED;
}

static JSValue js_gl_framebufferRenderbuffer(ARGS) {
  CTX();
  GLuint rb = argc > 3 ? obj_name(argv[3], OBJ_RENDERBUFFER) : 0;
  GLObj *g = bound_fb(w);
  if (g) {                  // con renderbuffers, el framebuffer tal cual
    fb_unalias(w, g);
    g->no_alias = 1;
  }
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
