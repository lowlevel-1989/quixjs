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

// Las pantallas de Transfer Save, con sus opciones (heredadas de tamatama:
// src/transfer/transfer.c, send.c, receive.c y screens.c):
//   Send Save       a quien, y se espera a que lo acepte y lo guarde
//   Receive Save    mientras esta abierta, espera unas partidas
//   Restore Backup  vuelve a las partidas que habia antes de la ultima recepcion
// El envio y la recepcion en si estan en sender.c y receiver.c.

#include <stdio.h>
#include <string.h>
#include "launcher/ui/ui.h"
#include "receiver.h"
#include "sender.h"
#include "transfer.h"

#define LOGO "Transfer Save"

static const UiColors *c(void) { return ui_colors(); }

// ---------- Send Save ----------

static void devices_end(Screen *s, int found) {
  ui_item(s, "Back");
  s->fixed = 1;
  s->notice = found ? NULL : "looking for devices in Receive Save...";
}

// La lista de dispositivos, mientras se elige
static int list_tick(Screen *screen, void *context) {
  Sender *s = context;
  sender_poll(s);
  screen->count = 0;
  for (int i = 0; i < s->count; i++) {
    char item[128];
    snprintf(item, sizeof item, "%s  %s", s->devices[i].signature, s->devices[i].name);
    ui_item(screen, item);
  }
  devices_end(screen, s->count);
  return 0;
}

// El envio: 1 cuando termina
static int send_tick(Screen *screen, void *context) {
  Sender *s = context;
  if (s->stage == SENDER_SENDING && s->chunks > 1) {
    snprintf(screen->lines[screen->line_count - 1], sizeof screen->lines[0], "sending %d%%",
             s->chunk * 100 / s->chunks);
  }
  return sender_poll(s);
}

// Mientras se ve un aviso: seguir oyendo la red
static int quiet_tick(Screen *screen, void *context) {
  (void)screen;
  sender_poll(context);
  return 0;
}

static int send_save(const char *game) {
  SaveSet set;
  if (!saves_find(0, &set)) return ui_message(LOGO, "send save", "there are no saves to send", NULL, NULL, NULL);
  static Sender s;
  if (!sender_open(&s)) return ui_message(LOGO, "send save", "cannot use the network", NULL, NULL, NULL);
  int result = 0;
  char date[32];
  saves_date_text(set.date, date, sizeof date);
  for (;;) {
    Screen screen;
    ui_reset(&screen, LOGO, "send save");
    ui_line(&screen, c()->text, "%s  %s", set.summary, date);
    ui_line(&screen, c()->text, "this device: %s", s.name);
    ui_signature(&screen, s.signature);
    ui_line(&screen, c()->text, "send it to:");
    screen.list_top = 236;
    screen.rows = 3;
    devices_end(&screen, 0);
    int chosen = ui_run(&screen, list_tick, &s);
    if (chosen == SCREEN_QUIT) {
      result = SCREEN_QUIT;
      break;
    }
    if (chosen < 0 || chosen >= s.count) break;

    const char *problem = sender_start(&s, chosen, game, &set);
    const char *text = problem;
    if (!problem) {
      Screen wait;
      ui_reset(&wait, LOGO, "send save");
      ui_line(&wait, c()->text, "waiting for %s", s.target.name);
      ui_signature(&wait, s.target.signature);
      ui_line(&wait, c()->text, "to accept it");
      ui_item(&wait, "Cancel");
      int r = ui_run(&wait, send_tick, &s);
      if (r == SCREEN_QUIT) {
        sender_cancel(&s);
        result = SCREEN_QUIT;
        break;
      }
      if (r < SCREEN_EVENT) sender_cancel(&s);
      text = s.result;
    }
    if (ui_message(LOGO, "send save", text, NULL, quiet_tick, &s) == SCREEN_QUIT) {
      result = SCREEN_QUIT;
      break;
    }
    if (strcmp(text, "save sent") == 0) break;
  }
  sender_close(&s);
  return result;
}

// ---------- Receive Save ----------

static int receive_tick(Screen *screen, void *context) {
  Receiver *r = context;
  int event = receiver_poll(r);
  screen->notice = r->notice;
  return event;
}

static int receiving_tick(Screen *screen, void *context) {
  Receiver *r = context;
  int event = receive_tick(screen, context);
  long got = (long)r->next_chunk * NET_CHUNK;
  if (got > r->offer.size) got = r->offer.size;
  if (r->offer.size > 0) {
    snprintf(screen->lines[0], sizeof screen->lines[0], "receiving %ld%%", got * 100 / r->offer.size);
  }
  return event;
}

// Accept / Decline: de quien, que y cuando. 1 con Accept, SCREEN_QUIT si se cierra.
static int ask(Receiver *r) {
  Offer *o = &r->offer;
  char date[32];
  saves_date_text((time_t)o->date, date, sizeof date);
  Screen screen;
  ui_reset(&screen, LOGO, "receive save");
  ui_line(&screen, c()->text, "a save from %s", o->name);
  ui_signature(&screen, o->signature);
  ui_line(&screen, c()->text, "%s  %s", o->summary, date);
  ui_line(&screen, c()->notice, "your saves will be kept as backup");
  ui_item(&screen, "Accept");
  ui_item(&screen, "Decline");
  int answer = ui_run(&screen, receive_tick, r);
  if (answer == SCREEN_QUIT) return SCREEN_QUIT;
  if (answer == SCREEN_EVENT + RECEIVER_CANCELLED) return 0;
  if (answer != 0) {
    receiver_decline(r);
    return 0;
  }
  receiver_accept(r);
  return r->stage == RECEIVER_ACCEPTED;
}

// Tras Accept, hasta que llega la partida (1) o se vuelve
static int wait_data(Receiver *r) {
  Screen screen;
  ui_reset(&screen, LOGO, "receive save");
  ui_line(&screen, c()->text, "receiving...");
  ui_item(&screen, "Cancel");
  int event = ui_run(&screen, receiving_tick, r);
  if (event == SCREEN_QUIT) return SCREEN_QUIT;
  if (event == SCREEN_EVENT + RECEIVER_DONE) return 1;
  receiver_cancel(r);
  return 0;
}

static int receive_save(const char *game) {
  static Receiver r;
  if (!receiver_open(&r, game)) return ui_message(LOGO, "receive save", "cannot use the network", NULL, NULL, NULL);
  int result = 0;
  for (;;) {
    Screen screen;
    ui_reset(&screen, LOGO, "receive save");
    ui_line(&screen, c()->text, "this device: %s", r.name);
    ui_signature(&screen, r.signature);
    ui_line(&screen, c()->text, "waiting for a save...");
    ui_line(&screen, c()->text, "choose this device in Send Save");
    ui_item(&screen, "Back");
    r.listening = 1;
    int event = ui_run(&screen, receive_tick, &r);
    r.listening = 0;
    if (event == SCREEN_QUIT) {
      result = SCREEN_QUIT;
      break;
    }
    if (event < SCREEN_EVENT) break;
    if (event != SCREEN_EVENT + RECEIVER_OFFER) continue;
    int accepted = ask(&r);
    if (accepted == SCREEN_QUIT) {
      result = SCREEN_QUIT;
      break;
    }
    if (!accepted) continue;
    int arrived = wait_data(&r);
    if (arrived == SCREEN_QUIT) {
      result = SCREEN_QUIT;
      break;
    }
    if (!arrived) continue;
    int received = strcmp(r.result, "save received") == 0;
    if (ui_message(LOGO, "receive save", r.result, NULL, receive_tick, &r) == SCREEN_QUIT) {
      result = SCREEN_QUIT;
      break;
    }
    if (received) break;
    r.notice = NULL;
  }
  receiver_close(&r);
  return result;
}

// ---------- Restore Backup ----------

static int restore(void) {
  SaveSet backup;
  if (!saves_find(1, &backup)) return ui_message(LOGO, "restore backup", "there is no backup yet", NULL, NULL, NULL);
  char date[32];
  saves_date_text(backup.date, date, sizeof date);
  Screen s;
  ui_reset(&s, LOGO, "restore backup");
  ui_line(&s, c()->text, "back to the saves before");
  ui_line(&s, c()->text, "the last ones received:");
  ui_line(&s, c()->highlight, "%s  %s", backup.summary, date);
  ui_line(&s, c()->notice, "the current ones will be the backup");
  ui_item(&s, "Restore");
  ui_item(&s, "Back");
  int chosen = ui_run(&s, NULL, NULL);
  if (chosen == SCREEN_QUIT) return SCREEN_QUIT;
  if (chosen != 0) return 0;
  int ok = saves_restore_backup();
  printf("[transfer] restore backup: %s\n", ok ? "done" : "failed");
  return ui_message(LOGO, "restore backup", ok ? "backup restored" : "cannot restore the backup", NULL, NULL, NULL);
}

// ---------- Delete Save ----------

// las partidas al backup, con confirmacion (Restore Backup las devuelve)
static int delete_save(void) {
  SaveSet now;
  if (!saves_find(0, &now)) return ui_message(LOGO, "delete save", "there are no saves", NULL, NULL, NULL);
  Screen s;
  ui_reset(&s, LOGO, "delete save");
  ui_line(&s, c()->text, "the game starts from zero;");
  ui_line(&s, c()->text, "restore backup brings");
  ui_line(&s, c()->text, "the saves back");
  ui_line(&s, c()->notice, "the old backup is lost");
  ui_item(&s, "Back");
  ui_item(&s, "Delete");
  int chosen = ui_run(&s, NULL, NULL);
  if (chosen == SCREEN_QUIT) return SCREEN_QUIT;
  if (chosen != 1) return 0;
  int r = saves_delete();
  printf("[transfer] delete save: %s\n", r > 0 ? "done" : r < 0 ? "no saves" : "failed");
  return ui_message(LOGO, "delete save", r > 0 ? "saves deleted" : r < 0 ? "there are no saves" : "cannot delete the saves",
                    NULL, NULL, NULL);
}

// ---------- la primera ----------

int transfer_screen(const char *game) {
  Screen s;
  ui_reset(&s, LOGO, NULL);
  ui_line(&s, c()->text, "send your saves to");
  ui_line(&s, c()->text, "another device on");
  ui_line(&s, c()->text, "the same network");
  ui_item(&s, "Send Save");
  ui_item(&s, "Receive Save");
  ui_item(&s, "Restore Backup");
  ui_item(&s, "Delete Save");
  ui_item(&s, "Back");
  s.fixed = 1;
  s.list_top = 220;
  for (;;) {
    int chosen = ui_run(&s, NULL, NULL);
    int r = 0;
    if (chosen == 0) r = send_save(game);
    else if (chosen == 1) r = receive_save(game);
    else if (chosen == 2) r = restore();
    else if (chosen == 3) r = delete_save();
    else return chosen == SCREEN_QUIT ? SCREEN_QUIT : 0;
    if (r == SCREEN_QUIT) return SCREEN_QUIT;
  }
}
