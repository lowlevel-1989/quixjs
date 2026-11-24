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

// platform/android/menu.c: el JNI de GameMenuActivity (el menu de un juego
// en Android): la cabecera, las opciones del menu, los logros, Options, los
// mods y los trucos. El juego lo abre game.c.

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game/achievements/achievements.h"
#include "game/cheats/cheats.h"
#include "platform/android/game.h"
#include "transfer/saves/saves.h"
#include "version.h"

#define MENU(name) Java_com_lowlevel1989_quixjs_GameMenuActivity_##name

JNIEXPORT jboolean JNICALL MENU(open)(JNIEnv *env, jclass type, jstring dir) {
  (void)type;
  return game_open(env, dir) ? JNI_TRUE : JNI_FALSE;
}

// {titulo, "quixjs <version>"}
JNIEXPORT jobjectArray JNICALL MENU(header)(JNIEnv *env, jclass type) {
  (void)type;
  const char *values[] = {game_title, "quixjs " QUIX_VERSION};
  return jni_strings(env, values, 2);
}

// Las opciones del menu, en el orden del launcher (launcher_run): Play,
// Achievements, Mods, Options, Cheats (no en hardcore), Transfer Save y
// Controls (si [controls] gamepad). Exit no: en Android se vuelve con Atras
JNIEXPORT jobjectArray JNICALL MENU(menu)(JNIEnv *env, jclass type) {
  (void)type;
  const char *items[8];
  int n = 0;
  items[n++] = "Play";
  if (game_loaded && achievements_count() > 0) items[n++] = "Achievements";
  char names[MAX_MOD_DIRS][64];
  if (game_loaded && settings_mod_dirs(game_cfg.mods.root, names) > 0) items[n++] = "Mods";
  items[n++] = "Options";
  if (game_loaded && cheats_count() > 0 && !game_settings.hardcore) items[n++] = "Cheats";
  if (game_loaded && saves_configured()) items[n++] = "Transfer Save";
  if (game_loaded && game_cfg.controls.gamepad) items[n++] = "Controls";
  return jni_strings(env, items, n);
}

// ---------- Achievements

// {titulo, descripcion, tipo, color (0xRRGGBB en decimal), fecha o "",
// "1" si se consiguio en hardcore o ""} por logro
JNIEXPORT jobjectArray JNICALL MENU(achievements)(JNIEnv *env, jclass type) {
  (void)type;
  int n = game_loaded ? achievements_count() : 0;
  const char **values = calloc((size_t)n * 6 + 1, sizeof *values);
  char (*colors)[16] = calloc((size_t)n + 1, sizeof *colors);
  char (*dates)[32] = calloc((size_t)n + 1, sizeof *dates);
  char (*types)[40] = calloc((size_t)n + 1, sizeof *types);
  for (int i = 0; i < n; i++) {
    snprintf(colors[i], sizeof colors[0], "%u", achievements_color(i));
    if (achievements_unlocked(i)) saves_date_text((time_t)achievements_unlocked(i), dates[i], sizeof dates[0]);
    // el tipo, legible: win_condition -> win condition
    snprintf(types[i], sizeof types[0], "%s", achievements_type(i));
    for (char *p = types[i]; *p; p++) if (*p == '_') *p = ' ';
    values[6 * i] = achievements_title(i);
    values[6 * i + 1] = achievements_description(i);
    values[6 * i + 2] = types[i];
    values[6 * i + 3] = colors[i];
    values[6 * i + 4] = dates[i];
    values[6 * i + 5] = achievements_hardcore(i) ? "1" : "";
  }
  jobjectArray out = jni_strings(env, values, 6 * n);
  free(values);
  free(colors);
  free(dates);
  free(types);
  return out;
}

JNIEXPORT void JNICALL MENU(resetAchievements)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  if (game_loaded) achievements_reset();
}

// ---------- Options

// el FPS que se usa: el de Options o, sin el, el de [engine] fps
static int fps_now(void) {
  int fps = game_settings.fps ? game_settings.fps : game_cfg.engine.fps;
  return fps == 30 ? 30 : 60;
}

// {show_fps, profile, vibration, el juego vibra ([controls] vibration), a 30 fps,
// logros en hardcore}
JNIEXPORT jbooleanArray JNICALL MENU(options)(JNIEnv *env, jclass type) {
  (void)type;
  jboolean values[] = {game_settings.show_fps ? JNI_TRUE : JNI_FALSE, game_settings.profile ? JNI_TRUE : JNI_FALSE,
                       game_settings.no_vibration ? JNI_FALSE : JNI_TRUE, game_cfg.controls.vibration ? JNI_TRUE : JNI_FALSE,
                       fps_now() == 30 ? JNI_TRUE : JNI_FALSE, game_settings.hardcore ? JNI_TRUE : JNI_FALSE};
  jbooleanArray out = (*env)->NewBooleanArray(env, 6);
  (*env)->SetBooleanArrayRegion(env, out, 0, 6, values);
  return out;
}

// 0 Show FPS, 1 Profiler, 2 Vibration, 3 FPS (30 o 60), 4 Achievements
// (Normal o Hardcore: en hardcore, ningun truco): cambia y se guarda enseguida
JNIEXPORT void JNICALL MENU(toggleOption)(JNIEnv *env, jclass type, jint which) {
  (void)env, (void)type;
  if (!game_loaded) return;
  if (which == 0) game_settings.show_fps = !game_settings.show_fps;
  else if (which == 1) game_settings.profile = !game_settings.profile;
  else if (which == 3) game_settings.fps = fps_now() == 30 ? 60 : 30;
  else if (which == 4) {
    game_settings.hardcore = !game_settings.hardcore;
    settings_apply_cheats(&game_settings);
  }
  else game_settings.no_vibration = !game_settings.no_vibration;
  settings_save(&game_settings, game_config_path);
}

// ---------- Mods

// {nombre, su numero si esta activo o ""} por carpeta de mods/
JNIEXPORT jobjectArray JNICALL MENU(mods)(JNIEnv *env, jclass type) {
  (void)type;
  static char names[MAX_MOD_DIRS][64], numbers[MAX_MOD_DIRS][8];
  const char *values[MAX_MOD_DIRS * 2];
  int n = game_loaded ? settings_mod_dirs(game_cfg.mods.root, names) : 0;
  for (int i = 0; i < n; i++) {
    // hasta que se toca, los de [game] mods
    int k = -1;
    if (game_settings.mods_set) k = settings_mod_index(&game_settings, names[i]);
    else {
      for (int j = 0; j < game_cfg.mods.count; j++) {
        if (strcmp(game_cfg.mods.names[j], names[i]) == 0) k = j;
      }
    }
    if (k >= 0) snprintf(numbers[i], sizeof numbers[0], "%d", k + 1);
    else numbers[i][0] = 0;
    values[2 * i] = names[i];
    values[2 * i + 1] = numbers[i];
  }
  return jni_strings(env, values, 2 * n);
}

JNIEXPORT void JNICALL MENU(toggleMod)(JNIEnv *env, jclass type, jstring name) {
  (void)type;
  if (!game_loaded) return;
  const char *text = (*env)->GetStringUTFChars(env, name, NULL);
  settings_toggle_mod(&game_settings, (const char (*)[64])game_cfg.mods.names, game_cfg.mods.count, text);
  (*env)->ReleaseStringUTFChars(env, name, text);
  settings_save(&game_settings, game_config_path);
}

// ---------- Cheats

// {nombre, descripcion, "1" o "0"} por truco
JNIEXPORT jobjectArray JNICALL MENU(cheats)(JNIEnv *env, jclass type) {
  (void)type;
  const char *values[32 * 3];
  int n = game_loaded ? cheats_count() : 0;
  if (n > 32) n = 32;
  for (int i = 0; i < n; i++) {
    values[3 * i] = cheats_name(i);
    values[3 * i + 1] = cheats_description(i);
    values[3 * i + 2] = settings_cheat_index(&game_settings, cheats_id(i)) >= 0 ? "1" : "0";
  }
  return jni_strings(env, values, 3 * n);
}

JNIEXPORT void JNICALL MENU(toggleCheat)(JNIEnv *env, jclass type, jint i) {
  (void)env, (void)type;
  // en hardcore no hay trucos (el menu no ofrece Cheats)
  if (!game_loaded || game_settings.hardcore || i < 0 || i >= cheats_count()) return;
  settings_toggle_cheat(&game_settings, cheats_id(i));
  settings_save(&game_settings, game_config_path);
}
