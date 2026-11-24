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

// El envio de las partidas, sin pantallas (las pantallas estan en
// transfer.c).
// Heredado de tamatama (src/transfer/sender.h); aqui la partida va en trozos
// de NET_CHUNK bytes, cada uno hasta que el otro dice que lo tiene (ACK).

#include "net.h"
#include "saves/saves.h"

#define SENDER_DEVICES 8

// Un dispositivo en Receive Save
typedef struct {
  struct sockaddr_in address;
  char signature[5];
  char name[64];
  uint64_t seen;
} Device;

typedef enum {
  SENDER_IDLE,       // ninguno: se elige a quien
  SENDER_OFFERING,   // la oferta, hasta que el otro diga que la tiene
  SENDER_ASKED,      // el otro pregunta Accept o Decline
  SENDER_SENDING,    // los trozos, hasta que diga que lo ha guardado
} SenderStage;

typedef struct {
  Net net;
  char name[64], signature[5];   // los de este dispositivo
  Device devices[SENDER_DEVICES];
  int count;
  // el envio en curso
  SenderStage stage;
  Device target;
  char offer[512];
  unsigned offers;               // cuantas ofertas ha hecho: la de ahora
  char offer_words[16];          // su numero, que va en OFFER
  uint8_t *data;
  long size;
  uint32_t crc;
  int chunk, chunks;             // el trozo que se manda y cuantos hay
  uint64_t next_retry, deadline;
  const char *result;            // cuando termina: "save sent", "declined"...
} Sender;

int  sender_open(Sender *s);
void sender_close(Sender *s);
// Una vez por imagen: oye la red, rehace la lista de dispositivos y, en un
// envio, repite lo que toca. Devuelve 1 cuando el envio termina (result).
int  sender_poll(Sender *s);
// Empieza a mandar set (del juego game) a devices[device]. NULL si empieza,
// o por que no.
const char *sender_start(Sender *s, int device, const char *game, const SaveSet *set);
// Corta el envio y se lo dice al otro
void sender_cancel(Sender *s);
