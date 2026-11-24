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
//
// gen_config.c es un archivo indice, como core/main.c: gen_config() esta
// aqui y el resto en gen_config/, partido por temas. Las partes se incluyen
// en orden y todo se compila como un solo archivo.

#include "gen_config/common.h"
#include "gen_config/manifest.c"
#include "gen_config/html.c"
#include "gen_config/engine.c"
#include "gen_config/output.c"

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
  char *html = read_text(gi.html);
  if (html) {
    find_canvas_id(html, &gi);
    find_body_elements(html, &gi);
    free(html);
  }
  char gw[1100];
  join(gw, sizeof(gw), gi.base, "greenworks.js");
  gi.greenworks = file_exists(gw);
  printer_print(stdout, dir, &gi);

  for (int i = 0; i < gi.script_count; i++) free(gi.scripts[i]);
  for (int i = 0; i < gi.font_count; i++) {
    free(gi.font_family[i]);
    free(gi.font_path[i]);
  }
  return 0;
}
