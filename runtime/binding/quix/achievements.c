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

// runtime/binding/quix/achievements.c: quix.achievements (los logros del
// port: la lista, si uno ya se consiguio y el icono del menu) y quix.steam
// (lo que el juego pide a Steam, que leen los [symbols]).

#include <stdio.h>
#include "core/app_context.h"
#include "game/achievements/achievements.h"
#include "common.h"

// quix.achievements.unlocked(id): si el logro del toml del juego ya se consiguio
static JSValue js_quix_achievements_unlocked(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_FALSE;
  const char *id = JS_ToCString(ctx, argv[0]);
  if (!id) return JS_EXCEPTION;
  int r = achievements_unlocked_by_id(id);
  JS_FreeCString(ctx, id);
  return JS_NewBool(ctx, r);
}

// quix.achievements.list(): los logros del port, en su orden del toml:
// [{ id, title, description, type, color ("#rrggbb"), unlocked (fecha unix
// en segundos, 0 si no), hardcore (conseguido en hardcore) }]. Para verlos
// dentro del juego (hook rpgmaker-mv/port/achievements-menu)
static JSValue js_quix_achievements_list(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  JSValue list = JS_NewArray(ctx);
  for (int i = 0; i < achievements_count(); i++) {
    JSValue o = JS_NewObject(ctx);
    char color[8];
    snprintf(color, sizeof color, "#%06x", achievements_color(i) & 0xffffff);
    JS_SetPropertyStr(ctx, o, "id", JS_NewString(ctx, achievements_id(i)));
    JS_SetPropertyStr(ctx, o, "title", JS_NewString(ctx, achievements_title(i)));
    JS_SetPropertyStr(ctx, o, "description", JS_NewString(ctx, achievements_description(i) ? achievements_description(i) : ""));
    JS_SetPropertyStr(ctx, o, "type", JS_NewString(ctx, achievements_type(i) ? achievements_type(i) : ""));
    JS_SetPropertyStr(ctx, o, "color", JS_NewString(ctx, color));
    JS_SetPropertyStr(ctx, o, "unlocked", JS_NewInt64(ctx, achievements_unlocked(i)));
    JS_SetPropertyStr(ctx, o, "hardcore", JS_NewBool(ctx, achievements_hardcore(i)));
    JS_SetPropertyUint32(ctx, list, i, o);
  }
  return list;
}

// quix.achievements.icon: [achievements_menu] icon del toml (0 si no hay)
static JSValue js_quix_achievementsIcon(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewInt64(ctx, achievements_menu_icon());
}

static JSValue js_quix_achievements_get_icon(JSContext *ctx, JSValueConst this_val) {
  return js_quix_achievementsIcon(ctx, this_val, 0, NULL);
}

static const JSCFunctionListEntry js_quix_achievements_funcs[] = {
  JS_CFUNC_DEF("list", 0, js_quix_achievements_list),
  JS_CFUNC_DEF("unlocked", 1, js_quix_achievements_unlocked),
  JS_CGETSET_DEF("icon", js_quix_achievements_get_icon, NULL),
};

void quix_add_achievements(JSContext *ctx, JSValueConst quix) {
  quix_add_group(ctx, quix, "achievements", js_quix_achievements_funcs, countof(js_quix_achievements_funcs));
  // quix.steam: los logros y estadisticas que el juego pide a Steam
  // (runtime/binding/nwjs/greenworks.js, o un script del port); los [symbols] del toml los leen
  // ("path:quix.steam.achievements.ID")
  JSValue steam = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, steam, "achievements", JS_NewObject(ctx));
  JS_SetPropertyStr(ctx, steam, "stats", JS_NewObject(ctx));
  // el nombre del jugador ([game] player): getSteamId().screenName
  AppContext *app = JS_GetContextOpaque(ctx);
  JS_SetPropertyStr(ctx, steam, "player", JS_NewString(ctx, app && app->cfg ? app->cfg->game.player : "Master"));
  JS_SetPropertyStr(ctx, quix, "steam", steam);
}
