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

// El canvas de WebGL: webgl_enabled, su framebuffer (y las texturas que
// se entregan para presentar) y el cambio de tamano.
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

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
