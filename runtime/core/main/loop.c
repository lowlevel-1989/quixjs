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

// Los pasos del bucle de cada vuelta (STEP, always_inline): la memoria
// y el profiler, los eventos, los timers y esperar al siguiente.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

// Lo de cada x segundos: la memoria (malloc y el guardian) y el profiler
STEP void housekeeping(Run *r, Loop *l) {
  // lo libre del heap de malloc, de vuelta al sistema (cada 10 s)
  if (real_ms() >= l->next_malloc_trim) {
    platform_mem_trim();
    l->next_malloc_trim = real_ms() + 10000;
  }
  // guardian de memoria: si al sistema le queda poca RAM (MemAvailable por
  // debajo de [engine] low_memory_mb), un GC completo de JS y lo
  // libre de malloc de vuelta, como mucho cada 5 s. Sin esto, en una
  // portatil de 1 GB el sistema empezaba a releer el programa del disco
  // (tirones de mas de 1 s) y acababa bloqueado.
  if (r->cfg.engine.low_memory_mb > 0 && real_ms() >= l->next_mem_check) {
    l->next_mem_check = real_ms() + 1000;
    long avail = platform_mem_available_mb();
    if (avail >= 0 && avail < r->cfg.engine.low_memory_mb && real_ms() >= l->next_mem_rescue) {
      // primero lo que no para el juego: los pixeles de las imagenes que ya
      // estan en la GPU; el GC completo (una parada de 100-200 ms en
      // una portatil) solo si con eso no basta
      int n = 0;
      size_t freed = js_image_release_uploaded(&n);
      if (freed) {
        platform_mem_trim();
        long now = platform_mem_available_mb();
        printf("[memory] %ld MB left: released %.0f MB of %d images already on the GPU, now %ld MB\n", avail,
               freed / 1048576.0, n, now);
        avail = now;
      }
      // un GC completo que apenas libero (lo que queda es memoria que el
      // juego usa): repetirlo cada 5 s era una parada de 230-400 ms cada
      // 5 s sin ganar nada (RX6S, 731 MB). El siguiente, cuando la RAM
      // libre baje GC_RETRY_DROP_MB mas o pase GC_RETRY_MS.
      if (avail >= 0 && avail < r->cfg.engine.low_memory_mb &&
          (avail <= l->gc_retry_below || real_ms() >= l->gc_retry_at)) {
        double g0 = real_ms();
        JS_RunGC(l->rt);
        platform_mem_trim();
        long after = platform_mem_available_mb();
        printf("[memory] %ld MB left: full collection (%.0f ms), now %ld MB\n", avail, real_ms() - g0, after);
        if (after - avail < GC_USEFUL_MB) {
          l->gc_retry_below = after - GC_RETRY_DROP_MB;
          l->gc_retry_at = real_ms() + GC_RETRY_MS;
        } else {
          l->gc_retry_below = LONG_MAX;
          l->gc_retry_at = 0;
        }
      }
      l->next_mem_rescue = real_ms() + 5000;
    }
  }
  // las muestras del profiler: una vez por segundo (JavaScriptCore las
  // guarda entre medias; recogerlas y leer su JSON en cada vuelta costaba
  // decenas de ms por imagen en la consola)
  if (profiler_running() && real_ms() >= l->next_profile_drain) {
    profiler_drain(l->ctx);
    l->next_profile_drain = real_ms() + 1000;
  }
  if (r->args.profile && l->next_profile_save > 0 && real_ms() >= l->next_profile_save) {
    profiler_save(l->ctx, r->args.profile, 60);
    l->next_profile_save = real_ms() + 10000;
  }
}

// Los eventos del backend, la consola de debug y --script
STEP int events(Run *r, Loop *l, double t_events) {
  backend_process_events(l->backend);

  // consola de debug: entre frames, sin JS corriendo
  if (r->args.debug_sock) {
    if (debug_poll(&r->debug_env)) return STEP_STOP;
    if (debug_paused()) {
      // en pausa: la ventana sigue respondiendo y mostrando el ultimo frame
      if (backend_should_close(l->backend)) return STEP_STOP;
      present_screen(l->ctx, l->backend, &r->scratch);
      sleep_ms(16);
      return STEP_AGAIN;
    }
  }

  if (r->fps.enabled) {
    r->fps.events_ms += real_ms() - t_events;
    r->fps.fr_events += real_ms() - t_events;
  }

  // guion de entrada (--script): teclas y capturas del frame actual
  if (r->args.script) {
    int code = 0;
    if (input_script_run(r->app.frame + l->script_offset, l->backend, script_screen, &r->shot_ctx, &code)) {
      r->app.exit_requested = 1;
      r->app.exit_code = code;
      return STEP_STOP;
    }
  }
  return STEP_GO;
}

// El JS fuera del frame: la entrada, las promesas, lo cargado en segundo
// plano y los timers
STEP void run_timers(Run *r, Loop *l) {
  double t_other = r->fps.enabled ? real_ms() : 0;
  l->err = dispatch_input(l->ctx, l->backend);
  if (l->err == 0) l->err = run_pending_jobs(l->ctx);
  worker_poll(l->ctx);   // lo cargado en segundo plano (imagenes, sonidos)
  if (l->err == 0) l->err = timer_run_due(l->ctx);
  if (r->fps.enabled) {
    r->fps.timers_ms += real_ms() - t_other;
    r->fps.fr_timers += real_ms() - t_other;
  }
}

// ¿Queda algo por hacer? Si si, espera hasta el siguiente timer o frame y
// devuelve STEP_GO; STEP_STOP si el juego ya no tiene nada pendiente
STEP int wait_next(Run *r, Loop *l) {
  // el audio que suena tambien avanza frame a frame (onended, fundidos)
  int frames_pending = timer_frame_count() > 0 || mixer_active() > 0 || video_active();
  int timers_pending = timer_count() > 0;
  int jobs_pending = JS_IsJobPending(l->rt);
  int images_pending = worker_pending() > 0;   // cargandose en segundo plano
  CanvasData *list[1];
  int has_screen = document_screen_canvases(l->ctx, list, 1) > 0;
  int alive = frames_pending || timers_pending || jobs_pending || images_pending || l->backend->input_len > 0 ||
              (r->args.script && input_script_active()) || r->args.debug_sock ||
              (l->keep_window && has_screen);
  if (!alive) return STEP_STOP;
  if (jobs_pending || l->backend->input_len > 0) return STEP_GO;

  // Esperar hasta el siguiente timer o frame
  double wake = -1;
  if (timers_pending) wake = timer_next_deadline();
  if (frames_pending || (l->keep_window && has_screen)) {
    if (wake < 0 || l->next_frame < wake) wake = l->next_frame;
  }
  // algo cargandose en segundo plano (imagenes, sonidos en el thread de
  // trabajo): hay que volver a mirar pronto
  if (images_pending && clock_is_fixed()) {
    // reloj fijo (tests): el reloj del juego salta al siguiente timer sin
    // esperar, asi que se duerme 1 ms de tiempo real para que el thread de
    // trabajo avance, sin adelantar el reloj del juego
    sleep_ms(1);
    if (wake < 0) return STEP_GO;
  } else if (images_pending && (wake < 0 || wake > clock_now_ms() + 2)) {
    wake = clock_now_ms() + 2;   // reloj real: como mucho 2 ms hasta volver a mirar
  }
  if (clock_is_fixed()) {
    if (wake > 0) {
      clock_advance_to(wake);
      // el frame avanza con el reloj aunque solo haya timers
      if (!frames_pending && wake > l->next_frame) l->next_frame = wake;
    }
  } else if (wake >= 0) {
    double t_sleep = r->fps.enabled ? real_ms() : 0;
    sleep_ms(wake - clock_now_ms());
    if (r->fps.enabled) {
      r->fps.idle_ms += real_ms() - t_sleep;
      r->fps.fr_idle += real_ms() - t_sleep;
    }
  }
  return STEP_GO;
}
