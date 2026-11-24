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

// runtime/gfx/gl/gpu2d/batch.c: los draw calls pendientes: el blending, recordar lo
// ultimo pintado y el flush (en el contexto de WebGL o en el auxiliar).
// Parte de runtime/gfx/gl/gpu2d.c (archivo indice).

#include "common.h"

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
// Apunta lo que se va a pintar si repinta todo el canvas (el primero es un
// relleno entero) y no lleva mascaras: el resultado depende solo de esto
static void remember(Gpu2D *g) {
  g->last_n = 0;
  if (g->cmds[0].mode != MODE_CLEAR) return;
  for (int i = 0; i < g->count; i++) if (g->cmds[i].mask) return;
  if (g->last_cap < g->count) {
    Cmd *n = realloc(g->last, sizeof(Cmd) * g->count);
    if (!n) return;
    g->last = n;
    g->last_cap = g->count;
  }
  if (!g->last_lut && !(g->last_lut = malloc(sizeof g->lut))) return;
  memcpy(g->last, g->cmds, sizeof(Cmd) * g->count);
  memcpy(g->last_lut, g->lut, (size_t)g->luts * 256 * 4);
  g->last_luts = g->luts;
  g->last_tex = g->tex;
  g->last_n = g->count;
}

// Lo pendiente es lo mismo que lo ultimo pintado entero, en la misma textura
static int same_as_last(const Gpu2D *g) {
  return g->last_n > 0 && g->last_n == g->count && g->last_tex == g->tex && g->last_luts == g->luts &&
         memcmp(g->last, g->cmds, sizeof(Cmd) * g->count) == 0 &&
         memcmp(g->last_lut, g->lut, (size_t)g->luts * 256 * 4) == 0;
}

static void draw_pending_to(Gpu2D *g, unsigned fbo) {
  if (!g->count) return;
  remember(g);
  qgl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
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

static void draw_pending(Gpu2D *g) {
  draw_pending_to(g, g->fbo);
}

// Pintar en el contexto de WebGL (el del juego), guardando su estado y
// devolviendolo despues, en lugar de en el contexto auxiliar. Cambiar de
// contexto en cada imagen cortaba el dibujo de la imagen en curso: en la RG
// CubeXX (panfrost), en Firework caminando, de ~680 a ~420 imagenes tardias
// (45 ms o mas) por pasada de media, sin las tandas malas (3+3); Fear &
// Hunger igual. QUIXJS_GPU2D_MAIN=0 vuelve al auxiliar.
static int main_ctx = -1;

static int flush_main(Gpu2D *g) {
  if (!g->mfbo || g->mfbo_tex != g->tex) {
    GLint prev = 0;
    qgl.GetIntegerv(GL_FRAMEBUFFER_BINDING, &prev);
    if (!g->mfbo) qgl.GenFramebuffers(1, &g->mfbo);
    qgl.BindFramebuffer(GL_FRAMEBUFFER, g->mfbo);
    qgl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g->tex, 0);
    int ok = qgl.CheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    qgl.BindFramebuffer(GL_FRAMEBUFFER, prev);
    if (!ok) return -1;
    g->mfbo_tex = g->tex;
  }
  GlState sv;
  gl_state_save(&sv);
  draw_pending_to(g, g->mfbo);
  gl_state_restore(&sv);
  return 0;
}

static int flush(Gpu2D *g) {
  if (!g->count) return 0;
  // La mascara de luces de TerraxLighting se repinta entera en cada
  // actualizacion; con las luces y la pantalla quietas sale igual: la textura
  // ya lo tiene y no se vuelve a pintar (ni a cambiar de contexto, ni a subir
  // los degradados)
  if (same_as_last(g)) {
    drop_cmds(g);
    return 0;
  }
  if (main_ctx < 0) {
    const char *e = getenv("QUIXJS_GPU2D_MAIN");
    main_ctx = !(e && atoi(e) == 0);
    if (!main_ctx) printf("[gpu-canvas] drawing in the auxiliary context\n");
  }
  if (main_ctx && ready && g->tex) {
    gl_make_current();
    if (flush_main(g) == 0) return 0;
  }
  if (begin() != 0) return -1;
  draw_pending(g);
  gl_aux_end();
  return 0;
}
