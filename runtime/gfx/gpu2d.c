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

// Canvas 2d en la GPU. Ver gpu2d.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gl.h"
#include "gpu2d.h"
#include "shaders.h"   // runtime/shaders, generado por tools/embed_shaders.py

#define LUT_ROWS 64          // gradientes por tanda (una fila de 256 colores cada uno)
#define MAX_CMDS 1024

typedef enum { MODE_COLOR = 0, MODE_LINEAR = 1, MODE_RADIAL = 2, MODE_CLEAR = 3 } PaintMode;

typedef struct {
  float       pos[8];        // las 4 esquinas en el dispositivo (tira de triangulos)
  float       uv[8];         // las mismas en el espacio de usuario (gradientes)
  float       mt[8];         // en la mascara (0..1)
  PaintMode   mode;
  float       color[4];      // color sin premultiplicar (0..1)
  float       alpha;         // globalAlpha
  float       grad[6];       // x0, y0, x1, y1, r0, r1
  int         lut;           // fila del gradiente
  CompositeOp op;
  uint8_t    *mask;          // cobertura de la caja (o NULL)
  int         mw, mh;
} Cmd;

struct Gpu2D {
  int       w, h;
  unsigned  tex;             // compartida con WebGL
  unsigned  fbo;             // del contexto auxiliar
  Cmd      *cmds;
  int       count, cap;
  uint8_t   lut[LUT_ROWS][256 * 4];
  int       luts;
  int       opaque;          // todo el canvas es opaco (tras un relleno opaco entero)
  int       external;        // tex es la textura de WebGL a la que se subia (gpu2d_retarget)
};

// los canvas que dibujan en una textura de WebGL (para gpu2d_forget_texture)
#define MAX_EXTERNAL 16
static Gpu2D *external_list[MAX_EXTERNAL];

static void external_set(Gpu2D *g, int on) {
  for (int i = 0; i < MAX_EXTERNAL; i++) {
    if (external_list[i] == g) external_list[i] = NULL;
  }
  if (!on) return;
  for (int i = 0; i < MAX_EXTERNAL; i++) {
    if (!external_list[i]) {
      external_list[i] = g;
      return;
    }
  }
}

// estado del contexto auxiliar (uno para todos los canvas). Un programa por
// pintura y con o sin mascara: sin ramas en el fragment shader, que en una
// GPU pequena (Mali-G31) es lo que cuesta.
typedef struct {
  unsigned prog;
  int u_size, u_color, u_alpha, u_g, u_r, u_lut, u_row, u_mask;
} Program;

static Program programs[3][2];   // [color, lineal, radial][sin, con mascara]
static unsigned lut_tex = 0, mask_tex = 0;
static int ready = 0;
static int aux_broken = 0;

int gpu2d_available(void) {
  return gl_available() && !aux_broken;
}

static unsigned compile(GLenum type, const char *src) {
  unsigned s = qgl.CreateShader(type);
  qgl.ShaderSource(s, 1, &src, NULL);
  qgl.CompileShader(s);
  GLint ok = 0;
  qgl.GetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512] = "";
    qgl.GetShaderInfoLog(s, sizeof(log), NULL, log);
    fprintf(stderr, "[gpu-canvas] shader: %s\n", log);
  }
  return s;
}

static int build(Program *p, int mode, int mask) {
  // runtime/shaders/gpu2d.vert y .frag, con MODE y MASK delante
  char head[64], vs[4096], fs[4096];
  snprintf(head, sizeof(head), "#define MODE %d\n#define MASK %d\n", mode, mask);
  snprintf(vs, sizeof(vs), "%s%s", head, SHADER_GPU2D_VERT);
  snprintf(fs, sizeof(fs), "%s%s", head, SHADER_GPU2D_FRAG);
  p->prog = qgl.CreateProgram();
  qgl.AttachShader(p->prog, compile(GL_VERTEX_SHADER, vs));
  qgl.AttachShader(p->prog, compile(GL_FRAGMENT_SHADER, fs));
  qgl.BindAttribLocation(p->prog, 0, "a_pos");
  qgl.BindAttribLocation(p->prog, 1, "a_uv");
  qgl.BindAttribLocation(p->prog, 2, "a_mt");
  qgl.LinkProgram(p->prog);
  GLint ok = 0;
  qgl.GetProgramiv(p->prog, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512] = "";
    qgl.GetProgramInfoLog(p->prog, sizeof(log), NULL, log);
    fprintf(stderr, "[gpu-canvas] link (mode %d, mask %d): %s\n", mode, mask, log);
    return -1;
  }
  p->u_size = qgl.GetUniformLocation(p->prog, "u_size");
  p->u_color = qgl.GetUniformLocation(p->prog, "u_color");
  p->u_alpha = qgl.GetUniformLocation(p->prog, "u_alpha");
  p->u_g = qgl.GetUniformLocation(p->prog, "u_g");
  p->u_r = qgl.GetUniformLocation(p->prog, "u_r");
  p->u_lut = qgl.GetUniformLocation(p->prog, "u_lut");
  p->u_row = qgl.GetUniformLocation(p->prog, "u_row");
  p->u_mask = qgl.GetUniformLocation(p->prog, "u_mask");
  return 0;
}

// Con el contexto auxiliar actual
static int setup(void) {
  if (ready) return 0;
  for (int m = 0; m < 3; m++) {
    for (int k = 0; k < 2; k++) {
      if (build(&programs[m][k], m, k) != 0) {
        fprintf(stderr, "[gpu-canvas] the shaders do not link: canvas stays on the CPU\n");
        aux_broken = 1;
        return -1;
      }
    }
  }
  GLuint t[2];
  qgl.GenTextures(2, t);
  lut_tex = t[0];
  mask_tex = t[1];
  for (int i = 0; i < 2; i++) {
    qgl.BindTexture(GL_TEXTURE_2D, t[i]);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  qgl.BindTexture(GL_TEXTURE_2D, lut_tex);
  qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, LUT_ROWS, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  ready = 1;
  return 0;
}

static int begin(void) {
  if (aux_broken || gl_aux_begin() != 0) {
    aux_broken = 1;
    return -1;
  }
  if (setup() != 0) {
    gl_aux_end();
    return -1;
  }
  return 0;
}

static int make_target(Gpu2D *g, const uint8_t *premultiplied) {
  if (!g->tex) qgl.GenTextures(1, &g->tex);
  qgl.BindTexture(GL_TEXTURE_2D, g->tex);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  qgl.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
  qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g->w, g->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, premultiplied);
  if (!g->fbo) qgl.GenFramebuffers(1, &g->fbo);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, g->fbo);
  qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->tex, 0);
  int ok = qgl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  if (ok && !premultiplied) {
    qgl.ClearColor(0, 0, 0, 0);
    qgl.Clear(GL_COLOR_BUFFER_BIT);
  }
  qgl.BindFramebuffer(GL_FRAMEBUFFER, 0);
  return ok ? 0 : -1;
}

Gpu2D *gpu2d_new(const Surface *s) {
  if (!gpu2d_available() || s->w <= 0 || s->h <= 0) return NULL;
  Gpu2D *g = calloc(1, sizeof(Gpu2D));
  if (!g) return NULL;
  g->w = s->w;
  g->h = s->h;
  // lo que ya tenia el canvas, premultiplicado
  uint8_t *pm = NULL;
  int empty = 1;
  for (size_t i = 3; i < (size_t)s->w * s->h * 4 && s->px; i += 4) {
    if (s->px[i]) {
      empty = 0;
      break;
    }
  }
  if (!empty && (pm = malloc((size_t)s->w * s->h * 4))) {
    for (size_t i = 0; i < (size_t)s->w * s->h; i++) {
      const uint8_t *p = s->px + i * 4;
      pm[i * 4 + 0] = (uint8_t)((p[0] * p[3] + 127) / 255);
      pm[i * 4 + 1] = (uint8_t)((p[1] * p[3] + 127) / 255);
      pm[i * 4 + 2] = (uint8_t)((p[2] * p[3] + 127) / 255);
      pm[i * 4 + 3] = p[3];
    }
  }
  int ok = begin() == 0;
  if (ok) {
    ok = make_target(g, pm) == 0;
    gl_aux_end();
  }
  free(pm);
  if (!ok) {
    gpu2d_free(g);
    return NULL;
  }
  return g;
}

static void drop_cmds(Gpu2D *g) {
  for (int i = 0; i < g->count; i++) free(g->cmds[i].mask);
  g->count = 0;
  g->luts = 0;
}

void gpu2d_free(Gpu2D *g) {
  if (!g) return;
  drop_cmds(g);
  free(g->cmds);
  if (g->external) {   // la textura es de WebGL: no se borra
    external_set(g, 0);
    g->external = 0;
    g->tex = 0;
  }
  if ((g->tex || g->fbo) && begin() == 0) {
    if (g->fbo) qgl.DeleteFramebuffers(1, &g->fbo);
    if (g->tex) qgl.DeleteTextures(1, &g->tex);
    gl_aux_end();
  }
  free(g);
}

int gpu2d_resize(Gpu2D *g, int w, int h) {
  drop_cmds(g);
  if (g->external) {   // la textura de WebGL se queda como esta; una propia nueva
    external_set(g, 0);
    g->external = 0;
    g->tex = 0;
  }
  g->opaque = 0;
  if (w <= 0 || h <= 0) w = h = 1;
  g->w = w;
  g->h = h;
  if (begin() != 0) return -1;
  int r = make_target(g, NULL);
  gl_aux_end();
  return r;
}

static void blend_for(CompositeOp op) {
  switch (op) {
    case OP_LIGHTER:          qgl.BlendFunc(GL_ONE, GL_ONE); break;
    case OP_DESTINATION_OUT:  qgl.BlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA); break;
    case OP_DESTINATION_OVER: qgl.BlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE); break;
    case OP_SOURCE_ATOP:      qgl.BlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
    case OP_XOR:              qgl.BlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
    case OP_SCREEN:           qgl.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR); break;
    default:                  qgl.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); break;
  }
}

static int op_supported(CompositeOp op) {
  return op == OP_SOURCE_OVER || op == OP_LIGHTER || op == OP_DESTINATION_OUT || op == OP_DESTINATION_OVER ||
         op == OP_SOURCE_ATOP || op == OP_XOR || op == OP_SCREEN;
}

// Pinta lo apuntado. Con el contexto auxiliar actual.
static void draw_pending(Gpu2D *g) {
  if (!g->count) return;
  qgl.BindFramebuffer(GL_FRAMEBUFFER, g->fbo);
  qgl.Viewport(0, 0, g->w, g->h);
  qgl.Disable(GL_SCISSOR_TEST);
  qgl.Disable(GL_DEPTH_TEST);
  qgl.Disable(GL_STENCIL_TEST);
  qgl.Disable(GL_CULL_FACE);
  qgl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  qgl.Enable(GL_BLEND);
  qgl.BlendEquation(GL_FUNC_ADD);
  qgl.BindBuffer(GL_ARRAY_BUFFER, 0);
  qgl.ActiveTexture(GL_TEXTURE0);
  qgl.BindTexture(GL_TEXTURE_2D, lut_tex);
  if (g->luts) {
    qgl.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
    qgl.TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, g->luts, GL_RGBA, GL_UNSIGNED_BYTE, g->lut);
  }
  for (int a = 0; a < 3; a++) qgl.EnableVertexAttribArray(a);
  qgl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
  const Program *cur = NULL;
  for (int i = 0; i < g->count; i++) {
    const Cmd *c = &g->cmds[i];
    if (c->mode == MODE_CLEAR) {
      // un relleno opaco de todo el canvas: en una GPU por tiles es gratis
      qgl.ClearColor(c->color[0], c->color[1], c->color[2], c->color[3]);
      qgl.Clear(GL_COLOR_BUFFER_BIT);
      continue;
    }
    const Program *p = &programs[c->mode][c->mask ? 1 : 0];
    if (p != cur) {
      qgl.UseProgram(p->prog);
      qgl.Uniform2f(p->u_size, (float)g->w, (float)g->h);
      qgl.Uniform1i(p->u_lut, 0);
      qgl.Uniform1i(p->u_mask, 1);
      cur = p;
    }
    if (c->mask) {
      qgl.ActiveTexture(GL_TEXTURE1);
      qgl.BindTexture(GL_TEXTURE_2D, mask_tex);
      qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, c->mw, c->mh, 0, GL_ALPHA, GL_UNSIGNED_BYTE, c->mask);
      qgl.ActiveTexture(GL_TEXTURE0);
    }
    blend_for(c->op);
    qgl.Uniform4f(p->u_color, c->color[0], c->color[1], c->color[2], c->color[3]);
    qgl.Uniform1f(p->u_alpha, c->alpha);
    qgl.Uniform4f(p->u_g, c->grad[0], c->grad[1], c->grad[2], c->grad[3]);
    qgl.Uniform2f(p->u_r, c->grad[4], c->grad[5]);
    qgl.Uniform1f(p->u_row, (c->lut + 0.5f) / LUT_ROWS);
    qgl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, c->pos);
    qgl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, c->uv);
    qgl.VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, c->mt);
    qgl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }
  for (int a = 0; a < 3; a++) qgl.DisableVertexAttribArray(a);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, 0);
  drop_cmds(g);
}

static int flush(Gpu2D *g) {
  if (!g->count) return 0;
  if (begin() != 0) return -1;
  draw_pending(g);
  gl_aux_end();
  return 0;
}

// Un comando nuevo con la pintura; NULL si la GPU no la hace
static Cmd *new_cmd(Gpu2D *g, const Matrix *ctm, const Paint *paint, double alpha, CompositeOp op) {
  if (!op_supported(op)) return NULL;
  PaintMode mode = MODE_COLOR;
  const Gradient *gr = NULL;
  if (paint->type == PAINT_GRADIENT) {
    gr = paint->gradient;
    if (!gr) return NULL;
    if (gr->type == GRADIENT_LINEAR) mode = MODE_LINEAR;
    else if (gr->x0 == gr->x1 && gr->y0 == gr->y1 && gr->r1 > gr->r0) mode = MODE_RADIAL;
    else return NULL;   // radial con dos centros: por software
  } else if (paint->type != PAINT_COLOR) {
    return NULL;        // patrones: por software
  }
  if ((g->count == MAX_CMDS || (gr && g->luts == LUT_ROWS)) && flush(g) != 0) return NULL;
  if (g->count == g->cap) {
    int cap = g->cap ? g->cap * 2 : 32;
    Cmd *n = realloc(g->cmds, sizeof(Cmd) * cap);
    if (!n) return NULL;
    g->cmds = n;
    g->cap = cap;
  }
  Cmd *c = &g->cmds[g->count++];
  memset(c, 0, sizeof(*c));
  c->mode = mode;
  c->alpha = (float)alpha;
  c->op = op;
  if (mode == MODE_COLOR) {
    c->color[0] = paint->color.r / 255.0f;
    c->color[1] = paint->color.g / 255.0f;
    c->color[2] = paint->color.b / 255.0f;
    c->color[3] = paint->color.a / 255.0f;
  } else {
    // la misma tabla de 256 colores que usa el software
    PaintSampler ps;
    paint_sampler_init(&ps, paint, ctm);
    c->lut = g->luts++;
    uint8_t *row = g->lut[c->lut];
    for (int i = 0; i < 256; i++) {
      const Color *k = &ps.lut[i * (GRADIENT_LUT - 1) / 255];
      row[i * 4 + 0] = k->r;
      row[i * 4 + 1] = k->g;
      row[i * 4 + 2] = k->b;
      row[i * 4 + 3] = k->a;
    }
    c->grad[0] = (float)gr->x0;
    c->grad[1] = (float)gr->y0;
    c->grad[2] = (float)gr->x1;
    c->grad[3] = (float)gr->y1;
    c->grad[4] = (float)gr->r0;
    c->grad[5] = (float)gr->r1;
  }
  // sin mascara: la textura blanca (cualquier coordenada vale)
  for (int i = 0; i < 8; i++) c->mt[i] = 0.5f;
  return c;
}

// Las 4 esquinas de un rectangulo de usuario, en el orden de la tira
static void set_rect(Cmd *c, double x, double y, double w, double h, const Matrix *ctm) {
  const double ux[4] = { x, x + w, x, x + w };
  const double uy[4] = { y, y, y + h, y + h };
  for (int i = 0; i < 4; i++) {
    double dx, dy;
    matrix_apply(ctm, ux[i], uy[i], &dx, &dy);
    c->pos[i * 2] = (float)dx;
    c->pos[i * 2 + 1] = (float)dy;
    c->uv[i * 2] = (float)ux[i];
    c->uv[i * 2 + 1] = (float)uy[i];
  }
}

// Si el rectangulo cubre todo el canvas (sin giro)
static int covers_canvas(const Gpu2D *g, double x, double y, double w, double h, const Matrix *ctm) {
  if (ctm->b != 0 || ctm->c != 0) return 0;
  double x0, y0, x1, y1;
  matrix_apply(ctm, x, y, &x0, &y0);
  matrix_apply(ctm, x + w, y + h, &x1, &y1);
  if (x0 > x1) { double t = x0; x0 = x1; x1 = t; }
  if (y0 > y1) { double t = y0; y0 = y1; y1 = t; }
  return x0 <= 0.5 && y0 <= 0.5 && x1 >= g->w - 0.5 && y1 >= g->h - 0.5;
}

int gpu2d_fill_rect(Gpu2D *g, double x, double y, double w, double h, const Matrix *ctm,
                    const Paint *paint, double alpha, CompositeOp op) {
  if (paint->type == PAINT_COLOR) {
    int a = (int)(paint->color.a * alpha + 0.5);
    // Un relleno opaco de todo el canvas tapa lo que hubiera: lo apuntado y
    // aun sin pintar ya no hace falta (una mascara de luces se repinta en cada
    // actualizacion y solo se ve la ultima) y se pinta con glClear
    if (op == OP_SOURCE_OVER && a >= 255 && covers_canvas(g, x, y, w, h, ctm)) {
      drop_cmds(g);
      Cmd *c = new_cmd(g, ctm, paint, 1.0, op);
      if (!c) return -1;
      c->mode = MODE_CLEAR;
      c->color[3] = 1.0f;
      g->opaque = 1;
      return 0;
    }
    // sumar negro a un canvas opaco no cambia nada
    if (op == OP_LIGHTER && g->opaque && paint->color.r == 0 && paint->color.g == 0 && paint->color.b == 0) {
      return 0;
    }
  }
  // sumar o pintar encima de algo opaco lo deja opaco; los demas modos no
  if (op != OP_LIGHTER && op != OP_SOURCE_OVER) g->opaque = 0;
  Cmd *c = new_cmd(g, ctm, paint, alpha, op);
  if (!c) return -1;
  set_rect(c, x, y, w, h, ctm);
  return 0;
}

int gpu2d_clear_rect(Gpu2D *g, double x, double y, double w, double h, const Matrix *ctm) {
  Paint black = { .type = PAINT_COLOR, .color = { 0, 0, 0, 255 } };
  g->opaque = 0;
  if (covers_canvas(g, x, y, w, h, ctm)) {
    drop_cmds(g);
    Cmd *c = new_cmd(g, ctm, &black, 1.0, OP_SOURCE_OVER);
    if (!c) return -1;
    c->mode = MODE_CLEAR;
    c->color[0] = c->color[1] = c->color[2] = c->color[3] = 0.0f;
    return 0;
  }
  Cmd *c = new_cmd(g, ctm, &black, 1.0, OP_DESTINATION_OUT);
  if (!c) return -1;
  set_rect(c, x, y, w, h, ctm);
  return 0;
}

int gpu2d_fill_mask(Gpu2D *g, const Mask *m, const Matrix *ctm, const Paint *paint, double alpha,
                    CompositeOp op) {
  int x0 = m->x0 < 0 ? 0 : m->x0, y0 = m->y0 < 0 ? 0 : m->y0;
  int x1 = m->x1 > m->w ? m->w : m->x1, y1 = m->y1 > m->h ? m->h : m->y1;
  if (x1 <= x0 || y1 <= y0) return 0;   // nada que pintar
  Matrix inv;
  if (matrix_invert(&inv, ctm) != 0) return 0;
  if (op != OP_LIGHTER && op != OP_SOURCE_OVER) g->opaque = 0;
  Cmd *c = new_cmd(g, ctm, paint, alpha, op);
  if (!c) return -1;
  int mw = x1 - x0, mh = y1 - y0;
  c->mask = malloc((size_t)mw * mh);
  if (!c->mask) {
    g->count--;
    return -1;
  }
  for (int y = 0; y < mh; y++) memcpy(c->mask + (size_t)y * mw, m->m + (size_t)(y0 + y) * m->w + x0, mw);
  c->mw = mw;
  c->mh = mh;
  const double dx[4] = { x0, x1, x0, x1 };
  const double dy[4] = { y0, y0, y1, y1 };
  for (int i = 0; i < 4; i++) {
    double ux, uy;
    matrix_apply(&inv, dx[i], dy[i], &ux, &uy);
    c->pos[i * 2] = (float)dx[i];
    c->pos[i * 2 + 1] = (float)dy[i];
    c->uv[i * 2] = (float)ux;
    c->uv[i * 2 + 1] = (float)uy;
    c->mt[i * 2] = (i & 1) ? 1.0f : 0.0f;
    c->mt[i * 2 + 1] = (i & 2) ? 1.0f : 0.0f;
  }
  return 0;
}

int gpu2d_read(Gpu2D *g, Surface *s) {
  if (s->w != g->w || s->h != g->h) {
    if (surface_resize(s, g->w, g->h) != 0) return -1;
  }
  if (begin() != 0) return -1;
  draw_pending(g);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, g->fbo);
  qgl.PixelStorei(GL_PACK_ALIGNMENT, 4);
  qgl.ReadPixels(0, 0, g->w, g->h, GL_RGBA, GL_UNSIGNED_BYTE, s->px);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, 0);
  gl_aux_end();
  // el canvas guarda el color sin premultiplicar
  for (size_t i = 0; i < (size_t)g->w * g->h; i++) {
    uint8_t *p = s->px + i * 4;
    int a = p[3];
    if (a == 0) {
      p[0] = p[1] = p[2] = 0;
    } else if (a < 255) {
      for (int k = 0; k < 3; k++) {
        int v = (p[k] * 255 + a / 2) / a;
        p[k] = (uint8_t)(v > 255 ? 255 : v);
      }
    }
  }
  return 0;
}

unsigned gpu2d_texture(Gpu2D *g, int *w, int *h) {
  if (flush(g) != 0) return 0;
  *w = g->w;
  *h = g->h;
  return g->tex;
}

// Dibujar a partir de ahora en dst, la textura de WebGL a la que se acaba de
// copiar el canvas entero (gpu_canvas_copy): asi las siguientes subidas no
// copian nada, solo pintan lo pendiente. La mascara de luces de RPG Maker se
// copiaba entera en cada imagen (866x630, ~1 ms en una RG CubeXX).
int gpu2d_retarget(Gpu2D *g, unsigned dst) {
  if (!g || !dst) return -1;
  if (g->external && g->tex == dst) return 0;
  if (begin() != 0) return -1;
  draw_pending(g);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, g->fbo);
  qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst, 0);
  int ok = qgl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  if (!ok) {
    qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->tex, 0);
  } else {
    if (!g->external && g->tex) qgl.DeleteTextures(1, &g->tex);
    g->tex = dst;
    g->external = 1;
    external_set(g, 1);
    printf("[gpu-canvas] %dx%d canvas draws straight into its WebGL texture\n", g->w, g->h);
  }
  qgl.BindFramebuffer(GL_FRAMEBUFFER, 0);
  gl_aux_end();
  return ok ? 0 : -1;
}

int gpu2d_is_target(const Gpu2D *g, unsigned tex) {
  return g && tex && g->external && g->tex == tex;
}

// WebGL va a borrar tex: los canvas que dibujaban en ella vuelven a una
// textura propia, con lo que tenian
void gpu2d_forget_texture(unsigned tex) {
  if (!tex) return;
  for (int i = 0; i < MAX_EXTERNAL; i++) {
    Gpu2D *g = external_list[i];
    if (!g || g->tex != tex) continue;
    external_set(g, 0);
    g->external = 0;
    if (begin() != 0) {
      g->tex = 0;
      continue;
    }
    draw_pending(g);
    unsigned own = 0;
    qgl.GenTextures(1, &own);
    qgl.BindTexture(GL_TEXTURE_2D, own);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    qgl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    qgl.BindFramebuffer(GL_FRAMEBUFFER, g->fbo);   // aun con tex: se lee de ahi
    qgl.CopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, g->w, g->h, 0);
    qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, own, 0);
    qgl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    g->tex = own;
    gl_aux_end();
  }
}
