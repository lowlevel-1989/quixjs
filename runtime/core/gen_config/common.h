// gen_config/common.h: lo que comparten las partes de gen_config.c: lo que
// se sabe del juego (GameInfo) y las rutas y archivos.

#pragma once

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include "js/js.h"
#include "core/config_loader.h"
#include "core/url.h"
#include "core/gen_config.h"

#define MAX_ITEMS 128

// un elemento con id del <body> del index.html ([html] elements)
typedef struct {
  char tag[16];
  char id[64];
  char type[16];
} BodyElement;

typedef struct {
  char base[1024];      // carpeta del index.html (ruta tal cual para el toml)
  char html[1024];      // ruta del index.html
  int  nwjs;
  int  width, height;
  char size_from[64];
  char engine[32];
  char rm_version[32];
  char pixi[32];
  char *scripts[MAX_ITEMS];
  int  script_count;
  char *font_family[MAX_ITEMS];
  char *font_path[MAX_ITEMS];
  int  font_count;
  char title[128];
  char canvas_id[64];   // el id del <canvas> del index.html ("" = ninguno)
  BodyElement elements[32];   // los elementos con id del <body>
  int  el_count;
  int  greenworks;      // trae greenworks.js (logros de Steam en nw.js)
} GameInfo;

static int file_exists(const char *p) {
  struct stat st;
  return stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

static char *read_text(const char *path) {
  size_t len;
  return (char *)url_read_file(path, &len);
}

static void join(char *out, size_t size, const char *a, const char *b) {
  if (!a || !*a || strcmp(a, ".") == 0) snprintf(out, size, "%s", b);
  else if (!b || !*b) snprintf(out, size, "%s", a);
  else snprintf(out, size, "%s/%s", a, b);
}

static void dir_of(char *out, size_t size, const char *path) {
  snprintf(out, size, "%s", path);
  char *slash = strrchr(out, '/');
  if (slash) *slash = 0;
  else snprintf(out, size, ".");
}

// "./a/../b/c" -> "b/c" (sin salir por encima del principio)
static void normalize(char *p) {
  char *parts[256];
  int n = 0;
  char tmp[1024];
  snprintf(tmp, sizeof(tmp), "%s", p);
  int absolute = tmp[0] == '/';
  char *save = NULL;
  for (char *t = strtok_r(tmp, "/", &save); t && n < 256; t = strtok_r(NULL, "/", &save)) {
    if (strcmp(t, ".") == 0 || !*t) continue;
    if (strcmp(t, "..") == 0 && n > 0 && strcmp(parts[n - 1], "..") != 0) { n--; continue; }
    parts[n++] = t;
  }
  char out[1024] = "";
  size_t o = 0;
  if (absolute) out[o++] = '/';
  for (int i = 0; i < n; i++) {
    int w = snprintf(out + o, sizeof(out) - o, "%s%s", i ? "/" : "", parts[i]);
    if (w < 0 || (size_t)w >= sizeof(out) - o) break;
    o += w;
  }
  out[o] = 0;
  strcpy(p, out[0] ? out : ".");
}
