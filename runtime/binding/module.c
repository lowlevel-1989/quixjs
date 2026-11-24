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

// <script type="module">. Ver module.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/scripts/load.h"
#include "module.h"

#define MAX_MODULES 256

// los modulos ya ejecutados (su ruta dentro de game_dir): un import repetido
// no vuelve a ejecutarlo
static char done[MAX_MODULES][256];
static int  done_count;

static int already(const char *rel) {
  for (int i = 0; i < done_count; i++)
    if (strcmp(done[i], rel) == 0) return 1;
  if (done_count < MAX_MODULES) snprintf(done[done_count++], sizeof done[0], "%s", rel);
  return 0;
}

// spec ("./x.js", "../y.js") relativo a la carpeta del modulo rel, sin "." ni
// ".." (dentro de game_dir; lo que intente salir se queda en la raiz)
static void join(const char *rel, const char *spec, char *out, size_t room) {
  char tmp[512];
  const char *slash = strrchr(rel, '/');
  if (spec[0] == '/') snprintf(tmp, sizeof tmp, "%s", spec + 1);
  else snprintf(tmp, sizeof tmp, "%.*s%s", slash ? (int)(slash - rel + 1) : 0, rel, spec);
  size_t n = 0;
  out[0] = 0;
  char *save = NULL;
  for (char *part = strtok_r(tmp, "/", &save); part; part = strtok_r(NULL, "/", &save)) {
    if (strcmp(part, ".") == 0 || !part[0]) continue;
    if (strcmp(part, "..") == 0) {
      char *s = strrchr(out, '/');
      if (s) *s = 0;
      else out[0] = 0;
      n = strlen(out);
      continue;
    }
    n += (size_t)snprintf(out + n, n < room ? room - n : 0, "%s%s", n ? "/" : "", part);
  }
}

// salta espacios, comentarios y "use strict"; al principio del texto
static const char *skip(const char *p) {
  for (;;) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ';') p++;
    if (p[0] == '/' && p[1] == '/') {
      while (*p && *p != '\n') p++;
    } else if (p[0] == '/' && p[1] == '*') {
      const char *e = strstr(p + 2, "*/");
      p = e ? e + 2 : p + strlen(p);
    } else if (strncmp(p, "\"use strict\"", 12) == 0 || strncmp(p, "'use strict'", 12) == 0) {
      p += 12;
    } else {
      return p;
    }
  }
}

// "export {};" (un export vacio: solo marca el archivo como modulo, lo que
// hace Construct 3 en sus plugins), en blanco. Solo al empezar una sentencia
static void blank_empty_exports(char *s) {
  for (char *p = strstr(s, "export"); p; p = strstr(p + 6, "export")) {
    char before = p > s ? p[-1] : '\n';
    if (before != '\n' && before != ';' && before != '}' && before != ' ' && before != '\t' && before != '\r') continue;
    char *q = p + 6;
    while (*q == ' ' || *q == '\t') q++;
    if (*q != '{') continue;
    q++;
    while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') q++;
    if (*q != '}') continue;
    q++;
    if (*q == ';') q++;
    memset(p, ' ', (size_t)(q - p));
  }
}

int module_run_file(JSContext *ctx, const Config *cfg, const char *rel) {
  if (already(rel)) return 0;
  char *src = NULL, *path = NULL;
  size_t len = 0;
  if (scripts_read_file(cfg->game.game_dir, rel, &src, &len, &path) != 0) return -1;
  blank_empty_exports(src);

  // los import "x"; del principio: antes, en orden
  const char *p = skip(src);
  int r = 0;
  while (r == 0 && strncmp(p, "import", 6) == 0) {
    const char *q = p + 6;
    while (*q == ' ' || *q == '\t') q++;
    if (*q != '"' && *q != '\'') break;   // import con nombres o import(): no es de efecto
    char quote = *q++;
    const char *end = strchr(q, quote);
    if (!end) break;
    char spec[256], dep[512];
    snprintf(spec, sizeof spec, "%.*s", (int)(end - q), q);
    join(rel, spec, dep, sizeof dep);
    int d = module_run_file(ctx, cfg, dep);
    if (d == -1) fprintf(stderr, "[module] %s: cannot load %s\n", rel, dep);
    if (d == -2) r = -2;   // la excepcion queda pendiente
    p = skip(end + 1);
  }

  // el resto, en su propia funcion: sus nombres no salen, y "use strict" va
  // dentro (un modulo siempre es estricto)
  if (r == 0) {
    size_t body = strlen(p);
    static const char head[] = "(function () {\"use strict\";\n", tail[] = "\n}).call(undefined);";
    char *code = malloc(sizeof head + body + sizeof tail);
    if (!code) {
      free(src);
      free(path);
      return -1;
    }
    memcpy(code, head, sizeof head - 1);
    memcpy(code + sizeof head - 1, p, body);
    memcpy(code + sizeof head - 1 + body, tail, sizeof tail);
    size_t n = sizeof head - 1 + body + sizeof tail - 1;
    if (JS_CheckSyntax(ctx, code, n, path) != 0) {
      fprintf(stderr, "Error compiling %s:\n", path);
      js_std_dump_error(ctx);
      r = 1;
    } else {
      JSValue v = JS_Eval(ctx, code, n, path, JS_EVAL_TYPE_GLOBAL);
      if (JS_IsException(v)) r = -2;
      JS_FreeValue(ctx, v);
    }
    free(code);
  }
  free(src);
  free(path);
  return r;
}
