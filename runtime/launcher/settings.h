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

#pragma once

// Los ajustes que el jugador cambia en el launcher (Options): se guardan en
// quixjs-settings.toml, junto al toml del juego, y valen tambien sin
// launcher.
//
//   show_fps = true    el contador de fps arriba a la derecha (como --fps)
//   profile  = true    el profiler: quixjs-profile.txt cada 10 s (como --profile)
//   cheats   = ["no_encounters"]   los trucos activos (ids de [[cheat]])
//   fullscreen = true  a pantalla completa (en el PC; --fullscreen tambien)
//   vibration = false  no vibrar (el mando o el telefono; sin la clave, si)
//   mods     = ["es"]  los mods activos (Mods), en orden: si dos traen el
//                      mismo archivo, gana el ultimo. Sin la clave, los de
//                      [game] mods del toml del juego

#define MAX_SETTING_CHEATS 32
#define MAX_SETTING_MODS 16

typedef struct {
  int show_fps;
  int profile;
  int fullscreen;   // en el PC: la ventana a pantalla completa
  int no_vibration; // Vibration: Off (vibration = false)
  int scaling;           // Scaling: 1 + SCALING_* (backend.h); 0: el de [engine] scaling
  int quality;           // Quality: 1 low, 2 medium, 3 high (0: la de [engine] quality)
  int fps;               // FPS: 30 o 60 (0: el de [engine] fps)
  char cheats[MAX_SETTING_CHEATS][64];   // los trucos activos (Cheats), por id
  int  cheats_count;
  int  mods_set;                          // 1: mods manda sobre [game] mods
  char mods[MAX_SETTING_MODS][64];
  int  mods_count;
} Settings;

void settings_load(Settings *s, const char *config_path);
int  settings_save(const Settings *s, const char *config_path);
// Los trucos activos de s, a runtime/cheats
void settings_apply_cheats(const Settings *s);

// Cheats y Mods: lo que cambia una opcion, igual en el launcher y en la app
// de Android (platform/android/launcher.c). Sin guardar: settings_save.
// La posicion de un truco o un mod activo (por id o nombre), o -1
int  settings_cheat_index(const Settings *s, const char *id);
int  settings_mod_index(const Settings *s, const char *name);
// Lo enciende o lo apaga
void settings_toggle_cheat(Settings *s, const char *id);
// La primera vez parte de los de [game] mods (defaults, count): desde ahi,
// mods manda (mods_set)
void settings_toggle_mod(Settings *s, const char (*defaults)[64], int count, const char *name);

// Las carpetas de mods/ (root), por orden alfabetico: cuantas
#define MAX_MOD_DIRS 32
int  settings_mod_dirs(const char *root, char names[][64]);
