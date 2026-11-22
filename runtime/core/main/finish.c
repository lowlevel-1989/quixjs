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

// Al terminar: --hold, --screenshot, los informes y liberar todo.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

// Al terminar: --hold, --screenshot, los informes y liberar todo. Devuelve
// el codigo de salida
STEP int finish(Run *r, Loop *l) {
  JSContext *ctx = l->ctx;
  Backend *backend = l->backend;
  FpsMeter *fps = &r->fps;
  // --hold: la ventana se queda abierta mostrando el ultimo frame
  if (r->args.hold_ms > 0 && r->args.backend == BACKEND_SDL) {
    if (fps->enabled) {
      // en la pausa final, la media de lo que corrio el juego
      if (fps->total >= 2 && fps->last > fps->first) fps->value = (fps->total - 1) * 1000.0 / (fps->last - fps->first);
      fps_overlay(fps, backend);
    }
    double until = clock_now_ms() + r->args.hold_ms;
    while (!backend_should_close(backend) && (clock_is_fixed() || clock_now_ms() < until)) {
      backend_process_events(backend);
      present_screen(ctx, backend, &r->scratch);
      if (clock_is_fixed()) clock_advance_to(clock_now_ms() + FRAME_MS);
      usleep(16000);
      if (clock_is_fixed() && clock_now_ms() >= until) break;
    }
  }

  if (r->args.screenshot) {
    const Surface *screen = compose_screen(ctx, &r->scratch);
    if (screen && screen->px) {
      if (image_save_png(screen->px, screen->w, screen->h, r->args.screenshot) == 0) {
        printf("Screenshot saved: %s\n", r->args.screenshot);
      } else {
        fprintf(stderr, "Failed to save screenshot: %s\n", r->args.screenshot);
      }
    } else {
      fprintf(stderr, "No canvas on screen: screenshot not saved\n");
    }
  }
  surface_free(&r->scratch);
  if (fps->enabled && fps->total >= 10) {
    // hasta el ultimo frame del juego (sin --hold ni el cierre)
    double secs = (fps->last - fps->first) / 1000.0;
    // sin ventana y con reloj fijo: el maximo que quixjs puede dibujar
    if (secs > 0) printf("FPS: %.1f (average of %ld frames)\n", (fps->total - 1) / secs, fps->total);
  }
  input_script_free();
  perf_print("whole session", &perf_all, real_ms());
  if (r->args.profile && profiler_running()) {
    profiler_save(ctx, r->args.profile, 60);
    printf("Profile: %s\n", r->args.profile);
  }
  profiler_stop();
  debug_shutdown();

  int exit_code = 0;
  if (l->failed) {
    exit_code = 1;
  } else if (r->app.exit_requested) {
    exit_code = r->app.exit_code;
  } else if (r->args.test) {
    fprintf(stderr, "TEST TIMEOUT: quix.exit() was not called after %ld frames\n", r->app.frame);
    exit_code = EXIT_TEST_TIMEOUT;
  }

  free_script_bundle(&r->bundle);
  timer_free(ctx);
  video_free_all();
  mixer_shutdown();
  audio_free(ctx);
  script_free(ctx);
  free_element_registry(ctx);
  events_free(ctx);

  JS_FreeContext(ctx);
  JS_FreeRuntime(l->rt);

  blob_free_all();
  gl_shutdown();
  font_quit();
  backend_cleanup(backend);
  free(backend);
  return exit_code;
}

// Select + R2 en un juego abierto desde el launcher: de vuelta al launcher.
// quixjs se ejecuta otra vez con las mismas opciones (el juego ya se cerro y
// guardo como al salir). Solo vuelve si falla
static void relaunch(char **argv) {
#ifndef __ANDROID__
  printf("Back to the launcher\n");
  fflush(NULL);
  execv("/proc/self/exe", argv);
  perror("[launcher] cannot start again");
#else
  (void)argv;
#endif
}
