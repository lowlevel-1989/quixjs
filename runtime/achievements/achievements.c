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

// Los logros: leer la definicion del toml del juego, evaluar las condiciones
// cada imagen y guardar los conseguidos. Ver achievements.h.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tomlc17.h"
#include "backend/toast.h"
#include "cheats/cheats.h"
#include "symbols/symbols.h"
#include "achievements.h"

#define MAX_ACHIEVEMENTS 256
#define MAX_CONDITIONS   8

typedef enum { OP_TRUTHY, OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE } CondOp;
typedef enum { OPD_SYMBOL, OPD_PREV, OPD_CONST } OperandKind;

typedef struct {
  OperandKind kind;
  int         symbol;
  SymValue    value;
} Operand;

typedef struct {
  Operand a, b;
  CondOp  op;
} Clause;

#define MAX_ALTERNATIVES 8

// Una condicion: una o varias alternativas separadas por || (basta una)
typedef struct {
  Clause alt[MAX_ALTERNATIVES];
  int    count;
} Condition;

typedef struct {
  char      id[64];
  char      title[128];
  char      description[256];
  char      type[32];        // progression, win_condition... (para agrupar)
  Condition conditions[MAX_CONDITIONS];
  int       condition_count;
  long long unlocked;
} Achievement;

static Achievement list[MAX_ACHIEVEMENTS];
static int         count = 0;
static int         menu_icon = 0;   // [achievements_menu] icon
static char        store_path[1024];
static char        game_id[128];   // el juego: una firma de otro juego no vale

// La firma de una linea del archivo: FNV-1a de 64 bits sobre la clave
// "quixjs", el juego, el id y la fecha. No es criptografia (quixjs es codigo
// abierto), pero editar el archivo a mano ya no basta para darse un logro.
static unsigned long long sign(const char *id, long long when) {
  char text[512];
  snprintf(text, sizeof text, "quixjs|%s|%s|%lld", game_id, id, when);
  unsigned long long h = 1469598103934665603ULL;
  for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
    h ^= *p;
    h *= 1099511628211ULL;
  }
  return h;
}

// ---------- la definicion ----------

static const char *skip(const char *p) {
  while (isspace((unsigned char)*p)) p++;
  return p;
}

static const char *parse_operand(const char *p, Operand *o) {
  p = skip(p);
  memset(o, 0, sizeof *o);
  if (*p == '"') {
    const char *end = strchr(p + 1, '"');
    if (!end || end - p - 1 >= (long)sizeof o->value.str) return NULL;
    o->kind = OPD_CONST;
    o->value.type = SV_STR;
    memcpy(o->value.str, p + 1, (size_t)(end - p - 1));
    return end + 1;
  }
  if (isdigit((unsigned char)*p) || ((*p == '-' || *p == '.') && (isdigit((unsigned char)p[1]) || p[1] == '.'))) {
    char *end;
    o->kind = OPD_CONST;
    o->value.type = SV_NUM;
    o->value.num = strtod(p, &end);
    return end;
  }
  char word[64];
  int n = 0;
  while ((isalnum((unsigned char)*p) || *p == '_' || *p == '$') && n < 63) word[n++] = *p++;
  word[n] = 0;
  if (!n) return NULL;
  if (strcmp(word, "true") == 0 || strcmp(word, "false") == 0) {
    o->kind = OPD_CONST;
    o->value.type = SV_NUM;
    o->value.num = word[0] == 't';
    return p;
  }
  int prev = strcmp(word, "prev") == 0 && *skip(p) == '(';
  if (prev) {
    p = skip(skip(p) + 1);
    n = 0;
    while ((isalnum((unsigned char)*p) || *p == '_' || *p == '$') && n < 63) word[n++] = *p++;
    word[n] = 0;
    p = skip(p);
    if (*p != ')') return NULL;
    p++;
  }
  o->symbol = symbols_find(word);
  if (o->symbol < 0) {
    fprintf(stderr, "[achievements] unknown symbol '%s' (add it to [symbols])\n", word);
    return NULL;
  }
  o->kind = prev ? OPD_PREV : OPD_SYMBOL;
  return p;
}

static int parse_clause(const char *text, Clause *c) {
  const char *p = parse_operand(text, &c->a);
  if (!p) return -1;
  p = skip(p);
  if (!*p) {
    c->op = OP_TRUTHY;
    return 0;
  }
  static const struct { const char *s; CondOp op; } ops[] = {
    {"==", OP_EQ}, {"!=", OP_NE}, {"<=", OP_LE}, {">=", OP_GE}, {"<", OP_LT}, {">", OP_GT},
  };
  size_t i;
  for (i = 0; i < sizeof ops / sizeof ops[0]; i++) {
    size_t n = strlen(ops[i].s);
    if (strncmp(p, ops[i].s, n) == 0) {
      c->op = ops[i].op;
      p += n;
      break;
    }
  }
  if (i == sizeof ops / sizeof ops[0]) return -1;
  p = parse_operand(p, &c->b);
  if (!p || *skip(p)) return -1;
  return 0;
}

// "a == 1 || b == 2": cada alternativa por separado
static int parse_condition(const char *text, Condition *c) {
  c->count = 0;
  char buf[512];
  snprintf(buf, sizeof buf, "%s", text);
  char *p = buf;
  for (;;) {
    char *bar = strstr(p, "||");
    if (bar) *bar = 0;
    if (c->count == MAX_ALTERNATIVES || parse_clause(p, &c->alt[c->count]) != 0) return -1;
    c->count++;
    if (!bar) return 0;
    p = bar + 2;
  }
}

// El archivo va cifrado con un XOR rotatorio con la contrasena "quixjs":
//   QJA1 <el texto cifrado, en hex>
// El texto son las lineas firmadas "id fecha firma". No es criptografia seria
// (la contrasena esta en el codigo, que es abierto): es que el archivo no se
// pueda leer ni tocar con un editor; y si se toca, la firma no cuadra.
#define STORE_MAGIC "QJA1"
#define STORE_MAX   (MAX_ACHIEVEMENTS * 128)

static void crypt(unsigned char *buf, size_t n) {
  static const char key[] = "quixjs";
  for (size_t i = 0; i < n; i++) buf[i] ^= (unsigned char)key[i % (sizeof key - 1)];
}

static void load_unlocked(void) {
  if (!store_path[0]) return;
  FILE *f = fopen(store_path, "r");
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
    fprintf(stderr, "[achievements] %s: not a quixjs achievements file, ignored\n", store_path);
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
    unsigned long long sig = 0;
    if (sscanf(line, "%63s %lld %llx", id, &when, &sig) < 3 || when <= 0 || sig != sign(id, when)) {
      fprintf(stderr, "[achievements] %s: an entry without a valid signature, ignored\n", store_path);
      continue;
    }
    for (int i = 0; i < count; i++) {
      if (strcmp(list[i].id, id) == 0) list[i].unlocked = when;
    }
  }
}

static void save_unlocked(void) {
  if (!store_path[0]) return;
  static unsigned char text[STORE_MAX];
  size_t n = 0;
  for (int i = 0; i < count && n < STORE_MAX - 128; i++) {
    if (!list[i].unlocked) continue;
    n += (size_t)snprintf((char *)text + n, STORE_MAX - n, "%s %lld %016llx\n", list[i].id, list[i].unlocked,
                          sign(list[i].id, list[i].unlocked));
  }
  crypt(text, n);
  char tmp[1100];
  snprintf(tmp, sizeof tmp, "%s.tmp", store_path);
  FILE *f = fopen(tmp, "w");
  if (!f) {
    fprintf(stderr, "[achievements] cannot write %s\n", store_path);
    return;
  }
  fprintf(f, STORE_MAGIC " ");
  for (size_t i = 0; i < n; i++) fprintf(f, "%02x", text[i]);
  fprintf(f, "\n");
  if (fclose(f) == 0) rename(tmp, store_path);
}

int ach_load(const char *config_path, const char *game_dir, const char *file, const char *game) {
  ach_free();
  FILE *f = fopen(config_path, "r");
  if (!f) return 0;
  toml_result_t result = toml_parse_file(f);
  fclose(f);
  if (!result.ok) return 0;   // config_load ya lo ha dicho

  int bad = 0;
  menu_icon = 0;
  toml_datum_t menu = toml_get(result.toptab, "achievements_menu");
  if (menu.type == TOML_TABLE) {
    toml_datum_t icon = toml_get(menu, "icon");
    if (icon.type == TOML_INT64 && icon.u.int64 > 0 && icon.u.int64 < 100000) menu_icon = (int)icon.u.int64;
  }
  toml_datum_t arr = toml_get(result.toptab, "achievements");
  if (arr.type == TOML_ARRAY) {
    for (int i = 0; i < arr.u.arr.size && count < MAX_ACHIEVEMENTS; i++) {
      toml_datum_t t = arr.u.arr.elem[i];
      if (t.type != TOML_TABLE) continue;
      Achievement *a = &list[count];
      memset(a, 0, sizeof *a);
      toml_datum_t id = toml_get(t, "id"), title = toml_get(t, "title");
      toml_datum_t desc = toml_get(t, "description"), conds = toml_get(t, "conditions");
      if (id.type != TOML_STRING || conds.type != TOML_ARRAY) {
        fprintf(stderr, "[achievements] achievement %d: it needs id and conditions\n", i + 1);
        bad = 1;
        continue;
      }
      snprintf(a->id, sizeof a->id, "%s", id.u.s);
      snprintf(a->title, sizeof a->title, "%s", title.type == TOML_STRING ? title.u.s : id.u.s);
      if (desc.type == TOML_STRING) snprintf(a->description, sizeof a->description, "%s", desc.u.s);
      toml_datum_t type = toml_get(t, "type");
      if (type.type == TOML_STRING) snprintf(a->type, sizeof a->type, "%s", type.u.s);
      int ok = 1;
      if (conds.u.arr.size == 0) fprintf(stderr, "[achievements] %s: no conditions, it never unlocks\n", a->id);
      for (int k = 0; k < conds.u.arr.size; k++) {
        toml_datum_t cd = conds.u.arr.elem[k];
        if (a->condition_count == MAX_CONDITIONS || cd.type != TOML_STRING ||
            parse_condition(cd.u.s, &a->conditions[a->condition_count]) != 0) {
          fprintf(stderr, "[achievements] %s: cannot read condition '%s'\n", a->id,
                  cd.type == TOML_STRING ? cd.u.s : "?");
          ok = 0;
          break;
        }
        a->condition_count++;
      }
      if (!ok) {
        bad = 1;
        continue;
      }
      count++;
    }
  }
  toml_free(result);

  snprintf(game_id, sizeof game_id, "%s", game ? game : "");
  store_path[0] = 0;
  if (file && file[0]) {
    if (file[0] == '/') snprintf(store_path, sizeof store_path, "%s", file);
    else snprintf(store_path, sizeof store_path, "%s/%s", game_dir, file);
  }
  load_unlocked();
  if (count) {
    int got = 0;
    for (int i = 0; i < count; i++) got += list[i].unlocked != 0;
    printf("Achievements: %d (%d unlocked), %d symbols\n", count, got, symbols_count());
  }
  return bad ? -1 : count;
}

int ach_menu_icon(void) {
  return menu_icon;
}

void ach_free(void) {
  count = 0;
}

int ach_count(void) { return count; }
const char *ach_id(int i) { return list[i].id; }
const char *ach_title(int i) { return list[i].title; }
const char *ach_description(int i) { return list[i].description; }
const char *ach_type(int i) { return list[i].type; }
long long ach_unlocked(int i) { return list[i].unlocked; }

int ach_unlocked_by_id(const char *id) {
  for (int i = 0; i < count; i++) {
    if (strcmp(list[i].id, id) == 0) return list[i].unlocked != 0;
  }
  return 0;
}

void ach_reload(void) {
  if (!count) return;
  for (int i = 0; i < count; i++) list[i].unlocked = 0;
  load_unlocked();
  int got = 0;
  for (int i = 0; i < count; i++) got += list[i].unlocked != 0;
  printf("[achievements] read again: %d unlocked\n", got);
}

void ach_reset(void) {
  for (int i = 0; i < count; i++) list[i].unlocked = 0;
  save_unlocked();
}

// ---------- las condiciones ----------

static const SymValue *operand_value(const Operand *o) {
  if (o->kind == OPD_CONST) return &o->value;
  return o->kind == OPD_PREV ? symbols_prev(o->symbol) : symbols_now(o->symbol);
}

static int clause_holds(const Clause *c) {
  const SymValue *a = operand_value(&c->a);
  if (a->type == SV_MISSING) return 0;
  if (c->op == OP_TRUTHY) return a->type == SV_NUM ? a->num != 0 : a->str[0] != 0;
  const SymValue *b = operand_value(&c->b);
  if (b->type == SV_MISSING) return 0;
  int cmp;
  if (a->type == SV_NUM && b->type == SV_NUM) cmp = a->num < b->num ? -1 : a->num > b->num;
  else if (a->type == SV_STR && b->type == SV_STR) cmp = strcmp(a->str, b->str);
  else return c->op == OP_NE;   // un numero y un texto nunca son iguales
  switch (c->op) {
    case OP_EQ: return cmp == 0;
    case OP_NE: return cmp != 0;
    case OP_LT: return cmp < 0;
    case OP_LE: return cmp <= 0;
    case OP_GT: return cmp > 0;
    case OP_GE: return cmp >= 0;
    default:    return 0;
  }
}

static int condition_holds(const Condition *c) {
  for (int i = 0; i < c->count; i++) {
    if (clause_holds(&c->alt[i])) return 1;
  }
  return 0;
}

// El color del distintivo segun el tipo: dorado los finales, azul el
// progreso, plateado lo secundario; otro tipo, un color fijo sacado del nombre
static unsigned type_color(const char *type) {
  if (!type[0]) return 0x7ab8ff;
  if (strstr(type, "win") || strstr(type, "ending")) return 0xe8b94a;
  if (strstr(type, "progress")) return 0x5aa0f0;
  if (strstr(type, "secondary") || strstr(type, "missable")) return 0xb8bcc8;
  static const unsigned palette[] = { 0xe07a5f, 0x81b29a, 0xc08ad8, 0x6cc4c4 };
  unsigned h = 0;
  for (const char *p = type; *p; p++) h = h * 31 + (unsigned char)*p;
  return palette[h % 4];
}

// La primera letra del titulo (o el primer caracter UTF-8)
static void initial(const char *title, char *out, size_t room) {
  const unsigned char *p = (const unsigned char *)title;
  while (*p && !(isalnum(*p) || *p >= 0x80)) p++;
  size_t n = !*p ? 0 : *p < 0x80 ? 1 : *p >= 0xF0 ? 4 : *p >= 0xE0 ? 3 : 2;
  if (n >= room) n = 0;
  memcpy(out, p, n);
  out[n] = 0;
  if (n == 1) out[0] = (char)toupper((unsigned char)out[0]);
}

unsigned ach_color(int i) { return type_color(list[i].type); }

static void push_unlocked(const Achievement *a) {
  int got = 0;
  for (int i = 0; i < count; i++) got += list[i].unlocked != 0;
  Toast t;
  memset(&t, 0, sizeof t);
  snprintf(t.header, sizeof t.header, "Achievement unlocked   %d/%d", got, count);
  snprintf(t.title, sizeof t.title, "%s", a->title);
  snprintf(t.text, sizeof t.text, "%s", a->description);
  initial(a->title, t.badge, sizeof t.badge);
  t.color = type_color(a->type);
  toast_push(&t);
}

void ach_tick(JSContext *ctx, Backend *backend, double now_ms) {
  (void)ctx;
  (void)backend;
  (void)now_ms;
  if (!count) return;
  // una partida en la que se usaron trucos no consigue logros, nunca
  if (cheats_playthrough_cheated()) return;
  int changed = 0;
  for (int i = 0; i < count; i++) {
    Achievement *a = &list[i];
    if (a->unlocked || a->condition_count == 0) continue;
    int ok = 1;
    for (int k = 0; k < a->condition_count && ok; k++) ok = condition_holds(&a->conditions[k]);
    if (!ok) continue;
    a->unlocked = (long long)time(NULL);
    changed = 1;
    printf("Achievement unlocked: %s (%s)\n", a->id, a->title);
    push_unlocked(a);
  }
  if (changed) save_unlocked();
}

// ---------- la consola ----------

void ach_describe(char *out, size_t room) {
  size_t n = 0;
  out[0] = 0;
  for (int i = 0; i < count && n < room; i++) {
    n += (size_t)snprintf(out + n, room - n, "%-8s %s  %s\n", list[i].unlocked ? "unlocked" : "locked",
                          list[i].id, list[i].title);
  }
  if (n < room) n += symbols_describe(out + n, room - n);
  if (n < room) n += cheats_describe(out + n, room - n);
  if (n == 0) snprintf(out, room, "no achievements in the config\n");
}
