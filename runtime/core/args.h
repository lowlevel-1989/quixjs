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

#pragma once

// Las opciones de la linea de comandos de quixjs.

#define BACKEND_DUMMY 1
#define BACKEND_SDL   2

typedef struct {
  const char *config_path;
  int show_help;
  int show_version;         // --version: la version y sale
  int backend;
  long max_frames;         // 0 = sin limite
  int fixed_step;          // reloj fijo (1 frame = 1000/60 ms)
  int test;                // modo test: dummy + reloj fijo + quix.exit() obligatorio
  const char *screenshot;  // PNG del ultimo frame al salir
  int hold_ms;             // --hold: con ventana, espera al terminar para ver el ultimo frame
  int fullscreen;          // ventana a pantalla completa (con barras)
  int resizable;           // ventana redimensionable (con barras)
  const char *script;      // --script: guion de entrada (teclas, capturas)
  const char *debug_sock;  // --debug: consola en este socket Unix
  int show_fps;            // --fps: contador en pantalla y en la salida
  int frame_rate;          // --frame-rate=N: el tope de fps, por encima del toml (para probar)
  int expose_gc;           // --expose-gc: gc() global, como en V8 (para benchmarks)
  int no_webgl;            // --no-webgl: getContext("webgl") da null
  int launcher;            // --launcher: el launcher, un menu antes del juego
  const char *profile;     // --profile[=archivo]: profiler desde el arranque, informe en el archivo
  const char *gen_config_dir;   // --gen-config: escribe un toml para el juego y sale
} Args;

int parse_args(int argc, char **argv, Args *args);
void print_help();
