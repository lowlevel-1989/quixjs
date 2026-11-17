/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// beginPath, moveTo, lineTo, arc, rect, curvas

#include <math.h>
#include <string.h>
#include "context2d.h"

static int read_numbers(JSContext *ctx, int argc, JSValueConst *argv, double *out, int n) {
  if (argc < n) return -1;
  for (int i = 0; i < n; i++) {
    if (JS_ToFloat64(ctx, &out[i], argv[i])) return -1;
    if (!isfinite(out[i])) return -1;
  }
  return 0;
}

// Escala media de la matriz (para decidir cuantos segmentos usar en curvas)
static double matrix_scale(const Matrix *m) {
  double s = sqrt(fabs(m->a * m->d - m->b * m->c));
  return s > 0 ? s : 1;
}

static void add_point(Context2D *c, double x, double y, int move) {
  double dx, dy;
  matrix_apply(&CTX2D_STATE(c)->transform, x, y, &dx, &dy);
  if (move) path_move_to(&c->path, dx, dy);
  else      path_line_to(&c->path, dx, dy);
}

JSValue js_ctx2d_beginPath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  path_clear(&c->path);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_closePath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  path_close(&c->path);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_moveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[2];
  if (read_numbers(ctx, argc, argv, v, 2) == 0) add_point(c, v[0], v[1], 1);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_lineTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[2];
  if (read_numbers(ctx, argc, argv, v, 2) == 0) add_point(c, v[0], v[1], 0);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_rect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[4];
  if (read_numbers(ctx, argc, argv, v, 4) != 0) return JS_UNDEFINED;
  add_point(c, v[0], v[1], 1);
  add_point(c, v[0] + v[2], v[1], 0);
  add_point(c, v[0] + v[2], v[1] + v[3], 0);
  add_point(c, v[0], v[1] + v[3], 0);
  path_close(&c->path);
  add_point(c, v[0], v[1], 1);
  return JS_UNDEFINED;
}

// Elipse (o arco si rx == ry) aplanada en el espacio de usuario
static void add_ellipse(Context2D *c, double cx, double cy, double rx, double ry, double rotation,
                        double start, double end, int ccw) {
  const double TAU = 2 * M_PI;
  double sweep;
  if (!ccw) {
    if (end - start >= TAU) sweep = TAU;
    else {
      sweep = fmod(end - start, TAU);
      if (sweep < 0) sweep += TAU;
    }
  } else {
    if (start - end >= TAU) sweep = -TAU;
    else {
      sweep = -fmod(start - end, TAU);
      if (sweep > 0) sweep -= TAU;
    }
  }

  double r = (rx > ry ? rx : ry) * matrix_scale(&CTX2D_STATE(c)->transform);
  // error maximo de 0.25 px entre la cuerda y el arco
  double step = r > 0.25 ? 2 * acos(1 - 0.25 / r) : M_PI / 2;
  int n = (int)ceil(fabs(sweep) / step);
  if (n < 4) n = 4;
  if (n > 512) n = 512;

  double cr = cos(rotation), sr = sin(rotation);
  int move = !path_has_current_point(&c->path);
  for (int i = 0; i <= n; i++) {
    double a = start + sweep * i / n;
    double ex = cos(a) * rx, ey = sin(a) * ry;
    double x = cx + ex * cr - ey * sr;
    double y = cy + ex * sr + ey * cr;
    add_point(c, x, y, i == 0 && move);
  }
}

JSValue js_ctx2d_arc(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[5];
  if (read_numbers(ctx, argc, argv, v, 5) != 0) return JS_UNDEFINED;
  if (v[2] < 0) return JS_ThrowRangeError(ctx, "The radius provided is negative");
  int ccw = argc > 5 && JS_ToBool(ctx, argv[5]);
  add_ellipse(c, v[0], v[1], v[2], v[2], 0, v[3], v[4], ccw);
  return JS_UNDEFINED;
}

JSValue js_ctx2d_ellipse(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[7];
  if (read_numbers(ctx, argc, argv, v, 7) != 0) return JS_UNDEFINED;
  if (v[2] < 0 || v[3] < 0) return JS_ThrowRangeError(ctx, "The radius provided is negative");
  int ccw = argc > 7 && JS_ToBool(ctx, argv[7]);
  add_ellipse(c, v[0], v[1], v[2], v[3], v[4], v[5], v[6], ccw);
  return JS_UNDEFINED;
}

// Punto actual en coordenadas de usuario
static int current_user_point(Context2D *c, double *x, double *y) {
  if (!path_has_current_point(&c->path)) return 0;
  double dx, dy;
  path_current_point(&c->path, &dx, &dy);
  Matrix inv;
  if (matrix_invert(&inv, &CTX2D_STATE(c)->transform) != 0) return 0;
  matrix_apply(&inv, dx, dy, x, y);
  return 1;
}

JSValue js_ctx2d_arcTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[5];
  if (read_numbers(ctx, argc, argv, v, 5) != 0) return JS_UNDEFINED;
  double x1 = v[0], y1 = v[1], x2 = v[2], y2 = v[3], r = v[4];
  if (r < 0) return JS_ThrowRangeError(ctx, "The radius provided is negative");

  double x0, y0;
  if (!current_user_point(c, &x0, &y0)) {
    add_point(c, x1, y1, 1);
    return JS_UNDEFINED;
  }

  double d0x = x0 - x1, d0y = y0 - y1;
  double d1x = x2 - x1, d1y = y2 - y1;
  double l0 = sqrt(d0x * d0x + d0y * d0y);
  double l1 = sqrt(d1x * d1x + d1y * d1y);
  double cross = d0x * d1y - d0y * d1x;
  if (r == 0 || l0 == 0 || l1 == 0 || fabs(cross) < 1e-9) {
    add_point(c, x1, y1, 0);
    return JS_UNDEFINED;
  }
  d0x /= l0; d0y /= l0;
  d1x /= l1; d1y /= l1;

  double cosang = d0x * d1x + d0y * d1y;
  double ang = acos(cosang < -1 ? -1 : (cosang > 1 ? 1 : cosang));
  double dist = r / tan(ang / 2);
  double t0x = x1 + d0x * dist, t0y = y1 + d0y * dist;
  double t1x = x1 + d1x * dist, t1y = y1 + d1y * dist;

  // centro: desde t0 perpendicular hacia dentro del angulo
  double bx = d0x + d1x, by = d0y + d1y;
  double bl = sqrt(bx * bx + by * by);
  bx /= bl; by /= bl;
  double cd = r / sin(ang / 2);
  double cx = x1 + bx * cd, cy = y1 + by * cd;

  double a0 = atan2(t0y - cy, t0x - cx);
  double a1 = atan2(t1y - cy, t1x - cx);
  int ccw = cross > 0;
  add_point(c, t0x, t0y, 0);
  add_ellipse(c, cx, cy, r, r, 0, a0, a1, ccw);
  return JS_UNDEFINED;
}

static int curve_segments(Context2D *c, const double *pts, int n) {
  // longitud del poligono de control en pixeles
  double len = 0;
  double sc = matrix_scale(&CTX2D_STATE(c)->transform);
  for (int i = 1; i < n; i++) {
    double dx = pts[i * 2] - pts[i * 2 - 2];
    double dy = pts[i * 2 + 1] - pts[i * 2 - 1];
    len += sqrt(dx * dx + dy * dy);
  }
  int segs = (int)ceil(len * sc / 4);
  if (segs < 4) segs = 4;
  if (segs > 256) segs = 256;
  return segs;
}

JSValue js_ctx2d_quadraticCurveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[4];
  if (read_numbers(ctx, argc, argv, v, 4) != 0) return JS_UNDEFINED;
  double x0, y0;
  if (!current_user_point(c, &x0, &y0)) {
    add_point(c, v[0], v[1], 1);
    x0 = v[0];
    y0 = v[1];
  }
  double pts[6] = { x0, y0, v[0], v[1], v[2], v[3] };
  int n = curve_segments(c, pts, 3);
  for (int i = 1; i <= n; i++) {
    double t = (double)i / n, u = 1 - t;
    double x = u * u * x0 + 2 * u * t * v[0] + t * t * v[2];
    double y = u * u * y0 + 2 * u * t * v[1] + t * t * v[3];
    add_point(c, x, y, 0);
  }
  return JS_UNDEFINED;
}

JSValue js_ctx2d_bezierCurveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[6];
  if (read_numbers(ctx, argc, argv, v, 6) != 0) return JS_UNDEFINED;
  double x0, y0;
  if (!current_user_point(c, &x0, &y0)) {
    add_point(c, v[0], v[1], 1);
    x0 = v[0];
    y0 = v[1];
  }
  double pts[8] = { x0, y0, v[0], v[1], v[2], v[3], v[4], v[5] };
  int n = curve_segments(c, pts, 4);
  for (int i = 1; i <= n; i++) {
    double t = (double)i / n, u = 1 - t;
    double b0 = u * u * u, b1 = 3 * u * u * t, b2 = 3 * u * t * t, b3 = t * t * t;
    double x = b0 * x0 + b1 * v[0] + b2 * v[2] + b3 * v[4];
    double y = b0 * y0 + b1 * v[1] + b2 * v[3] + b3 * v[5];
    add_point(c, x, y, 0);
  }
  return JS_UNDEFINED;
}

JSValue js_ctx2d_isPointInPath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Context2D *c = context2d_get(ctx, this_val);
  if (!c) return JS_EXCEPTION;
  double v[2];
  if (read_numbers(ctx, argc, argv, v, 2) != 0) return JS_FALSE;

  int evenodd = 0;
  if (argc > 2 && JS_IsString(argv[2])) {
    const char *r = JS_ToCString(ctx, argv[2]);
    evenodd = r && strcmp(r, "evenodd") == 0;
    if (r) JS_FreeCString(ctx, r);
  }

  // numero de vueltas del path alrededor del punto (coordenadas del canvas)
  int wind = 0;
  for (int i = 0; i < c->path.count; i++) {
    const SubPath *s = &c->path.subs[i];
    for (int j = 0; j < s->count; j++) {
      PathPoint a = s->pts[j];
      PathPoint b = s->pts[(j + 1) % s->count];
      if (a.y <= v[1]) {
        if (b.y > v[1] && (b.x - a.x) * (v[1] - a.y) - (v[0] - a.x) * (b.y - a.y) > 0) wind++;
      } else {
        if (b.y <= v[1] && (b.x - a.x) * (v[1] - a.y) - (v[0] - a.x) * (b.y - a.y) < 0) wind--;
      }
    }
  }
  return JS_NewBool(ctx, evenodd ? (wind & 1) : wind != 0);
}
