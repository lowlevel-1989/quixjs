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

// platform/android/game.h: el juego abierto por las pantallas de Android
// (GameMenuActivity y TransferActivity). game.c lo abre con lo mismo que
// quixjs al arrancar; menu.c y transfer.c lo usan.
#pragma once

#include <jni.h>
#include "core/config/loader.h"
#include "launcher/settings.h"

extern Config game_cfg;
extern Settings game_settings;
extern char game_config_path[1024];
extern char game_title[128], game_id[128];
extern int game_loaded;   // 1 si game_open fue bien

// El juego de la carpeta folder ({files}/games/<id>). 1 si va
int game_open(JNIEnv *env, jstring folder);

// Un String[] de Java con values (NULL cuenta como ""), y un String o null
jobjectArray jni_strings(JNIEnv *env, const char *const *values, int n);
jstring jni_string(JNIEnv *env, const char *value);
