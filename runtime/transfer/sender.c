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

// El envio. La oferta se repite cada segundo hasta que el otro contesta, y
// con su Accept cada trozo de la partida, hasta que dice que lo tiene: asi no
// importa que se pierda un paquete. Heredado de tamatama (src/transfer/sender.c).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "identity.h"
#include "sender.h"

#define DEVICE_GONE_MS 4000    // sin anunciarse, desaparece de la lista
#define RETRY_MS       1000
#define CHUNK_RETRY_MS 250
#define REACH_MS       5000    // para que el otro diga que tiene la oferta
#define ANSWER_MS      60000   // para que pulse Accept o Decline
#define DATA_MS        10000   // sin avanzar un trozo: no llega

int sender_open(Sender *s) {
  memset(s, 0, sizeof *s);
  identity_name(s->name, sizeof s->name);
  identity_new_signature(s->signature);
  return net_open(&s->net, 'S', s->name, s->signature);
}

void sender_close(Sender *s) {
  free(s->data);
  s->data = NULL;
  net_close(&s->net);
}

// Un anuncio de alguien en Receive Save: R <nombre>. Un dispositivo con
// varias redes llega por cada una con otra direccion: se distingue por la
// firma, y se queda la primera direccion.
static void seen_device(Sender *s, const Packet *p) {
  if (strncmp(p->words, "R ", 2) != 0) return;
  for (int i = 0; i < s->count; i++) {
    if (strcmp(s->devices[i].signature, p->signature) == 0) {
      s->devices[i].seen = net_now_ms();
      return;
    }
  }
  if (s->count == SENDER_DEVICES) return;
  Device *device = &s->devices[s->count++];
  device->address = p->from;
  snprintf(device->signature, sizeof device->signature, "%s", p->signature);
  snprintf(device->name, sizeof device->name, "%s", p->words + 2);
  identity_clean(device->name, sizeof device->name);
  device->seen = net_now_ms();
}

static void finish(Sender *s, const char *result) {
  s->result = result;
  s->stage = SENDER_IDLE;
  free(s->data);
  s->data = NULL;
  printf("[transfer] send to %s %s: %s\n", s->target.name, s->target.signature, result);
}

static void send_chunk(Sender *s) {
  char words[64];
  long at = (long)s->chunk * NET_CHUNK;
  long n = s->size - at < NET_CHUNK ? s->size - at : NET_CHUNK;
  snprintf(words, sizeof words, "%d %d %ld %08x", s->chunk, s->chunks, s->size, (unsigned)s->crc);
  net_send(&s->net, &s->target.address, "DATA", words, s->data + at, (size_t)n);
  s->next_retry = net_now_ms() + CHUNK_RETRY_MS;
}

// Lo que contesta el dispositivo al que se manda
static void answer(Sender *s, const Packet *p) {
  uint64_t now = net_now_ms();
  if (strcmp(p->kind, "WAIT") == 0 && s->stage == SENDER_OFFERING) {
    s->stage = SENDER_ASKED;
    s->deadline = now + ANSWER_MS;
  } else if (strcmp(p->kind, "OK") == 0 && s->stage != SENDER_SENDING) {
    s->stage = SENDER_SENDING;
    s->deadline = now + DATA_MS;
    s->chunk = 0;
    send_chunk(s);
  } else if (strcmp(p->kind, "ACK") == 0 && s->stage == SENDER_SENDING) {
    // el trozo de ahora ha llegado: el siguiente (el ultimo lo contesta DONE)
    if (atoi(p->words) == s->chunk && s->chunk + 1 < s->chunks) {
      s->chunk++;
      s->deadline = now + DATA_MS;
      send_chunk(s);
    }
  } else if (strcmp(p->kind, "NO") == 0) {
    finish(s, strcmp(p->words, "declined") == 0  ? "declined"
              : strcmp(p->words, "version") == 0 ? "the other device does not have this game"
              : strcmp(p->words, "busy") == 0    ? "the other device is busy"
                                                 : "rejected");
  } else if (strcmp(p->kind, "DONE") == 0 && s->stage == SENDER_SENDING) {
    finish(s, "save sent");
  } else if (strcmp(p->kind, "FAIL") == 0) {
    finish(s, "the other device could not save it");
  }
}

int sender_poll(Sender *s) {
  int sending = s->stage != SENDER_IDLE;
  Packet p;
  while (net_next(&s->net, &p)) {
    if (strcmp(p.kind, "HELLO") == 0 || strcmp(p.kind, "HERE") == 0) {
      seen_device(s, &p);
    } else if (s->stage != SENDER_IDLE && net_same(&p.from, &s->target.address) &&
               strcmp(p.signature, s->target.signature) == 0) {
      answer(s, &p);
    }
  }
  uint64_t now = net_now_ms();
  for (int i = 0; i < s->count;) {
    if (now - s->devices[i].seen > DEVICE_GONE_MS) s->devices[i] = s->devices[--s->count];
    else i++;
  }
  if (s->stage == SENDER_IDLE) return sending;
  if (now >= s->deadline) {
    finish(s, s->stage == SENDER_OFFERING ? "the other device does not answer"
              : s->stage == SENDER_ASKED  ? "no answer"
                                          : "the save did not arrive");
    return 1;
  }
  if (now >= s->next_retry) {
    if (s->stage == SENDER_SENDING) {
      send_chunk(s);
    } else {
      net_send(&s->net, &s->target.address, "OFFER", s->offer_words, s->offer, strlen(s->offer));
      s->next_retry = now + RETRY_MS;
    }
  }
  return 0;
}

const char *sender_start(Sender *s, int device, const char *game, const SaveSet *set) {
  if (device < 0 || device >= s->count) return "the device is gone";
  free(s->data);
  s->data = saves_pack(set, &s->size);
  if (!s->data) return "cannot read the save";
  s->target = s->devices[device];
  s->crc = saves_crc32(s->data, (size_t)s->size);
  s->chunks = (int)((s->size + NET_CHUNK - 1) / NET_CHUNK);
  snprintf(s->offer, sizeof s->offer, "name %s\ngame %s\nsize %ld\ndate %lld\nsummary %s\n", s->name, game,
           s->size, (long long)set->date, set->summary);
  // el numero distingue una oferta nueva de la repeticion de la anterior
  snprintf(s->offer_words, sizeof s->offer_words, "%u", ++s->offers);
  s->stage = SENDER_OFFERING;
  s->next_retry = 0;
  s->deadline = net_now_ms() + REACH_MS;
  s->result = NULL;
  return NULL;
}

void sender_cancel(Sender *s) {
  if (s->stage == SENDER_IDLE) return;
  net_send(&s->net, &s->target.address, "CANCEL", NULL, NULL, 0);
  finish(s, "cancelled");
}
