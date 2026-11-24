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

// Lo que se dibuja encima del juego, a la resolucion de la ventana: el
// contador de --fps, el aviso de un logro y el de carga.
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

// Lo que va en una esquina se aparta del borde. En un telefono (Android) la
// pantalla tiene las esquinas redondeadas (en un Pixel, un radio de un 10% del
// alto): pegado a la esquina, el contador de fps quedaba fuera de la vista
static int corner_margin(int wh, int margin) {
#ifdef __ANDROID__
  if (margin < wh / 20) margin = wh / 20;
#endif
  return margin;
}

// Rehace la textura de t si el texto cambia: fondo oscuro con margen y el
// texto encima, a la resolucion de la ventana (no del juego)
static int text_tex(TextTex *t, const char *text, const char *css, unsigned char r, unsigned char g,
                    unsigned char b, int pad_x, int pad_y) {
  if (t->tex && strcmp(t->text, text) == 0) return 1;
  snprintf(t->text, sizeof(t->text), "%s", text);
  FontHandle *font = font_from_css_fixed(css);   // la dibuja el thread que presenta
  Surface glyphs;
  if (!font || font_render(font, t->text, r, g, b, 255, 0, &glyphs) != 0) return 0;
  int pw = glyphs.w + pad_x * 2, ph = glyphs.h + pad_y * 2;
  Surface box;
  if (surface_init(&box, pw, ph) != 0) {
    surface_free(&glyphs);
    return 0;
  }
  for (int i = 0; i < pw * ph; i++) {
    box.px[i * 4] = 0; box.px[i * 4 + 1] = 0; box.px[i * 4 + 2] = 0; box.px[i * 4 + 3] = 190;
  }
  for (int y = 0; y < glyphs.h; y++) {
    for (int x = 0; x < glyphs.w; x++) {
      const uint8_t *gp = glyphs.px + ((size_t)y * glyphs.w + x) * 4;
      blend_pixel(box.px + ((size_t)(y + pad_y) * pw + x + pad_x) * 4, gp[0], gp[1], gp[2], gp[3], OP_SOURCE_OVER);
    }
  }
  if (!t->tex) qgl.GenTextures(1, &t->tex);
  qgl.BindTexture(GL_TEXTURE_2D, t->tex);
  tex_params();
  qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pw, ph, 0, GL_RGBA, GL_UNSIGNED_BYTE, box.px);
  t->w = pw;
  t->h = ph;
  surface_free(&box);
  surface_free(&glyphs);
  return 1;
}

// La tarjeta de backend->toast arriba a la derecha, como RetroAchievements:
// entra desde la derecha, se queda y se desvanece. Se dibuja una vez por aviso (y
// por tamano de letra) y despues solo se mueve.
#define TOAST_IN_MS  300
#define TOAST_OUT_MS 500

static void sdl_draw_toast(Backend* backend, SDLPlatformData* data, int ww, int wh) {
  const Toast *t = &backend->toast;
  if (!t->serial) return;
  int px = wh / 28 < 12 ? 12 : wh / 28;
  Uint32 now = SDL_GetTicks();
  if (t->serial != data->toast.serial) {
    data->toast.serial = t->serial;
    // sustituye al que se veia (toast_push, un grupo): ya dentro, sin entrar
    data->toast.age = t->replaces ? TOAST_IN_MS : 0;
    data->toast.last = now;
    data->toast.px = 0;
  }
  // el tiempo del aviso avanza por imagenes: una imagen que tarda mucho (el
  // juego cargando un mapa) cuenta como mucho 33 ms, asi la entrada se ve
  // aunque aparezca justo en una carga
  Uint32 dt = now - data->toast.last;
  data->toast.last = now;
  data->toast.age += dt > 33 ? 33 : dt;
  Uint32 age = data->toast.age;
  if (age >= TOAST_MS) return;
  if (data->toast.px != px) {
    Surface card;
    if (toast_render(t, px, &card) != 0) return;
    if (!data->toast.tex) qgl.GenTextures(1, &data->toast.tex);
    qgl.BindTexture(GL_TEXTURE_2D, data->toast.tex);
    tex_params();
    qgl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, card.w, card.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, card.px);
    data->toast.w = card.w;
    data->toast.h = card.h;
    data->toast.px = px;
    surface_free(&card);
  }
  // si la ventana es estrecha, la tarjeta se hace mas pequena
  int w = data->toast.w, h = data->toast.h, margin = corner_margin(wh, wh / 160 + 2);   // casi en la esquina
  if (w > ww - 2 * margin) {
    h = h * (ww - 2 * margin) / w;
    w = ww - 2 * margin;
  }
  float in = age < TOAST_IN_MS ? (float)age / TOAST_IN_MS : 1.0f;
  in = 1.0f - (1.0f - in) * (1.0f - in) * (1.0f - in);   // frena al llegar
  float alpha = age > TOAST_MS - TOAST_OUT_MS ? (float)(TOAST_MS - age) / TOAST_OUT_MS : 1.0f;
  // arriba a la derecha, debajo del contador de --fps; entra desde la derecha
  int y = margin + (backend->overlay[0] ? data->overlay.h + 4 : 0);
  int x = ww - margin - w + (int)((1.0f - in) * (w + margin));
  data->blit_alpha = alpha;
  sdl_blit(data, data->toast.tex, x, y, w, h, ww, wh, 0);
  data->blit_alpha = 1.0f;
}

// El aviso de carga (backend->loading): "Loading" y unos puntos que avanzan
// con el reloj, para que se vea que el juego sigue (con el juego parado, lo
// repinta el thread que presenta). La letra, a la altura de la ventana.
static int loading_tex(Backend* backend, SDLPlatformData* data, int wh) {
  if (!backend_loading_visible(backend)) return 0;
  int px = wh / 24 < 12 ? 12 : wh / 24;
  static const char *steps[4] = { "Loading   ", "Loading.  ", "Loading.. ", "Loading..." };
  if (data->loading.px != px) {
    data->loading.text[0] = 0;   // otro tamano: se rehace
    data->loading.px = px;
  }
  char css[48];
  snprintf(css, sizeof css, "bold %dpx monospace", px);
  return text_tex(&data->loading, steps[SDL_GetTicks() / 400 % 4], css, 255, 255, 255, px / 2, px / 4);
}

// La letra de --fps: 16 px, mas grande en ventanas altas (en un telefono, 16
// px de 1080 no se leen)
static int fps_tex(Backend* backend, SDLPlatformData* data, int wh) {
  if (!backend->overlay[0]) return 0;
  int px = wh / 45 < 16 ? 16 : wh / 45;
  if (data->overlay.px != px) {
    data->overlay.text[0] = 0;   // otro tamano: se rehace
    data->overlay.px = px;
  }
  char css[48];
  snprintf(css, sizeof css, "bold %dpx monospace", px);
  return text_tex(&data->overlay, backend->overlay, css, 255, 255, 0, px / 4, px / 16);
}

// backend->overlay arriba a la derecha (--fps), el aviso de un logro y el de
// carga (abajo a la derecha), a la resolucion de la ventana: se ven igual en
// ventanas pequenas y no tocan el canvas del juego
static void sdl_draw_overlay(Backend* backend, SDLPlatformData* data, int ww, int wh) {
  int fps = fps_tex(backend, data, wh);
  int loading = loading_tex(backend, data, wh);
  if (!fps && !backend->toast.serial && !loading) return;
  qgl.Enable(GL_BLEND);
  qgl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  qgl.Viewport(0, 0, ww, wh);
  int corner = corner_margin(wh, 4);
  if (fps)
    sdl_blit(data, data->overlay.tex, ww - data->overlay.w - corner, corner, data->overlay.w, data->overlay.h, ww,
             wh, 0);
  if (loading) {
    int m = corner_margin(wh, wh / 40);
    sdl_blit(data, data->loading.tex, ww - data->loading.w - m, wh - data->loading.h - m, data->loading.w,
             data->loading.h, ww, wh, 0);
  }
  sdl_draw_toast(backend, data, ww, wh);
  qgl.Disable(GL_BLEND);
}
