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

// El socket UDP, los anuncios y el formato de los paquetes. Heredado de
// tamatama (src/transfer/net.c); aqui los paquetes empiezan por QUIX.
//
// Por que no hace falta abrir puertos: los cortafuegos dejan pasar la
// respuesta a lo que este dispositivo ha mandado. Cada uno anuncia a cada
// direccion de su red, desde NET_PORT a NET_PORT, asi que cuando los dos se
// han anunciado, lo del otro ya cuenta como respuesta y entra.
//
// Cada segundo se anuncia por broadcast y a los vecinos que ya conoce el
// sistema (/proc/net/arp). El recorrido de toda la red va por tandas: solo
// mientras la cola de salida del socket esta por debajo de budget, para que
// lo importante (OFFER, DATA y sus respuestas) siga saliendo.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "platform/platform.h"
#include "identity.h"
#include "net.h"

#define HELLO_MS    1000
#define SWEEP_MS    3000      // un recorrido empieza como mucho cada tanto
#define SWEEP_STEP  64        // direcciones por llamada, como mucho
#define SEND_BUFFER (1 << 20)
#define MAX_HOSTS   1024      // las redes mas grandes: solo el /24 propio

uint64_t net_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

int net_same(const struct sockaddr_in *a, const struct sockaddr_in *b) {
  return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

static void read_only_hosts(Net *n) {
  n->only_count = 0;
  const char *env = getenv("QUIXJS_TRANSFER_HOSTS");
  if (!env || !*env) return;
  char list[256];
  snprintf(list, sizeof list, "%s", env);
  for (char *ip = strtok(list, ", "); ip && n->only_count < 8; ip = strtok(NULL, ", ")) {
    struct in_addr a;
    if (inet_aton(ip, &a)) n->only[n->only_count++] = ntohl(a.s_addr);
  }
}

int net_open(Net *n, char role, const char *name, const char *signature) {
  n->role = role;
  n->name = name;
  n->signature = signature;
  n->next_hello = 0;
  read_only_hosts(n);
  n->fd = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in any;
  memset(&any, 0, sizeof any);
  any.sin_family = AF_INET;
  any.sin_port = htons(NET_PORT);
  any.sin_addr.s_addr = htonl(INADDR_ANY);
  int yes = 1;
  // Sin SO_REUSEADDR: si otro programa ya tiene NET_PORT (otro quixjs en el
  // mismo equipo), este usa uno libre, y los demas le contestan a ese
  if (n->fd >= 0 && bind(n->fd, (struct sockaddr *)&any, sizeof any) != 0) {
    any.sin_port = 0;
    if (bind(n->fd, (struct sockaddr *)&any, sizeof any) != 0) net_close(n);
  }
  if (n->fd < 0 || setsockopt(n->fd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof yes) != 0) {
    fprintf(stderr, "[transfer] cannot open the UDP socket: %s\n", strerror(errno));
    net_close(n);
    return 0;
  }
  fcntl(n->fd, F_SETFL, fcntl(n->fd, F_GETFL) | O_NONBLOCK);
  // lo que deje el sistema; getsockopt dice cuanto
  int buffer = SEND_BUFFER;
  socklen_t size = sizeof buffer;
  setsockopt(n->fd, SOL_SOCKET, SO_SNDBUF, &buffer, sizeof buffer);
  if (getsockopt(n->fd, SOL_SOCKET, SO_SNDBUF, &buffer, &size) != 0 || buffer <= 0) buffer = 65536;
  n->budget = buffer / 2;
  n->next_sweep = 0;
  n->network_count = n->sweeping = 0;
  socklen_t length = sizeof any;
  getsockname(n->fd, (struct sockaddr *)&any, &length);
  printf("[transfer] UDP port %d as %s %s\n", ntohs(any.sin_port), name, signature);
  return 1;
}

void net_close(Net *n) {
  if (n->fd >= 0) close(n->fd);
  n->fd = -1;
}

// los que van muchas veces por segundo: no se escriben en el log (GAME: los
// mensajes de los juegos, lan/lan.c)
static int quiet(const char *kind) {
  return strcmp(kind, "HELLO") == 0 || strcmp(kind, "HERE") == 0 || strcmp(kind, "DATA") == 0 ||
         strcmp(kind, "ACK") == 0 || strcmp(kind, "GAME") == 0;
}

void net_send(Net *n, const struct sockaddr_in *to, const char *kind, const char *words,
              const void *body, size_t size) {
  static uint8_t packet[NET_CHUNK + 1024];
  int head = snprintf((char *)packet, 512, "QUIX %d %s %s%s%s\n", NET_PROTOCOL, kind, n->signature,
                      words ? " " : "", words ? words : "");
  if (head < 0 || head >= 512 || size > sizeof packet - (size_t)head) return;
  if (size) memcpy(packet + head, body, size);
  ssize_t sent = sendto(n->fd, packet, (size_t)head + size, 0, (const struct sockaddr *)to, sizeof *to);
  if (!quiet(kind)) {
    printf("[transfer] -> %s %s:%d%s\n", kind, inet_ntoa(to->sin_addr), ntohs(to->sin_port),
           sent < 0 ? " (failed)" : "");
  }
}

static void hello_to(Net *n, uint32_t address, const char *words) {
  struct sockaddr_in to;
  memset(&to, 0, sizeof to);
  to.sin_family = AF_INET;
  to.sin_port = htons(NET_PORT);
  to.sin_addr.s_addr = htonl(address);
  net_send(n, &to, "HELLO", words, NULL, 0);
}

// Las redes de las interfaces, sin loopback
static void find_networks(Net *n) {
  n->network_count = 0;
  struct ifreq requests[16];
  struct ifconf conf;
  conf.ifc_len = sizeof requests;
  conf.ifc_req = requests;
  if (ioctl(n->fd, SIOCGIFCONF, &conf) != 0) return;
  for (int i = 0; i < conf.ifc_len / (int)sizeof requests[0] && n->network_count < 8; i++) {
    struct ifreq request = requests[i];
    if (ioctl(n->fd, SIOCGIFFLAGS, &request) != 0 || !(request.ifr_flags & IFF_UP) ||
        (request.ifr_flags & IFF_LOOPBACK)) {
      continue;
    }
    uint32_t address = ntohl(((struct sockaddr_in *)&requests[i].ifr_addr)->sin_addr.s_addr);
    request = requests[i];
    if (ioctl(n->fd, SIOCGIFNETMASK, &request) != 0) continue;
    uint32_t mask = ntohl(((struct sockaddr_in *)&request.ifr_netmask)->sin_addr.s_addr);
    if (~mask >= MAX_HOSTS) mask = 0xffffff00u;
    if (~mask < 2) continue;   // /31 y /32: no hay a quien
    n->networks[n->network_count].first = (address & mask) + 1;
    n->networks[n->network_count].last = (address | ~mask) - 1;
    n->networks[n->network_count++].broadcast = address | ~mask;
  }
}

// Lo que espera en la cola de salida del socket, en bytes
static int queued(Net *n) {
  int bytes = 0;
  return ioctl(n->fd, SIOCOUTQ, &bytes) != 0 ? 0 : bytes;
}

// Cada segundo: broadcast y los vecinos conocidos
static void hello(Net *n, const char *words) {
  if (n->only_count) {
    for (int i = 0; i < n->only_count; i++) hello_to(n, n->only[i], words);
    return;
  }
  hello_to(n, INADDR_BROADCAST, words);
  for (int i = 0; i < n->network_count; i++) hello_to(n, n->networks[i].broadcast, words);
  // los equipos con los que el sistema ha hablado hace poco (en Linux, la
  // tabla ARP)
  uint32_t near[64];
  int count = platform_neighbours(near, 64);
  for (int i = 0; i < count; i++) hello_to(n, near[i], words);
}

// Una tanda del recorrido de las redes
static void sweep(Net *n, const char *words, uint64_t now) {
  if (n->only_count) return;
  if (n->sweeping >= n->network_count) {
    if (now < n->next_sweep) return;
    find_networks(n);
    n->next_sweep = now + SWEEP_MS;
    n->sweeping = 0;
    if (n->network_count) n->host = n->networks[0].first;
  }
  for (int sent = 0; sent < SWEEP_STEP && n->sweeping < n->network_count && queued(n) < n->budget; sent++) {
    hello_to(n, n->host, words);
    if (n->host++ == n->networks[n->sweeping].last && ++n->sweeping < n->network_count) {
      n->host = n->networks[n->sweeping].first;
    }
  }
}

// QUIX 1 <kind> <firma> [words]\n[body]: 1 si esta bien formado
static int parse(uint8_t *data, size_t size, Packet *p) {
  data[size] = 0;   // el body tambien se lee como texto
  uint8_t *end = memchr(data, '\n', size);
  if (end) *end = 0;
  p->body = end ? end + 1 : data + size;
  p->body_size = end ? size - (size_t)(end + 1 - data) : 0;
  char *save = NULL;
  char *mark = strtok_r((char *)data, " ", &save), *protocol = strtok_r(NULL, " ", &save);
  char *kind = strtok_r(NULL, " ", &save), *signature = strtok_r(NULL, " ", &save);
  char *words = strtok_r(NULL, "", &save);
  if (!mark || !protocol || !kind || !signature || strcmp(mark, "QUIX") != 0 || atoi(protocol) != NET_PROTOCOL) {
    return 0;
  }
  if (strlen(kind) >= sizeof p->kind || !identity_valid_signature(signature)) return 0;
  snprintf(p->kind, sizeof p->kind, "%s", kind);
  snprintf(p->signature, sizeof p->signature, "%s", signature);
  p->words = words ? words : "";
  return 1;
}

int net_next(Net *n, Packet *p) {
  uint64_t now = net_now_ms();
  char words[96];
  snprintf(words, sizeof words, "%c %s", n->role, n->name);
  sweep(n, words, now);   // el primero busca las redes
  if (now >= n->next_hello) {
    hello(n, words);
    n->next_hello = now + HELLO_MS;
  }
  for (;;) {
    socklen_t length = sizeof p->from;
    ssize_t got = recvfrom(n->fd, n->buffer, sizeof n->buffer - 1, 0, (struct sockaddr *)&p->from, &length);
    if (got <= 0) return 0;
    if (!parse(n->buffer, (size_t)got, p) || strcmp(p->signature, n->signature) == 0) continue;
    if (!quiet(p->kind)) {
      printf("[transfer] <- %s %s:%d\n", p->kind, inet_ntoa(p->from.sin_addr), ntohs(p->from.sin_port));
    }
    if (strcmp(p->kind, "HELLO") == 0) net_send(n, &p->from, "HERE", words, NULL, 0);
    return 1;
  }
}
