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

// platform/linux: Linux con glibc (el PC y las portatiles de PortMaster)

#include <arpa/inet.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include "platform/platform.h"

// ----------------------------------------------------------------- memoria

void platform_mem_init(void) {
  // Los bloques grandes del malloc de C (pixeles de imagenes y canvas) en su
  // propio mmap: al liberarlos la memoria vuelve al sistema enseguida. Sin
  // esto glibc sube su umbral y los mete en su heap, donde se fragmentan
  // (en una portatil eran 70 MB libres que no volvian al sistema).
  mallopt(M_MMAP_THRESHOLD, 128 * 1024);
}

void platform_mem_trim(void) {
  malloc_trim(0);
}

long platform_mem_available_mb(void) {
  FILE *f = fopen("/proc/meminfo", "r");
  if (!f) return -1;
  char line[128];
  long kb = -1;
  while (fgets(line, sizeof line, f)) {
    if (sscanf(line, "MemAvailable: %ld kB", &kb) == 1) break;
  }
  fclose(f);
  return kb < 0 ? -1 : kb / 1024;
}

int platform_malloc_stats(PlatformMallocStats *out) {
  // mallinfo: la de glibc 2.28 (la de las portatiles); mallinfo2 es posterior
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  struct mallinfo mi = mallinfo();
#pragma GCC diagnostic pop
  out->in_use_mb = (unsigned)mi.uordblks / 1048576.0;
  out->free_kept_mb = (unsigned)mi.fordblks / 1048576.0;
  out->mapped_mb = (unsigned)mi.hblkhd / 1048576.0;
  return 0;
}

// ---------------------------------------------------------------- fuentes

static const char *font_candidates[] = {
  "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
  "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf",
  "/usr/share/fonts/dejavu/DejaVuSans.ttf",
  "/usr/share/fonts/TTF/DejaVuSans.ttf",
  "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
  "/usr/share/fonts/google-noto/NotoSans-Regular.ttf",
  "/usr/share/fonts/google-noto-vf/NotoSans[wght].ttf",
  "/usr/share/fonts/noto/NotoSans-Regular.ttf",
  "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
  "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
  "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
  NULL
};

const char *platform_default_font(void) {
  const char *env = getenv("QUIXJS_FONT");
  if (env && access(env, R_OK) == 0) return env;
  for (int i = 0; font_candidates[i]; i++) {
    if (access(font_candidates[i], R_OK) == 0) return font_candidates[i];
  }
  return NULL;
}

// ------------------------------------------------------------------- red

int platform_neighbours(uint32_t *ips, int max) {
  FILE *arp = fopen("/proc/net/arp", "r");
  if (!arp) return 0;
  char line[256], ip[64];
  unsigned type, flags;
  int count = 0;
  while (count < max && fgets(line, sizeof line, arp)) {
    struct in_addr address;
    // flags & 2: la entrada esta completa (el equipo respondio)
    if (sscanf(line, "%63s 0x%x 0x%x", ip, &type, &flags) == 3 && (flags & 2) && inet_aton(ip, &address)) {
      ips[count++] = ntohl(address.s_addr);
    }
  }
  fclose(arp);
  return count;
}

// --------------------------------------------------------------- sistema

// El usuario de /etc/passwd con el uid de este proceso, o $USER
void platform_user_name(char *out, size_t room) {
  snprintf(out, room, "%s", getenv("USER") ? getenv("USER") : "user");
  FILE *file = fopen("/etc/passwd", "r");
  if (!file) return;
  char line[256];
  while (fgets(line, sizeof line, file)) {
    char *name = strtok(line, ":"), *password = strtok(NULL, ":"), *uid = strtok(NULL, ":");
    if (name && password && uid && (uid_t)atoi(uid) == getuid()) {
      snprintf(out, room, "%.31s", name);
      break;
    }
  }
  fclose(file);
}

// El NAME de /etc/os-release, sin comillas
void platform_system_name(char *out, size_t room) {
  snprintf(out, room, "Linux");
  FILE *file = fopen("/etc/os-release", "r");
  if (!file) return;
  char line[256];
  while (fgets(line, sizeof line, file)) {
    if (strncmp(line, "NAME=", 5) != 0) continue;
    char *value = line + 5;
    value[strcspn(value, "\r\n")] = 0;
    size_t n = strlen(value);
    if (n >= 2 && (value[0] == '"' || value[0] == '\'') && value[n - 1] == value[0]) {
      value[n - 1] = 0;
      value++;
    }
    if (*value) snprintf(out, room, "%.31s", value);
    break;
  }
  fclose(file);
}

// ---------------------------------------------------------------- archivos

const char *platform_file_path(const char *path, char *buf, size_t size) {
  (void)buf;
  (void)size;
  return path;
}

// ----------------------------------------------------------------- pantalla
//
// Los refrescos los da el DRM del kernel (Direct Rendering Manager: la
// pantalla y la tarjeta grafica, no la proteccion anticopia), pidiendo
// WAIT_VBLANK relativo 0: vuelve al momento con el numero y la hora del ultimo
// refresco. Vale con Weston (que tiene la pantalla) y sin el (SDL en KMSDRM),
// en cualquier tarjeta con pantalla (sun4i-drm en la RG CubeXX, rockchip en la
// R36S...). La tarjeta solo de GPU (panfrost) no lo tiene: se prueban todas.
// Cada salida de video de la tarjeta (pipe) tiene sus refrescos: en la RGB30
// (rockchip, VOP2) la pantalla esta en la segunda; se prueban las cuatro
// primeras y se usa la primera que responde.

// lo justo de <drm/drm.h> (interfaz estable del kernel; el sysroot no la trae)
#define QUIX_DRM_VBLANK_RELATIVE 0x1
#define QUIX_DRM_VBLANK_HIGH_CRTC_SHIFT 1
#define QUIX_DRM_VBLANK_HIGH_CRTC_MASK 0x3e
struct quix_drm_wait_vblank_request { int type; unsigned int sequence; unsigned long signal; };
struct quix_drm_wait_vblank_reply { int type; unsigned int sequence; long tval_sec; long tval_usec; };
union quix_drm_wait_vblank {
  struct quix_drm_wait_vblank_request request;
  struct quix_drm_wait_vblank_reply reply;
};
#define QUIX_DRM_IOCTL_WAIT_VBLANK _IOWR('d', 0x3a, union quix_drm_wait_vblank)

static int vblank_query(int fd, int pipe, unsigned int *seq, double *ms) {
  union quix_drm_wait_vblank vb;
  memset(&vb, 0, sizeof vb);
  vb.request.type = QUIX_DRM_VBLANK_RELATIVE |
                    ((pipe << QUIX_DRM_VBLANK_HIGH_CRTC_SHIFT) & QUIX_DRM_VBLANK_HIGH_CRTC_MASK);
  vb.request.sequence = 0;
  if (ioctl(fd, QUIX_DRM_IOCTL_WAIT_VBLANK, &vb) != 0) return -1;
  *seq = vb.reply.sequence;
  *ms = vb.reply.tval_sec * 1000.0 + vb.reply.tval_usec / 1000.0;
  return 0;
}

int platform_vblank(double *last_ms, double *period_ms) {
  static int fd = -2;            // -2: sin buscar; -1: no hay
  static int pipe = 0;           // la salida de video que responde
  static unsigned int seq0;      // un refresco de referencia, para medir lo que dura
  static double t0 = 0, period = 1000.0 / 60;
  if (fd == -2) {
    fd = -1;
    for (int i = 0; i < 8 && fd < 0; i++) {
      char dev[32];
      snprintf(dev, sizeof dev, "/dev/dri/card%d", i);
      int f = open(dev, O_RDWR | O_CLOEXEC);
      if (f < 0) continue;
      unsigned int s;
      double t;
      for (int p = 0; p < 4 && fd < 0; p++) {
        if (vblank_query(f, p, &s, &t) == 0) {
          fd = f;
          pipe = p;
          seq0 = s;
          t0 = t;
          printf("[display] refreshes from %s (pipe %d)\n", dev, p);
        }
      }
      if (fd < 0) close(f);
    }
  }
  if (fd < 0) return -1;
  unsigned int seq;
  double t;
  if (vblank_query(fd, pipe, &seq, &t) != 0) return -1;
  // lo que dura un refresco: desde la referencia (cuantos mas, mas exacto)
  if (seq - seq0 >= 60) {
    period = (t - t0) / (double)(seq - seq0);
    if (seq - seq0 > 6000) {   // la referencia se renueva de vez en cuando
      seq0 = seq;
      t0 = t;
    }
  }
  *last_ms = t;
  *period_ms = period;
  return 0;
}

// ------------------------------------------------------------------ vibrar

// En Linux vibran los mandos (el backend): no hay otro vibrador
int platform_vibrate(int ms, int amplitude) {
  (void)ms;
  (void)amplitude;
  return 0;
}
