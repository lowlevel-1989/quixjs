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

// La red local para juegos. Ver lan.h.
//
// Cada consola se anuncia con el rol 'G' y "<id del juego> <nombre>" (net.c:
// HELLO cada segundo, y HERE en respuesta); las demas la apuntan si tiene el
// mismo juego. Los mensajes son paquetes GAME, uno a uno a cada consola
// (algunos puntos de acceso no reparten bien el broadcast).

#include <stdio.h>
#include <string.h>
#include "transfer/identity.h"
#include "transfer/net.h"
#include "lan.h"

#define PEER_GONE_MS 4000   // sin anuncios ni mensajes en este tiempo: se fue
#define QUEUE        64     // mensajes recibidos que esperan a que el juego los lea

typedef struct {
  LanPeer peer;
  struct sockaddr_in address;
  uint64_t seen;
} Peer;

typedef struct {
  char from[5];
  size_t size;
  uint8_t data[LAN_MAX_MESSAGE];
} Message;

static Net net;
static int open_ = 0;
static char game[48], me[64], signature[5], announce[128];
static Peer peers[LAN_MAX_PEERS];
static int peer_count = 0;
static Message queue[QUEUE];
static int queue_head = 0, queue_count = 0;

// el id del juego va en el anuncio, separado por un espacio: sin espacios
// ni nada fuera de ASCII ("Fear & Hunger" -> "Fear_&_Hunger")
static void clean_id(const char *id, char *out, size_t room) {
  size_t n = 0;
  for (; *id && n + 1 < room; id++) out[n++] = *id > ' ' && *id <= '~' ? *id : '_';
  out[n] = 0;
}

int lan_open(const char *game_id) {
  char id[sizeof game];
  clean_id(game_id, id, sizeof id);
  if (!id[0]) return 0;
  if (open_ && strcmp(game, id) == 0) return 1;
  lan_close();
  snprintf(game, sizeof game, "%s", id);
  identity_name(me, sizeof me);
  identity_new_signature(signature);
  snprintf(announce, sizeof announce, "%s %s", game, me);
  if (!net_open(&net, 'G', announce, signature)) return 0;
  open_ = 1;
  peer_count = queue_head = queue_count = 0;
  printf("[lan] open: %s, %s %s\n", game, me, signature);
  return 1;
}

void lan_close(void) {
  if (!open_) return;
  net_close(&net);
  open_ = 0;
  printf("[lan] closed\n");
}

int lan_is_open(void) { return open_; }
const char *lan_my_id(void) { return open_ ? signature : NULL; }
const char *lan_my_name(void) { return open_ ? me : NULL; }

static Peer *find(const char *id) {
  for (int i = 0; i < peer_count; i++) {
    if (strcmp(peers[i].peer.id, id) == 0) return &peers[i];
  }
  return NULL;
}

// un anuncio: "G <juego> <nombre>"; solo los del mismo juego
static void seen(const Packet *p) {
  size_t len = strlen(game);
  if (strncmp(p->words, "G ", 2) != 0 || strncmp(p->words + 2, game, len) != 0 || p->words[2 + len] != ' ') return;
  Peer *peer = find(p->signature);
  if (!peer) {
    if (peer_count == LAN_MAX_PEERS) return;
    peer = &peers[peer_count++];
    snprintf(peer->peer.id, sizeof peer->peer.id, "%s", p->signature);
    snprintf(peer->peer.name, sizeof peer->peer.name, "%s", p->words + 3 + len);
    identity_clean(peer->peer.name, sizeof peer->peer.name);
    printf("[lan] %s %s joined\n", peer->peer.id, peer->peer.name);
  }
  peer->address = p->from;
  peer->seen = net_now_ms();
}

static void received(const Packet *p) {
  Peer *peer = find(p->signature);
  if (!peer || !net_same(&p->from, &peer->address) || p->body_size > LAN_MAX_MESSAGE) return;
  peer->seen = net_now_ms();
  // la cola llena: se pierde el mas viejo (el juego manda el estado entero)
  if (queue_count == QUEUE) {
    queue_head = (queue_head + 1) % QUEUE;
    queue_count--;
  }
  Message *m = &queue[(queue_head + queue_count++) % QUEUE];
  snprintf(m->from, sizeof m->from, "%s", p->signature);
  m->size = p->body_size;
  memcpy(m->data, p->body, p->body_size);
}

void lan_update(void) {
  if (!open_) return;
  Packet p;
  while (net_next(&net, &p)) {
    if (strcmp(p.kind, "HELLO") == 0 || strcmp(p.kind, "HERE") == 0) seen(&p);
    else if (strcmp(p.kind, "GAME") == 0) received(&p);
  }
  uint64_t now = net_now_ms();
  for (int i = 0; i < peer_count;) {
    if (now - peers[i].seen > PEER_GONE_MS) {
      printf("[lan] %s %s left\n", peers[i].peer.id, peers[i].peer.name);
      peers[i] = peers[--peer_count];
    } else {
      i++;
    }
  }
}

int lan_peers(LanPeer *out, int max) {
  int n = 0;
  for (int i = 0; i < peer_count && n < max; i++) out[n++] = peers[i].peer;
  return n;
}

int lan_send(const char *to, const void *data, size_t size) {
  if (!open_ || size > LAN_MAX_MESSAGE) return 0;
  int sent = 0;
  for (int i = 0; i < peer_count; i++) {
    if (to && strcmp(peers[i].peer.id, to) != 0) continue;
    net_send(&net, &peers[i].address, "GAME", NULL, data, size);
    sent = 1;
  }
  return sent;
}

int lan_message(char from[5], uint8_t *data, size_t *size) {
  if (!queue_count) return 0;
  Message *m = &queue[queue_head];
  queue_head = (queue_head + 1) % QUEUE;
  queue_count--;
  snprintf(from, 5, "%s", m->from);
  size_t n = m->size < *size ? m->size : *size;
  memcpy(data, m->data, n);
  *size = n;
  return 1;
}
