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

// Mostrar una imagen: las capas de los canvas escaladas a la ventana y el
// cambio de buffer (swap_window).
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

// Muestra lo dibujado y apunta cuando (para el comando pacing de la consola
// de debug)
static void swap_window(SDLPlatformData* data) {
  TRACE_START(ts_);
  SDL_GL_SwapWindow(data->window);
  TRACE_END(ts_, 0.0, "swap");
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  double now = ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
  if (data->shown_n < SHOWN_MAX) data->shown[data->shown_n++] = now;
}



// Varias capas: cada canvas 2d se sube solo si cambio; los WebGL van directos.
// Todas se escalan al rectangulo de la primera (el canvas del juego). Con el
// contexto que presenta ya actual (en este thread o en el que presenta).
static void draw_layers(Backend* backend, SDLPlatformData* data, const PresentLayer *layers, int n, int ww, int wh, int swap) {
  qgl.BindFramebuffer(GL_FRAMEBUFFER, 0);
  qgl.Viewport(0, 0, ww, wh);
  qgl.Disable(GL_SCISSOR_TEST);
  qgl.ClearColor(0, 0, 0, 1);
  qgl.Clear(GL_COLOR_BUFFER_BIT);

  for (int k = 0; k < 8; k++) data->layer_tex[k].used = 0;
  if (n > 0 && layers[0].w > 0 && layers[0].h > 0) {
    int w = layers[0].w, h = layers[0].h;
    double scale = (double)ww / w < (double)wh / h ? (double)ww / w : (double)wh / h;
    // pixel perfect: cada pixel del juego, el mismo numero de pixeles de la
    // pantalla (con 1,125x unos salen dobles y otros no: la letra se deforma)
    if (backend->scaling == SCALING_PIXEL && scale >= 1) scale = (int)scale;
    int dw = (int)(w * scale + 0.5), dh = (int)(h * scale + 0.5);
    int dx = (ww - dw) / 2, dy = (wh - dh) / 2;
    qgl.Enable(GL_BLEND);
    // dos pasadas con la lupa: el juego entero y, encima, el cuadro de la
    // lupa (magnifier.c)
    int passes = backend->magnify > 1 ? 2 : 1;
    for (int pass = 0; pass < passes; pass++) {
    if (pass == 1) {
      magnifier_lens(backend, &dx, &dy, &dw, &dh, ww, wh);
    }
    for (int i = 0; i < n; i++) {
      const PresentLayer *l = &layers[i];
      GLuint tex = l->gl_tex;
      int flip = tex != 0;
      if (!tex) {
        if (!l->rgba || l->w <= 0 || l->h <= 0) continue;
        // su textura (por canvas); se sube si cambio o es nueva
        int slot = -1;
        for (int k = 0; k < 8; k++) if (data->layer_tex[k].key == l->key && data->layer_tex[k].tex) { slot = k; break; }
        int fresh = 0;
        if (slot < 0) {
          for (int k = 0; k < 8; k++) if (!data->layer_tex[k].tex) { slot = k; break; }
          if (slot < 0) continue;
          qgl.GenTextures(1, &data->layer_tex[slot].tex);
          data->layer_tex[slot].key = l->key;
          data->layer_tex[slot].w = data->layer_tex[slot].h = 0;
          fresh = 1;
        }
        tex = data->layer_tex[slot].tex;
        data->layer_tex[slot].used = 1;
        qgl.BindTexture(GL_TEXTURE_2D, tex);
        qgl.PixelStorei(GL_UNPACK_ALIGNMENT, 4);
        if (data->layer_tex[slot].w != l->w || data->layer_tex[slot].h != l->h) {
          tex_params();
          qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, l->w, l->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, l->rgba);
          data->layer_tex[slot].w = l->w;
          data->layer_tex[slot].h = l->h;
        } else if ((l->dirty && pass == 0) || fresh) {
          qgl.TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, l->w, l->h, GL_RGBA, GL_UNSIGNED_BYTE, l->rgba);
        }
      }
      if (l->premultiplied) qgl.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      else qgl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      // ampliado: sharp bilinear (present.frag), salvo en Smooth; reducido,
      // el lineal tal cual
      int px = backend->scaling == SCALING_SMOOTH ? 0 : dw / l->w;
      int py = backend->scaling == SCALING_SMOOTH ? 0 : dh / l->h;
      data->blit_size[0] = (float)l->w;
      data->blit_size[1] = (float)l->h;
      data->blit_prescale[0] = px >= 1 && py >= 1 ? (float)px : 0.0f;
      data->blit_prescale[1] = px >= 1 && py >= 1 ? (float)py : 0.0f;
      sdl_blit(data, tex, dx, dy, dw, dh, ww, wh, flip);
      data->blit_prescale[0] = data->blit_prescale[1] = 0.0f;
    }
    }
    qgl.Disable(GL_SCISSOR_TEST);
    qgl.Disable(GL_BLEND);
    if (backend->magnify > 1) magnifier_icon(backend, wh);
  }
  // texturas de canvas que ya no estan en pantalla
  for (int k = 0; k < 8; k++) {
    if (data->layer_tex[k].tex && !data->layer_tex[k].used) {
      qgl.DeleteTextures(1, &data->layer_tex[k].tex);
      data->layer_tex[k].tex = 0;
      data->layer_tex[k].key = NULL;
    }
  }
  sdl_draw_overlay(backend, data, ww, wh);
  if (swap) swap_window(data);
}
