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

// runtime/binding/image/decode.c: decodificar las imagenes de Image: al
// momento (image_decode: data URIs, o cuando no hay thread) o en el thread de
// trabajo (image_decode_async, js_image_decode_pixels), y el tamano de un PNG
// sin abrirlo ([engine] lazy_images).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/app_context.h"
#include "core/url/url.h"
#include "core/debug/memstats.h"
#include "core/timer.h"
#include "core/worker.h"
#include "binding/blob.h"
#include "gfx/soft/image_loader.h"
#include "gfx/soft/image_scale.h"
#include "binding/document.h"
#include "binding/events.h"
#include "core/debug/trace.h"
#include "common.h"

// ---------- decodificar en segundo plano ----------
//
// Las imagenes de archivo o de Blob (las del juego) se decodifican en un thread
// aparte: un PNG grande son decenas de ms en una portatil, y en el thread
// principal eran una caida (aparece un enemigo, cambia el mapa). El thread solo
// toca bytes y pixeles; image_poll, en el thread principal, pone la imagen como
// cargada y lanza load. Las data: URIs (pequenas; PIXI las dibuja sin esperar
// a load) siguen siendo inmediatas.

typedef struct {
  JSValue   obj;      // la imagen (no se toca fuera del thread principal)
  unsigned  gen;
  uint8_t  *data;     // los bytes (copia) o
  size_t    len;
  char     *path;     // el archivo (con su transformacion, si tiene: descifrar)
  int       blob;     // data son los bytes de un Blob
  int       scale;    // [game] image_scale: se reduce si es grande (gfx/soft/image_scale.h)
  int       lazy;     // [engine] lazy_images: de un PNG solo el tamano (se decodifica al usarla)
  // js_image_decode_pixels: los pixeles van a quien los pidio, no a la imagen
  void    (*deliver)(void *user, uint8_t *px, int w, int h);
  void     *user;
  uint8_t  *px;       // el resultado
  int       w, h, ok;
  int       nat_w;    // el tamano de la imagen entera (el de px si no se redujo)
  int       nat_h;
  double    ms;
} ImageJob;


// el tamano de un PNG por su cabecera (IHDR); 0 si no es un PNG
static int png_size(const char *path, int *w, int *h) {
  if (url_has_transform(path)) return 0;
  size_t len = 0;
  uint8_t *head = url_read_head_transformed(path, 24, &len);
  int ok = head && len >= 24 && memcmp(head, "\x89PNG\r\n\x1a\n", 8) == 0 &&
           memcmp(head + 12, "IHDR", 4) == 0;
  if (ok) {
    *w = (head[16] << 24) | (head[17] << 16) | (head[18] << 8) | head[19];
    *h = (head[20] << 24) | (head[21] << 16) | (head[22] << 8) | head[23];
    ok = *w > 0 && *h > 0;
  }
  free(head);
  return ok;
}

// en el thread de trabajo
static void image_work(void *arg) {
  ImageJob *j = arg;
  double t = image_now_ms();
  if (j->lazy && j->path && png_size(j->path, &j->nat_w, &j->nat_h)) {
    j->ok = 1;   // sin pixeles: la imagen queda soltada (image_reload)
  } else if (j->scale && j->path) {
    // reducida (y desde la cache, si ya esta): descifra ella misma
    j->ok = image_scale_decode(j->path, &j->px, &j->w, &j->h, &j->nat_w, &j->nat_h) == 0;
  } else {
    if (j->path && url_has_transform(j->path)) {
      j->data = url_read_file_transformed(j->path, &j->len);
      free(j->path);
      j->path = NULL;
    }
    j->ok = (j->data || j->path) && image_decode_pixels(j->data, j->len, j->path, &j->px, &j->w, &j->h) == 0;
    j->nat_w = j->w;
    j->nat_h = j->h;
  }
  j->ms = image_now_ms() - t;
  TRACE_END(t, 5.0, "image decode %dx%d (worker)", j->w, j->h);
  // los bytes de un Blob se guardan (por si hay que decodificar otra vez);
  // los de un archivo cifrado, no: se vuelve a leer el archivo
  if (!j->blob) {
    free(j->data);
    j->data = NULL;
  }
}

// en el thread principal
static void image_done(JSContext *ctx, void *arg) {
  ImageJob *j = arg;
  if (j->deliver) {   // js_image_decode_pixels: los pixeles (o NULL) a quien los pidio
    j->deliver(j->user, j->ok ? j->px : NULL, j->w, j->h);
    free(j->px);
    free(j->path);
    free(j->data);
    free(j);
    return;
  }
  ImageData *img = JS_GetOpaque(j->obj, js_image_class_id);
  if (img && img->gen == j->gen) {
    JSValue ev;
    if (j->ok && !j->px) {          // lazy_images: solo el tamano
      img->nat_w = j->nat_w;
      img->nat_h = j->nat_h;
      img->dropped = 1;
      img->loaded = 1;
      img->fired = 1;
      ev = events_new(ctx, "load");
    } else if (j->ok) {
      surface_adopt(&img->surface, j->px, j->w, j->h);
      j->px = NULL;
      img->nat_w = j->nat_w;
      img->nat_h = j->nat_h;
      if (j->blob) {
        img->enc = j->data;
        img->enc_len = j->len;
        j->data = NULL;
      }
      JS_ReportExtraMemory(ctx, (size_t)j->w * (size_t)j->h * 4);
      img->loaded = 1;
      img->fired = 1;
      memstats_decode(j->ms);
      ev = events_new(ctx, "load");
    } else {
      fprintf(stderr, "[Image] failed to load: %.120s\n", img->src ? img->src : "?");
      ev = events_new(ctx, "error");
    }
    events_dispatch(ctx, j->obj, ev);
    JS_FreeValue(ctx, ev);
  }
  free(j->px);
  free(j->path);
  free(j->data);
  JS_FreeValue(ctx, j->obj);
  free(j);
}

// La imagen de src en segundo plano. 0 si se pidio (llegara load o error),
// 1 si no es de archivo ni de Blob (se hace al momento), -1 si no existe
int image_decode_async(JSContext *ctx, JSValueConst obj, ImageData *img, const char *src) {
  AppContext *app = JS_GetContextOpaque(ctx);
  ImageJob *j = calloc(1, sizeof(ImageJob));
  if (!j) return -1;
  if (strncmp(src, "blob:", 5) == 0) {
    size_t len = 0;
    const uint8_t *data = blob_lookup(src, &len);
    if (!data || !(j->data = malloc(len ? len : 1))) {
      free(j);
      return -1;
    }
    memcpy(j->data, data, len);   // el Blob se puede revocar antes de terminar
    j->len = len;
    j->blob = 1;
  } else {
    char path[1024];
    int kind = url_resolve(app->cfg->game.game_dir, src, path, sizeof(path));
    if (kind != URL_FILE) {
      free(j);
      return kind == URL_DATA ? 1 : -1;
    }
    j->path = strdup(path);
    j->scale = image_scale_wanted(src);
    j->lazy = app->cfg->engine.lazy_images;
  }
  j->obj = JS_DupValue(ctx, obj);
  j->gen = img->gen;
  if (worker_submit(image_work, image_done, j) != 0) {
    JS_FreeValue(ctx, j->obj);
    free(j->data);
    free(j->path);
    free(j);
    return 1;   // sin thread: al momento
  }
  return 0;
}

// Una imagen sin sus pixeles (soltados, o lazy_images): se decodifica en el
// thread de trabajo y los pixeles se entregan a deliver en el thread
// principal (deliver no se queda con ellos). 0 si se pidio; -1 si la imagen
// tiene sus pixeles o no es de archivo (el que llama sigue como siempre)
int js_image_decode_pixels(JSContext *ctx, JSValueConst val, int *w, int *h,
                           void (*deliver)(void *user, uint8_t *px, int w, int h), void *user) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded || !img->dropped || !img->src || img->enc) return -1;
  AppContext *app = JS_GetContextOpaque(ctx);
  // solo con [engine] lazy_images: las soltadas por falta de RAM (RPG Maker)
  // se vuelven a decodificar al momento, como siempre (una textura vacia unas
  // imagenes se veria)
  if (!app->cfg->engine.lazy_images) return -1;
  char path[1024];
  if (url_resolve(app->cfg->game.game_dir, img->src, path, sizeof(path)) != URL_FILE) return -1;
  // reducida (image_scale): su tamano final se sabe al decodificarla
  if (image_scale_wanted(img->src)) return -1;
  ImageJob *j = calloc(1, sizeof(ImageJob));
  if (!j) return -1;
  j->path = strdup(path);
  j->deliver = deliver;
  j->user = user;
  if (!j->path || worker_submit(image_work, image_done, j) != 0) {
    free(j->path);
    free(j);
    return -1;
  }
  *w = img->nat_w;
  *h = img->nat_h;
  return 0;
}

int image_decode(JSContext *ctx, ImageData *img, const char *src) {
  AppContext *app = JS_GetContextOpaque(ctx);
  char path[1024];

  if (strncmp(src, "blob:", 5) == 0) {
    size_t len = 0;
    const uint8_t *data = blob_lookup(src, &len);
    return data ? image_load_memory(data, len, &img->surface) : -1;
  }

  switch (url_resolve(app->cfg->game.game_dir, src, path, sizeof(path))) {
    case URL_DATA: {
      size_t len = 0;
      uint8_t *data = url_decode_data(src, &len);
      if (!data) return -1;
      int r = image_load_memory(data, len, &img->surface);
      free(data);
      return r;
    }
    case URL_FILE:
      if (image_scale_wanted(src)) {   // [game] image_scale: reducida si es grande
        uint8_t *px = NULL;
        int w = 0, h = 0, nw = 0, nh = 0;
        if (image_scale_decode(path, &px, &w, &h, &nw, &nh) != 0) return -1;
        surface_adopt(&img->surface, px, w, h);
        img->nat_w = nw;
        img->nat_h = nh;
        return 0;
      }
      if (url_has_transform(path)) {   // un archivo cifrado
        size_t len = 0;
        uint8_t *data = url_read_file_transformed(path, &len);
        if (!data) return -1;
        int r = image_load_memory(data, len, &img->surface);
        free(data);
        return r;
      }
      return image_load_file(path, &img->surface);
    default:
      return -1;
  }
}
