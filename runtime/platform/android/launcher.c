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

// platform/android/launcher.c: el launcher de quixjs para las pantallas de
// Android (GameMenuActivity y TransferActivity, android/java). Lo que
// enseñan y lo que cambian sale del mismo C que el launcher de Linux
// (runtime/launcher): el toml del juego (config_load), los logros
// (achievements), los trucos (cheats), quixjs-settings.toml (settings.c) y
// Transfer Save (transfer/sender.c, receiver.c, saves.c). Los archivos son
// los mismos que en una portatil. Y el mando en esas pantallas (Pad.java),
// con la asignacion de controller.map.
//
// Todo se llama desde el thread de la interfaz, en el proceso de la app (el
// juego va en otro: GameActivity). TransferActivity llama a sendPoll o
// receivePoll cada pocos milisegundos mientras se ve.
//
// Transfer Save adaptado de tamatama (src/platform/android/transfer.c, MIT):
// https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/src/platform/android/transfer.c

#include <android/keycodes.h>
#include <android/log.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "achievements/achievements.h"
#include "cheats/cheats.h"
#include "core/config_loader.h"
#include "launcher/settings.h"
#include "symbols/symbols.h"
#include "transfer/receiver.h"
#include "transfer/saves.h"
#include "transfer/sender.h"
#include "version.h"

#define MENU(name) Java_com_lowlevel1989_quixjs_GameMenuActivity_##name
#define TRANSFER(name) Java_com_lowlevel1989_quixjs_TransferActivity_##name

static Config cfg;
static Settings settings;
static char config_path[1024];
static char title[128], game_id[128];
static int loaded;

static void config_clear(void) {
  free(cfg.scripts.entry);
  for (int i = 0; i < cfg.scripts.libs_count; i++) free(cfg.scripts.libs[i]);
  for (int i = 0; i < cfg.scripts.includes_count; i++) free(cfg.scripts.includes[i]);
  memset(&cfg, 0, sizeof cfg);
}

// El juego de la carpeta dir ({files}/games/<id>, con port.toml): lo mismo
// que lee quixjs al arrancar (core/main/start.c, load_port), sin el motor de
// JS. Desde dir, como el juego: game_dir y [saves] dir son relativos. 1 si va
static int game_open(JNIEnv *env, jstring folder) {
  const char *dir = (*env)->GetStringUTFChars(env, folder, NULL);
  int ok = chdir(dir) == 0;
  snprintf(config_path, sizeof config_path, "%s/port.toml", dir);
  (*env)->ReleaseStringUTFChars(env, folder, dir);
  if (!ok) return 0;
  config_clear();
  loaded = 0;
  if (config_load(&cfg, config_path) != 0) return 0;
  snprintf(title, sizeof title, "%s", cfg.launcher.title[0] ? cfg.launcher.title : cfg.engine.name);
  snprintf(game_id, sizeof game_id, "%s", cfg.saves.id[0] ? cfg.saves.id : title);
  // [saves]: las partidas y, con ellas, el archivo de los logros
  const char *patterns[MAX_SAVE_PATTERNS];
  int npatterns = cfg.saves.files_count;
  for (int i = 0; i < npatterns; i++) patterns[i] = cfg.saves.files[i];
  const char *saves_dir = cfg.saves.dir[0] ? cfg.saves.dir : cfg.game.game_dir;
  const char *ach_file = cfg.saves.achievements;
  if (ach_file[0] && ach_file[0] != '/' && npatterns > 0 && npatterns < MAX_SAVE_PATTERNS)
    patterns[npatterns++] = ach_file;
  saves_setup(saves_dir, patterns, npatterns);
  symbols_load(config_path);
  ach_load(config_path, saves_dir, ach_file, game_id);
  cheats_load(config_path);
  settings_load(&settings, config_path);
  loaded = 1;
  return 1;
}

static jobjectArray strings(JNIEnv *env, const char *const *values, int n) {
  jclass type = (*env)->FindClass(env, "java/lang/String");
  jobjectArray out = (*env)->NewObjectArray(env, n, type, NULL);
  for (int i = 0; i < n; i++) {
    jstring value = (*env)->NewStringUTF(env, values[i] ? values[i] : "");
    (*env)->SetObjectArrayElement(env, out, i, value);
    (*env)->DeleteLocalRef(env, value);
  }
  return out;
}

static jstring string(JNIEnv *env, const char *value) { return value ? (*env)->NewStringUTF(env, value) : NULL; }

// ================================================================ el menu

JNIEXPORT jboolean JNICALL MENU(open)(JNIEnv *env, jclass type, jstring dir) {
  (void)type;
  return game_open(env, dir) ? JNI_TRUE : JNI_FALSE;
}

// {titulo, "quixjs <version>"}
JNIEXPORT jobjectArray JNICALL MENU(header)(JNIEnv *env, jclass type) {
  (void)type;
  const char *values[] = {title, "quixjs " QUIX_VERSION};
  return strings(env, values, 2);
}

// Las opciones del menu, en el orden del launcher (launcher_run): Play,
// Achievements, Mods, Options, Cheats, Transfer Save y Controls (si
// [controls] gamepad). Exit no: en Android se vuelve con Atras
JNIEXPORT jobjectArray JNICALL MENU(menu)(JNIEnv *env, jclass type) {
  (void)type;
  const char *items[8];
  int n = 0;
  items[n++] = "Play";
  if (loaded && ach_count() > 0) items[n++] = "Achievements";
  char names[MAX_MOD_DIRS][64];
  if (loaded && settings_mod_dirs(cfg.mods.root, names) > 0) items[n++] = "Mods";
  items[n++] = "Options";
  if (loaded && cheats_count() > 0) items[n++] = "Cheats";
  if (loaded && saves_configured()) items[n++] = "Transfer Save";
  if (loaded && cfg.controls.gamepad) items[n++] = "Controls";
  return strings(env, items, n);
}

// ---------- Achievements

// {titulo, descripcion, tipo, color (0xRRGGBB en decimal), fecha o ""} por logro
JNIEXPORT jobjectArray JNICALL MENU(achievements)(JNIEnv *env, jclass type) {
  (void)type;
  int n = loaded ? ach_count() : 0;
  const char **values = calloc((size_t)n * 5 + 1, sizeof *values);
  char (*colors)[16] = calloc((size_t)n + 1, sizeof *colors);
  char (*dates)[32] = calloc((size_t)n + 1, sizeof *dates);
  char (*types)[40] = calloc((size_t)n + 1, sizeof *types);
  for (int i = 0; i < n; i++) {
    snprintf(colors[i], sizeof colors[0], "%u", ach_color(i));
    if (ach_unlocked(i)) saves_date_text((time_t)ach_unlocked(i), dates[i], sizeof dates[0]);
    // el tipo, legible: win_condition -> win condition
    snprintf(types[i], sizeof types[0], "%s", ach_type(i));
    for (char *p = types[i]; *p; p++) if (*p == '_') *p = ' ';
    values[5 * i] = ach_title(i);
    values[5 * i + 1] = ach_description(i);
    values[5 * i + 2] = types[i];
    values[5 * i + 3] = colors[i];
    values[5 * i + 4] = dates[i];
  }
  jobjectArray out = strings(env, values, 5 * n);
  free(values);
  free(colors);
  free(dates);
  free(types);
  return out;
}

JNIEXPORT void JNICALL MENU(resetAchievements)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  if (loaded) ach_reset();
}

// ---------- Options

// {show_fps, profile, vibration, el juego vibra ([controls] vibration)}
JNIEXPORT jbooleanArray JNICALL MENU(options)(JNIEnv *env, jclass type) {
  (void)type;
  jboolean values[] = {settings.show_fps ? JNI_TRUE : JNI_FALSE, settings.profile ? JNI_TRUE : JNI_FALSE,
                       settings.no_vibration ? JNI_FALSE : JNI_TRUE, cfg.controls.vibration ? JNI_TRUE : JNI_FALSE};
  jbooleanArray out = (*env)->NewBooleanArray(env, 4);
  (*env)->SetBooleanArrayRegion(env, out, 0, 4, values);
  return out;
}

// 0 Show FPS, 1 Profiler, 2 Vibration: cambia y se guarda enseguida
JNIEXPORT void JNICALL MENU(toggleOption)(JNIEnv *env, jclass type, jint which) {
  (void)env, (void)type;
  if (!loaded) return;
  if (which == 0) settings.show_fps = !settings.show_fps;
  else if (which == 1) settings.profile = !settings.profile;
  else settings.no_vibration = !settings.no_vibration;
  settings_save(&settings, config_path);
}

// ---------- Mods

// {nombre, su numero si esta activo o ""} por carpeta de mods/
JNIEXPORT jobjectArray JNICALL MENU(mods)(JNIEnv *env, jclass type) {
  (void)type;
  static char names[MAX_MOD_DIRS][64], numbers[MAX_MOD_DIRS][8];
  const char *values[MAX_MOD_DIRS * 2];
  int n = loaded ? settings_mod_dirs(cfg.mods.root, names) : 0;
  for (int i = 0; i < n; i++) {
    // hasta que se toca, los de [game] mods
    int k = -1;
    if (settings.mods_set) k = settings_mod_index(&settings, names[i]);
    else {
      for (int j = 0; j < cfg.mods.count; j++) {
        if (strcmp(cfg.mods.names[j], names[i]) == 0) k = j;
      }
    }
    if (k >= 0) snprintf(numbers[i], sizeof numbers[0], "%d", k + 1);
    else numbers[i][0] = 0;
    values[2 * i] = names[i];
    values[2 * i + 1] = numbers[i];
  }
  return strings(env, values, 2 * n);
}

JNIEXPORT void JNICALL MENU(toggleMod)(JNIEnv *env, jclass type, jstring name) {
  (void)type;
  if (!loaded) return;
  const char *text = (*env)->GetStringUTFChars(env, name, NULL);
  settings_toggle_mod(&settings, (const char (*)[64])cfg.mods.names, cfg.mods.count, text);
  (*env)->ReleaseStringUTFChars(env, name, text);
  settings_save(&settings, config_path);
}

// ---------- Cheats

// {nombre, descripcion, "1" o "0"} por truco
JNIEXPORT jobjectArray JNICALL MENU(cheats)(JNIEnv *env, jclass type) {
  (void)type;
  const char *values[32 * 3];
  int n = loaded ? cheats_count() : 0;
  if (n > 32) n = 32;
  for (int i = 0; i < n; i++) {
    values[3 * i] = cheats_name(i);
    values[3 * i + 1] = cheats_description(i);
    values[3 * i + 2] = settings_cheat_index(&settings, cheats_id(i)) >= 0 ? "1" : "0";
  }
  return strings(env, values, 3 * n);
}

JNIEXPORT void JNICALL MENU(toggleCheat)(JNIEnv *env, jclass type, jint i) {
  (void)env, (void)type;
  if (!loaded || i < 0 || i >= cheats_count()) return;
  settings_toggle_cheat(&settings, cheats_id(i));
  settings_save(&settings, config_path);
}

// ========================================================= Transfer Save

static Sender sender;
static Receiver receiver;

JNIEXPORT jboolean JNICALL TRANSFER(start)(JNIEnv *env, jclass type, jstring dir) {
  (void)type;
  return game_open(env, dir) ? JNI_TRUE : JNI_FALSE;
}

// Las partidas (o el backup): {resumen, fecha}, o {} si no hay
JNIEXPORT jobjectArray JNICALL TRANSFER(saves)(JNIEnv *env, jclass type, jboolean backup) {
  (void)type;
  static SaveSet set;
  char date[32];
  if (!loaded || !saves_find(backup ? 1 : 0, &set)) return strings(env, NULL, 0);
  saves_date_text(set.date, date, sizeof date);
  const char *values[] = {set.summary, date};
  return strings(env, values, 2);
}

JNIEXPORT jboolean JNICALL TRANSFER(restore)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  int ok = loaded && saves_restore_backup();
  if (ok) ach_reload();
  return ok ? JNI_TRUE : JNI_FALSE;
}

// ---------- Send Save

// {nombre, firma} de este dispositivo, o null sin red
JNIEXPORT jobjectArray JNICALL TRANSFER(sendOpen)(JNIEnv *env, jclass type) {
  (void)type;
  if (!sender_open(&sender)) return NULL;
  const char *values[] = {sender.name, sender.signature};
  return strings(env, values, 2);
}

JNIEXPORT void JNICALL TRANSFER(sendClose)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  sender_close(&sender);
}

// true cuando el envio termina: sendResult dice como
JNIEXPORT jboolean JNICALL TRANSFER(sendPoll)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  return sender_poll(&sender) ? JNI_TRUE : JNI_FALSE;
}

// {firma, nombre} por cada dispositivo en Receive Save
JNIEXPORT jobjectArray JNICALL TRANSFER(sendDevices)(JNIEnv *env, jclass type) {
  (void)type;
  const char *values[SENDER_DEVICES * 2];
  for (int i = 0; i < sender.count; i++) {
    values[2 * i] = sender.devices[i].signature;
    values[2 * i + 1] = sender.devices[i].name;
  }
  return strings(env, values, 2 * sender.count);
}

// Manda las partidas al de la firma signature: null si empieza, o por que no
JNIEXPORT jstring JNICALL TRANSFER(sendStart)(JNIEnv *env, jclass type, jstring signature) {
  (void)type;
  static SaveSet set;
  if (!loaded || !saves_find(0, &set)) return string(env, "there are no saves to send");
  const char *text = (*env)->GetStringUTFChars(env, signature, NULL);
  int device = -1;
  for (int i = 0; i < sender.count; i++) {
    if (strcmp(sender.devices[i].signature, text) == 0) device = i;
  }
  (*env)->ReleaseStringUTFChars(env, signature, text);
  if (device < 0) return string(env, "that device is gone");
  return string(env, sender_start(&sender, device, game_id, &set));
}

// Cuanto va del envio (0-100), o -1 si aun no se mandan los trozos
JNIEXPORT jint JNICALL TRANSFER(sendProgress)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  if (sender.stage != SENDER_SENDING || sender.chunks <= 1) return -1;
  return sender.chunk * 100 / sender.chunks;
}

JNIEXPORT jstring JNICALL TRANSFER(sendResult)(JNIEnv *env, jclass type) {
  (void)type;
  return string(env, sender.result);
}

JNIEXPORT void JNICALL TRANSFER(sendCancel)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  sender_cancel(&sender);
}

// ---------- Receive Save

// {nombre, firma} de este dispositivo, o null sin red
JNIEXPORT jobjectArray JNICALL TRANSFER(receiveOpen)(JNIEnv *env, jclass type) {
  (void)type;
  if (!receiver_open(&receiver, game_id)) return NULL;
  const char *values[] = {receiver.name, receiver.signature};
  return strings(env, values, 2);
}

JNIEXPORT void JNICALL TRANSFER(receiveClose)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  receiver_close(&receiver);
}

// Si se atienden ofertas nuevas: solo en la pantalla de espera
JNIEXPORT void JNICALL TRANSFER(receiveListen)(JNIEnv *env, jclass type, jboolean on) {
  (void)env, (void)type;
  receiver.listening = on;
}

// 0, o un evento de receiver.h: 1 oferta, 2 cancelada, 3 terminada
JNIEXPORT jint JNICALL TRANSFER(receivePoll)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  int event = receiver_poll(&receiver);
  // Receive Save cambia el archivo de los logros
  if (event == RECEIVER_DONE) ach_reload();
  return event;
}

// La oferta: {nombre, firma, resumen, fecha}
JNIEXPORT jobjectArray JNICALL TRANSFER(receiveOffer)(JNIEnv *env, jclass type) {
  (void)type;
  const Offer *o = &receiver.offer;
  char date[32];
  saves_date_text((time_t)o->date, date, sizeof date);
  const char *values[] = {o->name, o->signature, o->summary, date};
  return strings(env, values, 4);
}

// Cuanto ha llegado (0-100) tras Accept
JNIEXPORT jint JNICALL TRANSFER(receiveProgress)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  long got = (long)receiver.next_chunk * NET_CHUNK;
  if (receiver.offer.size <= 0) return 0;
  if (got > receiver.offer.size) got = receiver.offer.size;
  return (jint)(got * 100 / receiver.offer.size);
}

JNIEXPORT void JNICALL TRANSFER(receiveAccept)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  receiver_accept(&receiver);
}

JNIEXPORT void JNICALL TRANSFER(receiveDecline)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  receiver_decline(&receiver);
}

JNIEXPORT void JNICALL TRANSFER(receiveCancel)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  receiver_cancel(&receiver);
}

JNIEXPORT jstring JNICALL TRANSFER(receiveResult)(JNIEnv *env, jclass type) {
  (void)type;
  return string(env, receiver.result);
}

// Lo ultimo que paso (una oferta rechazada...), o null
JNIEXPORT jstring JNICALL TRANSFER(receiveNotice)(JNIEnv *env, jclass type) {
  (void)type;
  return string(env, receiver.notice);
}

// ======================================================= el mando en el menu

// Las pantallas de Android (Pad.java) se mueven con el mando segun la misma
// asignacion que el juego: la linea de ese mando en controller.map (la que
// guarda Controls) o, sin ella, la que SDL da en Android a un mando que no
// conoce (botones en el orden de SDL, el stick izquierdo en a0 y a1). Para
// encontrar la linea, el GUID se calcula como SDL en Android
// (SDL_sysjoystick.c, Android_AddJoystick) con lo que Pad.java lee del
// InputDevice, igual que SDLControllerManager.java. Los botones salen del
// keycode como en SDL (keycode_to_SDL): asi "a:b12" de controller.map es el
// mismo boton en el menu y en el juego.

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
