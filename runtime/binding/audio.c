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

// WebAudio: las funciones nativas (__quixAudio) y el JS que va encima. Ver
// audio.h.

#include <stdio.h>
#include <string.h>
#include "audio/mixer.h"
#include "core/app_context.h"
#include "core/url.h"
#include "core/worker.h"
#include "audio.h"

static JSClassID js_clip_class_id;
static JSValue   tick_fn = { 0 };
static int       tick_ready = 0;

static void js_clip_finalizer(JSRuntime *rt, JSValue val) {
  clip_release(JS_GetOpaque(val, js_clip_class_id));
}

static JSClassDef js_clip_class = {
  "QuixAudioClip",
  .finalizer = js_clip_finalizer,
};

// El objeto de JS de un clip (null si no hay clip)
static JSValue clip_object(JSContext *ctx, AudioClip *c) {
  if (!c) return JS_NULL;
  JSValue obj = JS_NewObjectClass(ctx, js_clip_class_id);
  if (JS_IsException(obj)) {
    clip_release(c);
    return obj;
  }
  JS_SetOpaque(obj, c);
  JS_SetPropertyStr(ctx, obj, "sampleRate", JS_NewInt32(ctx, clip_rate(c)));
  JS_SetPropertyStr(ctx, obj, "numberOfChannels", JS_NewInt32(ctx, clip_channels(c)));
  JS_SetPropertyStr(ctx, obj, "length", JS_NewInt64(ctx, clip_length(c)));
  JS_SetPropertyStr(ctx, obj, "duration", JS_NewFloat64(ctx, (double)clip_length(c) / clip_rate(c)));
  JS_SetPropertyStr(ctx, obj, "loopStart", JS_NewInt64(ctx, clip_loop_start(c)));
  JS_SetPropertyStr(ctx, obj, "loopLength", JS_NewInt64(ctx, clip_loop_length(c)));
  return obj;
}

// decode(ArrayBuffer | vista) -> clip { duration, sampleRate, length,
// numberOfChannels } | null
static JSValue js_audio_decode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_NULL;
  size_t len = 0;
  uint8_t *data = JS_GetBufferBytes(ctx, argv[0], &len);
  if (!data) return JS_NULL;
  return clip_object(ctx, clip_from_ogg(data, len));
}

// ---------- cargar un sonido desde su archivo, en segundo plano ----------
//
// loadFile(url, cb): el thread de trabajo lee el archivo (descifrandolo si su
// extension tiene transformacion: core/url.h) y abre el ogg; despues, en el
// thread principal, cb(clip | null). Asi los bytes no pasan por el JS (un
// buffer de varios MB por sonido, que disparaba GC).

typedef struct {
  char      *path;
  JSValue    cb;
  AudioClip *clip;
} LoadJob;

static void load_work(void *arg) {
  LoadJob *j = arg;
  size_t len = 0;
  uint8_t *data = url_read_file_transformed(j->path, &len);
  j->clip = data ? clip_from_ogg_owned(data, len) : NULL;
}

static void load_done(JSContext *ctx, void *arg) {
  LoadJob *j = arg;
  JSValue clip = clip_object(ctx, j->clip);
  JSValue r = JS_Call(ctx, j->cb, JS_UNDEFINED, 1, &clip);
  if (JS_IsException(r)) js_std_dump_error(ctx);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, clip);
  JS_FreeValue(ctx, j->cb);
  free(j->path);
  free(j);
}

static JSValue js_audio_load_file(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 2 || !JS_IsFunction(ctx, argv[1])) return JS_FALSE;
  const char *url = JS_ToCString(ctx, argv[0]);
  if (!url) return JS_FALSE;
  AppContext *app = JS_GetContextOpaque(ctx);
  char path[1024];
  int kind = url_resolve(app->cfg->game.game_dir, url, path, sizeof(path));
  JS_FreeCString(ctx, url);
  if (kind != URL_FILE) return JS_FALSE;
  LoadJob *j = calloc(1, sizeof(LoadJob));
  if (!j) return JS_FALSE;
  j->path = strdup(path);
  j->cb = JS_DupValue(ctx, argv[1]);
  if (worker_submit(load_work, load_done, j) != 0) {
    load_work(j);   // sin thread: al momento
    load_done(ctx, j);
  }
  return JS_TRUE;
}

// play(clip, offset, loop, loopStart, loopEnd, rate) -> id (-1 si no suena)
static JSValue js_audio_play(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AudioClip *c = argc > 0 ? JS_GetOpaque(argv[0], js_clip_class_id) : NULL;
  if (!c) return JS_NewInt32(ctx, -1);
  double offset = 0, ls = 0, le = 0, rate = 1;
  if (argc > 1) JS_ToFloat64(ctx, &offset, argv[1]);
  int loop = argc > 2 && JS_ToBool(ctx, argv[2]);
  if (argc > 3) JS_ToFloat64(ctx, &ls, argv[3]);
  if (argc > 4) JS_ToFloat64(ctx, &le, argv[4]);
  if (argc > 5) JS_ToFloat64(ctx, &rate, argv[5]);
  return JS_NewInt32(ctx, voice_play(c, offset, loop, ls, le, (float)rate));
}

// set(id, volume, pan, rate, loop)
static JSValue js_audio_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  double vol = 1, pan = 0, rate = 1;
  if (argc < 5 || JS_ToInt32(ctx, &id, argv[0])) return JS_UNDEFINED;
  JS_ToFloat64(ctx, &vol, argv[1]);
  JS_ToFloat64(ctx, &pan, argv[2]);
  JS_ToFloat64(ctx, &rate, argv[3]);
  voice_set(id, (float)vol, (float)pan, (float)rate, JS_ToBool(ctx, argv[4]));
  return JS_UNDEFINED;
}

static JSValue js_audio_stop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  if (argc > 0 && !JS_ToInt32(ctx, &id, argv[0])) voice_stop(id);
  return JS_UNDEFINED;
}

static JSValue js_audio_playing(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  if (argc < 1 || JS_ToInt32(ctx, &id, argv[0])) return JS_FALSE;
  return JS_NewBool(ctx, voice_playing(id));
}

static JSValue js_audio_time(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  if (argc < 1 || JS_ToInt32(ctx, &id, argv[0])) return JS_NewFloat64(ctx, 0);
  return JS_NewFloat64(ctx, voice_time(id));
}

static const JSCFunctionListEntry js_audio_funcs[] = {
  JS_CFUNC_DEF("decode",  1, js_audio_decode),
  JS_CFUNC_DEF("loadFile", 2, js_audio_load_file),
  JS_CFUNC_DEF("play",    6, js_audio_play),
  JS_CFUNC_DEF("set",     5, js_audio_set),
  JS_CFUNC_DEF("stop",    1, js_audio_stop),
  JS_CFUNC_DEF("playing", 1, js_audio_playing),
  JS_CFUNC_DEF("time",    1, js_audio_time),
};

// WebAudio en JS sobre __quixAudio runtime/binding/webaudio.js:
// tools/embed_js.py lo mete en el binario al compilar; js_webaudio.h (en
// obj/gen/) define WEBAUDIO_JS, su texto. Ver docs/embedded-js.md.
#include "js_webaudio.h"

int js_api_add_audio(JSContext *ctx, int device) {
  JS_NewClassID(&js_clip_class_id);
  JS_NewClass(JS_GetRuntime(ctx), js_clip_class_id, &js_clip_class);
  JSValue proto = JS_NewObject(ctx);
  JS_SetClassProto(ctx, js_clip_class_id, proto);

  mixer_init(device);

  JSValue global = JS_GetGlobalObject(ctx);
  JSValue a = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, a, js_audio_funcs, countof(js_audio_funcs));
  // runtime/binding/webaudio.js: lo que necesita, en las globales temporales
  // __quixAudio y __quixAudioDevice; el archivo se llama a si mismo con ellas
  // al final y las borra. Deja __quixAudioTick, que se guarda aqui (tick_fn)
  // para audio_tick
  JS_SetPropertyStr(ctx, global, "__quixAudio", a);
  JS_SetPropertyStr(ctx, global, "__quixAudioDevice", JS_NewBool(ctx, mixer_has_device()));

  JSValue r = JS_Eval(ctx, WEBAUDIO_JS, strlen(WEBAUDIO_JS), "<quix-webaudio>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) {
    js_std_dump_error(ctx);
    JS_FreeValue(ctx, global);
    return -1;
  }
  JS_FreeValue(ctx, r);

  tick_fn = JS_GetPropertyStr(ctx, global, "__quixAudioTick");
  tick_ready = 1;
  JS_FreeValue(ctx, global);
  return 0;
}

void audio_tick(JSContext *ctx) {
  if (tick_ready && JS_IsFunction(ctx, tick_fn)) {
    JSValue r = JS_Call(ctx, tick_fn, JS_UNDEFINED, 0, NULL);
    if (JS_IsException(r)) js_std_dump_error(ctx);
    JS_FreeValue(ctx, r);
  }
  mixer_update();
}

void audio_free(JSContext *ctx) {
  if (tick_ready) JS_FreeValue(ctx, tick_fn);
  tick_ready = 0;
}
