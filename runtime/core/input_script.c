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

// El guion de entrada de --script. Ver input_script.h.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/keymap.h"
#include "gfx/image_loader.h"
#include "input_script.h"

typedef enum { ACT_PRESS, ACT_DOWN, ACT_UP, ACT_SHOT, ACT_LOG, ACT_EXIT } ActionType;

typedef struct {
  long       frame;
  ActionType type;
  char       arg[256];
} Action;

static Action *actions = NULL;
static int     count = 0;
static int     next = 0;
static char    pending_up[8][32];   // teclas de "press" que se sueltan el frame siguiente
static int     pending_count = 0;

int input_script_load(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    fprintf(stderr, "--script: cannot open %s\n", path);
    return -1;
  }
  char line[512];
  int lineno = 0;
  while (fgets(line, sizeof(line), f)) {
    lineno++;
    char *hash = strchr(line, '#');
    if (hash) *hash = 0;
    char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (!*p) continue;

    long frame;
    char cmd[32] = "", arg[256] = "";
    int n = sscanf(p, "%ld %31s %255[^\n]", &frame, cmd, arg);
    if (n < 2) {
      fprintf(stderr, "--script %s:%d: expected '<frame> <action> [arg]'\n", path, lineno);
      fclose(f);
      return -1;
    }
    // sin espacios al final
    size_t al = strlen(arg);
    while (al > 0 && isspace((unsigned char)arg[al - 1])) arg[--al] = 0;

    Action a;
    a.frame = frame;
    snprintf(a.arg, sizeof(a.arg), "%s", arg);
    if (strcmp(cmd, "press") == 0) a.type = ACT_PRESS;
    else if (strcmp(cmd, "down") == 0) a.type = ACT_DOWN;
    else if (strcmp(cmd, "up") == 0) a.type = ACT_UP;
    else if (strcmp(cmd, "shot") == 0) a.type = ACT_SHOT;
    else if (strcmp(cmd, "log") == 0) a.type = ACT_LOG;
    else if (strcmp(cmd, "exit") == 0) a.type = ACT_EXIT;
    else {
      fprintf(stderr, "--script %s:%d: unknown action '%s'\n", path, lineno, cmd);
      fclose(f);
      return -1;
    }
    if ((a.type == ACT_PRESS || a.type == ACT_DOWN || a.type == ACT_UP) && !keymap_find(a.arg)) {
      fprintf(stderr, "--script %s:%d: unknown key code '%s'\n", path, lineno, a.arg);
      fclose(f);
      return -1;
    }
    Action *na = realloc(actions, sizeof(Action) * (count + 1));
    if (!na) {
      fclose(f);
      return -1;
    }
    actions = na;
    // ordenadas por frame (estable)
    int i = count;
    while (i > 0 && actions[i - 1].frame > a.frame) {
      actions[i] = actions[i - 1];
      i--;
    }
    actions[i] = a;
    count++;
  }
  fclose(f);
  printf("Input script: %s (%d actions)\n", path, count);
  return 0;
}

int input_script_active(void) {
  return next < count || pending_count > 0;
}

static void key(Backend *backend, InputType type, const char *code) {
  InputEvent ev;
  if (keymap_make_event(&ev, type, code, 0, 0) == 0) backend_push_input(backend, &ev);
}

int input_script_run(long frame, Backend *backend,
                     const Surface *(*shot_screen)(void *user), void *user, int *exit_code) {
  // las teclas de "press" del frame anterior se sueltan ahora
  for (int i = 0; i < pending_count; i++) key(backend, INPUT_KEY_UP, pending_up[i]);
  pending_count = 0;

  while (next < count && actions[next].frame <= frame) {
    Action *a = &actions[next++];
    switch (a->type) {
      case ACT_PRESS:
        key(backend, INPUT_KEY_DOWN, a->arg);
        if (pending_count < 8) snprintf(pending_up[pending_count++], sizeof(pending_up[0]), "%s", a->arg);
        break;
      case ACT_DOWN:
        key(backend, INPUT_KEY_DOWN, a->arg);
        break;
      case ACT_UP:
        key(backend, INPUT_KEY_UP, a->arg);
        break;
      case ACT_SHOT: {
        const Surface *s = shot_screen ? shot_screen(user) : NULL;
        if (s && s->px && image_save_png(s->px, s->w, s->h, a->arg) == 0) printf("[script] frame %ld: shot %s\n", frame, a->arg);
        else fprintf(stderr, "[script] frame %ld: could not save %s\n", frame, a->arg);
        break;
      }
      case ACT_LOG:
        printf("[script] frame %ld: %s\n", frame, a->arg);
        break;
      case ACT_EXIT:
        *exit_code = atoi(a->arg);
        printf("[script] frame %ld: exit %d\n", frame, *exit_code);
        return 1;
    }
  }
  return 0;
}

void input_script_free(void) {
  free(actions);
  actions = NULL;
  count = next = pending_count = 0;
}
