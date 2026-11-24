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

// Entregar una imagen: con el thread o sin el (gpu_driver_present y el Presenter
// del driver), repintar durante una carga y las tres entradas del backend
// (present_layers, present y present_gl).
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

// Lo que el backend hace con una imagen (gfx/gl/driver.h, Presenter)

// la CPU espera a que la GPU termine lo dibujado en el canvas WebGL (otro
// contexto) antes de leerlo: con libmali no basta con mandarlo (gl_fence),
// ni con que la GPU espere la marca
static void frame_wait(GpuFrame *g) {
  SdlFrame *f = (SdlFrame *)g;
  TRACE_START(tw);
  gl_fence_wait_cpu(f->job.fence);
  TRACE_END(tw, 0.0, "gpu wait (cpu)");
  f->job.fence = NULL;
}

// Con el contexto del juego actual (gfx/gl/driver.h, one_context): dibuja la
// pantalla en el, guardando su estado y devolviendolo despues. 0 si ese
// contexto no es el de la ventana (WebGL con su propia ventana oculta)
static int one_context(SDLPlatformData *data) {
  if (!gpu_driver_get()->one_context || !gl_available()) return 0;
  gl_make_current();
  return SDL_GL_GetCurrentWindow() == data->window;
}

static void draw_in_game_context(Backend *backend, SDLPlatformData *data, const PresentJob *j, int ww, int wh) {
  GlState s;
  gl_state_save(&s);
  gl_state_plain();
  draw_layers(backend, data, j->layers, j->n, ww, wh, 1);
  gl_state_restore(&s);
}

static void frame_show(GpuFrame *g) {
  SdlFrame *f = (SdlFrame *)g;
  if (f->data->one) {
    draw_in_game_context(f->backend, f->data, &f->job, f->job.ww, f->job.wh);
  } else {
    draw_layers(f->backend, f->data, f->job.layers, f->job.n, f->job.ww, f->job.wh, 1);
    f->data->shown_fence = gl_fence();
  }
  f->data->shown_job = f->job;
  f->data->have_shown = 1;
}

// Sin el thread, con el juego parado a mitad de un trabajo largo
// (backend_loading_poke): la ultima imagen otra vez, con el aviso de carga.
// Con el contexto que presenta (las texturas que muestra no las toca el
// juego hasta la siguiente entrega) y luego el que estaba: el del juego no
// cambia. Sus capas 2d, sin subirlas: el juego puede estar cambiandolas.
static void sdl_repaint(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data || !data->gl || data->thr || !data->have_shown) return;
  SDL_Window *prev_win = SDL_GL_GetCurrentWindow();
  SDL_GLContext prev_ctx = SDL_GL_GetCurrentContext();
  if (data->one && one_context(data)) {
    PresentJob j = data->shown_job;
    for (int i = 0; i < j.n; i++) j.layers[i].dirty = 0;
    int ww, wh;
    SDL_GL_GetDrawableSize(data->window, &ww, &wh);
    draw_in_game_context(backend, data, &j, ww, wh);
    if (prev_ctx) SDL_GL_MakeCurrent(prev_win, prev_ctx);
    return;
  }
  SDL_GL_MakeCurrent(data->window, data->gl);
  gl_fence_wait_cpu(data->shown_fence);   // lo anterior con esas texturas, ya hecho
  PresentJob j = data->shown_job;
  for (int i = 0; i < j.n; i++) j.layers[i].dirty = 0;
  int ww, wh;
  SDL_GL_GetDrawableSize(data->window, &ww, &wh);
  draw_layers(backend, data, j.layers, j.n, ww, wh, 1);
  data->shown_fence = gl_fence();
  if (prev_ctx) SDL_GL_MakeCurrent(prev_win, prev_ctx);
}

static void frame_drop(GpuFrame *g) {
  SdlFrame *f = (SdlFrame *)g;
  gl_fence_wait_cpu(f->job.fence);   // la marca se suelta
  f->job.fence = NULL;
}

static const Presenter sdl_presenter = { frame_wait, frame_show, frame_drop };

// La ventana ya cabe en la pantalla? Con un gestor de ventanas (sway en
// ROCKNIX) la ventana aparece con el tamano del juego y el script del port
// la pone a pantalla completa segundos despues; si el juego es mas grande
// que la pantalla, hasta entonces se veria recortado (el menu "gigante").
// Mientras no quepa se muestra negro, como mucho FIT_WAIT_MS. Se mira solo
// al principio: cuando cabe (o se acaba el tiempo) fit_until queda a 0 y no
// cuesta nada mas. Con KMSDRM la ventana es la pantalla: cabe desde el
// primer frame.
static int window_fits(SDLPlatformData *data) {
  if (!data->fit_until) return 1;
  int ww, wh;
  SDL_Rect screen;
  SDL_GetWindowSize(data->window, &ww, &wh);
  int display = SDL_GetWindowDisplayIndex(data->window);
  int fits = display < 0 || SDL_GetDisplayBounds(display, &screen) != 0 ||
             (ww <= screen.w && wh <= screen.h);
  if (fits || SDL_TICKS_PASSED(SDL_GetTicks(), data->fit_until)) {
    data->fit_until = 0;
    return 1;
  }
  return 0;
}

static void sdl_present_layers(Backend* backend, const PresentLayer *layers, int n) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->gl) return;
  if (!window_fits(data)) {
    // negro hasta que quepa (el thread de present aun no ha empezado)
    SDL_GL_MakeCurrent(data->window, data->gl);
    qgl.ClearColor(0, 0, 0, 1);
    qgl.Clear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(data->window);
    // la cuenta para hacerla redimensionable sigue (sway la necesita asi para
    // cambiarle el tamano)
    if (data->frames_until_resizable > 0 && --data->frames_until_resizable == 0) {
      SDL_SetWindowResizable(data->window, SDL_TRUE);
      data->frames_until_resizable = -1;
    }
    return;
  }
  if (n > PRESENT_MAX_LAYERS) n = PRESENT_MAX_LAYERS;
  int ww, wh;
  SDL_GL_GetDrawableSize(data->window, &ww, &wh);

  if (!data->thr_tried && backend->present_ahead && gl_available()) thread_start(backend, data);
  if (data->thr && data->thr_throttled) {
    // el compositor frena el thread: se sigue sin el
    SDL_LockMutex(data->mx);
    wait_idle_locked(data);
    SDL_UnlockMutex(data->mx);
    thread_stop(backend, data);
    printf("[present] one frame ahead: stopped (each frame took more than %.0f ms to show)\n", 1.5 * data->thr_vper);
  }
  if (data->thr) {
    // las capas 2d son pixeles del juego: se espera a que se suban
    int async = backend->present_async;
    for (int i = 0; i < n; i++) if (!layers[i].gl_tex) async = 0;
    void *fence = gl_fence();   // lo dibujado en el canvas, a la GPU
    SDL_LockMutex(data->mx);
    wait_idle_locked(data);
    PresentJob *j = &data->job;
    memcpy(j->layers, layers, sizeof(PresentLayer) * n);
    j->n = n;
    j->ww = ww;
    j->wh = wh;
    j->at = backend->present_at;
    j->fence = fence;
    j->handed = mono_ms();
    memcpy(j->overlay, backend->overlay, sizeof j->overlay);
    j->toast = backend->toast;
    data->job_pending = 1;
    SDL_CondBroadcast(data->cv);
    if (!async) wait_idle_locked(data);
    SDL_UnlockMutex(data->mx);
  } else {
    SdlFrame *f = &data->frames[data->frame_i];
    data->frame_i ^= 1;
    memcpy(f->job.layers, layers, sizeof(PresentLayer) * n);
    f->job.n = n;
    f->job.ww = ww;
    f->job.wh = wh;
    f->backend = backend;
    f->data = data;
    // se puede mostrar mas tarde si todas sus capas son texturas entregadas
    f->base.later = backend->present_async;
    for (int i = 0; i < n; i++) if (!layers[i].gl_tex) f->base.later = 0;
    data->one = one_context(data);
    if (data->one) {
      // la pantalla, con el contexto del juego: la GPU lo hace en orden
      f->job.fence = NULL;
    } else {
      // lo dibujado en el canvas WebGL (otro contexto), a la GPU
      f->job.fence = gl_available() ? gl_fence() : NULL;
      SDL_GL_MakeCurrent(data->window, data->gl);
      // la textura que se mostro la ultima vez vuelve al juego: su dibujo en
      // pantalla, terminado (es de hace una imagen: no se espera)
      gl_fence_wait_cpu(data->shown_fence);
      data->shown_fence = NULL;
    }
    gpu_driver_present(&sdl_presenter, &f->base);   // cuando se ve: lo decide el driver
  }

  if (data->frames_until_resizable > 0 && --data->frames_until_resizable == 0) {
    SDL_SetWindowResizable(data->window, SDL_TRUE);
    data->frames_until_resizable = -1;
  }
}

// Frame de un canvas 2d (RGBA en memoria)
static void sdl_present(Backend* backend, const uint8_t *rgba, int w, int h) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->gl || !rgba || w <= 0 || h <= 0) return;
  PresentLayer l;
  memset(&l, 0, sizeof l);
  l.key = data;
  l.rgba = rgba;
  l.w = w;
  l.h = h;
  l.dirty = 1;
  sdl_present_layers(backend, &l, 1);
}

// Canvas WebGL: su textura directamente (comparte texturas con este contexto)
static void sdl_present_gl(Backend* backend, unsigned tex, int w, int h, int premultiplied) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->gl) return;
  PresentLayer l;
  memset(&l, 0, sizeof l);
  l.key = data;
  l.gl_tex = tex;
  l.w = w;
  l.h = h;
  l.premultiplied = premultiplied;
  sdl_present_layers(backend, &l, 1);
}
