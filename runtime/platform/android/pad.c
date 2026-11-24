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

// platform/android/pad.c: el JNI de Pad.java, el mando en las pantallas de
// Android (el menu de un juego y Transfer Save).
//
// Las pantallas de Android (Pad.java) se mueven con el mando segun la misma
// asignacion que el juego: la linea de ese mando en controller.map (la que
// guarda Controls) o, sin ella, la que SDL da en Android a un mando que no
// conoce (botones en el orden de SDL, el stick izquierdo en a0 y a1). Para
// encontrar la linea, el GUID se calcula como SDL en Android
// (SDL_sysjoystick.c, Android_AddJoystick) con lo que Pad.java lee del
// InputDevice, igual que SDLControllerManager.java. Los botones salen del
// keycode como en SDL (keycode_to_SDL): asi "a:b12" de controller.map es el
// mismo boton en el menu y en el juego.

#include <android/keycodes.h>
#include <android/log.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>

#define PAD_DEFAULT "a:b0,b:b1,x:b2,y:b3,back:b4,guide:b5,start:b6,leftstick:b7,rightstick:b8," \
                    "leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,leftx:a0,lefty:a1"
#define PAD_MAX 48

// Lo que hace cada entrada en el menu (Pad.java)
enum { PAD_NONE, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_ACCEPT, PAD_BACK };

static struct {
  char target[24];   // a, dpup, leftx, +leftx...
  char source[12];   // b3, a1, +a1, -a1, a1~
} pad[PAD_MAX];
static int pad_n;

// "a:b0,b:b1,..." (lo que va despues del GUID y el nombre)
static void pad_parse(const char *pairs) {
  pad_n = 0;
  char buf[2048];
  snprintf(buf, sizeof buf, "%s", pairs);
  char *save = NULL;
  for (char *p = strtok_r(buf, ",\r\n", &save); p && pad_n < PAD_MAX; p = strtok_r(NULL, ",\r\n", &save)) {
    char *colon = strchr(p, ':');
    if (!colon || strncmp(p, "platform", 8) == 0) continue;
    *colon = 0;
    snprintf(pad[pad_n].target, sizeof pad[0].target, "%s", p);
    snprintf(pad[pad_n].source, sizeof pad[0].source, "%s", colon + 1);
    pad_n++;
  }
}

// keycode_to_SDL de SDL (src/joystick/android/SDL_sysjoystick.c)
static int pad_button_of(int keycode) {
  switch (keycode) {
    case AKEYCODE_BUTTON_A: case AKEYCODE_DPAD_CENTER: return 0;
    case AKEYCODE_BUTTON_B: return 1;
    case AKEYCODE_BUTTON_X: return 2;
    case AKEYCODE_BUTTON_Y: return 3;
    case AKEYCODE_BACK: case AKEYCODE_BUTTON_SELECT: return 4;
    case AKEYCODE_BUTTON_MODE: return 5;
    case AKEYCODE_MENU: case AKEYCODE_BUTTON_START: return 6;
    case AKEYCODE_BUTTON_THUMBL: return 7;
    case AKEYCODE_BUTTON_THUMBR: return 8;
    case AKEYCODE_BUTTON_L1: return 9;
    case AKEYCODE_BUTTON_R1: return 10;
    case AKEYCODE_DPAD_UP: return 11;
    case AKEYCODE_DPAD_DOWN: return 12;
    case AKEYCODE_DPAD_LEFT: return 13;
    case AKEYCODE_DPAD_RIGHT: return 14;
    case AKEYCODE_BUTTON_L2: return 15;
    case AKEYCODE_BUTTON_R2: return 16;
    case AKEYCODE_BUTTON_C: return 17;
    case AKEYCODE_BUTTON_Z: return 18;
  }
  if (keycode >= AKEYCODE_BUTTON_1 && keycode <= AKEYCODE_BUTTON_16) return 20 + keycode - AKEYCODE_BUTTON_1;
  return -1;
}

static int pad_action_of(const char *target) {
  static const struct { const char *name; int action; } names[] = {
    {"dpup", PAD_UP}, {"-lefty", PAD_UP}, {"dpdown", PAD_DOWN}, {"+lefty", PAD_DOWN},
    {"dpleft", PAD_LEFT}, {"-leftx", PAD_LEFT}, {"dpright", PAD_RIGHT}, {"+leftx", PAD_RIGHT},
    {"a", PAD_ACCEPT}, {"start", PAD_ACCEPT}, {"b", PAD_BACK},
  };
  for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
    if (strcmp(names[i].name, target) == 0) return names[i].action;
  }
  return PAD_NONE;
}

// El GUID de SDL en Android: SDL_CreateJoystickGUID(BLUETOOTH, vendor,
// product, 0, NULL, desc) con la mascara de botones y la de ejes al final
static void pad_guid(const char *desc, int vendor, int product, int buttons, int axes, int hats, char *out) {
  if (hats > 0) buttons |= (1 << 11) | (1 << 12) | (1 << 13) | (1 << 14);   // la cruceta va como botones
  SDL_JoystickGUID guid;
  Uint16 *g = (Uint16 *)guid.data;
  memset(&guid, 0, sizeof guid);
  g[0] = SDL_SwapLE16(0x05);   // SDL_HARDWARE_BUS_BLUETOOTH
  g[1] = SDL_SwapLE16(SDL_crc16(0, desc, strlen(desc)));
  if (vendor) {
    g[2] = SDL_SwapLE16((Uint16)vendor);
    g[4] = SDL_SwapLE16((Uint16)product);
  } else {
    SDL_strlcpy((char *)&g[2], desc, sizeof guid.data - 4);
  }
  g[6] = SDL_SwapLE16((Uint16)buttons);
  g[7] = SDL_SwapLE16((Uint16)axes);
  SDL_JoystickGetGUIDString(guid, out, 33);
}

// El mando que se acaba de usar: su linea en el controller.map mas reciente
// de maps (la ultima de cada archivo), o la de SDL. true si hay linea
JNIEXPORT jboolean JNICALL Java_com_lowlevel1989_quixjs_Pad_load(JNIEnv *env, jclass type, jstring desc, jint vendor,
                                                                 jint product, jint buttons, jint axes, jint hats,
                                                                 jobjectArray maps) {
  (void)type;
  char guid[33];
  const char *text = (*env)->GetStringUTFChars(env, desc, NULL);
  pad_guid(text, vendor, product, buttons, axes, hats, guid);
  (*env)->ReleaseStringUTFChars(env, desc, text);
  static char best[2048];
  time_t best_time = 0;
  best[0] = 0;
  int n = maps ? (*env)->GetArrayLength(env, maps) : 0;
  for (int i = 0; i < n; i++) {
    jstring item = (jstring)(*env)->GetObjectArrayElement(env, maps, i);
    const char *path = (*env)->GetStringUTFChars(env, item, NULL);
    struct stat st;
    FILE *f = stat(path, &st) == 0 && st.st_mtime >= best_time ? fopen(path, "r") : NULL;
    (*env)->ReleaseStringUTFChars(env, item, path);
    (*env)->DeleteLocalRef(env, item);
    if (!f) continue;
    char line[2048];
    while (fgets(line, sizeof line, f)) {
      if (strncmp(line, guid, 32) != 0 || line[32] != ',') continue;
      const char *name_end = strchr(line + 33, ',');
      if (!name_end) continue;
      snprintf(best, sizeof best, "%s", name_end + 1);
      best_time = st.st_mtime;
    }
    fclose(f);
  }
  pad_parse(best[0] ? best : PAD_DEFAULT);
  __android_log_print(ANDROID_LOG_INFO, "quixjs", "[pad] %s: %s", guid, best[0] ? "controller.map" : "default");
  return best[0] ? JNI_TRUE : JNI_FALSE;
}

// Un boton de SDL (el de un keycode, o la cruceta de un hat: 11 a 14)
static int pad_button(int button) {
  char source[12];
  snprintf(source, sizeof source, "b%d", button);
  for (int i = 0; i < pad_n; i++) {
    if (strcmp(pad[i].source, source) == 0) return pad_action_of(pad[i].target);
  }
  return PAD_NONE;
}

// Lo que hace un keycode del mando, o -1 si no es un boton para SDL
JNIEXPORT jint JNICALL Java_com_lowlevel1989_quixjs_Pad_key(JNIEnv *env, jclass type, jint keycode) {
  (void)env, (void)type;
  int button = pad_button_of(keycode);
  return button < 0 ? -1 : pad_button(button);
}

JNIEXPORT jint JNICALL Java_com_lowlevel1989_quixjs_Pad_button(JNIEnv *env, jclass type, jint button) {
  (void)env, (void)type;
  return pad_button(button);
}

// El eje axis (en el orden de SDL) empujado hacia sign (-1 o 1)
JNIEXPORT jint JNICALL Java_com_lowlevel1989_quixjs_Pad_axis(JNIEnv *env, jclass type, jint axis, jint sign) {
  (void)env, (void)type;
  for (int i = 0; i < pad_n; i++) {
    const char *s = pad[i].source;
    int half = 0;
    if (*s == '+' || *s == '-') half = *s++ == '+' ? 1 : -1;
    if (*s != 'a' || atoi(s + 1) != axis) continue;
    int dir = sign;
    if (strchr(s, '~')) dir = -dir;
    if (half && half != sign) continue;
    const char *t = pad[i].target;
    char full[32];
    if (*t != '+' && *t != '-' && (strcmp(t, "leftx") == 0 || strcmp(t, "lefty") == 0)) {
      snprintf(full, sizeof full, "%c%s", dir < 0 ? '-' : '+', t);
      t = full;
    }
    int action = pad_action_of(t);
    if (action != PAD_NONE) return action;
  }
  return PAD_NONE;
}
