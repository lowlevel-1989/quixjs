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

// La red local para juegos (quix.lan): encontrar las consolas de la misma red
// que tienen el mismo juego abierto y mandarse mensajes cortos, por ejemplo
// la posicion de una nave varias veces por segundo. Sin abrir puertos ni
// escribir direcciones: va sobre la red de Transfer Save (transfer/net.c, el
// mismo socket UDP, sus anuncios y su formato), con el rol 'G' y paquetes
// GAME. Un mensaje puede perderse o llegar dos veces: el juego manda el
// estado entero cada vez, no cambios.
//
// Uso (runtime/binding/quix.c lo expone en JS):
//   lan_open("hexgl")         abre (el id del juego: solo entre el mismo)
//   lan_update()              en cada imagen: lee lo que llega y anuncia
//   lan_peers(...)            las consolas que se vieron hace poco
//   lan_send(firma, ...)      a una (o a todas, con NULL)
//   lan_message(...)          el siguiente mensaje recibido
//   lan_close()

#include <stddef.h>
#include <stdint.h>

#define LAN_MAX_PEERS   8
#define LAN_MAX_MESSAGE 4096

typedef struct {
  char id[5];      // su firma de 4 caracteres (otra cada vez que se abre)
  char name[64];   // usuario@hostname (sistema)
} LanPeer;

// 1 si va (o si ya estaba abierta con el mismo juego)
int  lan_open(const char *game_id);
void lan_close(void);
int  lan_is_open(void);
// esta consola: su firma y su nombre (NULL si no esta abierta)
const char *lan_my_id(void);
const char *lan_my_name(void);
void lan_update(void);
// las consolas con el mismo juego vistas en los ultimos segundos; cuantas
int  lan_peers(LanPeer *out, int max);
// a la consola de firma to, o a todas si to es NULL; 1 si se mando
int  lan_send(const char *to, const void *data, size_t size);
// el siguiente mensaje recibido: 1 si hay (from, su firma; data hasta size)
int  lan_message(char from[5], uint8_t *data, size_t *size);
