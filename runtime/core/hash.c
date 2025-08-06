/*
 * QuixJS - A lightweight JS runtime using SDL2 and QuickJS
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

#include <stdio.h>
#include <string.h>
#include "hash.h"

uint64_t fnv1a_hash_file(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (!f) return 0;

  uint64_t hash = 14695981039346656037ULL;  // FNV offset basis
  unsigned char buf[1024];
  size_t read;

  while ((read = fread(buf, 1, sizeof(buf), f)) > 0) {
    for (size_t i = 0; i < read; i++) {
      hash ^= buf[i];
      hash *= 1099511628211ULL;  // FNV prime
    }
  }
  fclose(f);
  return hash;
}

void hash_to_hex(uint64_t hash, char *out) {
  sprintf(out, "%016llx", (unsigned long long)hash);
}

int save_hash(const char *hash_path, uint64_t hash) {
  FILE *f = fopen(hash_path, "w");
  if (!f) return -1;

  char hex[17];
  hash_to_hex(hash, hex);

  fprintf(f, "%s\n", hex);
  fclose(f);
  return 0;
}

int check_cache_valid(const char *original_path, const char *hash_path) {
  uint64_t hash = fnv1a_hash_file(original_path);
  if (hash == 0) return 0;

  char saved_hash_hex[17];
  FILE *f = fopen(hash_path, "r");
  if (!f) return 0;
  if (fgets(saved_hash_hex, sizeof(saved_hash_hex), f) == NULL) {
    fclose(f);
    return 0;
  }
  fclose(f);

  saved_hash_hex[strcspn(saved_hash_hex, "\r\n")] = 0;

  char current_hash_hex[17];
  hash_to_hex(hash, current_hash_hex);

  return strcmp(saved_hash_hex, current_hash_hex) == 0;
}

