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

// La recepcion. Solo mientras esta abierta, este dispositivo se anuncia y
// atiende ofertas: una a la vez, y solo del mismo juego. Con Accept recibe
// los trozos en orden (ACK de cada uno), comprueba el CRC32 del total y lo
// guarda (las partidas de antes quedan como backup). A lo que el otro repite
// se le repite la ultima respuesta. Heredado de tamatama (src/transfer/receiver.c).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "identity.h"
#include "receiver.h"

#define DATA_MS 10000   // tras Accept, sin que llegue el siguiente trozo

int receiver_open(Receiver *r, const char *game) {
  memset(r, 0, sizeof *r);
  r->game = game;
  identity_name(r->name, sizeof r->name);
  identity_new_signature(r->signature);
  return net_open(&r->net, 'R', r->name, r->signature);
}

static void drop_data(Receiver *r) {
  free(r->data);
  r->data = NULL;
}

void receiver_close(Receiver *r) {
  drop_data(r);
  net_close(&r->net);
}

// Contesta al de la oferta, y lo recuerda para repetirlo
static void reply(Receiver *r, const char *kind, const char *words) {
  snprintf(r->reply_kind, sizeof r->reply_kind, "%s", kind);
  snprintf(r->reply_words, sizeof r->reply_words, "%s", words ? words : "");
  net_send(&r->net, &r->from, kind, words, NULL, 0);
}

static void reply_again(Receiver *r) {
  net_send(&r->net, &r->from, r->reply_kind, r->reply_words[0] ? r->reply_words : NULL, NULL, 0);
}

// La oferta: NULL si esta bien, o el motivo para rechazarla
static const char *parse_offer(Receiver *r, char *text) {
  Offer *o = &r->offer;
  char signature[5];
  snprintf(signature, sizeof signature, "%s", o->signature);
  memset(o, 0, sizeof *o);
  snprintf(o->signature, sizeof o->signature, "%s", signature);
  o->size = -1;
  char *save = NULL;
  for (char *line = strtok_r(text, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
    char *value = strchr(line, ' ');
    if (!value) continue;
    *value++ = 0;
    if (strcmp(line, "name") == 0) snprintf(o->name, sizeof o->name, "%s", value);
    else if (strcmp(line, "game") == 0) snprintf(o->game, sizeof o->game, "%s", value);
    else if (strcmp(line, "summary") == 0) snprintf(o->summary, sizeof o->summary, "%s", value);
    else if (strcmp(line, "size") == 0) o->size = strtol(value, NULL, 10);
    else if (strcmp(line, "date") == 0) o->date = strtoll(value, NULL, 10);
  }
  identity_clean(o->name, sizeof o->name);
  identity_clean(o->summary, sizeof o->summary);
  if (!o->name[0] || o->size < 1 || o->size > SAVES_MAX_BYTES + 65536) return "format";
  if (strcmp(o->game, r->game) != 0) return "version";
  return NULL;
}

// Una oferta nueva: 1 si hay que preguntar
static int new_offer(Receiver *r, const Packet *p) {
  if (!r->listening || r->stage == RECEIVER_ASKING || r->stage == RECEIVER_ACCEPTED) {
    net_send(&r->net, &p->from, "NO", "busy", NULL, 0);
    return 0;
  }
  r->from = p->from;
  snprintf(r->offer.signature, sizeof r->offer.signature, "%s", p->signature);
  snprintf(r->offer_number, sizeof r->offer_number, "%s", p->words);
  const char *problem = parse_offer(r, (char *)p->body);
  if (problem) {
    r->stage = RECEIVER_FINISHED;
    reply(r, "NO", problem);
    printf("[transfer] offer rejected: %s\n", problem);
    r->notice = strcmp(problem, "version") == 0 ? "an offer for another game" : "an offer that is not valid";
    return 0;
  }
  r->stage = RECEIVER_ASKING;
  reply(r, "WAIT", NULL);
  return 1;
}

static int done(Receiver *r, const char *result) {
  r->stage = RECEIVER_FINISHED;
  r->result = result;
  drop_data(r);
  printf("[transfer] save from %s %s: %s\n", r->offer.name, r->offer.signature, result);
  return RECEIVER_DONE;
}

// Un trozo: DATA <n> <trozos> <bytes> <crc32 en hex>, y sus bytes.
// Devuelve RECEIVER_DONE con el ultimo, o 0.
static int take_chunk(Receiver *r, const Packet *p) {
  int index, chunks;
  long size;
  unsigned crc;
  char extra;
  if (sscanf(p->words, "%d %d %ld %x%c", &index, &chunks, &size, &crc, &extra) != 4 || size != r->offer.size ||
      chunks != (int)((size + NET_CHUNK - 1) / NET_CHUNK) || index < 0 || index >= chunks) {
    reply(r, "FAIL", "format");
    return done(r, "the save did not arrive well");
  }
  char words[16];
  if (index < r->next_chunk) {   // repetido: el ACK se perdio
    snprintf(words, sizeof words, "%d", index);
    net_send(&r->net, &r->from, "ACK", words, NULL, 0);
    return 0;
  }
  if (index > r->next_chunk) return 0;
  long at = (long)index * NET_CHUNK;
  long expect = size - at < NET_CHUNK ? size - at : NET_CHUNK;
  if ((long)p->body_size != expect) {
    reply(r, "FAIL", "format");
    return done(r, "the save did not arrive well");
  }
  memcpy(r->data + at, p->body, p->body_size);
  r->next_chunk++;
  r->deadline = net_now_ms() + DATA_MS;
  if (r->next_chunk < chunks) {
    snprintf(words, sizeof words, "%d", index);
    net_send(&r->net, &r->from, "ACK", words, NULL, 0);
    return 0;
  }
  if (saves_crc32(r->data, (size_t)size) != crc) {
    reply(r, "FAIL", "crc");
    return done(r, "the save arrived damaged");
  }
  if (!saves_store(r->data, size)) {
    reply(r, "FAIL", "write");
    return done(r, "cannot write the save");
  }
  reply(r, "DONE", NULL);
  return done(r, "save received");
}

int receiver_poll(Receiver *r) {
  Packet p;
  while (net_next(&r->net, &p)) {
    int current = r->stage != RECEIVER_IDLE && net_same(&p.from, &r->from) &&
                  strcmp(p.signature, r->offer.signature) == 0;
    if (current && strcmp(p.kind, "CANCEL") == 0 &&
        (r->stage == RECEIVER_ASKING || r->stage == RECEIVER_ACCEPTED)) {
      r->stage = RECEIVER_IDLE;
      drop_data(r);
      r->notice = "the other device cancelled";
      return RECEIVER_CANCELLED;
    }
    if (current && strcmp(p.kind, "DATA") == 0 && r->stage == RECEIVER_ACCEPTED) {
      int event = take_chunk(r, &p);
      if (event) return event;
      continue;
    }
    int offer = strcmp(p.kind, "OFFER") == 0;
    if (current && (offer ? strcmp(p.words, r->offer_number) == 0 : strcmp(p.kind, "DATA") == 0)) {
      reply_again(r);
    } else if (offer && new_offer(r, &p)) {
      return RECEIVER_OFFER;
    }
  }
  if (r->stage == RECEIVER_ACCEPTED && net_now_ms() > r->deadline) return done(r, "the save did not arrive");
  return 0;
}

void receiver_accept(Receiver *r) {
  if (r->stage != RECEIVER_ASKING) return;
  drop_data(r);
  r->data = malloc((size_t)r->offer.size);
  if (!r->data) {
    r->stage = RECEIVER_FINISHED;
    reply(r, "NO", "busy");
    r->notice = "not enough memory for that save";
    return;
  }
  r->next_chunk = 0;
  r->stage = RECEIVER_ACCEPTED;
  r->deadline = net_now_ms() + DATA_MS;
  reply(r, "OK", NULL);
}

void receiver_decline(Receiver *r) {
  if (r->stage != RECEIVER_ASKING) return;
  r->stage = RECEIVER_FINISHED;
  reply(r, "NO", "declined");
  r->notice = "declined";
}

void receiver_cancel(Receiver *r) {
  if (r->stage != RECEIVER_ACCEPTED) return;
  r->stage = RECEIVER_FINISHED;
  drop_data(r);
  reply(r, "NO", "cancelled");
}
