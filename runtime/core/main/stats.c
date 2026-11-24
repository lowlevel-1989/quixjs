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

// --fps (el contador y la linea FPS:) y el resumen [perf] del log.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

// --fps: frames por segundo con el reloj real (aunque el juego use el fijo)
typedef struct {
  int     enabled;
  double  first;       // inicio de la medida (para la media)
  double  last;        // ultimo frame dibujado
  long    total;
  double  window_start;
  int     frames;
  double  value;
  double  js_ms, present_ms;   // suma desde la ultima linea
  double  timers_ms;           // JS fuera del frame: timers, promesas, eventos ("timers")
  double  idle_ms;             // esperando al siguiente frame ("idle")
  double  events_ms;           // eventos del backend y consola de debug
  double  wall_ms, prev_tick;  // tiempo real entre frames
  double  worst_ms;            // el frame mas largo del intervalo (los tirones)
  // en que se fue el tiempo desde el frame anterior, y el reparto del peor
  // frame del intervalo (lo escribe la linea FPS:, para explicar la caida)
  double  fr_timers, fr_events, fr_idle, prev_present;
  double  worst_parts[6];      // raf, state, present, timers, events, idle
  double  last_gap;            // tiempo real desde el frame anterior
  int     total_window;
} FpsMeter;

// El resumen del log ([perf]): siempre, sin --fps. Una linea por minuto y
// otra al salir, para que un log enviado desde una consola diga como fue
// (fps, frames de mas de 50 y de 100 ms y el driver)
typedef struct {
  double start, prev;
  long   frames, over50, over100;
} PerfSpan;

static PerfSpan perf_minute, perf_all;

static void perf_print(const char *what, const PerfSpan *p, double now) {
  double s = (now - p->start) / 1000.0;
  if (p->frames < 2 || s <= 0) return;
  printf("[perf] %s: %.1f fps, %ld frames, %ld over 50 ms, %ld over 100 ms (driver %s)\n", what, p->frames / s,
         p->frames, p->over50, p->over100, gpu_driver_get()->name);
}

static void perf_add(PerfSpan *p, double now) {
  if (p->start == 0) p->start = now;
  if (p->prev > 0) {
    double gap = now - p->prev;
    if (gap > 50) p->over50++;
    if (gap > 100) p->over100++;
  }
  p->prev = now;
  p->frames++;
}

static void perf_frame(void) {
  double now = real_ms();
  perf_add(&perf_minute, now);
  perf_add(&perf_all, now);
  if (now - perf_minute.start >= 60000) {
    perf_print("last minute", &perf_minute, now);
    perf_minute = (PerfSpan){ .start = now, .prev = now };
  }
}

static void fps_tick(FpsMeter *f) {
  double now = real_ms();
  if (f->window_start == 0) f->window_start = now;
  if (f->first == 0) f->first = now;
  if (f->prev_tick > 0) {
    f->wall_ms += now - f->prev_tick;
    if (now - f->prev_tick > f->worst_ms) f->worst_ms = now - f->prev_tick;
    f->last_gap = now - f->prev_tick;
  }
  f->prev_tick = now;
  f->frames++;
  f->total++;
  f->last = now;
  // cada medio segundo en pantalla; una vez por segundo en la salida
  static double last_print = 0;
  if (now - f->window_start >= 500) {
    f->value = f->frames * 1000.0 / (now - f->window_start);
    f->frames = 0;
    f->window_start = now;
    if (now - last_print >= 1000) {
      // por frame: JS (timers + rAF), compose (incluye leer la GPU) y present
      int nf = f->total_window > 0 ? f->total_window : 1;
      double known = (f->js_ms + f->present_ms + f->timers_ms + f->idle_ms + f->events_ms) / nf;
      // en la salida, la media de todo el intervalo (no la de la ultima ventana:
      // un juego con un ciclo de ~1 s caeria siempre en la misma parte)
      double avg = f->wall_ms > 0 ? nf * 1000.0 / f->wall_ms : f->value;
      printf("FPS: %.1f (worst %.0f ms; js %.1f ms, present %.1f ms, timers %.1f ms, events %.1f ms, idle %.1f ms, rest %.1f ms)\n",
             avg, f->worst_ms, f->js_ms / nf, f->present_ms / nf, f->timers_ms / nf, f->events_ms / nf,
             f->idle_ms / nf, f->wall_ms / nf - known);
      // el peor frame, por partes: raf (el JS del frame), state (video, audio,
      // simbolos, trucos, logros), present (el del frame anterior), timers y
      // events (entre frames), idle (esperando) y el resto
      const double *w = f->worst_parts;
      double wsum = w[0] + w[1] + w[2] + w[3] + w[4] + w[5];
      printf("      worst %.0f ms = raf %.1f, state %.1f, present %.1f, timers %.1f, events %.1f, idle %.1f, rest %.1f\n",
             f->worst_ms, w[0], w[1], w[2], w[3], w[4], w[5], f->worst_ms - wsum);
      f->js_ms = f->present_ms = f->timers_ms = f->idle_ms = f->events_ms = f->wall_ms = f->worst_ms = 0;
      f->total_window = 0;
      last_print = now;
    }
  } else if (f->value == 0 && f->total > 1 && now > f->first) {
    // todavia sin medio segundo: la media hasta ahora
    f->value = (f->total - 1) * 1000.0 / (now - f->first);
  }
}

// El texto del contador para el backend (lo dibuja arriba a la derecha)
static void fps_overlay(FpsMeter *f, Backend *backend) {
  if (f->value > 0) snprintf(backend->overlay, sizeof(backend->overlay), "%.1f fps", f->value);
  else snprintf(backend->overlay, sizeof(backend->overlay), "-- fps");
}
