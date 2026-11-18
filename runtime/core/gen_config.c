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

// quixjs --gen-config: escribir un toml para un juego. Ver gen_config.h.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include "js/js.h"
#include "config_loader.h"
#include "url.h"
#include "gen_config.h"

#define MAX_ITEMS 128

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

// ------------------------------------------------------------- package.json

static JSValue parse_json_file(JSContext *ctx, const char *path) {
  char *text = read_text(path);
  if (!text) return JS_UNDEFINED;
  JSValue v = JS_ParseJSON(ctx, text, strlen(text), path);
  free(text);
  if (JS_IsException(v)) {
    JSValue e = JS_GetException(ctx);
    JS_FreeValue(ctx, e);
    return JS_UNDEFINED;
  }
  return v;
}

static int get_int(JSContext *ctx, JSValueConst obj, const char *a, const char *b) {
  JSValue x = JS_GetPropertyStr(ctx, obj, a);
  JSValue y = b ? JS_GetPropertyStr(ctx, x, b) : JS_DupValue(ctx, x);
  int r = 0;
  if (JS_IsNumber(y)) JS_ToInt32(ctx, &r, y);
  JS_FreeValue(ctx, x);
  JS_FreeValue(ctx, y);
  return r;
}

static void get_str(JSContext *ctx, JSValueConst obj, const char *a, const char *b, char *out, size_t size) {
  out[0] = 0;
  JSValue x = JS_GetPropertyStr(ctx, obj, a);
  JSValue y = b ? JS_GetPropertyStr(ctx, x, b) : JS_DupValue(ctx, x);
  if (JS_IsString(y)) {
    const char *s = JS_ToCString(ctx, y);
    if (s) {
      snprintf(out, size, "%s", s);
      JS_FreeCString(ctx, s);
    }
  }
  JS_FreeValue(ctx, x);
  JS_FreeValue(ctx, y);
}

// package.json de nw.js: { "main": "index.html", "window": { width, height } }
static int read_manifest(JSContext *ctx, const char *dir, GameInfo *gi) {
  char path[1024];
  join(path, sizeof(path), dir, "package.json");
  if (!file_exists(path)) return 0;
  JSValue pkg = parse_json_file(ctx, path);
  if (!JS_IsObject(pkg)) return 0;

  char main[512];
  get_str(ctx, pkg, "main", NULL, main, sizeof(main));
  size_t ml = strlen(main);
  int ok = ml > 5 && strcasecmp(main + ml - 5, ".html") == 0;
  if (ok) {
    join(gi->html, sizeof(gi->html), dir, main);
    normalize(gi->html);
    gi->nwjs = 1;
    int w = get_int(ctx, pkg, "window", "width");
    int h = get_int(ctx, pkg, "window", "height");
    if (w > 0 && h > 0) {
      gi->width = w;
      gi->height = h;
      snprintf(gi->size_from, sizeof(gi->size_from), "package.json");
    }
    get_str(ctx, pkg, "window", "title", gi->title, sizeof(gi->title));
  }
  JS_FreeValue(ctx, pkg);
  return ok;
}

// ------------------------------------------------------------------- html

// Valor de un atributo dentro de una etiqueta (sin distinguir mayusculas)
static int tag_attr(const char *tag, const char *tag_end, const char *name, char *out, size_t size) {
  size_t nl = strlen(name);
  for (const char *p = tag; p + nl < tag_end; p++) {
    if (strncasecmp(p, name, nl) != 0 || !isspace((unsigned char)p[-1])) continue;
    const char *q = p + nl;
    while (q < tag_end && isspace((unsigned char)*q)) q++;
    if (*q != '=') continue;
    q++;
    while (q < tag_end && isspace((unsigned char)*q)) q++;
    char quote = (*q == '"' || *q == '\'') ? *q++ : 0;
    const char *e = q;
    while (e < tag_end && (quote ? *e != quote : (!isspace((unsigned char)*e) && *e != '>'))) e++;
    size_t len = (size_t)(e - q);
    if (len >= size) len = size - 1;
    memcpy(out, q, len);
    out[len] = 0;
    return 1;
  }
  return 0;
}

static int is_remote(const char *u) {
  return strncmp(u, "http://", 7) == 0 || strncmp(u, "https://", 8) == 0 || strncmp(u, "//", 2) == 0;
}

static void read_css_fonts(const char *base, const char *css_rel, GameInfo *gi) {
  char path[1024], css_dir[1024];
  join(path, sizeof(path), base, css_rel);
  char *css = read_text(path);
  if (!css) return;
  dir_of(css_dir, sizeof(css_dir), css_rel);

  for (char *p = css; (p = strcasestr(p, "@font-face")) != NULL; p++) {
    char *open = strchr(p, '{');
    char *close = open ? strchr(open, '}') : NULL;
    if (!close) break;
    *close = 0;

    char family[128] = "", url[512] = "";
    char *f = strcasestr(open, "font-family");
    if (f && (f = strchr(f, ':'))) {
      f++;
      while (isspace((unsigned char)*f) || *f == '"' || *f == '\'') f++;
      size_t n = strcspn(f, ";\"'\n");
      if (n >= sizeof(family)) n = sizeof(family) - 1;
      memcpy(family, f, n);
      family[n] = 0;
      while (n > 0 && isspace((unsigned char)family[n - 1])) family[--n] = 0;
    }
    char *u = strcasestr(open, "url(");
    if (u) {
      u += 4;
      while (isspace((unsigned char)*u) || *u == '"' || *u == '\'') u++;
      size_t n = strcspn(u, "\"')");
      if (n >= sizeof(url)) n = sizeof(url) - 1;
      memcpy(url, u, n);
      url[n] = 0;
    }
    *close = '}';
    p = close;

    if (!family[0] || !url[0] || is_remote(url) || gi->font_count >= MAX_ITEMS) continue;
    char rel[1024];
    join(rel, sizeof(rel), strcmp(css_dir, ".") ? css_dir : "", url);
    normalize(rel);
    gi->font_family[gi->font_count] = strdup(family);
    gi->font_path[gi->font_count] = strdup(rel);
    gi->font_count++;
  }
  free(css);
}

static void read_html(GameInfo *gi) {
  char *html = read_text(gi->html);
  if (!html) return;

  for (char *p = html; (p = strcasestr(p, "<script")) != NULL; p += 7) {
    char *end = strchr(p, '>');
    if (!end) break;
    char src[512], type[64];
    if (tag_attr(p, end, "src", src, sizeof(src)) && src[0] && !is_remote(src) && gi->script_count < MAX_ITEMS) {
      char *q = strpbrk(src, "?#");
      if (q) *q = 0;
      normalize(src);
      gi->scripts[gi->script_count++] = strdup(src);
      continue;
    }
    // un <script> con el codigo dentro: una data: URL con ese codigo, en su
    // orden (solo JavaScript normal: ni modulos ni plantillas o JSON)
    if (tag_attr(p, end, "src", src, sizeof(src))) continue;
    if (tag_attr(p, end, "type", type, sizeof(type)) && type[0] && strcasecmp(type, "text/javascript") != 0 &&
        strcasecmp(type, "application/javascript") != 0) {
      continue;
    }
    char *close = strcasestr(end + 1, "</script");
    if (!close || gi->script_count >= MAX_ITEMS) continue;
    const char *code = end + 1;
    size_t n = (size_t)(close - code);
    int blank = 1;
    for (size_t i = 0; i < n && blank; i++) blank = isspace((unsigned char)code[i]) != 0;
    if (blank) continue;
    // "data:text/javascript," y el codigo con % como %25 (lo unico que
    // url_decode_data cambia)
    size_t pct = 0;
    for (size_t i = 0; i < n; i++) pct += code[i] == '%';
    char *url = malloc(n + 2 * pct + 32);
    if (!url) continue;
    size_t o = (size_t)sprintf(url, "data:text/javascript,");
    for (size_t i = 0; i < n; i++) {
      if (code[i] == '%') {
        memcpy(url + o, "%25", 3);
        o += 3;
      } else {
        url[o++] = code[i];
      }
    }
    url[o] = 0;
    gi->scripts[gi->script_count++] = url;
  }

  for (char *p = html; (p = strcasestr(p, "<link")) != NULL; p += 5) {
    char *end = strchr(p, '>');
    if (!end) break;
    char rel[64], href[512];
    if (tag_attr(p, end, "rel", rel, sizeof(rel)) && strcasecmp(rel, "stylesheet") == 0 &&
        tag_attr(p, end, "href", href, sizeof(href)) && !is_remote(href)) {
      normalize(href);
      read_css_fonts(gi->base, href, gi);
    }
  }

  if (!gi->title[0]) {
    char *t = strcasestr(html, "<title>");
    if (t) {
      t += 7;
      size_t n = strcspn(t, "<");
      if (n >= sizeof(gi->title)) n = sizeof(gi->title) - 1;
      memcpy(gi->title, t, n);
      gi->title[n] = 0;
    }
  }
  free(html);
}

// ------------------------------------------------------------ motor y pixi

// Primer texto entre comillas despues de "key"
static int quoted_after(const char *text, const char *key, char *out, size_t size) {
  const char *p = strstr(text, key);
  if (!p) return 0;
  p += strlen(key);
  while (*p && *p != '"' && *p != '\'' && *p != '\n') p++;
  if (*p != '"' && *p != '\'') return 0;
  char q = *p++;
  size_t n = strcspn(p, q == '"' ? "\"" : "'");
  if (n >= size) n = size - 1;
  memcpy(out, p, n);
  out[n] = 0;
  return 1;
}

static int number_after(const char *text, const char *key) {
  const char *p = strstr(text, key);
  if (!p) return 0;
  p += strlen(key);
  while (*p && !isdigit((unsigned char)*p) && *p != ';' && *p != '\n') p++;
  return atoi(p);
}

static void detect_engine(GameInfo *gi) {
  char path[1024];
  strcpy(gi->engine, "html5");

  join(path, sizeof(path), gi->base, "js/rmmz_core.js");
  char *core = read_text(path);
  if (core) {
    strcpy(gi->engine, "rpgmaker-mz");
    quoted_after(core, "Utils.RPGMAKER_VERSION", gi->rm_version, sizeof(gi->rm_version));
    free(core);
  } else {
    join(path, sizeof(path), gi->base, "js/rpg_core.js");
    core = read_text(path);
    if (core) {
      strcpy(gi->engine, "rpgmaker-mv");
      quoted_after(core, "Utils.RPGMAKER_VERSION", gi->rm_version, sizeof(gi->rm_version));
      free(core);
    }
  }

  // tamano por defecto del motor si no lo dice package.json
  if (!gi->width) {
    if (strcmp(gi->engine, "rpgmaker-mv") == 0) {
      join(path, sizeof(path), gi->base, "js/rpg_managers.js");
      char *m = read_text(path);
      if (m) {
        gi->width = number_after(m, "SceneManager._screenWidth");
        gi->height = number_after(m, "SceneManager._screenHeight");
        free(m);
        if (gi->width) snprintf(gi->size_from, sizeof(gi->size_from), "js/rpg_managers.js");
      }
    } else if (strcmp(gi->engine, "rpgmaker-mz") == 0) {
      join(path, sizeof(path), gi->base, "data/System.json");
      char *sys = read_text(path);
      if (sys) {
        gi->width = number_after(sys, "\"screenWidth\"");
        gi->height = number_after(sys, "\"screenHeight\"");
        free(sys);
        if (gi->width) snprintf(gi->size_from, sizeof(gi->size_from), "data/System.json");
      }
    }
  }
  if (gi->width <= 0 || gi->height <= 0) {
    gi->width = 640;
    gi->height = 480;
    snprintf(gi->size_from, sizeof(gi->size_from), "por defecto");
  }

  // version de PixiJS por la cabecera de su archivo
  for (int i = 0; i < gi->script_count; i++) {
    const char *name = strrchr(gi->scripts[i], '/');
    name = name ? name + 1 : gi->scripts[i];
    if (strncasecmp(name, "pixi", 4) != 0 || strcasestr(name, "tilemap") || strcasestr(name, "picture") ||
        strcasestr(name, "filter")) continue;
    join(path, sizeof(path), gi->base, gi->scripts[i]);
    FILE *f = fopen(path, "rb");
    if (!f) continue;
    char head[512] = "";
    size_t n = fread(head, 1, sizeof(head) - 1, f);
    head[n] = 0;
    fclose(f);
    char *v = strstr(head, " - v");
    if (v) {
      v += 4;
      size_t len = strspn(v, "0123456789.");
      if (len >= sizeof(gi->pixi)) len = sizeof(gi->pixi) - 1;
      memcpy(gi->pixi, v, len);
      gi->pixi[len] = 0;
      break;
    }
  }
}

// ------------------------------------------------------------------ salida

static void toml_str(FILE *out, const char *s) {
  fputc('"', out);
  for (; *s; s++) {
    // los saltos de linea y tabuladores como escapes de TOML (el codigo de
    // un <script> los necesita); el resto de controles, fuera
    if (*s == '\n') { fputs("\\n", out); continue; }
    if (*s == '\t') { fputs("\\t", out); continue; }
    if (*s == '\r') { fputs("\\r", out); continue; }
    if (*s == '"' || *s == '\\') fputc('\\', out);
    if ((unsigned char)*s < 0x20) continue;
    fputc(*s, out);
  }
  fputc('"', out);
}

static int is_lib(const char *p) {
  return strncmp(p, "libs/", 5) == 0 || strncmp(p, "lib/", 4) == 0 || strstr(p, "/libs/") || strstr(p, "/lib/");
}

static void print_list(FILE *out, const char *key, char **items, int from, int to) {
  fprintf(out, "%-8s = [", key);
  for (int i = from; i < to; i++) {
    fprintf(out, "%s\n  ", i == from ? "" : ",");
    toml_str(out, items[i]);
  }
  fprintf(out, "%s]\n", to > from ? "\n" : "");
}

static void slug(const char *in, char *out, size_t size) {
  size_t o = 0;
  for (; *in && o < size - 1; in++) {
    if (isalnum((unsigned char)*in)) out[o++] = (char)tolower((unsigned char)*in);
    else if (o > 0 && out[o - 1] != '-') out[o++] = '-';
  }
  while (o > 0 && out[o - 1] == '-') o--;
  out[o] = 0;
  if (!o) snprintf(out, size, "game");
}

// &amp; &lt; &gt; &quot; &#39; del <title>
static void decode_entities(char *s) {
  static const char *ent[][2] = { { "&amp;", "&" }, { "&lt;", "<" }, { "&gt;", ">" },
                                  { "&quot;", "\"" }, { "&#39;", "'" }, { "&apos;", "'" } };
  char *w = s;
  for (char *r = s; *r;) {
    int done = 0;
    for (size_t i = 0; i < sizeof(ent) / sizeof(ent[0]); i++) {
      size_t n = strlen(ent[i][0]);
      if (strncmp(r, ent[i][0], n) == 0) {
        *w++ = ent[i][1][0];
        r += n;
        done = 1;
        break;
      }
    }
    if (!done) *w++ = *r++;
  }
  *w = 0;
}

static void print_config(FILE *out, const char *dir, GameInfo *gi) {
  decode_entities(gi->title);
  char dirname[1024], name[128];
  snprintf(dirname, sizeof(dirname), "%s", dir);
  normalize(dirname);
  const char *last = strrchr(dirname, '/');
  slug(last ? last + 1 : dirname, name, sizeof(name));

  fprintf(out, "# config.toml generado por: quixjs --gen-config ");
  fprintf(out, "%s\n", dir);
  fprintf(out, "# Se puede editar: es lo que describe el juego (quixjs -c este archivo).\n");
  if (gi->title[0]) fprintf(out, "# %s\n", gi->title);
  fprintf(out, "\n[engine]\n");
  fprintf(out, "name    = ");
  toml_str(out, gi->engine);
  fprintf(out, "\n");
  if (gi->rm_version[0]) {
    fprintf(out, "version = ");
    toml_str(out, gi->rm_version);
    fprintf(out, "\n");
  }
  if (gi->pixi[0]) {
    fprintf(out, "pixi    = ");
    toml_str(out, gi->pixi);
    fprintf(out, "\n");
  }
  fprintf(out, "nwjs    = %s%s\n", gi->nwjs ? "true" : "false",
          gi->nwjs ? "   # package.json de nw.js: require('fs'), process..." : "");

  fprintf(out, "\n[game]\n");
  // el titulo: el del menu de --launcher (y el id del juego si no hay id)
  fprintf(out, "title     = ");
  toml_str(out, gi->title[0] ? gi->title : name);
  fprintf(out, "\ngame_dir  = ");
  toml_str(out, gi->base);
  fprintf(out, "\ncache_dir = ");
  char cache[256];
  snprintf(cache, sizeof(cache), "cache/%s", name);
  toml_str(out, cache);
  fprintf(out, "\nwidth     = %d   # %s\n", gi->width, gi->size_from);
  fprintf(out, "height    = %d\n", gi->height);


  // RPG Maker en nw.js guarda en dirname(index.html)/save
  const char *save_pattern = NULL;
  if (gi->nwjs && strcmp(gi->engine, "rpgmaker-mv") == 0) save_pattern = "save/*.rpgsave";
  if (gi->nwjs && strcmp(gi->engine, "rpgmaker-mz") == 0) save_pattern = "save/*.rmmzsave";
  if (save_pattern) {
    // RPG Maker guarda en save/, al lado de su index.html (game_dir)
    fprintf(out, "\n# las partidas que pasa Transfer Save\n[saves]\n");
    fprintf(out, "dir   = \"save\"   # la carpeta de las partidas, desde [game] game_dir\n");
    fprintf(out, "files = [");
    toml_str(out, strrchr(save_pattern, '/') + 1);
    fprintf(out, "]   # dentro de dir\n");
  }

  if (gi->font_count > 0) {
    fprintf(out, "\n# fuentes de los @font-face de su CSS\n[fonts]\n");
    for (int i = 0; i < gi->font_count; i++) {
      toml_str(out, gi->font_family[i]);
      fprintf(out, " = ");
      toml_str(out, gi->font_path[i]);
      fprintf(out, "\n");
    }
  }

  // libs: las del principio que estan en una carpeta lib/libs; el resto en
  // includes y el ultimo es entry (el mismo orden que el index.html)
  int libs = 0;
  while (libs < gi->script_count - 1 && is_lib(gi->scripts[libs])) libs++;
  fprintf(out, "\n# los <script> del index.html, en el mismo orden\n[scripts]\n");
  print_list(out, "libs", gi->scripts, 0, libs);
  print_list(out, "includes", gi->scripts, libs, gi->script_count - 1);
  fprintf(out, "%-8s = ", "entry");
  toml_str(out, gi->scripts[gi->script_count - 1]);
  fprintf(out, "\n");
}

int gen_config(const char *dir) {
  GameInfo gi;
  memset(&gi, 0, sizeof(gi));

  JSRuntime *rt = JS_NewRuntime();
  JSContext *ctx = rt ? JS_NewContext(rt) : NULL;
  if (!ctx) return 1;

  // 0. un .html concreto (una pagina que no es index.html): ese
  size_t dl = strlen(dir);
  int html_file = file_exists(dir) && ((dl > 5 && strcasecmp(dir + dl - 5, ".html") == 0) ||
                                       (dl > 4 && strcasecmp(dir + dl - 4, ".htm") == 0));
  if (html_file) snprintf(gi.html, sizeof(gi.html), "%s", dir);
  // 1. package.json de nw.js en la carpeta o en www/
  char www[1024];
  join(www, sizeof(www), dir, "www");
  if (!html_file && !read_manifest(ctx, dir, &gi) && !read_manifest(ctx, www, &gi)) {
    // 2. sin manifiesto: index.html en la carpeta o en www/
    char p[1024];
    join(p, sizeof(p), dir, "index.html");
    if (file_exists(p)) snprintf(gi.html, sizeof(gi.html), "%s", p);
    else {
      join(p, sizeof(p), www, "index.html");
      if (file_exists(p)) snprintf(gi.html, sizeof(gi.html), "%s", p);
    }
  }
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  if (!gi.html[0] || !file_exists(gi.html)) {
    fprintf(stderr, "quixjs --gen-config: no index.html in %s (nor in %s/www), and it is not an .html file\n", dir, dir);
    return 1;
  }
  dir_of(gi.base, sizeof(gi.base), gi.html);

  read_html(&gi);
  if (gi.script_count == 0) {
    fprintf(stderr, "quixjs --gen-config: %s has no <script src>\n", gi.html);
    return 1;
  }
  if (gi.script_count > MAX_SCRIPT_FILES * 2) {
    fprintf(stderr, "quixjs --gen-config: warning: %d scripts, more than quixjs loads (%d per list)\n",
            gi.script_count, MAX_SCRIPT_FILES);
  }
  detect_engine(&gi);
  print_config(stdout, dir, &gi);

  for (int i = 0; i < gi.script_count; i++) free(gi.scripts[i]);
  for (int i = 0; i < gi.font_count; i++) {
    free(gi.font_family[i]);
    free(gi.font_path[i]);
  }
  return 0;
}
