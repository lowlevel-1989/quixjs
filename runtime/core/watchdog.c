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

// Salida de emergencia con Select + Start. Ver watchdog.h.

#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "watchdog.h"

#define MAX_DEV 16
#define STALL_MS 1000

static volatile double last_alive = 0;

static double now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void watchdog_alive(void) {
  last_alive = now_ms();
}

// 1 si el dispositivo tiene los botones Select y Start
static int has_select_start(int fd) {
  unsigned long keys[(KEY_MAX + 8 * sizeof(long)) / (8 * sizeof(long))];
  memset(keys, 0, sizeof(keys));
  if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keys)), keys) < 0) return 0;
#define HAS(k) (keys[(k) / (8 * sizeof(long))] & (1UL << ((k) % (8 * sizeof(long)))))
  return HAS(BTN_SELECT) && HAS(BTN_START);
#undef HAS
}

static void *watch(void *arg) {
  (void)arg;
  struct pollfd fds[MAX_DEV];
  int n = 0;
  DIR *d = opendir("/dev/input");
  if (!d) return NULL;
  struct dirent *e;
  while ((e = readdir(d)) && n < MAX_DEV) {
    if (strncmp(e->d_name, "event", 5) != 0) continue;
    char path[300];
    snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) continue;
    if (!has_select_start(fd)) {
      close(fd);
      continue;
    }
    fds[n].fd = fd;
    fds[n].events = POLLIN;
    n++;
  }
  closedir(d);
  if (!n) return NULL;
  int select_down = 0, start_down = 0;
  for (;;) {
    poll(fds, (nfds_t)n, 250);
    for (int i = 0; i < n; i++) {
      struct input_event ev;
      while (read(fds[i].fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
        if (ev.type != EV_KEY) continue;
        if (ev.code == BTN_SELECT) select_down = ev.value != 0;
        if (ev.code == BTN_START) start_down = ev.value != 0;
      }
    }
    // los dos pulsados y el bucle principal parado: no va a enterarse
    if (select_down && start_down && last_alive > 0 && now_ms() - last_alive > STALL_MS) {
      fprintf(stderr, "Select + Start with the game not responding: exit\n");
      _exit(0);
    }
  }
  return NULL;
}

void watchdog_start(void) {
  pthread_t t;
  last_alive = now_ms();
  if (pthread_create(&t, NULL, watch, NULL) == 0) pthread_detach(t);
}
