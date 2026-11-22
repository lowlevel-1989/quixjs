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

// --launcher: el launcher, el menu que sale antes de cargar el juego.
// Heredado de tamatama (https://github.com/lowlevel-1989/tamatama,
// src/shared/menu.c). Arriba el titulo del juego ([game] title; sin el, el
// [engine] name) y debajo una firma pequena, quixjs. Opciones:
//   Play            arranca el juego
//   Achievements    los logros del port (si hay [[achievements]])
//   Transfer Save   pasar las partidas a otro dispositivo (si hay [saves] files)
//   Controls        asignar los botones del mando (si [controls] gamepad = true)
//   Options         ver los fps, el perfilador (quixjs-settings.toml); los
//                   fps los fija el toml del juego
//   Cheats          activar trucos (si hay [[cheat]])
//   Exit

#include "backend/backend.h"
#include "core/config_loader.h"
#include "settings.h"

#define LAUNCHER_PLAY 1
#define LAUNCHER_EXIT 0

// Con el backend ya iniciado. Devuelve LAUNCHER_PLAY o LAUNCHER_EXIT
// (*exit_code: el de un exit de --script, o 0).
// settings: los de quixjs-settings.toml; Options los cambia y los guarda.
int launcher_run(Backend *backend, const Config *cfg, const char *config_path, Settings *settings,
                 int *exit_code);

// --controls: solo la pantalla Controls, con los colores del launcher. La usa
// la app de Android, que tiene su menu pero asigna el mando con este codigo
// (controller.map con el GUID de SDL)
void launcher_controls(Backend *backend, const Config *cfg);
// Las pantallas del launcher abiertas (ui_open) con los colores de [launcher]
void launcher_open_screens(Backend *backend, const Config *cfg);
// Monta la rom ([game] rom): si va comprimida, con una pantalla de progreso
// mientras se descomprime (una vez). 0 si va bien
int  launcher_mount_rom(Backend *backend, const Config *cfg, const char *first_script);
