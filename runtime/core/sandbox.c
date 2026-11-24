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

// El sandbox: un port solo puede leer dentro de su carpeta (la del toml) y
// escribir dentro de la de sus partidas (gamedata). "../entry.js" desde game_dir vale (sigue dentro); "/etc/...",
// "file:///home/..." o "../../otro-port" no: quien hace un port no puede
// abrir ni mandar archivos de otra persona. La ruta se pasa a absoluta, se
// quitan "." y "..", y se resuelven los enlaces simbolicos de la parte que
// ya existe (un enlace dentro del port que apunta fuera tampoco vale).

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sandbox.h"

static char root[PATH_MAX];     // leer; "" = sin sandbox (aun no se llamo sandbox_init)
static char writable[PATH_MAX]; // escribir

// path absoluta, sin "." ni ".." (solo con el texto)
static int normalize(const char *path, char *out, size_t room) {
  char buf[PATH_MAX * 2];
  if (path[0] == '/') {
    snprintf(buf, sizeof buf, "%s", path);
  } else {
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof cwd)) return -1;
    snprintf(buf, sizeof buf, "%s/%s", cwd, path);
  }
  size_t n = 0;
  out[0] = 0;
  char *save = NULL;
  for (char *part = strtok_r(buf, "/", &save); part; part = strtok_r(NULL, "/", &save)) {
    if (strcmp(part, ".") == 0 || part[0] == 0) continue;
    if (strcmp(part, "..") == 0) {
      char *slash = strrchr(out, '/');
      if (slash) *slash = 0;
      n = strlen(out);
      continue;
    }
    int w = snprintf(out + n, room - n, "/%s", part);
    if (w < 0 || (size_t)w >= room - n) return -1;
    n += (size_t)w;
  }
  if (!out[0]) snprintf(out, room, "/");
  return 0;
}

// con los enlaces simbolicos resueltos: la parte que existe con realpath y,
// detras, lo que aun no existe (un archivo que se va a crear)
static int resolve(const char *path, char *out, size_t room) {
  char norm[PATH_MAX], head[PATH_MAX];
  if (normalize(path, norm, sizeof norm) != 0) return -1;
  snprintf(head, sizeof head, "%s", norm);
  for (;;) {
    char real[PATH_MAX];
    if (realpath(head, real)) {
      const char *tail = norm + strlen(head);
      if (strcmp(real, "/") == 0 && tail[0] == '/') real[0] = 0;
      int w = snprintf(out, room, "%s%s", real, tail);
      return w < 0 || (size_t)w >= room ? -1 : 0;
    }
    char *slash = strrchr(head, '/');
    if (!slash) return -1;
    if (slash == head) {
      snprintf(head, sizeof head, "/");
    } else {
      *slash = 0;
    }
  }
}

static int inside(const char *path, const char *dir) {
  char real[PATH_MAX];
  if (!path || !path[0] || resolve(path, real, sizeof real) != 0) return 0;
  size_t n = strlen(dir);
  if (strcmp(dir, "/") == 0) return 1;
  return strncmp(real, dir, n) == 0 && (real[n] == 0 || real[n] == '/');
}

int sandbox_init(const char *config_path, const char *write_dir) {
  char base[PATH_MAX];
  const char *slash = config_path ? strrchr(config_path, '/') : NULL;
  if (slash) snprintf(base, sizeof base, "%.*s", (int)(slash - config_path), config_path);
  else snprintf(base, sizeof base, ".");
  if (!realpath(base, root)) {
    fprintf(stderr, "[sandbox] %s: %s\n", base, strerror(errno));
    root[0] = 0;
    return -1;
  }
  // la de las partidas puede no existir aun: con lo que existe resuelto
  if (resolve(write_dir && write_dir[0] ? write_dir : base, writable, sizeof writable) != 0 ||
      !inside(writable, root)) {
    fprintf(stderr, "[sandbox] the saves folder is outside the port folder: %s\n", write_dir ? write_dir : "");
    writable[0] = 0;
    return -1;
  }
  printf("[sandbox] read %s, write %s\n", root, writable);
  return 0;
}

int sandbox_allows(const char *path) {
  return !root[0] || inside(path, root);
}

int sandbox_allows_write(const char *path) {
  if (!root[0]) return 1;
  return writable[0] && inside(path, writable);
}

int sandbox_check(const char *path, const char *what) {
  if (sandbox_allows(path)) return 1;
  fprintf(stderr, "[sandbox] %s outside the port folder: %s\n", what, path ? path : "");
  return 0;
}

int sandbox_check_write(const char *path, const char *what) {
  if (sandbox_allows_write(path)) return 1;
  fprintf(stderr, "[sandbox] %s outside the saves folder: %s\n", what, path ? path : "");
  return 0;
}

// ---------------------------------------------------- abrir y comprobar

// la ruta de lo que de verdad esta abierto en fd (/proc/self/fd/<fd>) dentro
// de dir
static int fd_inside(int fd, const char *dir) {
  char link[64], real[PATH_MAX];
  snprintf(link, sizeof link, "/proc/self/fd/%d", fd);
  ssize_t n = readlink(link, real, sizeof real - 1);
  if (n <= 0) return 0;
  real[n] = 0;
  size_t d = strlen(dir);
  if (strcmp(dir, "/") == 0) return 1;
  return strncmp(real, dir, d) == 0 && (real[d] == 0 || real[d] == '/');
}

static int denied(const char *path, const char *what, int write) {
  fprintf(stderr, "[sandbox] %s outside the %s folder: %s\n", what, write ? "saves" : "port", path ? path : "");
  errno = EACCES;
  return -1;
}

int sandbox_open(const char *path, int flags, int mode, const char *what) {
  int write = (flags & (O_WRONLY | O_RDWR | O_CREAT | O_TRUNC | O_APPEND)) != 0;
  if (!root[0]) return open(path, flags | O_CLOEXEC, mode);
  // antes, por la ruta: no crear nada fuera; despues, por lo abierto
  if (!(write ? sandbox_allows_write(path) : sandbox_allows(path))) return denied(path, what, write);
  int fd = open(path, flags | O_CLOEXEC, mode);
  if (fd < 0) return -1;
  if (!fd_inside(fd, write ? writable : root)) {
    close(fd);
    return denied(path, what, write);
  }
  return fd;
}

FILE *sandbox_fopen(const char *path, const char *mode, const char *what) {
  int flags = strchr(mode, '+') ? O_RDWR : (mode[0] == 'r' ? O_RDONLY : O_WRONLY);
  if (mode[0] == 'w') flags |= O_CREAT | O_TRUNC;
  if (mode[0] == 'a') flags |= O_CREAT | O_APPEND;
  int fd = sandbox_open(path, flags, 0644, what);
  if (fd < 0) return NULL;
  FILE *f = fdopen(fd, mode);
  if (!f) close(fd);
  return f;
}

DIR *sandbox_opendir(const char *path, const char *what) {
  int fd = sandbox_open(path, O_RDONLY | O_DIRECTORY, 0, what);
  if (fd < 0) return NULL;
  DIR *d = fdopendir(fd);
  if (!d) close(fd);
  return d;
}

int sandbox_stat(const char *path, struct stat *st, const char *what) {
  int fd = sandbox_open(path, O_PATH, 0, what);
  if (fd < 0) return -1;
  int r = fstat(fd, st);
  close(fd);
  return r;
}

// la carpeta de path abierta y comprobada (en la de las partidas) y su ultimo
// nombre: para las llamadas *at. -1 si no
static int parent_at(const char *path, char *name, size_t room, const char *what) {
  const char *slash = path ? strrchr(path, '/') : NULL;
  const char *base = slash ? slash + 1 : path;
  if (!base || !base[0] || strcmp(base, ".") == 0 || strcmp(base, "..") == 0 ||
      snprintf(name, room, "%s", base) >= (int)room) {
    errno = EINVAL;
    return -1;
  }
  char dir[PATH_MAX];
  if (!slash) snprintf(dir, sizeof dir, ".");
  else if (slash == path) snprintf(dir, sizeof dir, "/");
  else snprintf(dir, sizeof dir, "%.*s", (int)(slash - path), path);
  if (root[0] && !sandbox_allows_write(path)) return denied(path, what, 1);
  int fd = open(dir, O_PATH | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0) return -1;
  // la carpeta puede ser la de las partidas misma (crear en ella)
  if (root[0] && !fd_inside(fd, writable)) {
    close(fd);
    return denied(path, what, 1);
  }
  return fd;
}

int sandbox_mkdir(const char *path, int mode, const char *what) {
  char name[NAME_MAX + 1];
  int fd = parent_at(path, name, sizeof name, what);
  if (fd < 0) return -1;
  int r = mkdirat(fd, name, mode);
  int e = errno;
  close(fd);
  errno = e;
  return r;
}

static int unlink_at(const char *path, int flags, const char *what) {
  char name[NAME_MAX + 1];
  int fd = parent_at(path, name, sizeof name, what);
  if (fd < 0) return -1;
  int r = unlinkat(fd, name, flags);
  int e = errno;
  close(fd);
  errno = e;
  return r;
}

int sandbox_unlink(const char *path, const char *what) { return unlink_at(path, 0, what); }
int sandbox_rmdir(const char *path, const char *what) { return unlink_at(path, AT_REMOVEDIR, what); }

int sandbox_rename(const char *from, const char *to, const char *what) {
  char a[NAME_MAX + 1], b[NAME_MAX + 1];
  int fa = parent_at(from, a, sizeof a, what);
  if (fa < 0) return -1;
  int fb = parent_at(to, b, sizeof b, what);
  if (fb < 0) {
    close(fa);
    return -1;
  }
  int r = renameat(fa, a, fb, b);
  int e = errno;
  close(fa);
  close(fb);
  errno = e;
  return r;
}
