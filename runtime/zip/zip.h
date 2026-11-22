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

// Leer un .zip sin descomprimirlo en disco (tambien un archivo que lo lleva
// pegado al final, como el Game.exe de un juego de nw.js): su indice (el directorio central)
// se lee una vez al abrirlo, y cada archivo se lee de su sitio dentro del zip
// con pread (varios threads a la vez: los que cargan imagenes). Los archivos
// guardados sin comprimir ("store") se leen tal cual; los comprimidos
// (deflate), con zlib. Sin zip64: un zip de mas de 4 GB no cabe en una
// tarjeta FAT. Ver README.md.

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

typedef struct {
  char     *name;        // la ruta dentro del zip, con '/' (las carpetas acaban en '/')
  uint16_t  method;      // 0 sin comprimir, 8 deflate
  uint32_t  crc;
  uint32_t  dostime;     // fecha y hora en formato MS-DOS
  uint64_t  comp_size;   // lo que ocupa dentro del zip
  uint64_t  size;        // lo que ocupa descomprimido
  uint64_t  data;        // donde empiezan sus datos dentro del zip
} ZipEntry;

typedef struct Zip Zip;

// NULL si no se puede leer (el motivo va a stderr)
Zip *zip_open(const char *path);
void zip_close(Zip *z);
const char *zip_path(const Zip *z);
int zip_count(const Zip *z);
const ZipEntry *zip_entry(const Zip *z, int i);
// El archivo de ese nombre (exacto, con '/'), o NULL
const ZipEntry *zip_find(const Zip *z, const char *name);
// 1 si algun archivo va comprimido (hay que convertirlo: zip_convert)
int zip_compressed(const Zip *z);
// Lo mismo abriendo el archivo: 1 comprimido, 0 no, -1 no se puede leer
int zip_file_compressed(const char *path);
// El archivo entero, con un 0 de mas al final (no cuenta en len). NULL si falla
uint8_t *zip_read(const Zip *z, const ZipEntry *e, size_t *len);
// Un FILE* de solo lectura sobre el archivo (para quien lee por partes: los
// videos). Sin comprimir lee del zip; comprimido, de una copia en memoria
FILE *zip_fopen(const Zip *z, const ZipEntry *e);
// El descriptor del zip (para mapear en memoria un archivo sin comprimir)
int zip_fd(const Zip *z);
// La fecha del archivo en segundos unix (para la cache de imagenes reducidas)
long long zip_mtime(const ZipEntry *e);

// Escribe en dst una copia de src con todo sin comprimir (lo que zip_read
// lee sin trabajo). progress (puede ser NULL) recibe lo hecho y el total en
// bytes. 0 si va bien; dst se borra si falla. Ver convert.c
int zip_convert_store(const char *src, const char *dst,
                      void (*progress)(void *user, uint64_t done, uint64_t total), void *user);
// Lo que ocupara esa copia (para mirar antes el espacio libre)
uint64_t zip_store_size(const Zip *z);
