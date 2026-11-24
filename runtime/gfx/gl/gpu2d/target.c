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

// runtime/gfx/gl/gpu2d/target.c: la texture y el framebuffer de cada canvas: crearlo,
// liberarlo y cambiar su tamano.
// Parte de runtime/gfx/gl/gpu2d.c (archivo indice).

#include "common.h"

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
  free(g->last);
  free(g->last_lut);
  if (g->mfbo) qgl.DeleteFramebuffers(1, &g->mfbo);   // del contexto de WebGL (el actual)
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
  g->last_n = 0;
  g->mfbo_tex = 0;
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
