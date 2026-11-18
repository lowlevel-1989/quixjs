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

// __quixVideo, sobre runtime/video. Ver video.h.

#include <stdio.h>
#include <string.h>
#include "core/app_context.h"
#include "core/url.h"
#include "video/video.h"
#include "video.h"

#define MAX_VIDEOS 4

typedef struct {
  Video  *video;
  Surface frame;
  int     visible;
  int     changed;
} Slot;

static Slot slots[MAX_VIDEOS];

static Slot *slot_of(JSContext *ctx, JSValueConst v) {
  int32_t id = -1;
  if (JS_ToInt32(ctx, &id, v) != 0 || id < 0 || id >= MAX_VIDEOS || !slots[id].video) return NULL;
  return &slots[id];
}

static void slot_free(Slot *s) {
  video_close(s->video);
  surface_free(&s->frame);
  memset(s, 0, sizeof(*s));
}

// open(url) -> id, o -1
static JSValue js_video_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1 || !video_supported()) return JS_NewInt32(ctx, -1);
  const char *url = JS_ToCString(ctx, argv[0]);
  if (!url) return JS_EXCEPTION;
  AppContext *app = JS_GetContextOpaque(ctx);
  char path[1024];
  int id = -1;
  if (url_resolve(app->cfg->game.game_dir, url, path, sizeof(path)) == URL_FILE) {
    for (int i = 0; i < MAX_VIDEOS; i++) {
      if (slots[i].video) continue;
      slots[i].video = video_open(path);
      if (slots[i].video) {
        surface_init(&slots[i].frame, 0, 0);
        id = i;
      }
      break;
    }
  }
  JS_FreeCString(ctx, url);
  return JS_NewInt32(ctx, id);
}

// info(id) -> { width, height, duration }
static JSValue js_video_info(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Slot *s = argc > 0 ? slot_of(ctx, argv[0]) : NULL;
  if (!s) return JS_NULL;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "width", JS_NewInt32(ctx, video_width(s->video)));
  JS_SetPropertyStr(ctx, o, "height", JS_NewInt32(ctx, video_height(s->video)));
  JS_SetPropertyStr(ctx, o, "duration", JS_NewFloat64(ctx, video_duration(s->video)));
  return o;
}

// state(id) -> { time, ended, frames }
static JSValue js_video_state(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Slot *s = argc > 0 ? slot_of(ctx, argv[0]) : NULL;
  if (!s) return JS_NULL;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "time", JS_NewFloat64(ctx, video_time(s->video)));
  JS_SetPropertyStr(ctx, o, "ended", JS_NewBool(ctx, video_ended(s->video)));
  JS_SetPropertyStr(ctx, o, "frames", JS_NewInt64(ctx, video_decoded_frames(s->video)));
  return o;
}

// magic: 0 play, 1 pause, 2 close
static JSValue js_video_cmd(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) {
  Slot *s = argc > 0 ? slot_of(ctx, argv[0]) : NULL;
  if (!s) return JS_UNDEFINED;
  if (magic == 0) video_play(s->video);
  else if (magic == 1) video_pause(s->video);
  else slot_free(s);
  return JS_UNDEFINED;
}

static JSValue js_video_volume(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Slot *s = argc > 1 ? slot_of(ctx, argv[0]) : NULL;
  double vol = 1;
  if (s && JS_ToFloat64(ctx, &vol, argv[1]) == 0) video_set_volume(s->video, (float)vol);
  return JS_UNDEFINED;
}

static JSValue js_video_visible(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  Slot *s = argc > 1 ? slot_of(ctx, argv[0]) : NULL;
  if (s) {
    s->visible = JS_ToBool(ctx, argv[1]);
    s->changed = 1;
  }
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_video_funcs[] = {
  JS_CFUNC_DEF("open", 1, js_video_open),
  JS_CFUNC_DEF("info", 1, js_video_info),
  JS_CFUNC_DEF("state", 1, js_video_state),
  JS_CFUNC_MAGIC_DEF("play", 1, js_video_cmd, 0),
  JS_CFUNC_MAGIC_DEF("pause", 1, js_video_cmd, 1),
  JS_CFUNC_MAGIC_DEF("close", 1, js_video_cmd, 2),
  JS_CFUNC_DEF("volume", 2, js_video_volume),
  JS_CFUNC_DEF("visible", 2, js_video_visible),
};

void js_api_add_video(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, o, js_video_funcs, sizeof(js_video_funcs) / sizeof(js_video_funcs[0]));
  JS_SetPropertyStr(ctx, o, "supported", JS_NewBool(ctx, video_supported()));
  JS_SetPropertyStr(ctx, global, "__quixVideo", o);
  JS_FreeValue(ctx, global);
}

void video_tick(void) {
  for (int i = 0; i < MAX_VIDEOS; i++) {
    if (slots[i].video && video_update(slots[i].video, &slots[i].frame)) slots[i].changed = 1;
  }
}

int video_active(void) {
  for (int i = 0; i < MAX_VIDEOS; i++) {
    if (slots[i].video && !video_ended(slots[i].video)) return 1;
  }
  return 0;
}

const Surface *video_screen(int *changed) {
  for (int i = 0; i < MAX_VIDEOS; i++) {
    Slot *s = &slots[i];
    if (s->video && s->visible && s->frame.px && !video_ended(s->video)) {
      *changed = s->changed;
      s->changed = 0;
      return &s->frame;
    }
  }
  return NULL;
}

void video_free_all(void) {
  for (int i = 0; i < MAX_VIDEOS; i++) {
    if (slots[i].video) slot_free(&slots[i]);
  }
}
