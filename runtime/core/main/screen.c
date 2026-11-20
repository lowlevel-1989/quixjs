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

// La pantalla: componer los canvas visibles, mostrar el frame y las capturas
// (--screenshot, --script, la consola de debug).
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

// Junta los canvas visibles del documento en un frame. Con un solo canvas
// se usa su superficie directamente (sin copias). Con varios, como en
// pantalla: cada uno ocupa todo el frame, que tiene el tamano del mas grande
// (HexGL dibuja su 3D a media resolucion en calidad baja y su HUD y el menu
// del port a la entera). Un <video> que se ve va encima, como en pantalla
static const Surface *compose_screen(JSContext *ctx, Surface *scratch) {
  CanvasData *list[MAX_SCREEN_CANVASES];
  int n = document_screen_canvases(ctx, list, MAX_SCREEN_CANVASES);
  if (n == 0) return NULL;
  for (int i = 0; i < n; i++) {
    if (list[i]->webgl) webgl_canvas_sync(list[i]);   // lo dibujado con WebGL
    canvas_sync_cpu(list[i]);                          // lo dibujado con gpu-canvas
  }
  const Surface *video = video_screen(NULL);
  if (n == 1 && !video) return &list[0]->surface;

  const Surface *base = &list[0]->surface;
  int w = 0, h = 0;
  for (int i = 0; i < n; i++) {
    if (list[i]->surface.w * list[i]->surface.h > w * h) {
      w = list[i]->surface.w;
      h = list[i]->surface.h;
    }
  }
  if (scratch->w != w || scratch->h != h) {
    if (surface_resize(scratch, w, h) != 0) return base;
  }
  int first = 0;
  if (base->w == w && base->h == h) {
    memcpy(scratch->px, base->px, (size_t)w * h * 4);
    first = 1;
  } else {
    surface_clear(scratch);
  }

  Matrix identity;
  matrix_identity(&identity);
  DrawParams dp = { .alpha = 1.0, .op = OP_SOURCE_OVER, .clip = NULL };
  for (int i = first; i < n; i++) {
    const Surface *s = &list[i]->surface;
    raster_draw_image(scratch, s, 0, 0, s->w, s->h, 0, 0, w, h, &identity, &dp);
  }
  if (video) raster_draw_image(scratch, video, 0, 0, video->w, video->h, 0, 0, w, h, &identity, &dp);
  return scratch;
}

// Muestra el frame. Si en pantalla hay un solo canvas y es WebGL, su textura
// va directa al backend (sin leerla de la GPU); si no, se compone en memoria.
static void present_screen(JSContext *ctx, Backend *backend, Surface *scratch) {
  // con GPU (backend sdl): cada canvas es una capa; los WebGL sin leerlos y
  // los 2d solo se suben si cambiaron
  if (backend_can_present_layers(backend)) {
    CanvasData *list[MAX_SCREEN_CANVASES];
    PresentLayer layers[MAX_SCREEN_CANVASES];
    int n = document_screen_canvases(ctx, list, MAX_SCREEN_CANVASES);
    int m = 0;
    for (int i = 0; i < n; i++) {
      CanvasData *cd = list[i];
      PresentLayer *l = &layers[m];
      memset(l, 0, sizeof(*l));
      l->key = cd;
      if (cd->webgl) {
        // entregada al thread que presenta: el canvas sigue en otra textura
        if (backend->present_async) l->gl_tex = webgl_canvas_handoff(cd, backend->present_thread ? 2 : 3, &l->w, &l->h, &l->premultiplied);
        if (!l->gl_tex) {
          backend->present_async = 0;
          l->gl_tex = webgl_canvas_texture(cd, &l->w, &l->h, &l->premultiplied);
        }
        if (!l->gl_tex) continue;
      } else {
        canvas_sync_cpu(cd);   // gpu-canvas en pantalla (raro): se lee
        l->rgba = cd->surface.px;
        l->w = cd->surface.w;
        l->h = cd->surface.h;
        l->dirty = cd->dirty;
        cd->dirty = 0;
      }
      m++;
    }
    // un <video> que se ve: encima de todo
    int vchanged = 0;
    const Surface *vs = video_screen(&vchanged);
    if (vs && m < MAX_SCREEN_CANVASES) {
      PresentLayer *l = &layers[m++];
      memset(l, 0, sizeof(*l));
      l->key = vs;
      l->rgba = vs->px;
      l->w = vs->w;
      l->h = vs->h;
      l->dirty = vchanged;
    }
    backend_present_layers(backend, layers, m);
    return;
  }
  const Surface *screen = compose_screen(ctx, scratch);
  if (screen) backend_present(backend, screen->px, screen->w, screen->h);
}

typedef struct {
  JSContext *ctx;
  Surface   *scratch;
} ShotCtx;

static const Surface *script_screen(void *user) {
  ShotCtx *sc = user;
  return compose_screen(sc->ctx, sc->scratch);
}
