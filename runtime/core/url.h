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

// Rutas de los recursos del juego: "img/a.png", "./img/a.png?v=1",
// "file:///ruta/a.png" y data URIs ("data:image/png;base64,...").

#include <stddef.h>
#include <stdint.h>

typedef enum {
  URL_FILE,
  URL_DATA,
  URL_REMOTE,    // http(s): no soportado (no hay red)
  URL_INVALID
} UrlKind;

// Convierte la url en una ruta del disco relativa a game_dir.
UrlKind url_resolve(const char *game_dir, const char *url, char *path, size_t size);
// Lo mismo sin los mods: la ruta dentro del juego (para saber que archivo
// del juego es; al leerlo, url_resolve da el del mod si lo trae)
UrlKind url_resolve_game(const char *game_dir, const char *url, char *path, size_t size);

// Decodifica una data URI. Devuelve un buffer con malloc (o NULL).
uint8_t *url_decode_data(const char *url, size_t *len);

// Lee un archivo entero. Devuelve un buffer con malloc terminado en 0 (o NULL).
uint8_t *url_read_file(const char *path, size_t *len);

// Una transformacion para los archivos con una extension (".rpgmvp"): por
// ejemplo descifrarlos. Cambia data en su sitio y puede acortarlo (*len).
// 0 si va bien. Se registra en el thread principal antes de usarla; despues se
// puede leer desde el thread de trabajo (worker.h)
typedef int (*UrlTransform)(uint8_t *data, size_t *len);
void url_add_transform(const char *ext, UrlTransform fn);
// 1 si path tiene una transformacion registrada
int  url_has_transform(const char *path);
// Como url_read_file, y con la transformacion de su extension aplicada
uint8_t *url_read_file_transformed(const char *path, size_t *len);

// Los mods activos ([game] mods): url_resolve da el archivo del mod si un
// archivo de la carpeta del juego (data_dir) esta tambien en root/<nombre>/;
// gana el ultimo de la lista
void url_set_mods(const char *data_dir, const char *root, char names[][64], int count);
