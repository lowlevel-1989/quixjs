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

// La consola de debug por socket (--debug) viene heredada de tamatama
// (https://github.com/lowlevel-1989/tamatama/tree/780b7e723f3dbe8a58bc54ed571f1518dac9191a, src/debug/debug_server.c): el
// mismo socket Unix, los comandos de texto que se atienden entre frames, con
// el juego quieto, y cada respuesta acabada en "ok" o "err MENSAJE". Aqui los
// comandos son los de un juego JS (eval en vez de leer memoria). El cliente es
// tools/console.py, que tambien viene de tamatama.
//
// Este archivo es el socket: abrir, aceptar clientes, leer lineas, responder
// y buscar el comando en la tabla (commands.c), y lo que el bucle mira
// en cada frame (debug_poll, debug_paused, debug_frame_done,
// debug_note_present). Los comandos: commands.c y measure.c.

#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "core/keymap.h"
#include "common.h"

#define MAX_CLIENTS 4
#define LINE_MAX_LEN 65536

static int    listen_fd = -1;
static char   sock_path[256];
static Client clients[MAX_CLIENTS];
DebugState    debug_state;

int debug_init(const char *path) {
  snprintf(sock_path, sizeof(sock_path), "%s", path);
  unlink(sock_path);
  listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (listen_fd < 0) return -1;
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock_path);
  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(listen_fd, MAX_CLIENTS) != 0) {
    fprintf(stderr, "--debug: cannot listen on %s: %s\n", sock_path, strerror(errno));
    close(listen_fd);
    listen_fd = -1;
    return -1;
  }
  fcntl(listen_fd, F_SETFL, O_NONBLOCK);
  for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = -1;
  printf("Debug console: %s (python3 tools/console.py)\n", sock_path);
  return 0;
}

void debug_reply(Client *c, const char *fmt, ...) {
  char buf[8192];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n < 0) return;
  if ((size_t)n >= sizeof(buf)) n = sizeof(buf) - 1;
  ssize_t w = write(c->fd, buf, (size_t)n);
  (void)w;
}

void debug_reply_raw(Client *c, const char *s) {
  size_t len = strlen(s);
  while (len > 0) {
    ssize_t w = write(c->fd, s, len);
    if (w <= 0) {
      if (errno == EAGAIN || errno == EINTR) continue;
      return;
    }
    s += w;
    len -= (size_t)w;
  }
}

void debug_key(Backend *b, InputType type, const char *code) {
  InputEvent ev;
  if (keymap_make_event(&ev, type, code, 0, 0) == 0) backend_push_input(b, &ev);
}

void debug_note_present(double ms) {
  if (debug_state.present_count < DEBUG_PRESENTS) debug_state.presents[debug_state.present_count++] = ms;
}

// Una linea del cliente: "comando argumentos". 1 para cerrar quixjs
static int handle(Client *c, DebugEnv *env, char *line) {
  while (*line == ' ') line++;
  char *arg = strchr(line, ' ');
  if (arg) {
    *arg++ = 0;
    while (*arg == ' ') arg++;
  } else {
    arg = line + strlen(line);
  }
  if (!*line) return 0;
  for (int i = 0; i < debug_command_count; i++) {
    if (strcmp(line, debug_commands[i].name) == 0) return debug_commands[i].fn(c, env, line, arg);
  }
  debug_reply(c, "err unknown command '%s' (help)\n", line);
  return 0;
}

int debug_poll(DebugEnv *env) {
  if (listen_fd < 0) return 0;

  // las teclas de "press" se sueltan en el siguiente punto seguro
  for (int i = 0; i < debug_state.pending_count; i++) debug_key(env->backend, INPUT_KEY_UP, debug_state.pending_up[i]);
  debug_state.pending_count = 0;

  int fd = accept(listen_fd, NULL, NULL);
  if (fd >= 0) {
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) if (clients[i].fd < 0) { slot = i; break; }
    if (slot < 0) close(fd);
    else {
      fcntl(fd, F_SETFL, O_NONBLOCK);
      clients[slot].fd = fd;
      clients[slot].buf = malloc(LINE_MAX_LEN);
      clients[slot].len = 0;
    }
  }

  int quit = 0;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    Client *c = &clients[i];
    if (c->fd < 0) continue;
    ssize_t n = read(c->fd, c->buf + c->len, LINE_MAX_LEN - 1 - c->len);
    if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR)) {
      close(c->fd);
      free(c->buf);
      c->fd = -1;
      continue;
    }
    if (n > 0) c->len += (size_t)n;
    c->buf[c->len] = 0;
    char *nl;
    while ((nl = strchr(c->buf, '\n')) != NULL) {
      *nl = 0;
      if (nl > c->buf && nl[-1] == '\r') nl[-1] = 0;
      if (handle(c, env, c->buf)) quit = 1;
      size_t used = (size_t)(nl - c->buf) + 1;
      memmove(c->buf, nl + 1, c->len - used + 1);
      c->len -= used;
    }
    if (c->len >= LINE_MAX_LEN - 1) c->len = 0;   // linea demasiado larga: se descarta
  }
  return quit;
}

int debug_paused(void) {
  return debug_state.paused;
}

void debug_frame_done(void) {
  if (debug_state.run_frames > 0 && --debug_state.run_frames == 0) debug_state.paused = 1;
}

void debug_shutdown(void) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].fd >= 0) {
      close(clients[i].fd);
      free(clients[i].buf);
      clients[i].fd = -1;
    }
  }
  if (listen_fd >= 0) {
    close(listen_fd);
    unlink(sock_path);
    listen_fd = -1;
  }
}
