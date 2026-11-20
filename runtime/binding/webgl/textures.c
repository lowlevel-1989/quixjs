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

// Texturas: texImage2D y texSubImage2D (desde imagenes, canvas y arrays),
// premultiplicar y voltear, y la cache de copias.
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "args.h"

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
