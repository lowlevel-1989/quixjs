/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// El nombre y la firma de este dispositivo. Heredado de tamatama
// (src/transfer/identity.c), sin la parte de Android.

#include "platform/platform.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "identity.h"

void identity_clean(char *text, size_t room) {
  size_t n = strlen(text);
  if (n >= room) text[n = room - 1] = 0;
  for (size_t i = 0; i < n; i++) {
    if (text[i] < 32 || text[i] > 126) text[i] = '?';
  }
}


void identity_name(char *out, size_t room) {
  char user[32], host[64] = "", system[32];
  platform_user_name(user, sizeof user);
  if (gethostname(host, sizeof host - 1) != 0 || !host[0]) snprintf(host, sizeof host, "device");
  platform_system_name(system, sizeof system);
  // si no cabe, se corta: identity_clean lo deja bien terminado
  if (snprintf(out, room, "%s@%s (%s)", user, host, system) >= (int)room) out[room - 1] = 0;
  identity_clean(out, room);
}

// Sin los que se confunden: 0 y O, 1, I y L
static const char SIGNATURE_CHARS[] = "23456789ABCDEFGHJKMNPQRSTUVWXYZ";

void identity_new_signature(char out[5]) {
  unsigned char bytes[4];
  int file = open("/dev/urandom", O_RDONLY);
  if (file < 0 || read(file, bytes, 4) != 4) {
    uint64_t seed = (uint64_t)time(NULL) ^ ((uint64_t)getpid() << 16);
    for (int i = 0; i < 4; i++) bytes[i] = (unsigned char)(seed >> (i * 8 + 3));
  }
  if (file >= 0) close(file);
  for (int i = 0; i < 4; i++) out[i] = SIGNATURE_CHARS[bytes[i] % (sizeof SIGNATURE_CHARS - 1)];
  out[4] = 0;
}

int identity_valid_signature(const char *text) {
  if (strlen(text) != 4) return 0;
  for (int i = 0; i < 4; i++) {
    if (!strchr(SIGNATURE_CHARS, text[i])) return 0;
  }
  return 1;
}
