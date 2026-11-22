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

// Image / HTMLImageElement. Ver image.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "core/app_context.h"
#include "core/url.h"
#include "core/memstats.h"
#include "core/timer.h"
#include "core/worker.h"
#include "blob.h"
#include "gfx/image_loader.h"
#include "gfx/image_scale.h"
#include "document.h"
#include "events.h"
#include "image.h"
#include "core/trace.h"

typedef struct ImageData {
  Surface surface;
  int     loaded;
  int     fired;     // ya se envio el evento load (complete = true)
  int     width;     // width/height asignados a mano (-1 = los de la imagen)
  int     height;
  char   *src;
  unsigned gen;      // cambia con cada src: un resultado de un src anterior se descarta
  int     nat_w;     // el tamano de la imagen (sigue igual si se sueltan sus pixeles)
  int     nat_h;
  int     used_2d;   // se dibujo en un canvas 2d: sus pixeles se quedan
  int     uploaded;  // ya esta en una textura de WebGL
  int     dropped;   // pixeles soltados (se decodifica otra vez si hace falta)
  uint8_t *enc;      // la imagen sin decodificar si vino de un Blob (el Blob se puede revocar)
  size_t  enc_len;
  struct ImageData *prev, *next;   // todas las imagenes vivas
} ImageData;

// Una imagen que ya esta en una textura de WebGL y nunca se dibujo en 2d tiene
// sus pixeles dos veces (en las portatiles la GPU usa la misma RAM). Cuando al
// sistema le falta RAM, js_image_release_uploaded suelta los de la CPU de las
// de este tamano o mas; si vuelven a hacer falta, se decodifican otra vez (eso
// cuesta: por eso solo con poca RAM, no siempre)
#define IMAGE_DROP_MIN_BYTES (128 * 1024)
static ImageData *all_images;

static JSClassID js_image_class_id;

static void js_image_finalizer(JSRuntime *rt, JSValue val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img) return;
  surface_free(&img->surface);
  free(img->src);
  free(img->enc);
  if (img->prev) img->prev->next = img->next;
  else all_images = img->next;
  if (img->next) img->next->prev = img->prev;
  js_free_rt(rt, img);
}

static double now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static int image_decode(JSContext *ctx, ImageData *img, const char *src);

// Los pixeles otra vez, si se soltaron (desde el Blob guardado o desde src)
static int image_reload(JSContext *ctx, ImageData *img) {
  if (!img->dropped) return 0;
  double t = now_ms();
  int r = img->enc ? image_load_memory(img->enc, img->enc_len, &img->surface)
                   : (img->src ? image_decode(ctx, img, img->src) : -1);
  if (r != 0) {
    fprintf(stderr, "[Image] cannot decode again: %.120s\n", img->src ? img->src : "?");
    return -1;
  }
  img->dropped = 0;
  printf("[Image] decoded again (%.1f ms): %.120s\n", now_ms() - t, img->src ? img->src : "?");
  return 0;
}

static JSClassDef js_image_class = {
  "HTMLImageElement",
  .finalizer = js_image_finalizer,
};

const Surface *js_image_get_surface(JSContext *ctx, JSValueConst val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded) return NULL;
  img->used_2d = 1;
  if (image_reload(ctx, img) != 0) return NULL;
  return &img->surface;
}

const Surface *js_image_get_surface_gpu(JSContext *ctx, JSValueConst val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded) return NULL;
  if (image_reload(ctx, img) != 0) return NULL;
  return &img->surface;
}

int js_image_natural_size(JSContext *ctx, JSValueConst val, int *w, int *h) {
  (void)ctx;
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded || img->nat_w <= 0 || img->nat_h <= 0) return -1;
  *w = img->nat_w;
  *h = img->nat_h;
  return 0;
}

// [engine] lazy_images: un juego WebGL que sube sus imagenes y no las
// vuelve a leer (Construct 2: hojas de 2048x2048, 16 MB cada una) no
// necesita los pixeles en la CPU; se sueltan al subirlas, no cuando ya falta
// RAM (una pantalla que sube 20 hojas en una imagen no daba tiempo)
void js_image_uploaded(JSContext *ctx, JSValueConst val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded) return;
  img->uploaded = 1;
  AppContext *app = JS_GetContextOpaque(ctx);
  size_t b = (size_t)img->surface.w * (size_t)img->surface.h * 4;
  if (app->cfg->engine.lazy_images && !img->used_2d && img->surface.px &&
      b >= IMAGE_DROP_MIN_BYTES) {
    surface_free(&img->surface);
    img->dropped = 1;
  }
}

size_t js_image_release_uploaded(int *count) {
  size_t bytes = 0;
  int n = 0;
  for (ImageData *img = all_images; img; img = img->next) {
    if (!img->loaded || !img->uploaded || img->used_2d || img->dropped || !img->surface.px) continue;
    size_t b = (size_t)img->surface.w * (size_t)img->surface.h * 4;
    if (b < IMAGE_DROP_MIN_BYTES) continue;
    surface_free(&img->surface);
    img->dropped = 1;
    bytes += b;
    n++;
  }
  if (count) *count = n;
  return bytes;
}

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
  int       scale;    // [game] image_scale: se reduce si es grande (gfx/image_scale.h)
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
  double t = now_ms();
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
  j->ms = now_ms() - t;
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
static int image_decode_async(JSContext *ctx, JSValueConst obj, ImageData *img, const char *src) {
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

static int image_decode(JSContext *ctx, ImageData *img, const char *src) {
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

static JSValue js_image_fire_load(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv,
                                  int magic, JSValue *data) {
  ImageData *img = JS_GetOpaque(data[0], js_image_class_id);
  if (img && img->loaded) {
    img->fired = 1;
    JSValue ev = events_new(ctx, "load");
    events_dispatch(ctx, data[0], ev);
    JS_FreeValue(ctx, ev);
  }
  return JS_UNDEFINED;
}

static JSValue js_image_get_src(JSContext *ctx, JSValueConst this_val) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;
  return JS_NewString(ctx, img->src ? img->src : "");
}

static JSValue js_image_set_src(JSContext *ctx, JSValueConst this_val, JSValueConst val) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;

  const char *src = JS_ToCString(ctx, val);
  if (!src) return JS_EXCEPTION;

  free(img->src);
  img->src = strdup(src);
  surface_free(&img->surface);
  img->loaded = 0;
  img->dropped = 0;
  img->used_2d = 0;
  img->uploaded = 0;
  img->nat_w = img->nat_h = 0;
  free(img->enc);
  img->enc = NULL;
  img->enc_len = 0;

  if (!*src) {
    JS_FreeCString(ctx, src);
    return JS_UNDEFINED;
  }

  img->fired = 0;
  img->gen++;
  int async = image_decode_async(ctx, this_val, img, src);
  if (async == 0) {
    JS_FreeCString(ctx, src);
    return JS_UNDEFINED;
  }
  if (async == 1 && image_decode(ctx, img, src) == 0) {
    // ya se puede dibujar (PIXI lo hace con data URIs sin esperar a load),
    // pero complete no es true hasta el evento load, como en el navegador
    img->loaded = 1;
    if (!img->nat_w) {   // si no se redujo (image_decode lo pone si si)
      img->nat_w = img->surface.w;
      img->nat_h = img->surface.h;
    }
    if (strncmp(src, "blob:", 5) == 0) {
      size_t len = 0;
      const uint8_t *data = blob_lookup(src, &len);
      if (data && (img->enc = malloc(len ? len : 1))) {
        memcpy(img->enc, data, len);
        img->enc_len = len;
      }
    }
    JSValue data[1] = { JS_DupValue(ctx, this_val) };
    JSValue fn = JS_NewCFunctionData(ctx, js_image_fire_load, 0, 0, 1, data);
    timer_set(ctx, fn, 0, 0, 0, NULL);
    JS_FreeValue(ctx, fn);
    JS_FreeValue(ctx, data[0]);
  } else {
    fprintf(stderr, "[Image] failed to load: %.120s\n", src);
    events_dispatch_later(ctx, this_val, "error");
  }
  JS_FreeCString(ctx, src);
  return JS_UNDEFINED;
}

// magic 0: width, 1: height, 2: naturalWidth, 3: naturalHeight
static JSValue js_image_get_size(JSContext *ctx, JSValueConst this_val, int magic) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;
  int natural = magic == 0 || magic == 2 ? img->nat_w : img->nat_h;
  if (!img->loaded) natural = 0;
  if (magic == 0 && img->width >= 0) return JS_NewInt32(ctx, img->width);
  if (magic == 1 && img->height >= 0) return JS_NewInt32(ctx, img->height);
  return JS_NewInt32(ctx, natural);
}

static JSValue js_image_set_size(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;
  int v = 0;
  if (JS_ToInt32(ctx, &v, val)) return JS_EXCEPTION;
  if (magic == 0) img->width = v;
  if (magic == 1) img->height = v;
  return JS_UNDEFINED;
}

static JSValue js_image_get_complete(JSContext *ctx, JSValueConst this_val) {
  ImageData *img = JS_GetOpaque2(ctx, this_val, js_image_class_id);
  if (!img) return JS_EXCEPTION;
  // sin src tambien es "complete" en el navegador
  return JS_NewBool(ctx, img->fired || !img->src || !*img->src);
}

static JSValue js_image_decode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  // img.decode(): la imagen ya esta decodificada
  JSValue resolving[2];
  JSValue promise = JS_NewPromiseCapability(ctx, resolving);
  JSValue r = JS_Call(ctx, resolving[0], JS_UNDEFINED, 0, NULL);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, resolving[0]);
  JS_FreeValue(ctx, resolving[1]);
  return promise;
}

static const JSCFunctionListEntry js_image_proto_funcs[] = {
  JS_CGETSET_DEF("src", js_image_get_src, js_image_set_src),
  JS_CGETSET_MAGIC_DEF("width",  js_image_get_size, js_image_set_size, 0),
  JS_CGETSET_MAGIC_DEF("height", js_image_get_size, js_image_set_size, 1),
  JS_CGETSET_MAGIC_DEF("naturalWidth",  js_image_get_size, NULL, 2),
  JS_CGETSET_MAGIC_DEF("naturalHeight", js_image_get_size, NULL, 3),
  JS_CGETSET_DEF("complete", js_image_get_complete, NULL),
  JS_CFUNC_DEF("decode", 0, js_image_decode),
};

static JSValue image_new(JSContext *ctx, JSValueConst proto) {
  JSValue obj = JS_NewObjectProtoClass(ctx, proto, js_image_class_id);
  if (JS_IsException(obj)) return obj;

  ImageData *img = js_mallocz(ctx, sizeof(ImageData));
  if (!img) {
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
  }
  img->width = -1;
  img->height = -1;
  img->next = all_images;
  if (all_images) all_images->prev = img;
  all_images = img;
  JS_SetOpaque(obj, img);

  element_setup(ctx, obj, "img");
  // element_setup no sabe que es una imagen: sin estilo de tamano fijo
  JS_SetPropertyStr(ctx, obj, "crossOrigin", JS_NULL);
  JS_SetPropertyStr(ctx, obj, "onload", JS_NULL);
  JS_SetPropertyStr(ctx, obj, "onerror", JS_NULL);
  return obj;
}

JSValue js_create_image(JSContext *ctx) {
  JSValue proto = JS_GetClassProto(ctx, js_image_class_id);
  JSValue obj = image_new(ctx, proto);
  JS_FreeValue(ctx, proto);
  return obj;
}

static JSValue js_image_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if (JS_IsException(proto)) return proto;
  JSValue obj = image_new(ctx, proto);
  JS_FreeValue(ctx, proto);
  if (JS_IsException(obj)) return obj;

  // new Image(width, height)
  if (argc > 0 && !JS_IsUndefined(argv[0])) JS_SetPropertyStr(ctx, obj, "width", JS_DupValue(ctx, argv[0]));
  if (argc > 1 && !JS_IsUndefined(argv[1])) JS_SetPropertyStr(ctx, obj, "height", JS_DupValue(ctx, argv[1]));
  return obj;
}

void js_api_add_image(JSContext *ctx) {
  JS_NewClassID(&js_image_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_image_class_id, &js_image_class);

  JSValue element_proto = js_element_proto(ctx);
  JSValue proto = JS_NewObjectProto(ctx, element_proto);
  JS_FreeValue(ctx, element_proto);
  JS_SetPropertyFunctionList(ctx, proto, js_image_proto_funcs, countof(js_image_proto_funcs));

  JSValue ctor = JS_NewCFunction2(ctx, js_image_ctor, "Image", 2, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, ctor, proto);
  JS_SetClassProto(ctx, js_image_class_id, proto);

  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, "Image", JS_DupValue(ctx, ctor));
  JS_SetPropertyStr(ctx, global, "HTMLImageElement", ctor);
  JS_FreeValue(ctx, global);
}
