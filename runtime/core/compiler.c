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

// Lee los scripts del juego y comprueba su sintaxis. Ver compiler.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "js/js.h"
#include "config_loader.h"
#include "compiler.h"
#include "url.h"
#include "backend/backend.h"

// El archivo entero, terminado en 0 (NULL si no existe)
static char *read_file(const char *path, size_t *plen) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = malloc(len > 0 ? len + 1 : 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  size_t n = len > 0 ? fread(buf, 1, len, f) : 0;
  buf[n] = 0;
  fclose(f);
  *plen = n;
  return buf;
}

// Lee rel_path (de game_dir) y comprueba su sintaxis: 0 bien, -1 no se puede
// leer, -2 error de sintaxis (ya escrito). Con url_resolve, como todo lo que
// carga el juego: si un mod activo trae el script, el del mod
static int load_script(JSContext *ctx, const char *game_dir, const char *rel_path, char **out_src, size_t *out_len,
                       char **out_path) {
  char path[1024];
  size_t len = 0;
  char *src;
  if (strncmp(rel_path, "data:", 5) == 0) {
    // un <script> escrito dentro del HTML (--gen-config lo guarda asi)
    src = (char *)url_decode_data(rel_path, &len);
    snprintf(path, sizeof(path), "inline script");
    printf("Loading: inline script (%zu bytes)\n", len);
  } else {
    url_resolve(game_dir, rel_path, path, sizeof(path));
    printf("Loading: %s\n", path);
    src = read_file(path, &len);
  }
  if (!src) {
    fprintf(stderr, "Error: cannot read %s\n", path);
    return -1;
  }
  if (JS_CheckSyntax(ctx, src, len, path) != 0) {
    fprintf(stderr, "Error compiling %s:\n", path);
    js_std_dump_error(ctx);
    free(src);
    return -2;
  }
  *out_src = src;
  *out_len = len;
  *out_path = strdup(path);
  return 0;
}

int compile_scripts(JSContext *ctx, const Config *cfg, ScriptBundle *bundle) {
  int has_entry = cfg->scripts.entry && cfg->scripts.entry[0];
  int total = cfg->scripts.libs_count + cfg->scripts.includes_count + (has_entry ? 1 : 0);
  bundle->sources = calloc(total ? total : 1, sizeof(char *));
  bundle->sizes = calloc(total ? total : 1, sizeof(size_t));
  bundle->filenames = calloc(total ? total : 1, sizeof(char *));
  bundle->paths = calloc(total ? total : 1, sizeof(char *));
  if (!bundle->sources || !bundle->sizes || !bundle->filenames || !bundle->paths) {
    free_script_bundle(bundle);
    return -1;
  }
  bundle->count = total;
  const char *list[total ? total : 1];
  int n = 0;
  for (int i = 0; i < cfg->scripts.libs_count; i++) list[n++] = cfg->scripts.libs[i];
  for (int i = 0; i < cfg->scripts.includes_count; i++) list[n++] = cfg->scripts.includes[i];
  if (has_entry) list[n++] = cfg->scripts.entry;
  for (int i = 0; i < total; i++) {
    bundle->filenames[i] = strdup(list[i]);
    backend_loading_poke();   // el aviso de carga, si se ve (Play en el launcher)
    if (load_script(ctx, cfg->game.game_dir, list[i], &bundle->sources[i], &bundle->sizes[i], &bundle->paths[i]) != 0) {
      fprintf(stderr, "Failed to load %s\n", list[i]);
    }
  }
  return 0;
}

void free_script_bundle(ScriptBundle *bundle) {
  if (!bundle) return;
  for (int i = 0; i < bundle->count; i++) {
    if (bundle->sources) free(bundle->sources[i]);
    if (bundle->filenames) free(bundle->filenames[i]);
    if (bundle->paths) free(bundle->paths[i]);
  }
  free(bundle->sources);
  free(bundle->sizes);
  free(bundle->filenames);
  free(bundle->paths);
  memset(bundle, 0, sizeof(*bundle));
}

int compiler_run_file(JSContext *ctx, const Config *cfg, const char *rel_path) {
  char *src = NULL, *path = NULL;
  size_t len = 0;
  int r = load_script(ctx, cfg->game.game_dir, rel_path, &src, &len, &path);
  // con un error de sintaxis el navegador da load igual (el error va a la
  // consola, ya escrito): RPG Maker solo se para si el archivo no carga
  if (r == -2) return 1;
  if (r != 0) return -1;
  JSValue val = JS_Eval(ctx, src, len, path, JS_EVAL_TYPE_GLOBAL);
  free(src);
  free(path);
  if (JS_IsException(val)) return -2;
  JS_FreeValue(ctx, val);
  return 0;
}
