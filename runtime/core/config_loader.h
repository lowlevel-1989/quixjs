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

// La configuracion de un juego, leida de su toml (config.toml por defecto; en
// un port, port.toml). Aqui solo esta lo que usa el runtime al arrancar; los
// hooks, los logros, los trucos y [symbols] los leen sus propios modulos.

#include <stdbool.h>

#define MAX_SCRIPT_FILES 256   // por lista (libs, includes): GDevelop trae ~120
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
  bool present_ahead; // una imagen de adelanto: la muestra otro thread (por defecto true)
  int  js_memory;     // MB de RAM que JavaScriptCore cree tener (0: la del aparato)
  int  low_memory_mb; // por debajo de esta RAM libre del sistema, GC completo (0: no)
  bool lazy_images;   // PNG: decodificar al usarla (no al cargar) y soltarla al subirla a la GPU
  int  scaling;       // [engine] scaling: 0 sharp (por defecto), 1 smooth, 2 pixel_perfect (backend.h)
  int  quality;       // [engine] quality: 1 low, 2 medium, 3 high (0: sin la opcion); ver quality_scale
  int  texture_budget; // MB de texturas de imagenes; por encima se sueltan las que no se usan (0: no)
} EngineConfig;

// [fonts] Familia = "ruta.ttf" (relativa a game_dir): como un @font-face del
// CSS del juego, que quixjs no lee
typedef struct {
  char family[64];
  char path[MAX_PATH_LEN];
} FontConfig;

typedef struct {
  // Con rom ([game] rom, runtime/rom): rom es el juego (un .zip o una carpeta,
  // con su ruta desde aqui), rom_root la carpeta de dentro donde empieza
  // ([game] game_dir) y game_dir pasa a ser la carpeta gamedata/ de quixjs
  // junto al toml, donde se monta la rom y escribe el juego. Sin rom, game_dir
  // es la carpeta del juego, como antes
  char game_dir[MAX_PATH_LEN];
  char rom[MAX_PATH_LEN];
  char rom_root[MAX_PATH_LEN];
  char cache_dir[MAX_PATH_LEN];   // cache/ junto al toml: shaders (<cache_dir>/shaders) e imagenes reducidas
  char version[32];               // la version del port o del juego ([game] version), informativa
  char language[16];              // navigator.language ([game] language, por defecto "en")
  int width;
  int height;
  // imagenes grandes a menos resolucion (gfx/image_scale.h): image_scale
  // (0 o 1: no; -1: "auto"), image_scale_min (px de un lado) y image_scale_dirs
  double image_scale;
  // la resolucion a la que se dibuja (rpgmaker-mv/render-scale): render_scale
  // (0 o 1: la del juego; -1: "auto", la que se ve en la pantalla)
  double render_scale;
  int    image_scale_min;
  char   image_scale_dirs[8][128];
  int    image_scale_dirs_count;
} GameConfig;

// Cada script es una ruta (desde game_dir) o una data: URL con el codigo (un
// <script> escrito dentro del HTML): sin limite de largo, en memoria propia
typedef struct {
  char *entry;
  char *libs[MAX_SCRIPT_FILES];
  int  libs_count;
  char *includes[MAX_SCRIPT_FILES];
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
// el mando (la Gamepad API); el launcher (Controls) deja asignar todos sus
// botones y las dos palancas, y se guarda en controller.map junto al toml del juego
// [controls.buttons] (opcional): el nombre de lo que hace cada boton en el
// juego ("a" = "Confirm"); con ella, Controls pide Start, esos botones y las
// dos palancas, cada uno por su nombre. vibration = true: el juego (o un
// mod suyo) hace vibrar; sin ella no vibra nada y Options no tiene Vibration
#define MAX_CONTROL_NAMES 24
typedef struct {
  int gamepad;
  int vibration;
  int names_count;
  char button[MAX_CONTROL_NAMES][16];   // como en SDL: a, b, dpup, leftshoulder...
  char name[MAX_CONTROL_NAMES][48];
  // [controls.keys] (opcional): acciones con nombre que pulsan una tecla
  // (Torch = "T"), para lo que el juego solo escucha en el teclado; su
  // boton se elige en Controls
  int keys_count;
  char key_button[8][48];   // el nombre de la accion
  char key_code[8][24];     // su KeyboardEvent.code ("quix:screenshot" y "quix:magnifier":
                            // las de screenshot = true y magnifier = true)
  // magnifier = true: Controls pide un boton suelto para la lupa (Select + L1
  // funciona siempre, en todos los juegos)
  int magnifier;

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
  char local_storage[MAX_PATH_LEN];             // localStorage en disco, relativo a dir; "" = en memoria
} SavesConfig;

#define MAX_MODS 16

// [game] mods: los mods activos
typedef struct {
  char root[MAX_PATH_LEN];         // la carpeta mods/ (junto al toml del juego)
  char names[MAX_MODS][64];        // los mods activos, en orden: si dos traen el
  int  count;                      // mismo archivo, gana el ultimo
} ModsConfig;

// [html] elements: los elementos con id del <body> del index.html
// ({ tag, id, type }, de --gen-config). quixjs no lee el HTML: cuando el
// juego pide uno por id (getElementById) y aun no existe, lo crea en el body
#define MAX_HTML_ELEMENTS 32
typedef struct {
  char tag[16];
  char id[64];
  char type[16];
} HtmlElement;

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
  HtmlElement html[MAX_HTML_ELEMENTS];
  int html_count;
} Config;

int config_load(Config* out, const char* filename);
// El id del juego: [saves] id, o el titulo, o el nombre del motor (los
// logros y la red local, quix.lan, van por juego)
const char *config_game_id(const Config *cfg);
