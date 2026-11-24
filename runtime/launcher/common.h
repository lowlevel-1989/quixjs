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

// runtime/launcher/common.h: lo que comparten las partes del
// launcher, y nadie mas (el resto de quixjs usa launcher.h):
//   launcher.c           el menu (launcher_run) y abrir las pantallas
//   screens.c   Achievements, Options, Cheats y Mods
//   controls.c  Controls: asignar el mando
//   rom.c       montar la rom, con su pantalla de progreso
// Cada pantalla devuelve SCREEN_QUIT si se cierra quixjs (ui.h).
#pragma once

#include "launcher.h"
#include "ui/ui.h"

int launcher_achievements_screen(const char *logo);
int launcher_options_screen(Backend *backend, const char *logo, const Config *cfg, const char *config_path,
                            Settings *st);
int launcher_cheats_screen(const char *logo, const char *config_path, Settings *st);
int launcher_mods_screen(const char *logo, const Config *cfg, const char *config_path, Settings *st);
int launcher_controls_screen(Backend *backend, const char *logo, const ControlsConfig *cc);
