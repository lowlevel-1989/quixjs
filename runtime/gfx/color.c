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

// Leer colores CSS y escribirlos como texto (fillStyle devuelve "#rrggbb"
// o "rgba(...)"). Ver color.h.

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "color.h"

typedef struct {
  const char   *name;
  unsigned int  rgb;
} NamedColor;

static const NamedColor named_colors[] = {
  { "black",   0x000000 }, { "white",   0xffffff }, { "red",     0xff0000 },
  { "lime",    0x00ff00 }, { "green",   0x008000 }, { "blue",    0x0000ff },
  { "yellow",  0xffff00 }, { "cyan",    0x00ffff }, { "aqua",    0x00ffff },
  { "magenta", 0xff00ff }, { "fuchsia", 0xff00ff }, { "gray",    0x808080 },
  { "grey",    0x808080 }, { "silver",  0xc0c0c0 }, { "maroon",  0x800000 },
  { "olive",   0x808000 }, { "teal",    0x008080 }, { "navy",    0x000080 },
  { "purple",  0x800080 }, { "orange",  0xffa500 }, { "pink",    0xffc0cb },
  { "brown",   0xa52a2a }, { "gold",    0xffd700 }, { "violet",  0xee82ee },
  { "indigo",  0x4b0082 }, { "darkgray", 0xa9a9a9 }, { "darkgrey", 0xa9a9a9 },
  { "lightgray", 0xd3d3d3 }, { "lightgrey", 0xd3d3d3 },
  { "darkred", 0x8b0000 }, { "darkgreen", 0x006400 }, { "darkblue", 0x00008b },
  { "skyblue", 0x87ceeb }, { "crimson", 0xdc143c }, { "coral",   0xff7f50 },
  { "salmon",  0xfa8072 }, { "khaki",   0xf0e68c }, { "beige",   0xf5f5dc },
  { "tomato",  0xff6347 }, { "orchid",  0xda70d6 }, { "turquoise", 0x40e0d0 },
  { NULL, 0 }
};

static int hexval(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  c = tolower(c);
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static int clamp255(double v) {
  if (v < 0) return 0;
  if (v > 255) return 255;
  return (int)(v + 0.5);
}

// Lee hasta 4 numeros separados por comas, espacios o '/'. Los que acaban
// en '%' se marcan en pct.
static int parse_numbers(const char *p, double *vals, int *pct, int max) {
  int n = 0;
  while (*p && n < max) {
    while (*p == ' ' || *p == ',' || *p == '/' || *p == '\t') p++;
    if (*p == ')' || *p == 0) break;
    char *end;
    double v = strtod(p, &end);
    if (end == p) return -1;
    p = end;
    pct[n] = 0;
    if (*p == '%') {
      pct[n] = 1;
      p++;
    } else if (strncmp(p, "deg", 3) == 0) {
      p += 3;
    }
    vals[n++] = v;
  }
  while (*p == ' ') p++;
  if (*p != ')') return -1;
  return n;
}

static double hue_to_rgb(double p, double q, double t) {
  if (t < 0) t += 1;
  if (t > 1) t -= 1;
  if (t < 1.0 / 6) return p + (q - p) * 6 * t;
  if (t < 1.0 / 2) return q;
  if (t < 2.0 / 3) return p + (q - p) * (2.0 / 3 - t) * 6;
  return p;
}

int color_parse(const char *str, Color *out) {
  if (!str) return -1;
  while (isspace((unsigned char)*str)) str++;

  char buf[128];
  size_t len = strlen(str);
  if (len >= sizeof(buf)) return -1;
  for (size_t i = 0; i <= len; i++) buf[i] = (char)tolower((unsigned char)str[i]);
  while (len > 0 && isspace((unsigned char)buf[len - 1])) buf[--len] = 0;

  if (buf[0] == '#') {
    int d[8];
    size_t n = len - 1;
    if (n != 3 && n != 4 && n != 6 && n != 8) return -1;
    for (size_t i = 0; i < n; i++) {
      d[i] = hexval(buf[i + 1]);
      if (d[i] < 0) return -1;
    }
    if (n == 3 || n == 4) {
      out->r = d[0] * 17;
      out->g = d[1] * 17;
      out->b = d[2] * 17;
      out->a = n == 4 ? d[3] * 17 : 255;
    } else {
      out->r = d[0] * 16 + d[1];
      out->g = d[2] * 16 + d[3];
      out->b = d[4] * 16 + d[5];
      out->a = n == 8 ? d[6] * 16 + d[7] : 255;
    }
    return 0;
  }

  double v[4];
  int pct[4];
  if (strncmp(buf, "rgb", 3) == 0) {
    const char *p = strchr(buf, '(');
    if (!p) return -1;
    int n = parse_numbers(p + 1, v, pct, 4);
    if (n < 3) return -1;
    for (int i = 0; i < 3; i++) if (pct[i]) v[i] = v[i] * 255.0 / 100.0;
    double a = 1.0;
    if (n == 4) a = pct[3] ? v[3] / 100.0 : v[3];
    out->r = clamp255(v[0]);
    out->g = clamp255(v[1]);
    out->b = clamp255(v[2]);
    out->a = clamp255(a * 255.0);
    return 0;
  }

  if (strncmp(buf, "hsl", 3) == 0) {
    const char *p = strchr(buf, '(');
    if (!p) return -1;
    int n = parse_numbers(p + 1, v, pct, 4);
    if (n < 3) return -1;
    double h = fmod(fmod(v[0], 360.0) + 360.0, 360.0) / 360.0;
    double s = v[1] / 100.0;
    double l = v[2] / 100.0;
    double a = 1.0;
    if (n == 4) a = pct[3] ? v[3] / 100.0 : v[3];
    double r, g, b;
    if (s <= 0) {
      r = g = b = l;
    } else {
      double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
      double pp = 2 * l - q;
      r = hue_to_rgb(pp, q, h + 1.0 / 3);
      g = hue_to_rgb(pp, q, h);
      b = hue_to_rgb(pp, q, h - 1.0 / 3);
    }
    out->r = clamp255(r * 255.0);
    out->g = clamp255(g * 255.0);
    out->b = clamp255(b * 255.0);
    out->a = clamp255(a * 255.0);
    return 0;
  }

  if (strcmp(buf, "transparent") == 0) {
    out->r = out->g = out->b = out->a = 0;
    return 0;
  }

  for (const NamedColor *nc = named_colors; nc->name; nc++) {
    if (strcmp(buf, nc->name) == 0) {
      out->r = (nc->rgb >> 16) & 0xff;
      out->g = (nc->rgb >> 8) & 0xff;
      out->b = nc->rgb & 0xff;
      out->a = 255;
      return 0;
    }
  }

  return -1;
}

void color_to_string(const Color *c, char *out, size_t size) {
  if (c->a == 255) {
    snprintf(out, size, "#%02x%02x%02x", c->r, c->g, c->b);
    return;
  }

  // El navegador guarda el alpha en 8 bits y lo escribe con los decimales
  // justos para volver al mismo byte (128 -> "0.5", 64 -> "0.25")
  char alpha[16];
  for (int decimals = 1; decimals <= 4; decimals++) {
    snprintf(alpha, sizeof(alpha), "%.*f", decimals, c->a / 255.0);
    if ((int)(atof(alpha) * 255.0 + 0.5) == c->a) break;
  }
  // sin ceros a la derecha
  char *dot = strchr(alpha, '.');
  if (dot) {
    char *end = alpha + strlen(alpha) - 1;
    while (end > dot && *end == '0') *end-- = 0;
    if (end == dot) *end = 0;
  }
  snprintf(out, size, "rgba(%d, %d, %d, %s)", c->r, c->g, c->b, alpha);
}
