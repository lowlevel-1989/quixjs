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
#include "document.h"
#include "events.h"
#include "image.h"

typedef struct {
  Surface surface;
  int     loaded;
  int     fired;     // ya se envio el evento load (complete = true)
  int     width;     // width/height asignados a mano (-1 = los de la imagen)
  int     height;
  char   *src;
  unsigned gen;      // cambia con cada src: un resultado de un src anterior se descarta
} ImageData;

static JSClassID js_image_class_id;

static void js_image_finalizer(JSRuntime *rt, JSValue val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img) return;
  surface_free(&img->surface);
  free(img->src);
  js_free_rt(rt, img);
}

static JSClassDef js_image_class = {
  "HTMLImageElement",
  .finalizer = js_image_finalizer,
};

const Surface *js_image_get_surface(JSContext *ctx, JSValueConst val) {
  ImageData *img = JS_GetOpaque(val, js_image_class_id);
  if (!img || !img->loaded) return NULL;
  return &img->surface;
}

// ---------- decodificar en segundo plano ----------
//
// Las imagenes de archivo o de Blob (las del juego) se decodifican en un hilo
// aparte: un PNG grande son decenas de ms en una portatil, y en el hilo
// principal eran una caida (aparece un enemigo, cambia el mapa). El hilo solo
// toca bytes y pixeles; image_poll, en el hilo principal, pone la imagen como
// cargada y lanza load. Las data: URIs (pequenas; PIXI las dibuja sin esperar
// a load) siguen siendo inmediatas.

typedef struct {
  JSValue   obj;      // la imagen (no se toca fuera del hilo principal)
  unsigned  gen;
  uint8_t  *data;     // los bytes (copia) o
  size_t    len;
  char     *path;     // el archivo (con su transformacion, si tiene: descifrar)
  uint8_t  *px;       // el resultado
  int       w, h, ok;
  double    ms;
} ImageJob;

static double now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// en el hilo de trabajo
static void image_work(void *arg) {
  ImageJob *j = arg;
  double t = now_ms();
  if (j->path && url_has_transform(j->path)) {
    j->data = url_read_file_transformed(j->path, &j->len);
    free(j->path);
    j->path = NULL;
  }
  j->ok = (j->data || j->path) && image_decode_pixels(j->data, j->len, j->path, &j->px, &j->w, &j->h) == 0;
  j->ms = now_ms() - t;
  free(j->data);
  j->data = NULL;
}

// en el hilo principal
static void image_done(JSContext *ctx, void *arg) {
  ImageJob *j = arg;
  ImageData *img = JS_GetOpaque(j->obj, js_image_class_id);
  if (img && img->gen == j->gen) {
    JSValue ev;
    if (j->ok) {
      surface_adopt(&img->surface, j->px, j->w, j->h);
      j->px = NULL;
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
  } else {
    char path[1024];
    int kind = url_resolve(app->cfg->game.game_dir, src, path, sizeof(path));
    if (kind != URL_FILE) {
      free(j);
      return kind == URL_DATA ? 1 : -1;
    }
    j->path = strdup(path);
  }
  j->obj = JS_DupValue(ctx, obj);
  j->gen = img->gen;
  if (worker_submit(image_work, image_done, j) != 0) {
    JS_FreeValue(ctx, j->obj);
    free(j->data);
    free(j->path);
    free(j);
    return 1;   // sin hilo: al momento
  }
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
  int natural = magic == 0 || magic == 2 ? img->surface.w : img->surface.h;
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
