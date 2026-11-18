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

// La recepcion de las partidas, sin pantallas (las pantallas estan en
// transfer.c). Heredado de tamatama (src/transfer/receiver.h).

#include "net.h"
#include "saves.h"

// Una oferta: de quien y que partida
typedef struct {
  char name[64], signature[5], game[64], summary[48];
  long size;
  long long date;
} Offer;

typedef enum {
  RECEIVER_IDLE,       // esperando una oferta
  RECEIVER_ASKING,     // llego una: Accept o Decline
  RECEIVER_ACCEPTED,   // llegando los trozos
  RECEIVER_FINISHED,   // contestado: se repite a lo que el otro repita
} ReceiverStage;

// Lo que devuelve receiver_poll
enum {
  RECEIVER_OFFER = 1,  // llego una oferta: esta en offer
  RECEIVER_CANCELLED,  // el que envia la ha cancelado
  RECEIVER_DONE,       // tras Accept, termino: el resultado en result
};

typedef struct {
  Net net;
  char name[64], signature[5];   // los de este dispositivo
  const char *game;              // el id del juego de aqui
  int listening;                 // se atienden ofertas nuevas; si no, "busy"
  ReceiverStage stage;
  struct sockaddr_in from;
  Offer offer;
  char offer_number[16];         // el de OFFER: el mismo es una repeticion
  char reply_kind[8], reply_words[24];   // lo ultimo contestado
  uint8_t *data;                 // la partida que va llegando
  int next_chunk;
  uint64_t deadline;
  const char *result;            // "save received", "the save arrived damaged"...
  const char *notice;            // lo ultimo que paso: una oferta rechazada...
} Receiver;

int  receiver_open(Receiver *r, const char *game);
void receiver_close(Receiver *r);
// Una vez por imagen: oye la red y contesta. Devuelve 0 o un evento.
int  receiver_poll(Receiver *r);
void receiver_accept(Receiver *r);
void receiver_decline(Receiver *r);
// Tras Accept, deja de esperar la partida
void receiver_cancel(Receiver *r);
