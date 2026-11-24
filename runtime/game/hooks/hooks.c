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

// runtime/game/hooks/hooks.c: los hooks de [hooks] (ver hooks.h). Este
// archivo tiene la lista y lo comun a las partes: crear un hook, olvidar los
// de la carga anterior, leer un archivo del port y comprobar un nombre. Las
// demas partes: define.c, mods.c, api.c e install.c; lo que comparten, en
// common.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/sandbox.h"
#include "common.h"

Hook        hooks_list[MAX_HOOKS];
int         hooks_total = 0;
int         hooks_installed = 0;
ModScript   hooks_mod_scripts[MAX_MOD_SCRIPTS];
int         hooks_mod_script_count = 0;
const char *hooks_kind_names[] = { "gpu-canvas", "skip", "once-per-frame" };

// la ruta de la funcion: solo nombres y puntos (va dentro del JS que la pone)
int hooks_path_ok(const char *key) {
  if (key[0] == 0 || strlen(key) >= sizeof(hooks_list[0].path)) return 0;
  for (const char *p = key; *p; p++) {
    if (!(*p == '.' || *p == '_' || *p == '$' || (*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') ||
          (*p >= 'A' && *p <= 'Z'))) {
      return 0;
    }
  }
  return 1;
}

// un sitio nuevo en la lista para la ruta key, o NULL si no caben mas
Hook *hooks_new_hook(const char *key) {
  if (hooks_total >= MAX_HOOKS) {
    fprintf(stderr, "[hooks] %s: more than %d hooks, left out\n", key, MAX_HOOKS);
    return NULL;
  }
  Hook *h = &hooks_list[hooks_total++];
  memset(h, 0, sizeof *h);
  snprintf(h->path, sizeof(h->path), "%s", key);
  return h;
}

// lo de los hooks de los mods de la carga anterior
void hooks_forget(void) {
  for (int i = 0; i < hooks_total; i++) {
    free(hooks_list[i].mod_source);
    hooks_list[i].mod_source = NULL;
  }
  hooks_total = hooks_installed = 0;
  for (int i = 0; i < hooks_mod_script_count; i++) free(hooks_mod_scripts[i].source);
  hooks_mod_script_count = 0;
}

// el archivo entero (malloc), o NULL
char *hooks_read_file(const char *path) {
  // un enlace simbolico del mod que apunta fuera del port tampoco vale
  if (!sandbox_check(path, "mod script")) return NULL;
  FILE *f = sandbox_fopen(path, "rb", "mod script");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *text = size >= 0 ? malloc((size_t)size + 1) : NULL;
  if (text && fread(text, 1, (size_t)size, f) != (size_t)size) {
    free(text);
    text = NULL;
  }
  if (text) text[size] = 0;
  fclose(f);
  return text;
}

// un nombre que va dentro del JS que pone el hook: letras, numeros, . _ - y /
// (sin "..")
int hooks_safe_name(const char *s) {
  if (!s[0] || strstr(s, "..")) return 0;
  for (; *s; s++) {
    if (!(*s == '.' || *s == '_' || *s == '-' || *s == '/' || (*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z') ||
          (*s >= 'A' && *s <= 'Z'))) {
      return 0;
    }
  }
  return 1;
}
