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

// runtime/gfx/gl/gpu2d/texture.c: el canvas como textura: leerlo, su texture, dibujar en
// una textura de WebGL (retarget) y olvidarla cuando WebGL la borra.
// Parte de runtime/gfx/gl/gpu2d.c (archivo indice).

#include "common.h"

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
  g->mfbo_tex = 0;   // el framebuffer de WebGL se vuelve a enganchar (ver flush_main)
  g->last_n = 0;     // y lo ultimo pintado no esta en la textura nueva
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
    g->mfbo_tex = 0;
    g->last_n = 0;
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
