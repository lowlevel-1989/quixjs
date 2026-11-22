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

// El programa: lee las opciones y el toml del juego, crea el backend y el
// motor de JS, carga los scripts y corre el loop principal (eventos,
// timers, requestAnimationFrame, presentar y esperar al siguiente frame).
//
// main.c es un archivo indice: main() esta aqui y el resto en main/, partido
// por temas para que se pueda leer. Las partes se incluyen en orden y todo se
// compila como un solo archivo: el compilador ve lo mismo que cuando era uno,
// y los pasos (STEP, always_inline) quedan dentro de main(), que vuelve a
// ser una sola funcion: sin llamadas de mas en cada frame. Ver main/README.md.

#include "main/common.h"
#include "main/input.c"
#include "main/clock.c"
#include "main/fonts.c"
#include "main/stats.c"
#include "main/screen.c"
#include "main/run.h"
#include "main/start.c"
#include "main/screenshot.c"
#include "main/loop.c"
#include "main/frame.c"
#include "main/finish.c"

int main(int argc, char **argv) {
  // salida por lineas: los mensajes y los errores salen en orden en los logs
  setvbuf(stdout, NULL, _IOLBF, 0);

  Run run = { 0 };
  Loop loop = { 0 };
  Run *r = &run;
  Loop *l = &loop;
  int code;
  if (info_option(argc, argv, &r->args, &code)) return code;   // --help, --version, --gen-config

  if (!start_window(r, l)) return 1;            // el toml, el backend y la ventana
  if (!load_port(r, l)) return 1;               // partidas, logros, simbolos, trucos, hooks, --script
  if (!launcher(r, l, &code)) return code;      // el menu (--launcher): jugar o salir
  if (!start_js(r, l)) return 1;                // mods, motor de JS, APIs, fuentes, scripts del juego
  start_tools(r, l);                            // --fps, --profile, --debug, el tope de fps

  // Loop principal: una vuelta por frame (o por timer)
  while (!l->failed) {
    double t_events = r->fps.enabled ? real_ms() : 0;
    watchdog_alive();
    housekeeping(r, l);                         // memoria y profiler, cada x segundos
    int step = events(r, l, t_events);          // backend, consola de debug, --script
    if (step == STEP_STOP) break;
    if (step == STEP_AGAIN) continue;
    run_timers(r, l);                           // entrada, promesas, timers

    l->now = clock_now_ms();
    if (l->err == 0 && l->now >= l->next_frame) {
      game_frame(r, l);                         // requestAnimationFrame, video, audio, logros
      pace_frame(r, l);                         // a que refresco va esta imagen
      present_frame(r, l);                      // mostrarla y cuando toca la siguiente
      if (frame_limit(r, l)) break;             // --frames
    }

    // En un navegador un error en un callback no para la pagina; en los
    // tests si (para que el fallo se vea enseguida)
    if (l->err != 0 && r->args.test) {
      l->failed = 1;
      break;
    }
    if (r->app.exit_requested) break;
    if (backend_should_close(l->backend)) break;
    if (wait_next(r, l) == STEP_STOP) break;
  }

  // Select + R2 con --launcher: de vuelta al launcher
  int back = r->args.launcher && l->backend->back_to_menu && !r->app.exit_requested;
  // la ventana nueva, como estaba esta (runtime/core/main/start.c)
  if (back && l->backend->fullscreen) setenv("QUIXJS_FULLSCREEN", "1", 1);
  code = finish(r, l);                          // --hold, --screenshot, informes, liberar todo
  if (back) relaunch(argv);
  return code;
}
