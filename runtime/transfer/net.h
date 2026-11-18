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

// La red de Transfer Save: un socket UDP y paquetes de texto (el protocolo
// esta en README.es.md). Heredado de tamatama (src/transfer/net.h).

#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>

#define NET_PORT     47474
#define NET_PROTOCOL 1
#define NET_CHUNK    16384   // bytes de partida por paquete DATA

// Un paquete recibido: QUIX 1 <kind> <firma> <words>\n<body>
typedef struct {
  struct sockaddr_in from;
  char kind[8];          // HELLO, HERE, OFFER, WAIT, OK, NO, DATA, ACK...
  char signature[5];     // la firma de quien lo manda
  const char *words;     // el resto de la primera linea, o ""
  const uint8_t *body;
  size_t body_size;
} Packet;

typedef struct {
  int fd;
  char role;             // 'S' en Send Save, 'R' en Receive Save
  const char *name, *signature;
  uint64_t next_hello, next_sweep;
  // las redes: su broadcast y el rango de direcciones que se recorre
  struct {
    uint32_t broadcast, first, last;
  } networks[8];
  int network_count;
  int sweeping;          // la red que se recorre ahora, o network_count
  uint32_t host;         // la siguiente direccion
  int budget;            // bytes en la cola de salida hasta los que se recorre
  // QUIXJS_TRANSFER_HOSTS=ip,ip: solo esas direcciones (pruebas en un equipo)
  uint32_t only[8];
  int only_count;
  uint8_t buffer[NET_CHUNK + 1024];
} Net;

uint64_t net_now_ms(void);
// Abre el socket en NET_PORT (o en uno libre si esta ocupado). 1 si va.
int  net_open(Net *n, char role, const char *name, const char *signature);
void net_close(Net *n);
// El siguiente paquete de otro dispositivo: 1 si hay. Mientras, anuncia
// este cada segundo y contesta a los anuncios de los demas.
int  net_next(Net *n, Packet *p);
// Manda un paquete; words y body pueden ser NULL
void net_send(Net *n, const struct sockaddr_in *to, const char *kind, const char *words,
              const void *body, size_t size);
// 1 si los dos vienen del mismo sitio (direccion y puerto)
int  net_same(const struct sockaddr_in *a, const struct sockaddr_in *b);
