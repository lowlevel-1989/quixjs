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

// runtime/game/hooks/common.h: lo que comparten las partes de los hooks, y
// nadie mas (el resto de quixjs usa hooks.h):
//   hooks.c    la lista y lo comun: crear un hook, olvidarlos, leer un
//              archivo del port y comprobar un nombre
//   define.c   leer [hooks] del toml (hooks_load)
//   mods.c     los hooks y los scripts de los mods
//   api.c      la API de JS y los hooks de motor embebidos (engines.h: solo
//              api.c lo incluye, para no tener sus tablas dos veces)
//   install.c  poner los hooks en el juego y volver a ponerlos si los pisa
#pragma once

#include "js/js.h"
#include "hooks.h"

#define MAX_HOOKS 128

typedef enum { HOOK_GPU_CANVAS, HOOK_SKIP, HOOK_ONCE_PER_FRAME, HOOK_ENGINE } HookKind;

typedef struct {
  char        path[256];
  HookKind    kind;
  const char *engine_name;     // HOOK_ENGINE: "rpgmaker-mv/logic/request-images-once"
  const char *engine_source;   // la funcion que recibe la original
  int         installed;
  // el hook de un mod (hooks_load_mods): su nombre ("rumble/rumble.js") y su
  // codigo, leido del archivo (engine_name y engine_source apuntan aqui). Va
  // encima de otro hook de la misma ruta
  char        mod_name[128];
  char       *mod_source;
} Hook;

// los scripts de los mods (scripts = [...] de su hooks.toml): corren una vez,
// en orden, antes de poner sus hooks (lo que comparten los archivos de un mod)
#define MAX_MOD_SCRIPTS 32
typedef struct {
  char  name[128];   // "multiplayer/net.js"
  char *source;
  int   ran;
} ModScript;

extern Hook        hooks_list[MAX_HOOKS];
extern int         hooks_total;
extern int         hooks_installed;     // cuantos de la lista ya estan puestos
extern ModScript   hooks_mod_scripts[MAX_MOD_SCRIPTS];
extern int         hooks_mod_script_count;
extern const char *hooks_kind_names[];  // "gpu-canvas", "skip", "once-per-frame"

// hooks.c
int   hooks_path_ok(const char *key);   // la ruta de una funcion: nombres y puntos
Hook *hooks_new_hook(const char *key);  // un sitio nuevo en la lista, o NULL
void  hooks_forget(void);               // lo de la carga anterior
char *hooks_read_file(const char *path);   // el archivo entero (malloc), o NULL
int   hooks_safe_name(const char *s);   // letras, numeros, . _ - y / (sin "..")

// api.c: los hooks de motor embebidos (runtime/engines/)
// el hook "motor/nombre": 1 y su nombre y su codigo, o 0 si no existe
int         hooks_engine_find(const char *name, const char **out_name, const char **out_source);
const char *hooks_engine_preset(const char *engine);   // su hooks.toml, o NULL
const char *hooks_engine_example(void);                // un nombre, para los avisos

// mods.c
void hooks_run_mod_scripts(JSContext *ctx);
