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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include "platform/platform.h"
#include "core/url.h"
#include "font.h"

#define MAX_FAMILIES 64
#define MAX_CACHED   64
#define MAX_FIXED    8

typedef struct {
  char family[96];
  char path[512];
} FamilyEntry;

struct FontHandle {
  char      path[512];
  int       size;
  int       style;
  int       outline;
  int       fixed;     // de font_from_css_fixed: no se cierra ni se mueve
  TTF_Font *font;
};

static FamilyEntry families[MAX_FAMILIES];
static int         family_count = 0;
static FontHandle  cache[MAX_CACHED];
static int         cache_count = 0;
static int         ready = 0;
static char        default_path[512] = "";
static FontHandle  fixed[MAX_FIXED];
static int         fixed_count = 0;

// Un solo thread a la vez: el juego dibuja su texto en el principal y los
// avisos encima del juego (fps, "Loading...", un logro) se dibujan en el
// thread que presenta (backend/sdl/thread.c). La cache y SDL_ttf (FreeType)
// no aguantan dos a la vez: un juego se cerraba al quitar "Loading...".
// Recursivo: una funcion publica puede llamar a otra (font_from_css a
// font_init)
static pthread_mutex_t lock;
static pthread_once_t  lock_once = PTHREAD_ONCE_INIT;
static void lock_make(void) {
  pthread_mutexattr_t a;
  pthread_mutexattr_init(&a);
  pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&lock, &a);
  pthread_mutexattr_destroy(&a);
}
static void font_lock(void) {
  pthread_once(&lock_once, lock_make);
  pthread_mutex_lock(&lock);
}
static void font_unlock(void) {
  pthread_mutex_unlock(&lock);
}


int font_init(void) {
  font_lock();
  if (ready) {
    font_unlock();
    return 0;
  }
  if (TTF_Init() != 0) {
    fprintf(stderr, "SDL_ttf could not initialize: %s\n", TTF_GetError());
    font_unlock();
    return -1;
  }
  // la de la plataforma (en Linux: QUIXJS_FONT o una del sistema)
  const char *def = platform_default_font();
  if (def) snprintf(default_path, sizeof(default_path), "%s", def);
  if (!default_path[0]) fprintf(stderr, "[font] no default font found (set QUIXJS_FONT)\n");
  ready = 1;
  font_unlock();
  return 0;
}

void font_quit(void) {
  font_lock();
  if (ready) {
    for (int i = 0; i < cache_count; i++) {
      if (cache[i].font) TTF_CloseFont(cache[i].font);
    }
    for (int i = 0; i < fixed_count; i++) {
      if (fixed[i].font) TTF_CloseFont(fixed[i].font);
    }
    cache_count = 0;
    fixed_count = 0;
    family_count = 0;
    TTF_Quit();
    ready = 0;
  }
  font_unlock();
}

static int family_eq(const char *a, const char *b) {
  return strcasecmp(a, b) == 0;
}

static int register_unlocked(const char *family, const char *path) {
  if (!url_exists(path)) {
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

static int is_registered_unlocked(const char *family) {
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

// Abre la fuente: del disco por su ruta o, si esta en la rom ([game] rom),
// desde memoria. De cada archivo de la rom se guarda una copia mientras dura
// el juego (SDL_ttf la lee mientras la fuente esta abierta; son pocas)
#define MAX_ROM_FONTS 16
static struct { char path[512]; uint8_t *data; size_t len; } rom_fonts[MAX_ROM_FONTS];

static TTF_Font *open_ttf(const char *path, int size) {
  char abs[1024];
  if (access(path, R_OK) == 0 || path[0] == 0) return TTF_OpenFont(platform_file_path(path, abs, sizeof abs), size);
  int k = 0;
  while (k < MAX_ROM_FONTS && rom_fonts[k].data && strcmp(rom_fonts[k].path, path) != 0) k++;
  if (k == MAX_ROM_FONTS) return NULL;
  if (!rom_fonts[k].data) {
    rom_fonts[k].data = url_read_file(path, &rom_fonts[k].len);
    if (!rom_fonts[k].data) return NULL;
    snprintf(rom_fonts[k].path, sizeof rom_fonts[k].path, "%s", path);
  }
  return TTF_OpenFontRW(SDL_RWFromConstMem(rom_fonts[k].data, (int)rom_fonts[k].len), 1, size);
}

// Una fuente que llega en memoria (FontFace con un ArrayBuffer: GDevelop la
// descarga con fetch): una copia en rom_fonts con la ruta "mem:<familia>",
// que open_ttf abre desde ahi
static int register_data_unlocked(const char *family, const void *data, size_t len) {
  char path[512];
  snprintf(path, sizeof path, "mem:%s", family);
  int k = 0;
  while (k < MAX_ROM_FONTS && rom_fonts[k].data && strcmp(rom_fonts[k].path, path) != 0) k++;
  if (k == MAX_ROM_FONTS || len == 0) return -1;
  uint8_t *copy = malloc(len);
  if (!copy) return -1;
  memcpy(copy, data, len);
  free(rom_fonts[k].data);
  rom_fonts[k].data = copy;
  rom_fonts[k].len = len;
  snprintf(rom_fonts[k].path, sizeof rom_fonts[k].path, "%s", path);
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

static FontHandle *get_font(const char *path, int size, int style, int outline) {
  for (int i = 0; i < cache_count; i++) {
    FontHandle *h = &cache[i];
    if (h->size == size && h->style == style && h->outline == outline && strcmp(h->path, path) == 0) return h;
  }
  TTF_Font *font = open_ttf(path, size);
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

static FontHandle *from_css_unlocked(const char *css, int fixed_handle) {
  if (!ready && font_init() != 0) return NULL;
  double size;
  int style;
  char path[512];
  if (parse_css(css, &size, &style, path, sizeof(path)) != 0) return NULL;
  int px = (int)lround(size);
  if (px < 1) px = 1;
  if (px > 512) px = 512;
  if (!fixed_handle) return get_font(path, px, style, 0);
  for (int i = 0; i < fixed_count; i++) {
    FontHandle *h = &fixed[i];
    if (h->size == px && h->style == style && strcmp(h->path, path) == 0) return h;
  }
  if (fixed_count == MAX_FIXED) return NULL;
  TTF_Font *font = open_ttf(path, px);
  if (!font) return NULL;
  TTF_SetFontStyle(font, style);
  FontHandle *h = &fixed[fixed_count++];
  snprintf(h->path, sizeof(h->path), "%s", path);
  h->size = px;
  h->style = style;
  h->outline = 0;
  h->fixed = 1;
  h->font = font;
  return h;
}

// Si a la fuente le faltan letras del texto (un parche coreano con la fuente
// del juego, que solo tiene latinas; o un plugin que pide "Dotum", que no
// esta), la primera fuente registrada que las tenga todas, y si no, la de
// por defecto: como un navegador, que las saca de otra fuente. Solo se mira
// fuera de ASCII (y del plano basico de Unicode: TTF_GlyphIsProvided).
static int covers(TTF_Font *font, const char *utf8) {
  const unsigned char *p = (const unsigned char *)utf8;
  while (*p) {
    unsigned cp;
    if (*p < 0x80) { p++; continue; }
    if ((*p & 0xE0) == 0xC0 && p[1]) { cp = ((p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu); p += 2; }
    else if ((*p & 0xF0) == 0xE0 && p[1] && p[2]) { cp = ((p[0] & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu); p += 3; }
    else { p++; while ((*p & 0xC0) == 0x80) p++; continue; }   // fuera del plano basico o roto
    if (!TTF_GlyphIsProvided(font, (Uint16)cp)) return 0;
  }
  return 1;
}

static int ascii_only(const char *utf8) {
  for (const unsigned char *p = (const unsigned char *)utf8; *p; p++) if (*p >= 0x80) return 0;
  return 1;
}

// get_font puede mover la cache: de f solo se usan sus datos, copiados antes
static FontHandle *with_glyphs(FontHandle *f, const char *utf8) {
  if (!utf8 || f->fixed || ascii_only(utf8) || covers(f->font, utf8)) return f;
  char path[512];
  snprintf(path, sizeof(path), "%s", f->path);
  int size = f->size, style = f->style, outline = f->outline;
  for (int i = 0; i <= family_count; i++) {
    const char *fp = i < family_count ? families[i].path : default_path;
    if (!fp[0] || strcmp(fp, path) == 0) continue;
    FontHandle *h = get_font(fp, size, style, outline);
    if (h && covers(h->font, utf8)) return h;
  }
  return get_font(path, size, style, outline);
}

static void metrics_unlocked(FontHandle *f, FontMetrics *m) {
  m->ascent = TTF_FontAscent(f->font);
  m->descent = -TTF_FontDescent(f->font);
  m->height = TTF_FontHeight(f->font);
}

static int text_width_unlocked(FontHandle *f, const char *utf8) {
  int w = 0, h = 0;
  if (!utf8 || !*utf8) return 0;
  f = with_glyphs(f, utf8);
  if (!f || TTF_SizeUTF8(f->font, utf8, &w, &h) != 0) return 0;
  return w;
}

static int render_unlocked(FontHandle *f, const char *utf8, unsigned char r, unsigned char g,
                unsigned char b, unsigned char a, int outline, Surface *out) {
  if (!utf8 || !*utf8) return -1;
  f = with_glyphs(f, utf8);
  if (!f) return -1;
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

// Las funciones publicas: cada una con el candado (ver lock, arriba)
int font_register(const char *family, const char *path) {
  font_lock();
  int r = register_unlocked(family, path);
  font_unlock();
  return r;
}

int font_register_data(const char *family, const void *data, size_t len) {
  font_lock();
  int r = register_data_unlocked(family, data, len);
  font_unlock();
  return r;
}

int font_is_registered(const char *family) {
  font_lock();
  int r = is_registered_unlocked(family);
  font_unlock();
  return r;
}

FontHandle *font_from_css(const char *css) {
  font_lock();
  FontHandle *h = from_css_unlocked(css, 0);
  font_unlock();
  return h;
}

FontHandle *font_from_css_fixed(const char *css) {
  font_lock();
  FontHandle *h = from_css_unlocked(css, 1);
  font_unlock();
  return h;
}

void font_metrics(FontHandle *f, FontMetrics *m) {
  font_lock();
  metrics_unlocked(f, m);
  font_unlock();
}

int font_text_width(FontHandle *f, const char *utf8) {
  font_lock();
  int r = text_width_unlocked(f, utf8);
  font_unlock();
  return r;
}

int font_render(FontHandle *f, const char *utf8, unsigned char r, unsigned char g,
                unsigned char b, unsigned char a, int outline, Surface *out) {
  font_lock();
  int ret = render_unlocked(f, utf8, r, g, b, a, outline, out);
  font_unlock();
  return ret;
}
