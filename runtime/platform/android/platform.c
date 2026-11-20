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

// platform/android: Android (bionic), para la app de android/ (make
// PLATFORM=android). Implementa platform/platform.h y SDL_main, por donde
// entra GameActivity (android/java): argv trae la carpeta del juego
// importado ({files}/games/<id>, con su port.toml) y la de la app ({files}).
//
// Adaptado de tamatama (src/platform/android/platform.c, MIT):
// https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/src/platform/android/platform.c

#include <android/log.h>
#include <jni.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/system_properties.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "platform/platform.h"

// ----------------------------------------------------------------- memoria

void platform_mem_init(void) {}

void platform_mem_trim(void) {
  // bionic no tiene malloc_trim; M_PURGE (Android 9+) devuelve lo libre
#ifdef M_PURGE
  mallopt(M_PURGE, 0);
#endif
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
  struct mallinfo mi = mallinfo();
  out->in_use_mb = (unsigned)mi.uordblks / 1048576.0;
  out->free_kept_mb = (unsigned)mi.fordblks / 1048576.0;
  out->mapped_mb = (unsigned)mi.hblkhd / 1048576.0;
  return 0;
}

// ---------------------------------------------------------------- fuentes

// la que deja GameActivity en {files}/fonts (QUIXJS_FONT, de SDL_main)
const char *platform_default_font(void) {
  const char *env = getenv("QUIXJS_FONT");
  return env && access(env, R_OK) == 0 ? env : NULL;
}

// ------------------------------------------------------------------- red

// la tabla ARP no se puede leer desde Android 10: Transfer Save busca con
// broadcast (transfer/net.c)
int platform_neighbours(uint32_t *ips, int max) {
  (void)ips;
  (void)max;
  return 0;
}

// --------------------------------------------------------------- sistema

void platform_user_name(char *out, size_t room) {
  snprintf(out, room, "android");
}

// el modelo del telefono (ro.product.model), como tamatama
void platform_system_name(char *out, size_t room) {
  char model[PROP_VALUE_MAX] = "";
  __system_property_get("ro.product.model", model);
  snprintf(out, room, "%.31s", model[0] ? model : "Android");
}

// ---------------------------------------------------------------- archivos

// SDL_RWFromFile toma las rutas relativas por assets del APK: absolutas
const char *platform_file_path(const char *path, char *buf, size_t size) {
  if (!path || path[0] == '/' || !getcwd(buf, size)) return path;
  size_t n = strlen(buf);
  snprintf(buf + n, size - n, "/%s", path);
  return buf;
}

// ----------------------------------------------------------------- pantalla

// sin acceso al DRM: el loop va con su reloj
int platform_vblank(double *last_ms, double *period_ms) {
  (void)last_ms;
  (void)period_ms;
  return -1;
}

// ------------------------------------------------------------------ vibrar

// El telefono vibra con GameActivity.vibrate(ms, amplitude), como vibracion de un juego
// (la de SDL no dice para que es: Android la toma por respuesta al tacto y,
// si esa esta apagada en los ajustes, no vibra). Desde el thread del juego:
// SDL lo une a la JVM. Adaptado de tamatama (src/platform/android/platform.c,
// MIT; aqui con la fuerza):
// https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/src/platform/android/platform.c#L110-L127
int platform_vibrate(int ms, int amplitude) {
  JNIEnv *env = SDL_AndroidGetJNIEnv();
  jobject activity = SDL_AndroidGetActivity();
  if (!env || !activity) return 0;
  jclass type = (*env)->GetObjectClass(env, activity);
  jmethodID method = (*env)->GetStaticMethodID(env, type, "vibrate", "(II)Z");
  int done = method && (*env)->CallStaticBooleanMethod(env, type, method, (jint)ms, (jint)amplitude);
  if ((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionClear(env);
    done = 0;
  }
  (*env)->DeleteLocalRef(env, type);
  (*env)->DeleteLocalRef(env, activity);
  return done;
}

// ------------------------------------------------------------- la entrada

// stdout y stderr de quixjs: a logcat (etiqueta "quixjs") y a log.txt en la
// carpeta del juego, como el log.txt del port de PortMaster
static FILE *log_file;

static void *log_pump(void *arg) {
  int fd = (int)(intptr_t)arg;
  char buf[1024];
  ssize_t n;
  while ((n = read(fd, buf, sizeof buf - 1)) > 0) {
    buf[n] = 0;
    __android_log_write(ANDROID_LOG_INFO, "quixjs", buf);
    if (log_file) {
      fwrite(buf, 1, (size_t)n, log_file);
      fflush(log_file);
    }
  }
  return NULL;
}

static void log_start(const char *dir) {
  char path[1024];
  snprintf(path, sizeof path, "%s/log.txt", dir);
  log_file = fopen(path, "w");
  int fds[2];
  if (pipe(fds) != 0) return;
  dup2(fds[1], 1);
  dup2(fds[1], 2);
  setvbuf(stderr, NULL, _IOLBF, 0);
  pthread_t th;
  pthread_create(&th, NULL, log_pump, (void *)(intptr_t)fds[0]);
  pthread_detach(th);
}

// exit() ejecutaria despues los destructores de las librerias del sistema
// con los threads de la interfaz vivos, y el proceso caeria: este, registrado
// el ultimo, se ejecuta el primero y termina sin ellos (como tamatama)
static void exit_now(void) {
  fflush(NULL);
  _exit(0);
}

// el main de quixjs (runtime/core/main.c), renombrado en android/CMakeLists.txt
int quixjs_main(int argc, char **argv);

static char *program_args[6];
static int program_argc;

static void *run_program(void *result) {
  *(int *)result = quixjs_main(program_argc, program_args);
  return NULL;
}

// argv de GameActivity.getArguments: la carpeta del juego y la de la app.
// quixjs arranca derecho en el juego (quixjs --fullscreen -c port.toml),
// sin su launcher: el menu es GameMenuActivity, con vistas de Android, y lo
// elegido alli (quixjs-settings.toml: Show FPS, Profiler, Mods, Cheats) lo
// lee quixjs al arrancar. Desde la carpeta del juego, en un thread con stack
// de PC (JavaScriptCore necesita mas que el thread que crea Java).
// Con un tercer argumento, "controls": solo la pantalla Controls del
// launcher (quixjs --controls), para asignar el mando con el mismo codigo y
// el mismo controller.map que en Linux
int SDL_main(int argc, char *argv[]) {
  atexit(exit_now);
  if (argc < 3) return 1;
  if (chdir(argv[1]) != 0) return 1;
  log_start(argv[1]);
  char font[1024];
  snprintf(font, sizeof font, "%s/fonts/DejaVuSans-Bold.ttf", argv[2]);
  setenv("QUIXJS_FONT", font, 1);
  program_args[0] = "quixjs";
  program_args[1] = "--fullscreen";
  program_args[2] = "-c";
  program_args[3] = "port.toml";
  program_argc = 4;
  if (argc > 3 && strcmp(argv[3], "controls") == 0) program_args[program_argc++] = "--controls";
  int result = 1;
  pthread_attr_t attributes;
  pthread_t thread;
  pthread_attr_init(&attributes);
  pthread_attr_setstacksize(&attributes, 16 << 20);
  if (pthread_create(&thread, &attributes, run_program, &result) != 0) return quixjs_main(program_argc, program_args);
  pthread_join(thread, NULL);
  return result;
}
