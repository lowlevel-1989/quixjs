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

// Los pasos de cada frame (STEP, always_inline): el frame del juego,
// el ritmo fijo en pantalla, mostrarlo y --frames.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

// Lo que se acuerda de un frame a otro: static, como en el main() de antes
static double   vb_target;   // el refresco al que apunta la ultima imagen (0 sin refrescos)
static int      vb_frames;   // cuantos refrescos dura cada imagen
static double   ready_ms;    // cuando suelen estar listas las imagenes
static unsigned hook_frames;
static long     dec_count;   // imagenes decodificadas hasta el frame anterior
static double   dec_ms;

// El frame del juego: requestAnimationFrame, video, audio, el estado del
// port (simbolos, trucos, logros, avisos) y el contador de --fps
STEP void game_frame(Run *r, Loop *l) {
  JSContext *ctx = l->ctx;
  l->t0 = r->fps.enabled ? real_ms() : 0;
  l->frame_slot = l->next_frame;   // el momento que le tocaba a esta imagen
  // quix.frameTime: el refresco al que apuntara esta imagen (constante de
  // una a otra), o el momento que le tocaba sin refrescos
  if (vb_target > 0) {
    double vlast, vper;
    if (platform_vblank(&vlast, &vper) != 0) vper = 1000.0 / 60;
    r->app.frame_time = vb_target + vb_frames * vper + (clock_now_ms() - real_ms());
  } else {
    r->app.frame_time = l->frame_slot;
  }
  if (hooks_pending()) hooks_install(ctx);
  if (++hook_frames % 30 == 0) hooks_check(ctx);
  l->err = timer_run_frame(ctx, l->now);
  l->t_raf = r->fps.enabled ? real_ms() : 0;
  input_frame_done(l->backend);
  video_tick();
  audio_tick(ctx);
  // el estado del juego: los simbolos, los trucos activos, los logros y
  // los avisos (en este orden)
  symbols_tick(ctx);
  cheats_tick(ctx);
  ach_tick(ctx, l->backend, real_ms());
  toast_tick(l->backend, real_ms());

  l->t1 = r->fps.enabled ? real_ms() : 0;
  if (r->fps.enabled) {
    FpsMeter *fps = &r->fps;
    // el reparto de este frame, si es el peor del intervalo (antes de
    // fps_tick, que puede escribir la linea)
    if (fps->prev_tick > 0 && l->t1 - fps->prev_tick >= fps->worst_ms) {
      double parts[6] = { l->t_raf - l->t0, l->t1 - l->t_raf, fps->prev_present, fps->fr_timers, fps->fr_events, fps->fr_idle };
      memcpy(fps->worst_parts, parts, sizeof parts);
    }
    fps->fr_timers = fps->fr_events = fps->fr_idle = 0;
    fps_tick(fps);
    fps_overlay(fps, l->backend);
  }
}

// Ritmo fijo en pantalla. Con la pantalla a 60 Hz y el juego a 30, si las
// imagenes llegan unas antes y otras despues de un refresco, unas duran
// 16,7 ms y otras 50: se ve a saltitos, o a medio camino, aunque vaya a 30.
//
// Con los refrescos de la pantalla (platform_vblank) y el juego a la mitad
// del refresco o menos: cada imagen apunta a un refresco (el anterior mas
// N, en la rejilla real de refrescos) y se entrega VB_LEAD ms antes. Con
// Weston (el compositor Wayland de algunas portatiles) cae entre el refresco
// intermedio y el suyo (Weston prepara cada uno ~7 ms antes); sin Weston
// (KMSDRM) el cambio es en el refresco siguiente a la entrega, que es el
// suyo. Asi dura siempre N refrescos. Si llega tarde, apunta al siguiente
// refresco. Sin refrescos (o a 60 fps, donde el vsync basta), en un punto
// fijo de su turno: cuando estan listas casi todas las imagenes (el mas alto
// de las ultimas, que baja poco a poco) mas 2 ms
STEP void pace_frame(Run *r, Loop *l) {
  Backend *backend = l->backend;
  double ready = clock_now_ms() - l->frame_slot;
  ready_ms = ready > ready_ms ? ready : ready_ms * 0.995 + ready * 0.005;
  if (!r->cfg.engine.vsync || clock_is_fixed()) return;

  double present_at = l->frame_slot + (ready_ms + 2 < l->frame_ms - 3 ? ready_ms + 2 : l->frame_ms - 3);
  double vlast, vper;
  int vb_n = 0;
  // solo si los fps son una fraccion exacta del refresco y los
  // refrescos del sistema son los de la pantalla de la ventana (con
  // varias pantallas, la consulta podria ser de otra)
  if (platform_vblank(&vlast, &vper) == 0 && vper > 4) {
    double ratio = l->frame_ms / vper, hz = backend_refresh_rate(backend);
    vb_n = (int)(ratio + 0.5);
    if (fabs(ratio - vb_n) > 0.05 || (hz > 0 && fabs(hz - 1000.0 / vper) > 1.5)) vb_n = 0;
  }
  // con el thread que presenta (present_ahead): esta imagen se entrega
  // cuando ya se haya mostrado la anterior (en su refresco menos
  // VB_LEAD) y el juego sigue sin esperar. Hasta entonces, el hueco
  // es para el GC.
  int ahead = backend->present_thread && vb_n >= 2;
  double t_hold = r->fps.enabled ? real_ms() : 0;
  if (ahead) {
    double gap = vb_target > 0 ? vb_target - VB_LEAD - real_ms() : 0;
    if (gap > 3) JS_IdleWork(JS_GetRuntime(l->ctx), gap - 1.5);
    backend_present_wait(backend);
  }
  backend->present_async = 0;
  backend->present_at = 0;
  // sin el thread: la imagen se entrega igual (el canvas sigue en otra
  // textura) y el driver decide cuando se ve (gfx/drivers/, present)
  if (!backend->present_thread) backend->present_async = 1;
  if (vb_n >= 2) {
    double now = real_ms();
    double target = vb_target > 0 ? vb_target + vb_n * vper : vlast + vb_n * vper;
    target = vlast + ceil((target - vlast) / vper - 0.5) * vper;   // en la rejilla de refrescos
    while (target - VB_LEAD < now) target += vper;                  // tarde: el siguiente
    vb_target = target;
    vb_frames = vb_n;
    present_at = target - VB_LEAD + (clock_now_ms() - now);         // a la escala del loop
  } else {
    vb_target = 0;
  }
  if (ahead && vb_target > 0) {
    backend->present_async = 1;
    backend->present_at = vb_target - VB_LEAD;
  } else {
    // el hueco hasta presentar: para el GC (lo que deja pendiente
    // lo haria si no en medio del JavaScript de las imagenes siguientes)
    if (present_at - clock_now_ms() > 3) JS_IdleWork(JS_GetRuntime(l->ctx), present_at - clock_now_ms() - 1.5);
    while (clock_now_ms() < present_at) sleep_ms(present_at - clock_now_ms());
  }
  if (r->fps.enabled) r->fps.fr_idle += real_ms() - t_hold;
}

// Mostrar la imagen, apuntar sus tiempos (--fps, la consola de debug, el
// log) y calcular cuando toca la siguiente
STEP void present_frame(Run *r, Loop *l) {
  present_screen(l->ctx, l->backend, &r->scratch);
  if (worker_pending() > 0) backend_loading_hold(l->backend);   // aun carga cosas
  backend_loading_tick(l->backend);
  perf_frame();
  if (r->args.debug_sock) {
    // cuando se mostraron de verdad (con el thread que presenta, mas tarde)
    double shown[64];
    int k = backend_presented(l->backend, shown, 64);
    if (k < 0) debug_note_present(real_ms());
    for (int i = 0; i < k; i++) debug_note_present(shown[i]);
  }
  if (r->fps.enabled) {
    FpsMeter *fps = &r->fps;
    double present = real_ms() - l->t1;
    fps->prev_present = present;
    fps->js_ms += l->t1 - l->t0;
    fps->present_ms += present;   // componer o leer de la GPU + mostrar
    fps->total_window++;
    // un tiron (mas de 100 ms desde el frame anterior): que lo explique el
    // log (el JS de este frame, el dibujado y las imagenes decodificadas)
    if (fps->last_gap > 100) {
      printf("[slow frame] %.0f ms: js %.1f ms, present %.1f ms, images decoded %ld (%.0f ms)\n", fps->last_gap,
             l->t1 - l->t0, present, quix_mem.decode_count - dec_count, quix_mem.decode_ms - dec_ms);
    }
    dec_count = quix_mem.decode_count;
    dec_ms = quix_mem.decode_ms;
  }

  profiler_frame_end(l->ctx);
  r->app.frame++;
  if (r->args.debug_sock) debug_frame_done();
  // el tope puede cambiar en caliente (comando fps de la consola de debug)
  l->frame_ms = 1000.0 / (r->cfg.engine.fps > 0 ? r->cfg.engine.fps : 60);
  l->next_frame += l->frame_ms;
  if (l->next_frame <= l->now) l->next_frame = l->now + l->frame_ms;
  // con los refrescos: la siguiente imagen empieza ya (se entrega a su hora)
  if (vb_target > 0) l->next_frame = clock_now_ms();
}

// --frames: 1 si ya se llego al limite
STEP int frame_limit(Run *r, Loop *l) {
  if (r->args.max_frames > 0 && r->app.frame >= r->args.max_frames) {
    if (!r->app.exit_requested) printf("Frame limit reached (%ld)\n", r->app.frame);
    return 1;
  }
  return 0;
}
