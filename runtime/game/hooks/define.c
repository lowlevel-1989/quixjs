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

// runtime/game/hooks/define.c: leer [hooks] del toml del juego (hooks_load):
// "gpu-canvas", "skip", "once-per-frame", un hook de motor ("motor/categoria/nombre"),
// uno del port ("hooks/nombre", su .js junto al toml), "off", y engine =
// "<motor>" con los hooks de ese motor (los del juego mandan).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tomlc17.h"
#include "common.h"

// Un hook del port: "hooks/<nombre>" es games/<port>/hooks/<nombre>.js, junto
// al toml (dir), como el de un mod. 1 si lo era (y lo agrego), 0 si no
// es de esos, -1 si lo era y no se pudo
static int add_port_hook(const char *key, const char *value, const char *dir) {
  if (!dir || strncmp(value, "hooks/", 6) != 0) return 0;
  char file[1024];
  if (!hooks_safe_name(value) || !hooks_path_ok(key)) {
    fprintf(stderr, "[hooks] %s: \"%s\": expected a hook of the port, \"hooks/<name>\" (hooks/<name>.js next to the toml)\n", key, value);
    return -1;
  }
  snprintf(file, sizeof file, "%s/%s.js", dir, value);
  char *source = hooks_read_file(file);
  if (!source) {
    fprintf(stderr, "[hooks] %s: cannot read %s\n", key, file);
    return -1;
  }
  Hook *h = hooks_new_hook(key);
  if (!h) {
    free(source);
    return -1;
  }
  snprintf(h->mod_name, sizeof h->mod_name, "%s", value);
  h->mod_source = source;
  h->kind = HOOK_ENGINE;
  h->engine_name = h->mod_name;
  h->engine_source = h->mod_source;
  return 1;
}

// Agrega un hook (la ruta del juego y lo que se pone). 0 si vale, -1 si no.
// dir: la carpeta del toml del juego (sus hooks/), o NULL (los del motor)
static int add_hook(const char *key, toml_datum_t v, const char *dir) {
  if (v.type == TOML_STRING) {
    int r = add_port_hook(key, v.u.s, dir);
    if (r) return r > 0 ? 0 : -1;
  }
  int kind = -1;
  const char *engine_name = NULL, *engine_source = NULL;
  for (int k = 0; k < 3 && v.type == TOML_STRING; k++) {
    if (strcmp(v.u.s, hooks_kind_names[k]) == 0) kind = k;
  }
  // "motor/hook": uno de runtime/engines/
  if (v.type == TOML_STRING && kind < 0 && hooks_engine_find(v.u.s, &engine_name, &engine_source)) kind = HOOK_ENGINE;
  if (kind < 0 || !hooks_path_ok(key)) {
    fprintf(stderr, "[hooks] %s: expected \"gpu-canvas\", \"skip\", \"once-per-frame\", \"off\", a hook of the port (\"hooks/<name>\") or an engine hook (%s...)\n",
            key, hooks_engine_example());
    return -1;
  }
  Hook *h = hooks_new_hook(key);
  if (!h) return -1;
  h->kind = (HookKind)kind;
  h->engine_name = engine_name;
  h->engine_source = engine_source;
  return 0;
}

// lo que dice el toml del juego para una ruta (o un datum vacio)
static toml_datum_t own_value(toml_datum_t t, const char *key) {
  toml_datum_t none = {0};
  if (t.type != TOML_TABLE) return none;
  for (int i = 0; i < t.u.tab.size; i++) {
    if (strcmp(t.u.tab.key[i], key) == 0) return t.u.tab.value[i];
  }
  return none;
}

static int is_off(toml_datum_t v) {
  return v.type == TOML_STRING && strcmp(v.u.s, "off") == 0;
}

int hooks_load(const char *config_path) {
  hooks_forget();
  // la carpeta del toml: sus hooks/ (los hooks del port)
  char dir[1024];
  const char *slash = strrchr(config_path, '/');
  if (slash) snprintf(dir, sizeof dir, "%.*s", (int)(slash - config_path), config_path);
  else snprintf(dir, sizeof dir, ".");
  FILE *f = fopen(config_path, "r");
  if (!f) return 0;
  toml_result_t result = toml_parse_file(f);
  fclose(f);
  if (!result.ok) return 0;
  int bad = 0;
  toml_datum_t t = toml_get(result.toptab, "hooks");
  // engine = "rpgmaker-mv": los hooks del motor (runtime/engines/<motor>/
  // hooks.toml). Los que el juego pone en su [hooks] mandan: otro hook en la
  // misma ruta, o "off" para no usarlo
  toml_datum_t engine = own_value(t, "engine");
  if (engine.type == TOML_STRING) {
    const char *preset = hooks_engine_preset(engine.u.s);
    if (!preset) {
      fprintf(stderr, "[hooks] engine = \"%s\": no hooks for that engine\n", engine.u.s);
      bad = 1;
    } else {
      toml_result_t pr = toml_parse(preset, (int)strlen(preset));
      toml_datum_t pt = pr.ok ? toml_get(pr.toptab, "hooks") : (toml_datum_t){0};
      for (int i = 0; pt.type == TOML_TABLE && i < pt.u.tab.size; i++) {
        const char *key = pt.u.tab.key[i];
        if (own_value(t, key).type != TOML_UNKNOWN) continue;   // el juego manda
        if (add_hook(key, pt.u.tab.value[i], NULL) != 0) bad = 1;
      }
      if (pr.ok) toml_free(pr);
    }
  }
  if (t.type == TOML_TABLE) {
    for (int i = 0; i < t.u.tab.size; i++) {
      const char *key = t.u.tab.key[i];
      toml_datum_t v = t.u.tab.value[i];
      if (strcmp(key, "engine") == 0 || is_off(v)) continue;
      if (add_hook(key, v, dir) != 0) bad = 1;
    }
  }
  toml_free(result);
  if (hooks_total) printf("Hooks: %d\n", hooks_total);
  return bad ? -1 : hooks_total;
}
