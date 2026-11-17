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

#pragma once

// La configuracion de un juego, leida de su toml (config.toml por defecto; en
// un port, port.toml). Aqui solo esta lo que usa el runtime al arrancar; los
// hooks, los logros, los trucos y [symbols] los leen sus propios modulos.

#include <stdbool.h>

#define MAX_SCRIPT_FILES 64
#define MAX_FONTS        16
#define MAX_PATH_LEN 256

typedef struct {
  char name[64];
  char version[32];
  char canvas_id[64];
  char pixi[32];      // informativo: version de PixiJS que trae el juego
  bool nwjs;          // modo nw.js: require('fs'), process...
  bool webgl;         // getContext("webgl") (por defecto true)
  int  fps;           // imagenes por segundo de requestAnimationFrame (por defecto 60)
  bool vsync;         // esperar al refresco de la pantalla al presentar (por defecto true)
  bool present_ahead; // una imagen de adelanto: la muestra otro hilo (por defecto true)
  int  js_memory;     // MB de RAM que JavaScriptCore cree tener (0: la del aparato)
  int  low_memory_mb; // por debajo de esta RAM libre del sistema, recoleccion completa (0: no)
} EngineConfig;

// [fonts] Familia = "ruta.ttf" (relativa a game_dir): como un @font-face del
// CSS del juego, que quixjs no lee
typedef struct {
  char family[64];
  char path[MAX_PATH_LEN];
} FontConfig;

typedef struct {
  char game_dir[MAX_PATH_LEN];
  char cache_dir[MAX_PATH_LEN];   // sin uso (era la cache de bytecode de QuickJS)
  int width;
  int height;
} GameConfig;

typedef struct {
  char entry[MAX_PATH_LEN];
  char libs[MAX_SCRIPT_FILES][MAX_PATH_LEN];
  int  libs_count;
  char includes[MAX_SCRIPT_FILES][MAX_PATH_LEN];
  int  includes_count;
} ScriptConfig;

// [launcher] (--launcher): el menu de entrada de un port
typedef struct {
  char title[128];        // el titulo del juego (por defecto [engine] name)
  char background[32];    // colores CSS: fondo, texto y la opcion elegida
  char text[32];
  char accent[32];
} LauncherConfig;

// [controls]: lo que espera el juego del mando. gamepad = true: el juego lee
// el mando (la Gamepad API); el lanzador (Controls) deja asignar todos sus
// botones y las dos palancas, y se guarda en controller.map junto al toml del juego
typedef struct {
  int gamepad;
} ControlsConfig;

#define MAX_SAVE_PATTERNS 16

// [saves]: las partidas que pasa Transfer Save y donde se guardan los logros
typedef struct {
  char id[128];                                 // el juego (por defecto el titulo)
  char dir[MAX_PATH_LEN];                       // la carpeta de las partidas, ya con game_dir
                                                // delante si era relativa ("" = game_dir)
  char files[MAX_SAVE_PATTERNS][MAX_PATH_LEN];  // patrones relativos a dir
  int  files_count;
  char achievements[MAX_PATH_LEN];              // relativo a dir (con las partidas); "" = no se guardan
} SavesConfig;

#define MAX_MODS 16

// [game] mods: los mods activos
typedef struct {
  char root[MAX_PATH_LEN];         // la carpeta mods/ (junto al toml del juego)
  char names[MAX_MODS][64];        // los mods activos, en orden: si dos traen el
  int  count;                      // mismo archivo, gana el ultimo
} ModsConfig;

typedef struct {
  EngineConfig engine;
  LauncherConfig launcher;
  ControlsConfig controls;
  ModsConfig mods;
  SavesConfig saves;
  GameConfig game;
  ScriptConfig scripts;
  FontConfig fonts[MAX_FONTS];
  int fonts_count;
} Config;

int config_load(Config* out, const char* filename);
