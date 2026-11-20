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

// Los @font-face de fonts/gamefont.css (RPG Maker MV y MZ).
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

// Los @font-face de un CSS del juego (url relativa a game_dir; de un mod si
// lo trae): font-family y la primera url() de cada uno, relativa al CSS. Las
// que no existen se saltan (y queda la de [fonts])
static void register_css_fonts(const char *game_dir, const char *css_url) {
  char css_path[1024];
  if (url_resolve(game_dir, css_url, css_path, sizeof(css_path)) != URL_FILE) return;
  size_t len = 0;
  char *css = (char *)url_read_file(css_path, &len);
  if (!css) return;
  char dir[512] = "";
  const char *slash = strrchr(css_url, '/');
  if (slash) snprintf(dir, sizeof(dir), "%.*s/", (int)(slash - css_url), css_url);
  for (char *p = css; (p = strcasestr(p, "@font-face")) != NULL;) {
    char *open = strchr(p, '{'), *close = open ? strchr(open, '}') : NULL;
    if (!close) break;
    *close = 0;
    char family[96] = "", url[512] = "";
    char *f = strcasestr(open, "font-family");
    if (f && (f = strchr(f, ':'))) {
      f++;
      while (*f == ' ' || *f == '\t' || *f == '"' || *f == '\'') f++;
      size_t n = strcspn(f, ";\"'\r\n");
      if (n >= sizeof(family)) n = sizeof(family) - 1;
      memcpy(family, f, n);
      family[n] = 0;
      while (n > 0 && (family[n - 1] == ' ' || family[n - 1] == '\t')) family[--n] = 0;
    }
    char *u = strcasestr(open, "url(");
    if (u) {
      u += 4;
      while (*u == ' ' || *u == '"' || *u == '\'') u++;
      size_t n = strcspn(u, "\"')");
      if (n >= sizeof(url)) n = sizeof(url) - 1;
      memcpy(url, u, n);
      url[n] = 0;
    }
    p = close + 1;
    if (!family[0] || !url[0] || strstr(url, "://")) continue;
    char rel[1024], path[1024];
    snprintf(rel, sizeof(rel), "%s%s", url[0] == '/' ? "" : dir, url[0] == '/' ? url + 1 : url);
    if (url_resolve(game_dir, rel, path, sizeof(path)) != URL_FILE || access(path, R_OK) != 0) continue;
    if (font_register(family, path) == 0) printf("Font: %s -> %s (%s)\n", family, path, css_url);
  }
  free(css);
}
