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

// El thread que presenta ([engine] present_ahead): recibe cada imagen con su
// momento y la muestra a su hora, sin que el juego espere.
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

//
// [engine] present_ahead: con WebGL, la ventana y el contexto que presenta
// pasan a otro thread. El juego entrega cada imagen (la textura de su canvas,
// que deja de usar: webgl_canvas_handoff) con el momento en que mostrarla, y
// sigue con la siguiente sin esperar. Este thread espera ese momento, a que la
// GPU la termine (gl_fence) y la muestra. Si una imagen del juego tarda mas
// de lo normal, la anterior ya esta lista: no se nota. Las capas 2d (pixeles
// en memoria del juego) se entregan esperando a que se hayan mostrado.

static double mono_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// Con sway, en la RGB30, la primera vez que se abre el juego tras encender la
// consola el cambio de imagen del thread tarda ~50 ms (tres refrescos) en
// cada imagen: el juego se queda a 20 fps (los siguientes arranques, a 30).
// Tras THROTTLED_SWAPS asi seguidos (~2 s) el thread se para y se presenta
// sin el (gpu_present: el driver decide como).
#define THROTTLED_SWAPS 60

// Con el aviso de carga, si el juego no entrega imagen en LOADING_REPAINT_MS
// (esta parado cargando), el thread vuelve a mostrar la ultima con el aviso
#define LOADING_REPAINT_MS 250

// Lo que el thread que presenta toma del backend para dibujar cada imagen:
// el texto del contador, los avisos, el escalado y la lupa (Options y el
// mando los cambian mientras se juega)
static void shadow_update(Backend *shadow, Backend *backend, const char *overlay, Toast toast) {
  memcpy(shadow->overlay, overlay, sizeof shadow->overlay);
  shadow->toast = toast;
  shadow->loading = backend_loading_visible(backend);
  shadow->scaling = backend->scaling;
  shadow->magnify = backend->magnify;
  shadow->mag_size = backend->mag_size;
  shadow->mag_x = backend->mag_x;
  shadow->mag_y = backend->mag_y;
}

static int present_thread(void *arg) {
  Backend *backend = arg;
  SDLPlatformData *data = backend->platform_data;
  Backend shadow;   // lo que dibuja del backend (shadow_update)
  memset(&shadow, 0, sizeof shadow);
  int ok = SDL_GL_MakeCurrent(data->window, data->gl) == 0;
  SDL_LockMutex(data->mx);
  data->thr_ok = ok ? 1 : -1;
  SDL_CondBroadcast(data->cv);
  SDL_UnlockMutex(data->mx);
  if (!ok) return 0;
  SDL_GL_SetSwapInterval(backend->no_vsync ? 0 : 1);

  // la ultima imagen mostrada: sus texturas no vuelven al juego hasta que
  // entregue la siguiente (y para eso espera a job_busy)
  PresentJob last;
  int have_last = 0;
  double last_shown = 0;

  for (;;) {
    SDL_LockMutex(data->mx);
    int repaint = 0;
    while (!data->job_pending && !data->thr_quit) {
      if (!backend->loading || !have_last) {
        SDL_CondWait(data->cv, data->mx);
        continue;
      }
      double wait = last_shown + LOADING_REPAINT_MS - mono_ms();
      if (wait <= 0) {
        // con retardo, el aviso puede no verse aun: se vuelve a mirar luego
        if (backend_loading_visible(backend)) {
          repaint = 1;
          break;
        }
        last_shown = mono_ms();
        continue;
      }
      SDL_CondWaitTimeout(data->cv, data->mx, (Uint32)wait + 1);
    }
    if (data->thr_quit && !data->job_pending) {
      SDL_UnlockMutex(data->mx);
      break;
    }
    if (repaint) {
      // el juego esta parado cargando: la ultima imagen otra vez, con el
      // aviso (sus capas 2d, sin subirlas de nuevo: el juego puede estar
      // cambiando sus pixeles)
      data->job_busy = 1;
      SDL_UnlockMutex(data->mx);
      for (int i = 0; i < last.n; i++) last.layers[i].dirty = 0;
      shadow_update(&shadow, backend, last.overlay, last.toast);
      draw_layers(&shadow, data, last.layers, last.n, last.ww, last.wh, 0);
      qgl.Flush();
      SDL_LockMutex(data->mx);
      data->job_busy = 0;
      SDL_CondBroadcast(data->cv);
      SDL_UnlockMutex(data->mx);
      SDL_GL_SwapWindow(data->window);
      last_shown = mono_ms();
      continue;
    }
    PresentJob job = data->job;   // copia: el juego ya puede preparar la siguiente
    data->job_pending = 0;
    data->job_busy = 1;
    SDL_UnlockMutex(data->mx);

    if (job.at > 0) {
      double wait = job.at - mono_ms();
      if (wait > 0) {
        struct timespec ts;
        double t = job.at;
        ts.tv_sec = (time_t)(t / 1000);
        ts.tv_nsec = (long)((t - ts.tv_sec * 1000.0) * 1e6);
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) == EINTR) {}
      }
    }
    // a que la GPU termine la imagen, como diga el driver (thread_wait). Con
    // la traza, la CPU: cuanto le quedaba al llegar su momento, y cuanto
    // tardo desde que el juego la entrego
#if QUIX_TRACE
    TRACE_START(tg);
    gl_fence_wait_cpu(job.fence);
    TRACE_END(tg, 0.0, "gpu frame: %.2f ms after handed, waited", quix_trace_ms() - job.handed);
#else
    gpu_driver()->thread_wait(job.fence);
#endif
    shadow_update(&shadow, backend, job.overlay, job.toast);
    draw_layers(&shadow, data, job.layers, job.n, job.ww, job.wh, 0);
    last = job;
    have_last = 1;
    // copiada: sus texturas ya se pueden volver a usar (lo que haga el juego
    // con ellas va detras en la GPU) y el juego no espera al cambio de
    // imagen, que puede tardar hasta un refresco
    qgl.Flush();
    SDL_LockMutex(data->mx);
    data->job_busy = 0;
    SDL_CondBroadcast(data->cv);
    SDL_UnlockMutex(data->mx);
    double t_swap = mono_ms();
    swap_window(data);
    last_shown = mono_ms();
    // el cambio de imagen, que espera al compositor, tarda mas de un refresco
    // y medio una y otra vez: ver thread_throttled
    if (data->thr_vper > 0 && !backend->no_vsync) {
      data->thr_slow = mono_ms() - t_swap > 1.5 * data->thr_vper ? data->thr_slow + 1 : 0;
      if (data->thr_slow >= THROTTLED_SWAPS) {
        SDL_LockMutex(data->mx);
        data->thr_throttled = 1;
        SDL_UnlockMutex(data->mx);
      }
    }

    SDL_LockMutex(data->mx);
    data->job_done++;
    for (int i = 0; i < data->shown_n && data->pub_n < SHOWN_MAX; i++) data->pub[data->pub_n++] = data->shown[i];
    data->shown_n = 0;
    SDL_CondBroadcast(data->cv);
    SDL_UnlockMutex(data->mx);
  }
  SDL_GL_MakeCurrent(data->window, NULL);
  return 0;
}

static void thread_start(Backend *backend, SDLPlatformData *data) {
  data->thr_tried = 1;
  if (!backend->present_ahead || !gl_available()) return;
  // solo sirve con los refrescos de la pantalla (la imagen de adelanto apunta
  // a uno); y solo con los drivers que lo permiten (gfx/drivers/)
  double vlast, vper;
  if (platform_vblank(&vlast, &vper) != 0) {
    printf("[present] one frame ahead: not used (no screen refreshes)\n");
    return;
  }
  data->thr_vper = vper;
  if (!gpu_driver()->thread_wait) {
    printf("[present] one frame ahead: not used with the %s driver\n", gpu_driver()->name);
    return;
  }
  int fps = backend->fps > 0 ? backend->fps : 60;
  if (gpu_driver()->thread_max_fps && fps > gpu_driver()->thread_max_fps) {
    printf("[present] one frame ahead: not used at %d fps with the %s driver\n", fps, gpu_driver()->name);
    return;
  }
  SDL_GL_MakeCurrent(data->window, data->gl);
  if (gl_offthread_begin() != 0) {
    printf("[present] one frame ahead: not available (needs EGL surfaceless)\n");
    return;
  }
  data->mx = SDL_CreateMutex();
  data->cv = SDL_CreateCond();
  data->thr_ok = 0;
  data->thr = data->mx && data->cv ? SDL_CreateThread(present_thread, "quix-present", backend) : NULL;
  if (data->thr) {
    SDL_LockMutex(data->mx);
    while (data->thr_ok == 0) SDL_CondWait(data->cv, data->mx);
    SDL_UnlockMutex(data->mx);
  }
  if (!data->thr || data->thr_ok < 0) {
    if (data->thr) SDL_WaitThread(data->thr, NULL);
    data->thr = NULL;
    gl_offthread_end();
    printf("[present] one frame ahead: not available (presenting thread)\n");
    return;
  }
  backend->present_thread = 1;
  printf("[present] one frame ahead (presenting thread)\n");
}

static void thread_stop(Backend *backend, SDLPlatformData *data) {
  if (!data->thr) return;
  SDL_LockMutex(data->mx);
  data->thr_quit = 1;
  SDL_CondBroadcast(data->cv);
  SDL_UnlockMutex(data->mx);
  SDL_WaitThread(data->thr, NULL);
  data->thr = NULL;
  backend->present_thread = 0;
  gl_offthread_end();
}

// espera a que el thread haya mostrado todo lo entregado (con el cerrojo)
static void wait_idle_locked(SDLPlatformData *data) {
  while (data->job_pending || data->job_busy) SDL_CondWait(data->cv, data->mx);
}

static void sdl_present_wait(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->thr) return;
  SDL_LockMutex(data->mx);
  wait_idle_locked(data);
  SDL_UnlockMutex(data->mx);
}

static int sdl_presented(Backend* backend, double *ms, int max) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  int n = 0;
  if (data->thr) {
    SDL_LockMutex(data->mx);
    for (; n < data->pub_n && n < max; n++) ms[n] = data->pub[n];
    data->pub_n = 0;
    SDL_UnlockMutex(data->mx);
  } else {
    for (; n < data->shown_n && n < max; n++) ms[n] = data->shown[n];
    data->shown_n = 0;
  }
  return n;
}
