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

// Fuentes TrueType con SDL_ttf: registrar familias, elegir la fuente de
// ctx.font, medir y dibujar texto. Ver font.h.

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "platform/platform.h"
#include "font.h"

#define MAX_FAMILIES 64
#define MAX_CACHED   64

typedef struct {
  char family[96];
  char path[512];
} FamilyEntry;

struct FontHandle {
  char      path[512];
  int       size;
  int       style;
  int       outline;
  TTF_Font *font;
};

static FamilyEntry families[MAX_FAMILIES];
static int         family_count = 0;
static FontHandle  cache[MAX_CACHED];
static int         cache_count = 0;
static int         ready = 0;
static char        default_path[512] = "";


int font_init(void) {
  if (ready) return 0;
  if (TTF_Init() != 0) {
    fprintf(stderr, "SDL_ttf could not initialize: %s\n", TTF_GetError());
    return -1;
  }
  // la de la plataforma (en Linux: QUIXJS_FONT o una del sistema)
  const char *def = platform_default_font();
  if (def) snprintf(default_path, sizeof(default_path), "%s", def);
  if (!default_path[0]) fprintf(stderr, "[font] no default font found (set QUIXJS_FONT)\n");
  ready = 1;
  return 0;
}

void font_quit(void) {
  if (!ready) return;
  for (int i = 0; i < cache_count; i++) {
    if (cache[i].font) TTF_CloseFont(cache[i].font);
  }
  cache_count = 0;
  family_count = 0;
  TTF_Quit();
  ready = 0;
}

static int family_eq(const char *a, const char *b) {
  return strcasecmp(a, b) == 0;
}

int font_register(const char *family, const char *path) {
  if (access(path, R_OK) != 0) {
    fprintf(stderr, "[font] file not found for \"%s\": %s\n", family, path);
    return -1;
  }
  for (int i = 0; i < family_count; i++) {
    if (family_eq(families[i].family, family)) {
      snprintf(families[i].path, sizeof(families[i].path), "%s", path);
      return 0;
    }
  }
  if (family_count >= MAX_FAMILIES) return -1;
  snprintf(families[family_count].family, sizeof(families[0].family), "%s", family);
  snprintf(families[family_count].path, sizeof(families[0].path), "%s", path);
  family_count++;
  return 0;
}

int font_is_registered(const char *family) {
  for (int i = 0; i < family_count; i++) {
    if (family_eq(families[i].family, family)) return 1;
  }
  return 0;
}

static const char *family_path(const char *family) {
  for (int i = 0; i < family_count; i++) {
    if (family_eq(families[i].family, family)) return families[i].path;
  }
  return NULL;
}

// "italic bold 20px/1.2 'My Font', serif" -> size 20, bold, path de 'My Font'
static int parse_css(const char *css, double *size, int *style, char *path, size_t path_size) {
  char buf[256];
  snprintf(buf, sizeof(buf), "%s", css);

  *size = -1;
  *style = TTF_STYLE_NORMAL;
  char *families_str = NULL;

  char *p = buf;
  while (*p) {
    while (isspace((unsigned char)*p)) p++;
    if (!*p) break;
    char *tok = p;
    while (*p && !isspace((unsigned char)*p)) p++;
    char saved = *p;
    *p = 0;

    char *end;
    double v = strtod(tok, &end);
    if (end != tok && (strncmp(end, "px", 2) == 0 || strncmp(end, "pt", 2) == 0 || strncmp(end, "em", 2) == 0)) {
      *size = strncmp(end, "pt", 2) == 0 ? v * 4.0 / 3.0 : (strncmp(end, "em", 2) == 0 ? v * 16.0 : v);
      *p = saved;
      families_str = p;
      break;
    }
    if (strcasecmp(tok, "bold") == 0 || strcasecmp(tok, "bolder") == 0 ||
        (isdigit((unsigned char)tok[0]) && atoi(tok) >= 600)) {
      *style |= TTF_STYLE_BOLD;
    } else if (strcasecmp(tok, "italic") == 0 || strcasecmp(tok, "oblique") == 0) {
      *style |= TTF_STYLE_ITALIC;
    }
    *p = saved;
  }
  if (*size <= 0) return -1;

  path[0] = 0;
  if (families_str) {
    // primera familia registrada de la lista
    char *save = NULL;
    for (char *fam = strtok_r(families_str, ",", &save); fam; fam = strtok_r(NULL, ",", &save)) {
      while (isspace((unsigned char)*fam) || *fam == '"' || *fam == '\'') fam++;
      char *e = fam + strlen(fam);
      while (e > fam && (isspace((unsigned char)e[-1]) || e[-1] == '"' || e[-1] == '\'')) *--e = 0;
      const char *fp = family_path(fam);
      if (fp) {
        snprintf(path, path_size, "%s", fp);
        break;
      }
    }
  }
  if (!path[0]) snprintf(path, path_size, "%s", default_path);
  return path[0] ? 0 : -1;
}

static FontHandle *get_font(const char *path, int size, int style, int outline) {
  for (int i = 0; i < cache_count; i++) {
    FontHandle *h = &cache[i];
    if (h->size == size && h->style == style && h->outline == outline && strcmp(h->path, path) == 0) return h;
  }
  TTF_Font *font = TTF_OpenFont(path, size);
  if (!font) {
    fprintf(stderr, "[font] %s: %s\n", path, TTF_GetError());
    return NULL;
  }
  TTF_SetFontStyle(font, style);
  if (outline > 0) TTF_SetFontOutline(font, outline);

  // cache llena: se descarta la mas antigua
  if (cache_count == MAX_CACHED) {
    TTF_CloseFont(cache[0].font);
    memmove(&cache[0], &cache[1], sizeof(FontHandle) * (MAX_CACHED - 1));
    cache_count--;
  }
  FontHandle *h = &cache[cache_count++];
  snprintf(h->path, sizeof(h->path), "%s", path);
  h->size = size;
  h->style = style;
  h->outline = outline;
  h->font = font;
  return h;
}

FontHandle *font_from_css(const char *css) {
  if (!ready && font_init() != 0) return NULL;
  double size;
  int style;
  char path[512];
  if (parse_css(css, &size, &style, path, sizeof(path)) != 0) return NULL;
  int px = (int)lround(size);
  if (px < 1) px = 1;
  if (px > 512) px = 512;
  return get_font(path, px, style, 0);
}

void font_metrics(FontHandle *f, FontMetrics *m) {
  m->ascent = TTF_FontAscent(f->font);
  m->descent = -TTF_FontDescent(f->font);
  m->height = TTF_FontHeight(f->font);
}

int font_text_width(FontHandle *f, const char *utf8) {
  int w = 0, h = 0;
  if (!utf8 || !*utf8) return 0;
  if (TTF_SizeUTF8(f->font, utf8, &w, &h) != 0) return 0;
  return w;
}

int font_render(FontHandle *f, const char *utf8, unsigned char r, unsigned char g,
                unsigned char b, unsigned char a, int outline, Surface *out) {
  if (!utf8 || !*utf8) return -1;
  FontHandle *use = f;
  if (outline > 0) {
    use = get_font(f->path, f->size, f->style, outline);
    if (!use) return -1;
  }

  SDL_Color white = { 255, 255, 255, 255 };
  SDL_Surface *s = TTF_RenderUTF8_Blended(use->font, utf8, white);
  if (!s) return -1;
  SDL_Surface *rgba = SDL_ConvertSurfaceFormat(s, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(s);
  if (!rgba) return -1;

  if (surface_init(out, rgba->w, rgba->h) != 0) {
    SDL_FreeSurface(rgba);
    return -1;
  }
  SDL_LockSurface(rgba);
  for (int y = 0; y < rgba->h; y++) {
    const uint8_t *src = (const uint8_t *)rgba->pixels + (size_t)y * rgba->pitch;
    uint8_t *dst = out->px + (size_t)y * out->w * 4;
    for (int x = 0; x < rgba->w; x++) {
      dst[x * 4 + 0] = r;
      dst[x * 4 + 1] = g;
      dst[x * 4 + 2] = b;
      dst[x * 4 + 3] = (uint8_t)(src[x * 4 + 3] * a / 255);
    }
  }
  SDL_UnlockSurface(rgba);
  SDL_FreeSurface(rgba);
  return 0;
}
