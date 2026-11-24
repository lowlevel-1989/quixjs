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

// runtime/game/achievements/store.c: el archivo de los logros conseguidos
// ([saves] achievements): leerlo, guardarlo, la firma de cada linea y el
// cifrado. Tambien volver a leerlo (Receive Save, Restore Backup) y
// olvidarlos (Reset achievements).

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "core/sandbox.h"

// La firma de una linea del archivo: FNV-1a de 64 bits sobre la clave
// "quixjs", el juego, el id y la fecha. No es criptografia (quixjs es codigo
// abierto), pero editar el archivo a mano ya no basta para darse un logro.
// FNV-1a de 64 bits sobre "quixjs", el juego y text
unsigned long long achievements_sign(const char *text) {
  unsigned long long h = 1469598103934665603ULL;
  const char *parts[3] = { "quixjs|", ach_game_id, "|" };
  for (int k = 0; k < 4; k++) {
    const unsigned char *p = (const unsigned char *)(k < 3 ? parts[k] : text);
    for (; *p; p++) {
      h ^= *p;
      h *= 1099511628211ULL;
    }
  }
  return h;
}

static unsigned long long sign(const char *id, long long when) {
  char text[512];
  snprintf(text, sizeof text, "%s|%lld", id, when);
  return achievements_sign(text);
}

// la segunda firma de un logro conseguido en hardcore: sin ella (o mal) es
// normal; asi quitarla o copiarla de otro logro no lo hace hardcore
static unsigned long long sign_hardcore(const char *id, long long when) {
  char text[512];
  snprintf(text, sizeof text, "%s|%lld|hardcore", id, when);
  return achievements_sign(text);
}

// El archivo va cifrado con un XOR rotatorio con la contrasena "quixjs":
//   QJA1 <el texto cifrado, en hex>
// El texto son las lineas firmadas "id fecha firma", y en las de un logro
// conseguido en hardcore, una cuarta: su firma de hardcore. No es criptografia seria
// (la contrasena esta en el codigo, que es abierto): es que el archivo no se
// pueda leer ni tocar con un editor; y si se toca, la firma no cuadra.
#define STORE_MAGIC "QJA1"
#define STORE_MAX   (MAX_ACHIEVEMENTS * 128)

static void crypt(unsigned char *buf, size_t n) {
  static const char key[] = "quixjs";
  for (size_t i = 0; i < n; i++) buf[i] ^= (unsigned char)key[i % (sizeof key - 1)];
}

void achievements_store_load(void) {
  if (!ach_store_path[0]) return;
  FILE *f = sandbox_fopen(ach_store_path, "r", "[saves] achievements");
  if (!f) return;
  static char hex[STORE_MAX * 2 + 64];
  size_t got = fread(hex, 1, sizeof hex - 1, f);
  fclose(f);
  hex[got] = 0;
  // lo que un editor de texto puede poner al guardar: el BOM de UTF-8 al
  // principio, \r\n, espacios o saltos de linea en medio
  char *start = hex;
  if ((unsigned char)start[0] == 0xEF && (unsigned char)start[1] == 0xBB && (unsigned char)start[2] == 0xBF) start += 3;
  while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') start++;
  int at = 0;
  sscanf(start, STORE_MAGIC " %n", &at);
  if (!at) {
    fprintf(stderr, "[achievements] %s: not a quixjs achievements file, ignored\n", ach_store_path);
    return;
  }
  static unsigned char text[STORE_MAX + 1];
  size_t n = 0;
  // las cifras hex, saltando lo que no lo es (espacios, \r, saltos)
  int half = -1;
  for (const char *p = start + at; *p && n < STORE_MAX; p++) {
    int d = *p >= '0' && *p <= '9' ? *p - '0' : *p >= 'a' && *p <= 'f' ? *p - 'a' + 10
          : *p >= 'A' && *p <= 'F' ? *p - 'A' + 10 : -1;
    if (d < 0) continue;
    if (half < 0) half = d;
    else {
      text[n++] = (unsigned char)(half << 4 | d);
      half = -1;
    }
  }
  crypt(text, n);
  text[n] = 0;
  for (char *line = (char *)text, *next; line && *line; line = next) {
    next = strchr(line, '\n');
    if (next) *next++ = 0;
    char id[64];
    long long when = 0;
    unsigned long long sig = 0, hsig = 0;
    int fields = sscanf(line, "%63s %lld %llx %llx", id, &when, &sig, &hsig);
    if (fields < 3 || when <= 0 || sig != sign(id, when)) {
      fprintf(stderr, "[achievements] %s: an entry without a valid signature, ignored\n", ach_store_path);
      continue;
    }
    for (int i = 0; i < ach_total; i++) {
      if (strcmp(ach_list[i].id, id) != 0) continue;
      ach_list[i].unlocked = when;
      ach_list[i].hardcore = fields == 4 && hsig == sign_hardcore(id, when);
    }
  }
}

void achievements_store_save(void) {
  if (!ach_store_path[0]) return;
  static unsigned char text[STORE_MAX];
  size_t n = 0;
  for (int i = 0; i < ach_total && n < STORE_MAX - 128; i++) {
    if (!ach_list[i].unlocked) continue;
    const Achievement *a = &ach_list[i];
    n += (size_t)snprintf((char *)text + n, STORE_MAX - n, "%s %lld %016llx", a->id, a->unlocked, sign(a->id, a->unlocked));
    if (a->hardcore) n += (size_t)snprintf((char *)text + n, STORE_MAX - n, " %016llx", sign_hardcore(a->id, a->unlocked));
    n += (size_t)snprintf((char *)text + n, STORE_MAX - n, "\n");
  }
  crypt(text, n);
  char tmp[1100];
  snprintf(tmp, sizeof tmp, "%s.tmp", ach_store_path);
  FILE *f = sandbox_fopen(tmp, "w", "[saves] achievements");
  if (!f) {
    fprintf(stderr, "[achievements] cannot write %s\n", ach_store_path);
    return;
  }
  fprintf(f, STORE_MAGIC " ");
  for (size_t i = 0; i < n; i++) fprintf(f, "%02x", text[i]);
  fprintf(f, "\n");
  if (fclose(f) == 0) sandbox_rename(tmp, ach_store_path, "[saves] achievements");
}

void achievements_reload(void) {
  if (!ach_total) return;
  for (int i = 0; i < ach_total; i++) ach_list[i].unlocked = ach_list[i].hardcore = 0;
  achievements_store_load();
  int got = 0;
  for (int i = 0; i < ach_total; i++) got += ach_list[i].unlocked != 0;
  printf("[achievements] read again: %d unlocked\n", got);
}

void achievements_reset(void) {
  for (int i = 0; i < ach_total; i++) ach_list[i].unlocked = ach_list[i].hardcore = 0;
  achievements_store_save();
}
