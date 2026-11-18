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

// WebGL 1 sobre OpenGL ES 2.0. Ver webgl.h.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gfx/gl.h"
#include "binding/image.h"
#include "core/memstats.h"
#include "webgl.h"
#include "gfx/gpu2d.h"
#include "gfx/shader_cache.h"
#include "backend/backend.h"
#include "webgl_constants.h"
#include "core/trace.h"

#define UNPACK_FLIP_Y_WEBGL            0x9240
#define UNPACK_PREMULTIPLY_ALPHA_WEBGL 0x9241
#define CONTEXT_LOST_WEBGL             0x9242
#define UNPACK_COLORSPACE_CONVERSION_WEBGL 0x9243
#define BROWSER_DEFAULT_WEBGL          0x9244
#define WEBGL_DEPTH_STENCIL            0x84F9
#define WEBGL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define UNMASKED_VENDOR_WEBGL          0x9245
#define UNMASKED_RENDERER_WEBGL        0x9246
#define MAX_UNITS 32

typedef struct WebGLCtx {
  JSValue canvas;
  GLuint  fbo, color, depth_stencil, depth, stencil;
  GLuint  color2;              // la otra textura del canvas (webgl_canvas_handoff)
  GLuint  color3;              // con 3: la entregada antes de color2 (la que se ve)
  int     fresh;               // se ha dibujado en el canvas desde la ultima entrega
  int     handed;              // color2 tiene la ultima imagen entregada
  int     width, height;
  int     alpha, want_depth, want_stencil, premultiplied, preserve, antialias;
  int     lost;
  int     flip_y, premultiply;
  int     dirty;
  GLuint  current_fb;          // framebuffer GL que hay ligado
  // lo ligado, como objetos JS (getParameter)
  JSValue program, array_buffer, element_buffer, framebuffer, renderbuffer;
  JSValue textures[MAX_UNITS][2];
  int     unit;
} WebGLCtx;

typedef enum { OBJ_BUFFER, OBJ_FRAMEBUFFER, OBJ_PROGRAM, OBJ_RENDERBUFFER, OBJ_SHADER, OBJ_TEXTURE, OBJ_COUNT } ObjKind;

typedef struct {
  GLuint  name;
  ObjKind kind;
  int     deleted;
  // buffers: su tamano y uso (bufferData), para bufferSubData sin esperas
  GLsizeiptr size;
  GLenum     usage;
} GLObj;

typedef struct {
  GLint  loc;
  GLuint program;
} GLLoc;

static JSClassID js_webgl_class_id;
static JSClassID js_globj_class_id;
static JSClassID js_glloc_class_id;
static JSValue   obj_protos[OBJ_COUNT];
static JSValue   loc_proto;
static int       enabled = 1;
static int       headless = 0;
static int       classes_ready = 0;
static WebGLCtx *active = NULL;

int webgl_enabled(void) { return enabled; }

void webgl_set_enabled(int on, int no_window) {
  enabled = on;
  headless = no_window;
}

// ------------------------------------------------------------ framebuffer

static void fb_free(WebGLCtx *w) {
  if (w->fbo) qgl.DeleteFramebuffers(1, &w->fbo);
  if (w->color) qgl.DeleteTextures(1, &w->color);
  if (w->color2) qgl.DeleteTextures(1, &w->color2);
  if (w->color3) qgl.DeleteTextures(1, &w->color3);
  if (w->depth_stencil) qgl.DeleteRenderbuffers(1, &w->depth_stencil);
  if (w->depth) qgl.DeleteRenderbuffers(1, &w->depth);
  if (w->stencil) qgl.DeleteRenderbuffers(1, &w->stencil);
  w->fbo = w->color = w->color2 = w->color3 = w->depth_stencil = w->depth = w->stencil = 0;
  w->handed = 0;
}

static int fb_create(WebGLCtx *w, int width, int height) {
  if (width < 1) width = 1;
  if (height < 1) height = 1;
  w->width = width;
  w->height = height;

  GLint prev_tex = 0, prev_rb = 0;
  qgl.GetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);
  qgl.GetIntegerv(GL_RENDERBUFFER_BINDING, &prev_rb);

  qgl.GenFramebuffers(1, &w->fbo);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, w->fbo);

  qgl.GenTextures(1, &w->color);
  qgl.BindTexture(GL_TEXTURE_2D, w->color);
  qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, w->color, 0);

  if (w->want_depth || w->want_stencil) {
    if (gl_has_extension("GL_OES_packed_depth_stencil")) {
      qgl.GenRenderbuffers(1, &w->depth_stencil);
      qgl.BindRenderbuffer(GL_RENDERBUFFER, w->depth_stencil);
      qgl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, width, height);
      qgl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, w->depth_stencil);
      qgl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, w->depth_stencil);
    } else {
      if (w->want_depth) {
        qgl.GenRenderbuffers(1, &w->depth);
        qgl.BindRenderbuffer(GL_RENDERBUFFER, w->depth);
        qgl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
        qgl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, w->depth);
      }
      if (w->want_stencil) {
        qgl.GenRenderbuffers(1, &w->stencil);
        qgl.BindRenderbuffer(GL_RENDERBUFFER, w->stencil);
        qgl.RenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height);
        qgl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, w->stencil);
      }
    }
  }

  GLenum status = qgl.CheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) fprintf(stderr, "[webgl] framebuffer incomplete: 0x%x\n", status);

  // como el navegador: el buffer empieza transparente
  qgl.ClearColor(0, 0, 0, 0);
  qgl.ClearDepthf(1);
  qgl.ClearStencil(0);
  qgl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  qgl.BindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);
  qgl.BindRenderbuffer(GL_RENDERBUFFER, (GLuint)prev_rb);
  w->current_fb = w->fbo;
  return status == GL_FRAMEBUFFER_COMPLETE ? 0 : -1;
}

// El contexto GL es compartido: si cambia el contexto WebGL activo se vuelve
// a ligar su framebuffer
static void begin(WebGLCtx *w) {
  gl_make_current();
  if (active != w) {
    active = w;
    qgl.BindFramebuffer(GL_FRAMEBUFFER, w->current_fb);
  }
}

static WebGLCtx *webgl_get(JSContext *ctx, JSValueConst v) {
  WebGLCtx *w = JS_GetOpaque2(ctx, v, js_webgl_class_id);
  if (w) begin(w);
  return w;
}

static void mark_drawn(WebGLCtx *w) {
  if (w->current_fb == w->fbo) w->dirty = w->fresh = 1;
}

void webgl_canvas_sync(CanvasData *cd) {
  WebGLCtx *w = cd->webgl;
  if (!w || !w->dirty || w->lost || !cd->surface.px) return;
  begin(w);
  int width = w->width < cd->surface.w ? w->width : cd->surface.w;
  int height = w->height < cd->surface.h ? w->height : cd->surface.h;
  uint8_t *tmp = malloc((size_t)w->width * w->height * 4);
  if (!tmp) return;

  qgl.BindFramebuffer(GL_FRAMEBUFFER, w->fbo);
  GLint align = 4;
  qgl.GetIntegerv(GL_PACK_ALIGNMENT, &align);
  qgl.PixelStorei(GL_PACK_ALIGNMENT, 4);
  qgl.ReadPixels(0, 0, w->width, w->height, GL_RGBA, GL_UNSIGNED_BYTE, tmp);
  qgl.PixelStorei(GL_PACK_ALIGNMENT, align);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, w->current_fb);

  // GL tiene el origen abajo: se dan la vuelta a las filas. Con
  // premultipliedAlpha el color viene multiplicado por alpha.
  for (int y = 0; y < height; y++) {
    const uint8_t *src = tmp + (size_t)(w->height - 1 - y) * w->width * 4;
    uint8_t *dst = cd->surface.px + (size_t)y * cd->surface.w * 4;
    if (!w->alpha) {
      for (int x = 0; x < width; x++) {
        dst[x * 4] = src[x * 4]; dst[x * 4 + 1] = src[x * 4 + 1]; dst[x * 4 + 2] = src[x * 4 + 2]; dst[x * 4 + 3] = 255;
      }
    } else if (w->premultiplied) {
      for (int x = 0; x < width; x++) {
        int a = src[x * 4 + 3];
        if (a == 255 || a == 0) {
          memcpy(dst + x * 4, src + x * 4, 4);
        } else {
          for (int c = 0; c < 3; c++) {
            int v = src[x * 4 + c] * 255 / a;
            dst[x * 4 + c] = (uint8_t)(v > 255 ? 255 : v);
          }
          dst[x * 4 + 3] = (uint8_t)a;
        }
      }
    } else {
      memcpy(dst, src, (size_t)width * 4);
    }
  }
  free(tmp);
  w->dirty = 0;
}

unsigned webgl_canvas_texture(CanvasData *cd, int *w, int *h, int *premultiplied) {
  WebGLCtx *g = cd->webgl;
  if (!g || g->lost || !g->color) return 0;
  // lo pendiente del contexto WebGL llega antes de que lo use el que presenta
  gl_make_current();
  qgl.Flush();
  *w = g->width;
  *h = g->height;
  *premultiplied = g->alpha ? g->premultiplied : 1;
  return g->color;
}

static GLuint handoff_texture(WebGLCtx *g) {
  GLint prev_tex = 0;
  GLuint t = 0;
  qgl.GetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);
  qgl.GenTextures(1, &t);
  qgl.BindTexture(GL_TEXTURE_2D, t);
  qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g->width, g->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  qgl.BindTexture(GL_TEXTURE_2D, (GLuint)prev_tex);
  return t;
}

unsigned webgl_canvas_handoff(CanvasData *cd, int buffers, int *w, int *h, int *premultiplied) {
  WebGLCtx *g = cd->webgl;
  // con preserveDrawingBuffer el juego cuenta con lo dibujado: no se cambia
  if (!g || g->lost || !g->color || g->preserve) return 0;
  *w = g->width;
  *h = g->height;
  *premultiplied = g->alpha ? g->premultiplied : 1;
  // sin dibujar nada desde la ultima (RPG Maker se salta imagenes cuando va
  // lento, y en los cambios de escena): se vuelve a mostrar la ultima, como
  // el navegador. La otra textura tiene una imagen mas vieja.
  if (!g->fresh) {
    if (g->handed) return g->color2;
    return 0;
  }
  g->fresh = 0;
  g->handed = 1;
  gl_make_current();
  GLint prev_fb = 0;
  qgl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fb);
  // la imagen terminada se entrega; el juego sigue en otra (como en el
  // navegador sin preserveDrawingBuffer, lo que tenga no importa). Con 2, la
  // otra es la entregada antes; con 3 (se muestra la imagen anterior: la
  // entregada antes aun se tiene que ver), la de antes de esa
  GLuint done = g->color;
  GLuint next = buffers >= 3 ? g->color3 : g->color2;
  if (!next) next = handoff_texture(g);
  if (buffers >= 3) g->color3 = g->color2;
  g->color2 = done;
  g->color = next;
  qgl.BindFramebuffer(GL_FRAMEBUFFER, g->fbo);
  qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->color, 0);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, (GLuint)prev_fb);
  return done;
}

void webgl_canvas_resized(CanvasData *cd) {
  WebGLCtx *w = cd->webgl;
  if (!w || w->lost) return;
  begin(w);
  int was_default = w->current_fb == w->fbo;
  fb_free(w);
  fb_create(w, cd->surface.w, cd->surface.h);
  if (!was_default) qgl.BindFramebuffer(GL_FRAMEBUFFER, w->current_fb);
  else w->current_fb = w->fbo;
  qgl.Viewport(0, 0, w->width, w->height);
  w->dirty = 1;
}

// ---------------------------------------------------------------- clases

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

// ----------------------------------------------------------- argumentos

static GLuint U(JSContext *ctx, int argc, JSValueConst *argv, int i) {
  uint32_t v = 0;
  if (i < argc) JS_ToUint32(ctx, &v, argv[i]);
  return v;
}
static GLint I(JSContext *ctx, int argc, JSValueConst *argv, int i) {
  int32_t v = 0;
  if (i < argc) JS_ToInt32(ctx, &v, argv[i]);
  return v;
}
static GLfloat F(JSContext *ctx, int argc, JSValueConst *argv, int i) {
  double v = 0;
  if (i < argc) JS_ToFloat64(ctx, &v, argv[i]);
  return (GLfloat)v;
}
static GLboolean B(JSContext *ctx, int argc, JSValueConst *argv, int i) {
  return (i < argc && JS_ToBool(ctx, argv[i])) ? GL_TRUE : GL_FALSE;
}

#define ARGS JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv
#define CTX() WebGLCtx *w = webgl_get(ctx, this_val); if (!w) return JS_EXCEPTION; if (w->lost) return JS_UNDEFINED
#define u(i) U(ctx, argc, argv, i)
#define n(i) I(ctx, argc, argv, i)
#define f(i) F(ctx, argc, argv, i)
#define b(i) B(ctx, argc, argv, i)

// funciones que solo reciben numeros
#define SIMPLE(js, call) static JSValue js_gl_##js(ARGS) { CTX(); call; return JS_UNDEFINED; }
SIMPLE(blendColor,            qgl.BlendColor(f(0), f(1), f(2), f(3)))
SIMPLE(blendEquation,         qgl.BlendEquation(u(0)))
SIMPLE(blendEquationSeparate, qgl.BlendEquationSeparate(u(0), u(1)))
SIMPLE(blendFunc,             qgl.BlendFunc(u(0), u(1)))
SIMPLE(blendFuncSeparate,     qgl.BlendFuncSeparate(u(0), u(1), u(2), u(3)))
SIMPLE(clearColor,            qgl.ClearColor(f(0), f(1), f(2), f(3)))
SIMPLE(clearDepth,            qgl.ClearDepthf(f(0)))
SIMPLE(clearStencil,          qgl.ClearStencil(n(0)))
SIMPLE(colorMask,             qgl.ColorMask(b(0), b(1), b(2), b(3)))
SIMPLE(cullFace,              qgl.CullFace(u(0)))
SIMPLE(depthFunc,             qgl.DepthFunc(u(0)))
SIMPLE(depthMask,             qgl.DepthMask(b(0)))
SIMPLE(depthRange,            qgl.DepthRangef(f(0), f(1)))
SIMPLE(disable,               qgl.Disable(u(0)))
SIMPLE(enable,                qgl.Enable(u(0)))
SIMPLE(disableVertexAttribArray, qgl.DisableVertexAttribArray(u(0)))
SIMPLE(enableVertexAttribArray,  qgl.EnableVertexAttribArray(u(0)))
SIMPLE(finish,                qgl.Finish())
SIMPLE(flush,                 qgl.Flush())
SIMPLE(frontFace,             qgl.FrontFace(u(0)))
SIMPLE(generateMipmap,        qgl.GenerateMipmap(u(0)))
SIMPLE(hint,                  qgl.Hint(u(0), u(1)))
SIMPLE(lineWidth,             qgl.LineWidth(f(0)))
SIMPLE(polygonOffset,         qgl.PolygonOffset(f(0), f(1)))
SIMPLE(sampleCoverage,        qgl.SampleCoverage(f(0), b(1)))
SIMPLE(scissor,               qgl.Scissor(n(0), n(1), n(2), n(3)))
SIMPLE(stencilFunc,           qgl.StencilFunc(u(0), n(1), u(2)))
SIMPLE(stencilFuncSeparate,   qgl.StencilFuncSeparate(u(0), u(1), n(2), u(3)))
SIMPLE(stencilMask,           qgl.StencilMask(u(0)))
SIMPLE(stencilMaskSeparate,   qgl.StencilMaskSeparate(u(0), u(1)))
SIMPLE(stencilOp,             qgl.StencilOp(u(0), u(1), u(2)))
SIMPLE(stencilOpSeparate,     qgl.StencilOpSeparate(u(0), u(1), u(2), u(3)))
SIMPLE(texParameterf,         qgl.TexParameterf(u(0), u(1), f(2)))
SIMPLE(texParameteri,         qgl.TexParameteri(u(0), u(1), n(2)))
SIMPLE(vertexAttrib1f,        qgl.VertexAttrib1f(u(0), f(1)))
SIMPLE(vertexAttrib2f,        qgl.VertexAttrib2f(u(0), f(1), f(2)))
SIMPLE(vertexAttrib3f,        qgl.VertexAttrib3f(u(0), f(1), f(2), f(3)))
SIMPLE(vertexAttrib4f,        qgl.VertexAttrib4f(u(0), f(1), f(2), f(3), f(4)))
SIMPLE(vertexAttribPointer,   qgl.VertexAttribPointer(u(0), n(1), u(2), b(3), n(4), (const void *)(intptr_t)n(5)))
SIMPLE(viewport,              qgl.Viewport(n(0), n(1), n(2), n(3)))
SIMPLE(copyTexImage2D,        qgl.CopyTexImage2D(u(0), n(1), u(2), n(3), n(4), n(5), n(6), n(7)))
SIMPLE(copyTexSubImage2D,     qgl.CopyTexSubImage2D(u(0), n(1), n(2), n(3), n(4), n(5), n(6), n(7)))

static JSValue js_gl_activeTexture(ARGS) {
  CTX();
  GLuint t = u(0);
  if (t >= GL_TEXTURE0 && t < GL_TEXTURE0 + MAX_UNITS) w->unit = (int)(t - GL_TEXTURE0);
  qgl.ActiveTexture(t);
  return JS_UNDEFINED;
}

static JSValue js_gl_clear(ARGS) {
  CTX();
  qgl.Clear(u(0));
  mark_drawn(w);
  return JS_UNDEFINED;
}

static JSValue js_gl_drawArrays(ARGS) {
  CTX();
  qgl.DrawArrays(u(0), n(1), n(2));
  mark_drawn(w);
  return JS_UNDEFINED;
}

static JSValue js_gl_drawElements(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  qgl.DrawElements(u(0), n(1), u(2), (const void *)(intptr_t)n(3));
  mark_drawn(w);
  return JS_UNDEFINED;
}

static JSValue js_gl_getError(ARGS) {
  WebGLCtx *w = webgl_get(ctx, this_val);
  if (!w) return JS_EXCEPTION;
  return JS_NewInt32(ctx, (int)qgl.GetError());
}

static JSValue js_gl_isEnabled(ARGS) {
  CTX();
  return JS_NewBool(ctx, qgl.IsEnabled(u(0)));
}

static JSValue js_gl_pixelStorei(ARGS) {
  CTX();
  GLenum p = u(0);
  if (p == UNPACK_FLIP_Y_WEBGL) w->flip_y = n(1) != 0;
  else if (p == UNPACK_PREMULTIPLY_ALPHA_WEBGL) w->premultiply = n(1) != 0;
  else if (p == UNPACK_COLORSPACE_CONVERSION_WEBGL) {}
  else qgl.PixelStorei(p, n(1));
  return JS_UNDEFINED;
}

static JSValue js_gl_checkFramebufferStatus(ARGS) {
  CTX();
  return JS_NewInt32(ctx, (int)qgl.CheckFramebufferStatus(u(0)));
}

// ------------------------------------------------------------- objetos

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

// ------------------------------------------------------- shaders y programas

// 1 si el mediump de los fragmentos es de menos precision que highp y la GPU
// tiene highp en ellos (Mali: 16 bits frente a 32; en un PC son iguales)
static int tile_needs_highp(void) {
  static int answer = -1;
  if (answer >= 0) return answer;
  GLint range[2] = { 0, 0 }, medium = 23, high = 0;
  qgl.GetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT, range, &medium);
  qgl.GetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &high);
  answer = medium < 23 && high >= 23;
  if (answer) printf("[webgl] mediump is %d bits here: the tilemap shader uses highp\n", medium + 1);
  return answer;
}

static JSValue js_gl_shaderSource(ARGS) {
  CTX();
  GLuint s = argc > 0 ? obj_name(argv[0], OBJ_SHADER) : 0;
  const char *src = argc > 1 ? JS_ToCString(ctx, argv[1]) : NULL;
  // Los tiles de RPG Maker (pixi-tilemap, el shader con uSamplerSize) calculan
  // la posicion en la textura en pixeles: con mediump de 16 bits (Mali) se
  // equivoca en 1-2 px en texturas grandes y salen lineas entre los tiles.
  // En ese shader, highp si la GPU lo tiene y su mediump es de menos
  // precision. Solo ese: highp en todos costaba ~6 fps en una RG CubeXX.
  char *patched = NULL;
  if (s && src && strstr(src, "uSamplerSize") && strstr(src, "precision mediump float") && tile_needs_highp()) {
    patched = strdup(src);
    for (char *p = patched; patched && (p = strstr(p, "precision mediump float")) != NULL; p += 10) {
      memcpy(p + 10, "highp  ", 7);
    }
  }
  if (s && src) {
    const char *use = patched ? patched : src;
    qgl.ShaderSource(s, 1, &use, NULL);
  }
  free(patched);
  if (src) JS_FreeCString(ctx, src);
  return JS_UNDEFINED;
}

static JSValue js_gl_compileShader(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  if (argc > 0) qgl.CompileShader(obj_name(argv[0], OBJ_SHADER));
  return JS_UNDEFINED;
}

static JSValue js_gl_attachShader(ARGS) {
  CTX();
  if (argc > 1) qgl.AttachShader(obj_name(argv[0], OBJ_PROGRAM), obj_name(argv[1], OBJ_SHADER));
  return JS_UNDEFINED;
}

static JSValue js_gl_detachShader(ARGS) {
  CTX();
  if (argc > 1) qgl.DetachShader(obj_name(argv[0], OBJ_PROGRAM), obj_name(argv[1], OBJ_SHADER));
  return JS_UNDEFINED;
}

static JSValue js_gl_linkProgram(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  // de la cache de disco si ya se enlazo otra vez (gfx/shader_cache.c)
  if (argc > 0) shader_cache_link(obj_name(argv[0], OBJ_PROGRAM));
  return JS_UNDEFINED;
}

static JSValue js_gl_validateProgram(ARGS) {
  CTX();
  if (argc > 0) qgl.ValidateProgram(obj_name(argv[0], OBJ_PROGRAM));
  return JS_UNDEFINED;
}

static JSValue js_gl_useProgram(ARGS) {
  CTX();
  JSValueConst obj = argc > 0 ? argv[0] : JS_NULL;
  qgl.UseProgram(obj_name(obj, OBJ_PROGRAM));
  set_slot(ctx, &w->program, obj);
  return JS_UNDEFINED;
}

static JSValue js_gl_bindAttribLocation(ARGS) {
  CTX();
  const char *name = argc > 2 ? JS_ToCString(ctx, argv[2]) : NULL;
  if (name) {
    qgl.BindAttribLocation(obj_name(argv[0], OBJ_PROGRAM), u(1), name);
    shader_cache_bind(obj_name(argv[0], OBJ_PROGRAM), u(1), name);
    JS_FreeCString(ctx, name);
  }
  return JS_UNDEFINED;
}

static JSValue js_gl_getShaderParameter(ARGS) {
  CTX();
  GLuint s = argc > 0 ? obj_name(argv[0], OBJ_SHADER) : 0;
  GLenum p = u(1);
  GLint v = 0;
  if (!s) return JS_NULL;
  qgl.GetShaderiv(s, p, &v);
  if (p == GL_SHADER_TYPE) return JS_NewInt32(ctx, v);
  return JS_NewBool(ctx, v);
}

static JSValue js_gl_getProgramParameter(ARGS) {
  CTX();
  GLuint pr = argc > 0 ? obj_name(argv[0], OBJ_PROGRAM) : 0;
  GLenum p = u(1);
  GLint v = 0;
  if (!pr) return JS_NULL;
  qgl.GetProgramiv(pr, p, &v);
  if (p == GL_LINK_STATUS || p == GL_DELETE_STATUS || p == GL_VALIDATE_STATUS) return JS_NewBool(ctx, v);
  return JS_NewInt32(ctx, v);
}

static JSValue info_log(JSContext *ctx, GLuint id, int program) {
  GLint len = 0;
  if (program) qgl.GetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
  else qgl.GetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
  if (len <= 1) return JS_NewString(ctx, "");
  char *buf = malloc((size_t)len);
  if (!buf) return JS_NewString(ctx, "");
  if (program) qgl.GetProgramInfoLog(id, len, NULL, buf);
  else qgl.GetShaderInfoLog(id, len, NULL, buf);
  JSValue r = JS_NewString(ctx, buf);
  free(buf);
  return r;
}

static JSValue js_gl_getShaderInfoLog(ARGS) {
  CTX();
  return info_log(ctx, argc > 0 ? obj_name(argv[0], OBJ_SHADER) : 0, 0);
}

static JSValue js_gl_getProgramInfoLog(ARGS) {
  CTX();
  return info_log(ctx, argc > 0 ? obj_name(argv[0], OBJ_PROGRAM) : 0, 1);
}

static JSValue js_gl_getShaderSource(ARGS) {
  CTX();
  GLuint s = argc > 0 ? obj_name(argv[0], OBJ_SHADER) : 0;
  GLint len = 0;
  if (!s) return JS_NULL;
  qgl.GetShaderiv(s, GL_SHADER_SOURCE_LENGTH, &len);
  char *buf = malloc((size_t)(len > 0 ? len : 1));
  if (!buf) return JS_NULL;
  buf[0] = 0;
  if (len > 0) qgl.GetShaderSource(s, len, NULL, buf);
  JSValue r = JS_NewString(ctx, buf);
  free(buf);
  return r;
}

static JSValue js_gl_getAttribLocation(ARGS) {
  CTX();
  const char *name = argc > 1 ? JS_ToCString(ctx, argv[1]) : NULL;
  GLint r = -1;
  if (name) {
    r = qgl.GetAttribLocation(obj_name(argv[0], OBJ_PROGRAM), name);
    JS_FreeCString(ctx, name);
  }
  return JS_NewInt32(ctx, r);
}

static JSValue js_gl_getUniformLocation(ARGS) {
  CTX();
  const char *name = argc > 1 ? JS_ToCString(ctx, argv[1]) : NULL;
  if (!name) return JS_NULL;
  GLuint pr = obj_name(argv[0], OBJ_PROGRAM);
  GLint loc = qgl.GetUniformLocation(pr, name);
  JS_FreeCString(ctx, name);
  if (loc < 0) return JS_NULL;
  JSValue o = JS_NewObjectProtoClass(ctx, loc_proto, js_glloc_class_id);
  GLLoc *l = js_mallocz(ctx, sizeof(GLLoc));
  if (!l) {
    JS_FreeValue(ctx, o);
    return JS_EXCEPTION;
  }
  l->loc = loc;
  l->program = pr;
  JS_SetOpaque(o, l);
  return o;
}

static JSValue active_info(ARGS, int uniform) {
  GLuint pr = argc > 0 ? obj_name(argv[0], OBJ_PROGRAM) : 0;
  if (!pr) return JS_NULL;
  char name[256];
  GLsizei len = 0;
  GLint size = 0;
  GLenum type = 0;
  name[0] = 0;
  if (uniform) qgl.GetActiveUniform(pr, u(1), sizeof(name), &len, &size, &type, name);
  else qgl.GetActiveAttrib(pr, u(1), sizeof(name), &len, &size, &type, name);
  if (len <= 0) return JS_NULL;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "name", JS_NewString(ctx, name));
  JS_SetPropertyStr(ctx, o, "size", JS_NewInt32(ctx, size));
  JS_SetPropertyStr(ctx, o, "type", JS_NewInt32(ctx, (int)type));
  return o;
}

static JSValue js_gl_getActiveUniform(ARGS) {
  CTX();
  return active_info(ctx, this_val, argc, argv, 1);
}

static JSValue js_gl_getActiveAttrib(ARGS) {
  CTX();
  return active_info(ctx, this_val, argc, argv, 0);
}

static JSValue js_gl_getAttachedShaders(ARGS) {
  CTX();
  return JS_NewArray(ctx);
}

static JSValue js_gl_getShaderPrecisionFormat(ARGS) {
  CTX();
  GLint range[2] = { 127, 127 }, precision = 23;
  qgl.GetShaderPrecisionFormat(u(0), u(1), range, &precision);
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "rangeMin", JS_NewInt32(ctx, range[0]));
  JS_SetPropertyStr(ctx, o, "rangeMax", JS_NewInt32(ctx, range[1]));
  JS_SetPropertyStr(ctx, o, "precision", JS_NewInt32(ctx, precision));
  return o;
}

// -------------------------------------------------------------- uniforms

static GLint loc_of(JSValueConst v) {
  GLLoc *l = JS_GetOpaque(v, js_glloc_class_id);
  return l ? l->loc : -1;
}

#define UNI(js, call) static JSValue js_gl_##js(ARGS) { CTX(); \
  GLint L = argc > 0 ? loc_of(argv[0]) : -1; if (L >= 0) call; return JS_UNDEFINED; }
UNI(uniform1f, qgl.Uniform1f(L, f(1)))
UNI(uniform2f, qgl.Uniform2f(L, f(1), f(2)))
UNI(uniform3f, qgl.Uniform3f(L, f(1), f(2), f(3)))
UNI(uniform4f, qgl.Uniform4f(L, f(1), f(2), f(3), f(4)))
UNI(uniform1i, qgl.Uniform1i(L, n(1)))
UNI(uniform2i, qgl.Uniform2i(L, n(1), n(2)))
UNI(uniform3i, qgl.Uniform3i(L, n(1), n(2), n(3)))
UNI(uniform4i, qgl.Uniform4i(L, n(1), n(2), n(3), n(4)))

// Float32Array / Int32Array / array JS -> datos contiguos
static void *array_data(JSContext *ctx, JSValueConst v, int is_float, int *count, int *owned) {
  *owned = 0;
  *count = 0;
  size_t off = 0, blen = 0, bpe = 0, size = 0;
  if (JS_BufferKind(ctx, v) == JS_BUFFER_TYPED_ARRAY) {
    JSValue buf = JS_GetTypedArrayBuffer(ctx, v, &off, &blen, &bpe);
    uint8_t *p = JS_IsException(buf) ? NULL : JS_GetArrayBuffer(ctx, &size, buf);
    JS_FreeValue(ctx, buf);
    if (p && bpe == 4) {
      *count = (int)(blen / 4);
      // un Int32Array en un uniform de float (o al reves) se convierte
      return p + off;
    }
    return NULL;
  }
  if (!JS_IsArray(ctx, v)) return NULL;
  uint32_t len = 0;
  JSValue l = JS_GetPropertyStr(ctx, v, "length");
  JS_ToUint32(ctx, &len, l);
  JS_FreeValue(ctx, l);
  void *out = malloc(len * 4 + 4);
  if (!out) return NULL;
  for (uint32_t i = 0; i < len; i++) {
    JSValue x = JS_GetPropertyUint32(ctx, v, i);
    if (is_float) {
      double d = 0;
      JS_ToFloat64(ctx, &d, x);
      ((float *)out)[i] = (float)d;
    } else {
      int32_t d = 0;
      JS_ToInt32(ctx, &d, x);
      ((int32_t *)out)[i] = d;
    }
    JS_FreeValue(ctx, x);
  }
  *owned = 1;
  *count = (int)len;
  return out;
}

#define UNIV(js, per, is_float, call) static JSValue js_gl_##js(ARGS) { CTX(); \
  GLint L = argc > 0 ? loc_of(argv[0]) : -1; if (L < 0 || argc < 2) return JS_UNDEFINED; \
  int cnt, owned; void *d = array_data(ctx, argv[argc - 1], is_float, &cnt, &owned); \
  if (d && cnt >= per) { call; } if (owned) { free(d); } return JS_UNDEFINED; }
UNIV(uniform1fv, 1, 1, qgl.Uniform1fv(L, cnt / 1, d))
UNIV(uniform2fv, 2, 1, qgl.Uniform2fv(L, cnt / 2, d))
UNIV(uniform3fv, 3, 1, qgl.Uniform3fv(L, cnt / 3, d))
UNIV(uniform4fv, 4, 1, qgl.Uniform4fv(L, cnt / 4, d))
UNIV(uniform1iv, 1, 0, qgl.Uniform1iv(L, cnt / 1, d))
UNIV(uniform2iv, 2, 0, qgl.Uniform2iv(L, cnt / 2, d))
UNIV(uniform3iv, 3, 0, qgl.Uniform3iv(L, cnt / 3, d))
UNIV(uniform4iv, 4, 0, qgl.Uniform4iv(L, cnt / 4, d))
UNIV(uniformMatrix2fv, 4, 1, qgl.UniformMatrix2fv(L, cnt / 4, GL_FALSE, d))
UNIV(uniformMatrix3fv, 9, 1, qgl.UniformMatrix3fv(L, cnt / 9, GL_FALSE, d))
UNIV(uniformMatrix4fv, 16, 1, qgl.UniformMatrix4fv(L, cnt / 16, GL_FALSE, d))

#define ATTRV(js, per, call) static JSValue js_gl_##js(ARGS) { CTX(); \
  int cnt, owned; void *d = argc > 1 ? array_data(ctx, argv[1], 1, &cnt, &owned) : NULL; \
  if (d && cnt >= per) { call; } if (d && owned) { free(d); } return JS_UNDEFINED; }
ATTRV(vertexAttrib1fv, 1, qgl.VertexAttrib1fv(u(0), d))
ATTRV(vertexAttrib2fv, 2, qgl.VertexAttrib2fv(u(0), d))
ATTRV(vertexAttrib3fv, 3, qgl.VertexAttrib3fv(u(0), d))
ATTRV(vertexAttrib4fv, 4, qgl.VertexAttrib4fv(u(0), d))

// --------------------------------------------------------------- buffers

// bytes de un ArrayBuffer o una vista
static uint8_t *bytes_of(JSContext *ctx, JSValueConst v, size_t *len) {
  if (JS_BufferKind(ctx, v) != JS_BUFFER_NONE) return JS_GetBufferBytes(ctx, v, len);
  // DataView
  size_t size = 0;
  JSValue bv = JS_GetPropertyStr(ctx, v, "buffer");
  JSValue bo = JS_GetPropertyStr(ctx, v, "byteOffset");
  JSValue bl = JS_GetPropertyStr(ctx, v, "byteLength");
  uint32_t o = 0, l = 0;
  JS_ToUint32(ctx, &o, bo);
  JS_ToUint32(ctx, &l, bl);
  uint8_t *p = JS_BufferKind(ctx, bv) == JS_BUFFER_ARRAY_BUFFER ? JS_GetArrayBuffer(ctx, &size, bv) : NULL;
  JS_FreeValue(ctx, bv);
  JS_FreeValue(ctx, bo);
  JS_FreeValue(ctx, bl);
  *len = p ? l : 0;
  return p ? p + o : NULL;
}

// El buffer enlazado a target (su GLObj), o NULL
static GLObj *bound_buffer(WebGLCtx *w, GLenum target) {
  if (target == GL_ARRAY_BUFFER) return obj_get(w->array_buffer);
  if (target == GL_ELEMENT_ARRAY_BUFFER) return obj_get(w->element_buffer);
  return NULL;
}

static JSValue js_gl_bufferData(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  if (argc < 3) return JS_UNDEFINED;
  GLsizeiptr size;
  if (JS_IsNumber(argv[1])) {
    size = (GLsizeiptr)n(1);
    qgl.BufferData(u(0), size, NULL, u(2));
  } else {
    size_t len = 0;
    uint8_t *p = bytes_of(ctx, argv[1], &len);
    size = (GLsizeiptr)len;
    qgl.BufferData(u(0), size, p, u(2));
  }
  GLObj *b = bound_buffer(w, u(0));
  if (b) {
    b->size = size;
    b->usage = u(2);
  }
  return JS_UNDEFINED;
}

static JSValue js_gl_bufferSubData(ARGS) {
  CTX();
  if (argc < 3) return JS_UNDEFINED;
  size_t len = 0;
  uint8_t *p = bytes_of(ctx, argv[2], &len);
  if (!p) return JS_UNDEFINED;
  // Un buffer de STREAM/DYNAMIC que se reescribe desde el principio (los
  // lotes de sprites de PIXI): se le pide memoria nueva antes (orphaning).
  // Si no, el driver espera a que la GPU acabe de usarlo (en panfrost, ~1.6
  // ms cada vez). Lo que hubiera detras de lo escrito se pierde: con esos
  // usos se vuelve a escribir antes de dibujar.
  //
  // (Probado y descartado en la RG CubeXX: orphaning tambien de los buffers
  // que se reescriben enteros, como el quad de 64 bytes de los filtros de
  // PIXI, que espera a la GPU una mediana de 4.4 ms, y con un tamano distinto
  // cada vez para que mesa no reutilice la memoria. bufferSubData bajaba de
  // ~500 a ~110 us por llamada, pero la batalla iba peor: sin esas esperas el
  // juego manda a la GPU mas de lo que acaba y el ritmo de las imagenes se
  // vuelve irregular, de ~100 a 300-600 imagenes tarde.)
  GLObj *b = bound_buffer(w, u(0));
  if (b && n(1) == 0 && b->size >= (GLsizeiptr)len && (b->usage == GL_STREAM_DRAW || b->usage == GL_DYNAMIC_DRAW)) {
    qgl.BufferData(u(0), b->size, NULL, b->usage);
  }
  TRACE_START(tb);
  qgl.BufferSubData(u(0), (GLintptr)n(1), (GLsizeiptr)len, p);
  TRACE_END(tb, 1.0, "bufferSubData %zu bytes", len);
  return JS_UNDEFINED;
}

// --------------------------------------------------------------- texturas

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "prepare_pixels supone little endian (el alfa de RGBA en el byte alto de un uint32)"
#endif

// Copia de la imagen con UNPACK_FLIP_Y / UNPACK_PREMULTIPLY_ALPHA aplicados
// (premultiply_row).
//
// El resultado va a un buffer que se reutiliza (no se libera): pedir memoria
// nueva en cada subida obligaba al kernel a preparar cada pagina al escribirla
// (~825 en una imagen de 960x880), y eso era la mayor parte del tiempo. Si
// una imagen pide mas de 4 MB (mas que una hoja de animacion de 960x880), se
// usa y se suelta: el buffer fijo no pasa de 4 MB.
#define PREPARE_KEEP_MAX (4u << 20)
static uint8_t *prepare_buf;
static size_t prepare_cap;

// Una fila premultiplicada (c * a / 255 redondeado, la misma cuenta en las
// dos versiones). En aarch64, 16 pixeles a la vez con NEON: en la RG CubeXX
// una ventana de 816x396 pasa de ~5,7 ms a ~2,0 ms
static void premultiply_row_scalar(uint8_t *d, const uint8_t *s, int n) {
  for (int x = 0; x < n; x++) {
    uint32_t p, o;
    memcpy(&p, s + x * 4, 4);   // sin suponer alineacion (un ImageData puede no estarlo)
    uint32_t a = p >> 24;   // RGBA en memoria: el alfa en el byte alto (little endian)
    if (a == 255) {
      o = p;
    } else if (a == 0) {
      o = 0;
    } else {
      uint32_t r = p & 255, g = (p >> 8) & 255, b = (p >> 16) & 255;
      r = r * a + 128; r = (r + (r >> 8)) >> 8;
      g = g * a + 128; g = (g + (g >> 8)) >> 8;
      b = b * a + 128; b = (b + (b >> 8)) >> 8;
      o = r | (g << 8) | (b << 16) | (a << 24);
    }
    memcpy(d + x * 4, &o, 4);
  }
}

#if defined(__aarch64__)
#include <arm_neon.h>
// (c * a + 128 + ((c * a + 128) >> 8)) >> 8: con a = 255 da c, con a = 0 da 0
static inline uint8x8_t mul255(uint8x8_t c, uint8x8_t a) {
  uint16x8_t t = vaddq_u16(vmull_u8(c, a), vdupq_n_u16(128));
  return vshrn_n_u16(vaddq_u16(t, vshrq_n_u16(t, 8)), 8);
}

static inline uint8x16_t mul255q(uint8x16_t c, uint8x16_t a) {
  return vcombine_u8(mul255(vget_low_u8(c), vget_low_u8(a)), mul255(vget_high_u8(c), vget_high_u8(a)));
}

static void premultiply_row(uint8_t *d, const uint8_t *s, int n) {
  int x = 0;
  for (; x + 16 <= n; x += 16) {
    uint8x16x4_t p = vld4q_u8(s + x * 4);   // R, G, B y A de 16 pixeles, separados
    p.val[0] = mul255q(p.val[0], p.val[3]);
    p.val[1] = mul255q(p.val[1], p.val[3]);
    p.val[2] = mul255q(p.val[2], p.val[3]);
    vst4q_u8(d + x * 4, p);
  }
  premultiply_row_scalar(d + x * 4, s + x * 4, n - x);
}
#else
#define premultiply_row premultiply_row_scalar
#endif

static void prepare_done(uint8_t *conv) {
  if (conv && conv != prepare_buf) free(conv);
}

static uint8_t *prepare_pixels(WebGLCtx *w, const uint8_t *src, int width, int height) {
  if (!w->flip_y && !w->premultiply) return NULL;
  size_t row = (size_t)width * 4, need = row * height;
  uint8_t *out;
  if (need > PREPARE_KEEP_MAX) {
    out = malloc(need);
  } else {
    if (need > prepare_cap) {
      uint8_t *nb = realloc(prepare_buf, need);
      if (!nb) return NULL;
      prepare_buf = nb;
      prepare_cap = need;
    }
    out = prepare_buf;
  }
  if (!out) return NULL;
  for (int y = 0; y < height; y++) {
    const uint8_t *s = src + row * (w->flip_y ? height - 1 - y : y);
    uint8_t *d = out + row * y;
    if (!w->premultiply) {
      memcpy(d, s, row);
      continue;
    }
    premultiply_row(d, s, width);
  }
  return out;
}

// Las texturas que ya tienen el tamano de un gpu-canvas copiado: la siguiente
// copia va con glCopyTexSubImage2D. Volver a darles memoria cada imagen
// (glCopyTexImage2D) hace que el driver espere a que la GPU deje de usar la
// anterior: en panfrost eran ~10 ms por imagen con la mascara de luces.
#define COPY_CACHE 8
static struct { GLuint tex; int w, h; } copy_cache[COPY_CACHE];

static void copy_cache_forget(GLuint tex) {
  for (int i = 0; i < COPY_CACHE; i++) {
    if (copy_cache[i].tex == tex) copy_cache[i].tex = 0;
  }
}

static GLuint bound_texture_2d(void) {
  GLint t = 0;
  qgl.GetIntegerv(GL_TEXTURE_BINDING_2D, &t);
  return (GLuint)t;
}

// gpu-canvas como fuente: se copia en la GPU (glCopyTexImage2D desde su
// textura), sin leerlo. Solo si el juego la quiere premultiplicada y sin dar
// la vuelta (lo normal en PIXI); si no, se lee como cualquier canvas.
// Devuelve 0 si la ha copiado.
static int gpu_canvas_copy(JSContext *ctx, WebGLCtx *w, JSValueConst v, GLenum target, GLint level, int sub,
                           GLint xoff, GLint yoff) {
  CanvasData *cd = js_canvas_get(ctx, v);
  if (!cd || !cd->gpu || !w->premultiply || w->flip_y) return -1;
  int gw = 0, gh = 0;
  unsigned tex = gpu2d_texture(cd->gpu, &gw, &gh);
  if (!tex) return -1;
  GLuint bound = target == GL_TEXTURE_2D ? bound_texture_2d() : 0;
  // el canvas entero en 0,0 (texImage2D, o texSubImage2D en 0,0 como hace PIXI
  // al volver a subir)
  int whole = level == 0 && (!sub || (xoff == 0 && yoff == 0));
  // ya dibuja en esta textura (gpu2d_retarget): pintado lo pendiente, nada
  // que copiar
  if (whole && gpu2d_is_target(cd->gpu, bound)) return 0;
  static GLuint copy_fbo = 0;
  GLint prev = 0;
  qgl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &prev);
  if (!copy_fbo) qgl.GenFramebuffers(1, &copy_fbo);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, copy_fbo);
  qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  int ok = qgl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  int same = 0;
  if (ok) {
    GLuint dst = target == GL_TEXTURE_2D ? bound_texture_2d() : 0;
    int slot = -1;
    for (int i = 0; dst && i < COPY_CACHE; i++) {
      if (copy_cache[i].tex == dst) {
        slot = i;
        same = copy_cache[i].w == gw && copy_cache[i].h == gh;
      }
    }
    if (sub) {
      qgl.CopyTexSubImage2D(target, level, xoff, yoff, 0, 0, gw, gh);
    } else if (same && level == 0) {
      qgl.CopyTexSubImage2D(target, 0, 0, 0, 0, 0, gw, gh);
    } else {
      qgl.CopyTexImage2D(target, level, GL_RGBA, 0, 0, gw, gh, 0);
      if (dst && level == 0) {
        same = 1;   // ahora tiene el tamano del canvas
        if (slot < 0) {
          for (int i = 0; i < COPY_CACHE; i++) if (!copy_cache[i].tex) { slot = i; break; }
          if (slot < 0) slot = (int)(dst % COPY_CACHE);
        }
        copy_cache[slot].tex = dst;
        copy_cache[slot].w = gw;
        copy_cache[slot].h = gh;
      }
    }
  }
  qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, (GLuint)prev);
  // copiado entero a una textura de su tamano: desde ahora dibuja en ella
  if (ok && whole && same && bound) gpu2d_retarget(cd->gpu, bound);
  return ok ? 0 : -1;
}

// Imagen o canvas como fuente (RGBA). Tambien ImageData.
static int source_pixels(JSContext *ctx, JSValueConst v, const uint8_t **px, int *width, int *height) {
  CanvasData *cd = js_canvas_get(ctx, v);
  if (cd && cd->webgl) webgl_canvas_sync(cd);
  const Surface *s = cd ? js_get_image_source(ctx, v) : js_image_get_surface_gpu(ctx, v);
  if (s) {
    *px = s->px;
    *width = s->w;
    *height = s->h;
    return s->px ? 0 : -1;
  }
  // ImageData { width, height, data }
  if (JS_IsObject(v)) {
    JSValue d = JS_GetPropertyStr(ctx, v, "data");
    JSValue wv = JS_GetPropertyStr(ctx, v, "width");
    JSValue hv = JS_GetPropertyStr(ctx, v, "height");
    int32_t ww = 0, hh = 0;
    JS_ToInt32(ctx, &ww, wv);
    JS_ToInt32(ctx, &hh, hv);
    size_t len = 0;
    uint8_t *p = JS_IsObject(d) ? bytes_of(ctx, d, &len) : NULL;
    JS_FreeValue(ctx, d);
    JS_FreeValue(ctx, wv);
    JS_FreeValue(ctx, hv);
    if (p && ww > 0 && hh > 0 && len >= (size_t)ww * hh * 4) {
      *px = p;
      *width = ww;
      *height = hh;
      return 0;
    }
  }
  return -1;
}

// la memoria de la textura atada (nivel 0), para memstats
static void texture_size(WebGLCtx *w, GLenum target, GLint level, long width, long height) {
  if (target != GL_TEXTURE_2D || level != 0) return;
  GLObj *t = obj_get(w->textures[w->unit][0]);
  if (!t) return;
  long bytes = width * height * 4;
  memstats_texture(bytes - (long)t->size, t->size ? 0 : 1);
  t->size = bytes;
}

static JSValue js_gl_texImage2D(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  GLenum target = u(0);
  GLint level = n(1);
  if (argc == 6) {
    // texImage2D(target, level, internalformat, format, type, source)
    if (gpu_canvas_copy(ctx, w, argv[5], target, level, 0, 0, 0) == 0) return JS_UNDEFINED;
    if (target == GL_TEXTURE_2D) copy_cache_forget(bound_texture_2d());
    const uint8_t *px;
    int width, height;
    if (source_pixels(ctx, argv[5], &px, &width, &height) != 0) return JS_UNDEFINED;
    TRACE_START(tc);
    uint8_t *conv = prepare_pixels(w, px, width, height);
    TRACE_END(tc, 0.5, "texImage2D %dx%d premultiply", width, height);
    TRACE_START(tu);
    GLint align = 4;
    qgl.GetIntegerv(GL_UNPACK_ALIGNMENT, &align);
    qgl.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
    qgl.TexImage2D(target, level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, conv ? conv : px);
    qgl.PixelStorei(GL_UNPACK_ALIGNMENT, align);
    TRACE_END(tu, 0.5, "texImage2D %dx%d %s upload", width, height, js_canvas_get(ctx, argv[5]) ? "canvas" : "image");
    prepare_done(conv);
    texture_size(w, target, level, width, height);
    js_image_uploaded(ctx, argv[5]);
  } else if (argc >= 8) {
    // texImage2D(target, level, internalformat, width, height, border, format, type, pixels)
    if (target == GL_TEXTURE_2D) copy_cache_forget(bound_texture_2d());
    size_t len = 0;
    uint8_t *p = (argc > 8 && JS_IsObject(argv[8])) ? bytes_of(ctx, argv[8], &len) : NULL;
    qgl.TexImage2D(target, level, n(2), n(3), n(4), n(5), u(6), u(7), p);
    texture_size(w, target, level, n(3), n(4));
  }
  return JS_UNDEFINED;
}

static JSValue js_gl_texSubImage2D(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  if (argc == 7) {
    // texSubImage2D(target, level, x, y, format, type, source)
    if (gpu_canvas_copy(ctx, w, argv[6], u(0), n(1), 1, n(2), n(3)) == 0) return JS_UNDEFINED;
    const uint8_t *px;
    int width, height;
    if (source_pixels(ctx, argv[6], &px, &width, &height) != 0) return JS_UNDEFINED;
    TRACE_START(tc);
    uint8_t *conv = prepare_pixels(w, px, width, height);
    TRACE_END(tc, 0.5, "texSubImage2D %dx%d premultiply", width, height);
    TRACE_START(tt);
    GLint align = 4;
    qgl.GetIntegerv(GL_UNPACK_ALIGNMENT, &align);
    qgl.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
    qgl.TexSubImage2D(u(0), n(1), n(2), n(3), width, height, GL_RGBA, GL_UNSIGNED_BYTE, conv ? conv : px);
    qgl.PixelStorei(GL_UNPACK_ALIGNMENT, align);
    TRACE_END(tt, 1.0, "texSubImage2D %dx%d", width, height);
    prepare_done(conv);
    js_image_uploaded(ctx, argv[6]);
  } else if (argc >= 9) {
    size_t len = 0;
    uint8_t *p = JS_IsObject(argv[8]) ? bytes_of(ctx, argv[8], &len) : NULL;
    if (p) qgl.TexSubImage2D(u(0), n(1), n(2), n(3), n(4), n(5), u(6), u(7), p);
  }
  return JS_UNDEFINED;
}

static JSValue js_gl_compressedTexImage2D(ARGS) {
  CTX();
  return JS_UNDEFINED;   // sin texturas comprimidas (getSupportedExtensions no las anuncia)
}

static JSValue js_gl_readPixels(ARGS) {
  CTX();
  if (argc < 7) return JS_UNDEFINED;
  size_t len = 0;
  uint8_t *p = bytes_of(ctx, argv[6], &len);
  TRACE_START(tr);
  if (p && len >= (size_t)n(2) * n(3) * 4) qgl.ReadPixels(n(0), n(1), n(2), n(3), u(4), u(5), p);
  TRACE_END(tr, 1.0, "readPixels %dx%d", (int)n(2), (int)n(3));
  return JS_UNDEFINED;
}

// ------------------------------------------------------------ consultas

static JSValue int_array(JSContext *ctx, const GLint *v, int n) {
  JSValue len = JS_NewInt32(ctx, n);
  JSValue a = JS_NewTypedArray(ctx, 1, &len, JS_TYPED_ARRAY_INT32);
  for (int i = 0; i < n; i++) JS_SetPropertyUint32(ctx, a, i, JS_NewInt32(ctx, v[i]));
  return a;
}

static JSValue float_array(JSContext *ctx, const GLfloat *v, int n) {
  JSValue len = JS_NewInt32(ctx, n);
  JSValue a = JS_NewTypedArray(ctx, 1, &len, JS_TYPED_ARRAY_FLOAT32);
  for (int i = 0; i < n; i++) JS_SetPropertyUint32(ctx, a, i, JS_NewFloat64(ctx, v[i]));
  return a;
}

static JSValue js_gl_getParameter(ARGS) {
  CTX();
  GLenum p = u(0);
  GLint iv[4] = { 0 };
  GLfloat fv[4] = { 0 };
  GLboolean bv[4] = { 0 };
  switch (p) {
    case GL_VENDOR:                   return JS_NewString(ctx, "quixjs");
    case GL_RENDERER:                 return JS_NewString(ctx, "quixjs WebGL");
    case GL_VERSION:                  return JS_NewString(ctx, "WebGL 1.0 (OpenGL ES 2.0, quixjs)");
    case GL_SHADING_LANGUAGE_VERSION: return JS_NewString(ctx, "WebGL GLSL ES 1.0 (quixjs)");
    case UNMASKED_VENDOR_WEBGL:       return JS_NewString(ctx, (const char *)qgl.GetString(GL_VENDOR));
    case UNMASKED_RENDERER_WEBGL:     return JS_NewString(ctx, gl_renderer_name());
    case UNPACK_FLIP_Y_WEBGL:         return JS_NewBool(ctx, w->flip_y);
    case UNPACK_PREMULTIPLY_ALPHA_WEBGL: return JS_NewBool(ctx, w->premultiply);
    case UNPACK_COLORSPACE_CONVERSION_WEBGL: return JS_NewInt32(ctx, BROWSER_DEFAULT_WEBGL);
    case GL_CURRENT_PROGRAM:          return JS_DupValue(ctx, w->program);
    case GL_ARRAY_BUFFER_BINDING:     return JS_DupValue(ctx, w->array_buffer);
    case GL_ELEMENT_ARRAY_BUFFER_BINDING: return JS_DupValue(ctx, w->element_buffer);
    case GL_FRAMEBUFFER_BINDING:      return JS_DupValue(ctx, w->framebuffer);
    case GL_RENDERBUFFER_BINDING:     return JS_DupValue(ctx, w->renderbuffer);
    case GL_TEXTURE_BINDING_2D:       return JS_DupValue(ctx, w->textures[w->unit][0]);
    case GL_TEXTURE_BINDING_CUBE_MAP: return JS_DupValue(ctx, w->textures[w->unit][1]);
    case GL_VIEWPORT:
    case GL_SCISSOR_BOX:
      qgl.GetIntegerv(p, iv);
      return int_array(ctx, iv, 4);
    case GL_MAX_VIEWPORT_DIMS:
      qgl.GetIntegerv(p, iv);
      return int_array(ctx, iv, 2);
    case GL_COLOR_CLEAR_VALUE:
    case GL_BLEND_COLOR:
      qgl.GetFloatv(p, fv);
      return float_array(ctx, fv, 4);
    case GL_DEPTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
      qgl.GetFloatv(p, fv);
      return float_array(ctx, fv, 2);
    case GL_DEPTH_CLEAR_VALUE:
    case GL_LINE_WIDTH:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_SAMPLE_COVERAGE_VALUE:
      qgl.GetFloatv(p, fv);
      return JS_NewFloat64(ctx, fv[0]);
    case GL_COLOR_WRITEMASK: {
      qgl.GetBooleanv(p, bv);
      JSValue a = JS_NewArray(ctx);
      for (int i = 0; i < 4; i++) JS_SetPropertyUint32(ctx, a, i, JS_NewBool(ctx, bv[i]));
      return a;
    }
    case GL_BLEND: case GL_CULL_FACE: case GL_DEPTH_TEST: case GL_DITHER: case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE: case GL_SAMPLE_COVERAGE: case GL_SCISSOR_TEST: case GL_STENCIL_TEST:
    case GL_DEPTH_WRITEMASK: case GL_SAMPLE_COVERAGE_INVERT:
      qgl.GetBooleanv(p, bv);
      return JS_NewBool(ctx, bv[0]);
    case GL_COMPRESSED_TEXTURE_FORMATS:
      return int_array(ctx, iv, 0);
    default:
      while (qgl.GetError() != GL_NO_ERROR) {}
      qgl.GetIntegerv(p, iv);
      if (qgl.GetError() != GL_NO_ERROR) return JS_NULL;
      return JS_NewInt32(ctx, iv[0]);
  }
}

static JSValue js_gl_getBufferParameter(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetBufferParameteriv(u(0), u(1), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getRenderbufferParameter(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetRenderbufferParameteriv(u(0), u(1), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getTexParameter(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetTexParameteriv(u(0), u(1), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getFramebufferAttachmentParameter(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetFramebufferAttachmentParameteriv(u(0), u(1), u(2), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getVertexAttrib(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetVertexAttribiv(u(0), u(1), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getVertexAttribOffset(ARGS) {
  CTX();
  void *ptr = NULL;
  qgl.GetVertexAttribPointerv(u(0), u(1), &ptr);
  return JS_NewInt64(ctx, (int64_t)(intptr_t)ptr);
}

static JSValue js_gl_getUniform(ARGS) {
  CTX();
  return JS_NULL;   // TODO: casi ningun juego lo usa
}

static JSValue js_gl_isContextLost(ARGS) {
  WebGLCtx *w = JS_GetOpaque2(ctx, this_val, js_webgl_class_id);
  if (!w) return JS_EXCEPTION;
  return JS_NewBool(ctx, w->lost);
}

static JSValue js_gl_getContextAttributes(ARGS) {
  WebGLCtx *w = JS_GetOpaque2(ctx, this_val, js_webgl_class_id);
  if (!w) return JS_EXCEPTION;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "alpha", JS_NewBool(ctx, w->alpha));
  JS_SetPropertyStr(ctx, o, "depth", JS_NewBool(ctx, w->want_depth));
  JS_SetPropertyStr(ctx, o, "stencil", JS_NewBool(ctx, w->want_stencil));
  JS_SetPropertyStr(ctx, o, "antialias", JS_FALSE);
  JS_SetPropertyStr(ctx, o, "premultipliedAlpha", JS_NewBool(ctx, w->premultiplied));
  JS_SetPropertyStr(ctx, o, "preserveDrawingBuffer", JS_NewBool(ctx, w->preserve));
  JS_SetPropertyStr(ctx, o, "failIfMajorPerformanceCaveat", JS_FALSE);
  JS_SetPropertyStr(ctx, o, "powerPreference", JS_NewString(ctx, "default"));
  return o;
}

// ------------------------------------------------------------- extensiones

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

#define FN(name, len) JS_CFUNC_DEF(#name, len, js_gl_##name)
static const JSCFunctionListEntry js_webgl_proto[] = {
  JS_CGETSET_DEF("canvas", js_gl_get_canvas, NULL),
  JS_CGETSET_MAGIC_DEF("drawingBufferWidth", js_gl_get_dbw, NULL, 0),
  JS_CGETSET_MAGIC_DEF("drawingBufferHeight", js_gl_get_dbw, NULL, 1),
  FN(activeTexture, 1), FN(attachShader, 2), FN(bindAttribLocation, 3), FN(bindBuffer, 2),
  FN(bindFramebuffer, 2), FN(bindRenderbuffer, 2), FN(bindTexture, 2), FN(blendColor, 4),
  FN(blendEquation, 1), FN(blendEquationSeparate, 2), FN(blendFunc, 2), FN(blendFuncSeparate, 4),
  FN(bufferData, 3), FN(bufferSubData, 3), FN(checkFramebufferStatus, 1), FN(clear, 1),
  FN(clearColor, 4), FN(clearDepth, 1), FN(clearStencil, 1), FN(colorMask, 4), FN(compileShader, 1),
  FN(compressedTexImage2D, 7), FN(copyTexImage2D, 8), FN(copyTexSubImage2D, 8),
  FN(createBuffer, 0), FN(createFramebuffer, 0), FN(createProgram, 0), FN(createRenderbuffer, 0),
  FN(createShader, 1), FN(createTexture, 0), FN(cullFace, 1),
  FN(deleteBuffer, 1), FN(deleteFramebuffer, 1), FN(deleteProgram, 1), FN(deleteRenderbuffer, 1),
  FN(deleteShader, 1), FN(deleteTexture, 1), FN(depthFunc, 1), FN(depthMask, 1), FN(depthRange, 2),
  FN(detachShader, 2), FN(disable, 1), FN(disableVertexAttribArray, 1), FN(drawArrays, 3),
  FN(drawElements, 4), FN(enable, 1), FN(enableVertexAttribArray, 1), FN(finish, 0), FN(flush, 0),
  FN(framebufferRenderbuffer, 4), FN(framebufferTexture2D, 5), FN(frontFace, 1), FN(generateMipmap, 1),
  FN(getActiveAttrib, 2), FN(getActiveUniform, 2), FN(getAttachedShaders, 1), FN(getAttribLocation, 2),
  FN(getBufferParameter, 2), FN(getContextAttributes, 0), FN(getError, 0), FN(getExtension, 1),
  FN(getFramebufferAttachmentParameter, 3), FN(getParameter, 1), FN(getProgramInfoLog, 1),
  FN(getProgramParameter, 2), FN(getRenderbufferParameter, 2), FN(getShaderInfoLog, 1),
  FN(getShaderParameter, 2), FN(getShaderPrecisionFormat, 2), FN(getShaderSource, 1),
  FN(getSupportedExtensions, 0), FN(getTexParameter, 2), FN(getUniform, 2), FN(getUniformLocation, 2),
  FN(getVertexAttrib, 2), FN(getVertexAttribOffset, 2), FN(hint, 2), FN(isBuffer, 1),
  FN(isContextLost, 0), FN(isEnabled, 1), FN(isFramebuffer, 1), FN(isProgram, 1), FN(isRenderbuffer, 1),
  FN(isShader, 1), FN(isTexture, 1), FN(lineWidth, 1), FN(linkProgram, 1), FN(pixelStorei, 2),
  FN(polygonOffset, 2), FN(readPixels, 7), FN(renderbufferStorage, 4), FN(sampleCoverage, 2),
  FN(scissor, 4), FN(shaderSource, 2), FN(stencilFunc, 3), FN(stencilFuncSeparate, 4),
  FN(stencilMask, 1), FN(stencilMaskSeparate, 2), FN(stencilOp, 3), FN(stencilOpSeparate, 4),
  FN(texImage2D, 9), FN(texParameterf, 3), FN(texParameteri, 3), FN(texSubImage2D, 9),
  FN(uniform1f, 2), FN(uniform1fv, 2), FN(uniform1i, 2), FN(uniform1iv, 2),
  FN(uniform2f, 3), FN(uniform2fv, 2), FN(uniform2i, 3), FN(uniform2iv, 2),
  FN(uniform3f, 4), FN(uniform3fv, 2), FN(uniform3i, 4), FN(uniform3iv, 2),
  FN(uniform4f, 5), FN(uniform4fv, 2), FN(uniform4i, 5), FN(uniform4iv, 2),
  FN(uniformMatrix2fv, 3), FN(uniformMatrix3fv, 3), FN(uniformMatrix4fv, 3), FN(useProgram, 1),
  FN(validateProgram, 1), FN(vertexAttrib1f, 2), FN(vertexAttrib1fv, 2), FN(vertexAttrib2f, 3),
  FN(vertexAttrib2fv, 2), FN(vertexAttrib3f, 4), FN(vertexAttrib3fv, 2), FN(vertexAttrib4f, 5),
  FN(vertexAttrib4fv, 2), FN(vertexAttribPointer, 6), FN(viewport, 4),
};

static JSValue js_illegal_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  return JS_ThrowTypeError(ctx, "Illegal constructor");
}

static void classes_init(JSContext *ctx) {
  if (classes_ready) return;
  JSRuntime *rt = JS_GetRuntime(ctx);
  JS_NewClassID(&js_webgl_class_id);
  JS_NewClass(rt, js_webgl_class_id, &js_webgl_class);
  JS_NewClassID(&js_globj_class_id);
  JS_NewClass(rt, js_globj_class_id, &js_globj_class);
  JS_NewClassID(&js_glloc_class_id);
  JS_NewClass(rt, js_glloc_class_id, &js_glloc_class);

  JSValue global = JS_GetGlobalObject(ctx);

  JSValue proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, proto, js_webgl_proto, countof(js_webgl_proto));
  JS_SetPropertyFunctionList(ctx, proto, js_webgl_constants, countof(js_webgl_constants));
  JSValue ctor = JS_NewCFunction2(ctx, js_illegal_ctor, "WebGLRenderingContext", 0, JS_CFUNC_constructor, 0);
  JS_SetPropertyFunctionList(ctx, ctor, js_webgl_constants, countof(js_webgl_constants));
  JS_SetConstructor(ctx, ctor, proto);
  JS_SetClassProto(ctx, js_webgl_class_id, proto);
  JS_SetPropertyStr(ctx, global, "WebGLRenderingContext", ctor);

  static const char *names[OBJ_COUNT] = { "WebGLBuffer", "WebGLFramebuffer", "WebGLProgram",
                                          "WebGLRenderbuffer", "WebGLShader", "WebGLTexture" };
  for (int i = 0; i < OBJ_COUNT; i++) {
    obj_protos[i] = JS_NewObject(ctx);
    JSValue c = JS_NewCFunction2(ctx, js_illegal_ctor, names[i], 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, c, obj_protos[i]);
    JS_SetPropertyStr(ctx, global, names[i], c);
    // el prototipo vive mientras el constructor este en global
    JS_FreeValue(ctx, obj_protos[i]);
  }
  loc_proto = JS_NewObject(ctx);
  JSValue lc = JS_NewCFunction2(ctx, js_illegal_ctor, "WebGLUniformLocation", 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, lc, loc_proto);
  JS_SetPropertyStr(ctx, global, "WebGLUniformLocation", lc);
  JS_FreeValue(ctx, loc_proto);

  JS_FreeValue(ctx, global);
  classes_ready = 1;
}

void js_api_add_webgl(JSContext *ctx) {
  // las clases (WebGLRenderingContext...) existen desde el principio: los
  // juegos las miran antes de pedir el contexto. El contexto GL se crea con
  // el primer getContext("webgl").
  if (enabled) classes_init(ctx);
}

static int attr_bool(JSContext *ctx, JSValueConst attrs, const char *name, int def) {
  if (!JS_IsObject(attrs)) return def;
  JSValue v = JS_GetPropertyStr(ctx, attrs, name);
  int r = JS_IsUndefined(v) ? def : JS_ToBool(ctx, v);
  JS_FreeValue(ctx, v);
  return r;
}

JSValue js_create_webgl_context(JSContext *ctx, JSValueConst canvas, JSValueConst attrs) {
  if (!enabled) return JS_NULL;
  if (gl_init(headless) != 0) {
    enabled = 0;   // sin GL: los juegos usan su renderer de canvas
    return JS_NULL;
  }
  classes_init(ctx);
  gl_make_current();

  CanvasData *cd = js_canvas_get(ctx, canvas);
  if (!cd) return JS_NULL;

  JSValue obj = JS_NewObjectClass(ctx, js_webgl_class_id);
  if (JS_IsException(obj)) return obj;
  WebGLCtx *w = js_mallocz(ctx, sizeof(WebGLCtx));
  if (!w) {
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
  }
  w->canvas = canvas;
  JS_SetHidden(ctx, obj, "canvas", JS_DupValue(ctx, canvas));
  w->program = w->array_buffer = w->element_buffer = w->framebuffer = w->renderbuffer = JS_NULL;
  for (int i = 0; i < MAX_UNITS; i++) w->textures[i][0] = w->textures[i][1] = JS_NULL;
  w->alpha = attr_bool(ctx, attrs, "alpha", 1);
  w->want_depth = attr_bool(ctx, attrs, "depth", 1);
  w->want_stencil = attr_bool(ctx, attrs, "stencil", 0);
  w->premultiplied = attr_bool(ctx, attrs, "premultipliedAlpha", 1);
  w->preserve = attr_bool(ctx, attrs, "preserveDrawingBuffer", 0);
  w->antialias = 0;
  JS_SetOpaque(obj, w);

  active = w;
  if (fb_create(w, cd->surface.w, cd->surface.h) != 0) {
    // sin framebuffer valido no se puede dibujar
    JS_FreeValue(ctx, obj);
    return JS_NULL;
  }
  qgl.Viewport(0, 0, w->width, w->height);
  cd->webgl = w;
  return obj;
}
