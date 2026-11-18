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

// El thread de trabajo. Ver worker.h.

#include <pthread.h>
#include <stdlib.h>
#include "worker.h"

typedef struct Job {
  struct Job *next;
  void (*work)(void *arg);
  void (*done)(JSContext *ctx, void *arg);
  void *arg;
} Job;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static Job *todo_head, *todo_tail, *done_head, *done_tail;
static int  started = 0;
static int  pending = 0;   // solo el thread principal

static void *run(void *unused) {
  (void)unused;
  for (;;) {
    pthread_mutex_lock(&lock);
    while (!todo_head) pthread_cond_wait(&cond, &lock);
    Job *j = todo_head;
    todo_head = j->next;
    if (!todo_head) todo_tail = NULL;
    pthread_mutex_unlock(&lock);

    j->work(j->arg);

    pthread_mutex_lock(&lock);
    j->next = NULL;
    if (done_tail) done_tail->next = j;
    else done_head = j;
    done_tail = j;
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}

int worker_submit(void (*work)(void *arg), void (*done)(JSContext *ctx, void *arg), void *arg) {
  Job *j = calloc(1, sizeof(Job));
  if (!j) return -1;
  j->work = work;
  j->done = done;
  j->arg = arg;
  pthread_mutex_lock(&lock);
  if (!started) {
    pthread_t th;
    if (pthread_create(&th, NULL, run, NULL) != 0) {
      pthread_mutex_unlock(&lock);
      free(j);
      return -1;
    }
    pthread_detach(th);
    started = 1;
  }
  if (todo_tail) todo_tail->next = j;
  else todo_head = j;
  todo_tail = j;
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&lock);
  pending++;
  return 0;
}

void worker_poll(JSContext *ctx) {
  if (!pending) return;
  pthread_mutex_lock(&lock);
  Job *list = done_head;
  done_head = done_tail = NULL;
  pthread_mutex_unlock(&lock);
  while (list) {
    Job *j = list;
    list = j->next;
    pending--;
    j->done(ctx, j->arg);
    free(j);
  }
}

int worker_pending(void) { return pending; }
