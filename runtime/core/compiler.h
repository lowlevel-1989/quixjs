/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

#include "js/js.h"
#include "config_loader.h"

// Los scripts del juego ([scripts] libs, includes y entry del toml del
// juego), en orden: su codigo fuente, listo para ejecutar. El nombre
// "compiler" viene de la primera version, con QuickJS, que guardaba el
// bytecode compilado; JavaScriptCore compila por su cuenta y no hay nada que
// guardar.
typedef struct {
  char **sources;     // codigo fuente (terminado en 0)
  size_t *sizes;
  char **filenames;   // como en el toml del juego
  char **paths;       // la ruta del archivo (sale en los errores y el profiler)
  int count;
} ScriptBundle;

// Lee los scripts y comprueba su sintaxis (los errores salen ya aqui)
int compile_scripts(JSContext *ctx, const Config *cfg, ScriptBundle *bundle);
void free_script_bundle(ScriptBundle *bundle);

// Lee y ejecuta un script del juego, como un <script src> anadido despues.
// 0 bien, 1 error de sintaxis (ya escrito; el navegador da load igual),
// -1: no existe, -2: excepcion al ejecutarlo (queda pendiente en ctx).
int compiler_run_file(JSContext *ctx, const Config *cfg, const char *rel_path);
