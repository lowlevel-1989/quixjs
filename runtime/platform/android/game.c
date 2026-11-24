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

// platform/android/game.c: el launcher de quixjs para las pantallas de
// Android (GameMenuActivity y TransferActivity, android/java). Lo que
// enseñan y lo que cambian sale del mismo C que el launcher de Linux
// (runtime/launcher): el toml del juego (config_load), los logros
// (achievements), los trucos (cheats), quixjs-settings.toml (settings.c) y
// Transfer Save (transfer/sender.c, receiver.c, saves.c). Los archivos son
// los mismos que en una portatil.
//
// Las partes, por pantalla: este archivo abre el juego y pasa textos a Java;
// menu.c es el JNI de GameMenuActivity, transfer.c el de TransferActivity y
// pad.c el de Pad.java (el mando en esas pantallas).
//
// Todo se llama desde el thread de la interfaz, en el proceso de la app (el
// juego va en otro: GameActivity).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "game/achievements/achievements.h"
#include "game/cheats/cheats.h"
#include "platform/android/game.h"
#include "game/symbols/symbols.h"
#include "transfer/saves/saves.h"

Config game_cfg;
Settings game_settings;
char game_config_path[1024];
char game_title[128], game_id[128];
int game_loaded;

static void config_clear(void) {
  free(game_cfg.scripts.entry);
  for (int i = 0; i < game_cfg.scripts.libs_count; i++) free(game_cfg.scripts.libs[i]);
  for (int i = 0; i < game_cfg.scripts.includes_count; i++) free(game_cfg.scripts.includes[i]);
  memset(&game_cfg, 0, sizeof game_cfg);
}

// El juego de la carpeta dir ({files}/games/<id>, con port.toml): lo mismo
// que lee quixjs al arrancar (core/main/start.c, load_port), sin el motor de
// JS. Desde dir, como el juego: game_dir y [saves] dir son relativos. 1 si va
int game_open(JNIEnv *env, jstring folder) {
  const char *dir = (*env)->GetStringUTFChars(env, folder, NULL);
  int ok = chdir(dir) == 0;
  snprintf(game_config_path, sizeof game_config_path, "%s/port.toml", dir);
  (*env)->ReleaseStringUTFChars(env, folder, dir);
  if (!ok) return 0;
  config_clear();
  game_loaded = 0;
  if (config_load(&game_cfg, game_config_path) != 0) return 0;
  snprintf(game_title, sizeof game_title, "%s", game_cfg.launcher.title[0] ? game_cfg.launcher.title : game_cfg.engine.name);
  snprintf(game_id, sizeof game_id, "%s", game_cfg.saves.id[0] ? game_cfg.saves.id : game_title);
  // [saves]: las partidas y, con ellas, el archivo de los logros
  const char *patterns[MAX_SAVE_PATTERNS];
  int npatterns = game_cfg.saves.files_count;
  for (int i = 0; i < npatterns; i++) patterns[i] = game_cfg.saves.files[i];
  const char *saves_dir = game_cfg.saves.dir[0] ? game_cfg.saves.dir : game_cfg.game.game_dir;
  const char *ach_file = game_cfg.saves.achievements;
  if (ach_file[0] && ach_file[0] != '/' && npatterns > 0 && npatterns < MAX_SAVE_PATTERNS)
    patterns[npatterns++] = ach_file;
  saves_setup(saves_dir, patterns, npatterns);
  saves_keep(ach_file[0] && ach_file[0] != '/' ? ach_file : NULL);
  symbols_load(game_config_path);
  achievements_load(game_config_path, saves_dir, ach_file, game_id);
  cheats_load(game_config_path);
  settings_load(&game_settings, game_config_path);
  game_loaded = 1;
  return 1;
}

jobjectArray jni_strings(JNIEnv *env, const char *const *values, int n) {
  jclass type = (*env)->FindClass(env, "java/lang/String");
  jobjectArray out = (*env)->NewObjectArray(env, n, type, NULL);
  for (int i = 0; i < n; i++) {
    jstring value = (*env)->NewStringUTF(env, values[i] ? values[i] : "");
    (*env)->SetObjectArrayElement(env, out, i, value);
    (*env)->DeleteLocalRef(env, value);
  }
  return out;
}

jstring jni_string(JNIEnv *env, const char *value) { return value ? (*env)->NewStringUTF(env, value) : NULL; }
