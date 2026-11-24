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

// platform/android/transfer.c: el JNI de TransferActivity (Transfer Save en
// Android): las partidas y su backup, Send Save y Receive Save. El juego lo
// abre game.c. TransferActivity llama a sendPoll o receivePoll cada pocos
// milisegundos mientras se ve.
//
// Adaptado de tamatama (src/platform/android/transfer.c, MIT):
// https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/src/platform/android/transfer.c

#include <jni.h>
#include <string.h>
#include <time.h>
#include "game/achievements/achievements.h"
#include "platform/android/game.h"
#include "transfer/receiver.h"
#include "transfer/saves/saves.h"
#include "transfer/sender.h"

#define TRANSFER(name) Java_com_lowlevel1989_quixjs_TransferActivity_##name

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
  if (!game_loaded || !saves_find(backup ? 1 : 0, &set)) return jni_strings(env, NULL, 0);
  saves_date_text(set.date, date, sizeof date);
  const char *values[] = {set.summary, date};
  return jni_strings(env, values, 2);
}

JNIEXPORT jboolean JNICALL TRANSFER(restore)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  int ok = game_loaded && saves_restore_backup();
  if (ok) achievements_reload();
  return ok ? JNI_TRUE : JNI_FALSE;
}

// Delete Save: 1 bien, 0 si falla, -1 si no hay partidas
JNIEXPORT jint JNICALL TRANSFER(deleteSave)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  return game_loaded ? saves_delete() : -1;
}

// ---------- Send Save

// {nombre, firma} de este dispositivo, o null sin red
JNIEXPORT jobjectArray JNICALL TRANSFER(sendOpen)(JNIEnv *env, jclass type) {
  (void)type;
  if (!sender_open(&sender)) return NULL;
  const char *values[] = {sender.name, sender.signature};
  return jni_strings(env, values, 2);
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
  return jni_strings(env, values, 2 * sender.count);
}

// Manda las partidas al de la firma signature: null si empieza, o por que no
JNIEXPORT jstring JNICALL TRANSFER(sendStart)(JNIEnv *env, jclass type, jstring signature) {
  (void)type;
  static SaveSet set;
  if (!game_loaded || !saves_find(0, &set)) return jni_string(env, "there are no saves to send");
  const char *text = (*env)->GetStringUTFChars(env, signature, NULL);
  int device = -1;
  for (int i = 0; i < sender.count; i++) {
    if (strcmp(sender.devices[i].signature, text) == 0) device = i;
  }
  (*env)->ReleaseStringUTFChars(env, signature, text);
  if (device < 0) return jni_string(env, "that device is gone");
  return jni_string(env, sender_start(&sender, device, game_id, &set));
}

// Cuanto va del envio (0-100), o -1 si aun no se mandan los trozos
JNIEXPORT jint JNICALL TRANSFER(sendProgress)(JNIEnv *env, jclass type) {
  (void)env, (void)type;
  if (sender.stage != SENDER_SENDING || sender.chunks <= 1) return -1;
  return sender.chunk * 100 / sender.chunks;
}

JNIEXPORT jstring JNICALL TRANSFER(sendResult)(JNIEnv *env, jclass type) {
  (void)type;
  return jni_string(env, sender.result);
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
  return jni_strings(env, values, 2);
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
  if (event == RECEIVER_DONE) achievements_reload();
  return event;
}

// La oferta: {nombre, firma, resumen, fecha}
JNIEXPORT jobjectArray JNICALL TRANSFER(receiveOffer)(JNIEnv *env, jclass type) {
  (void)type;
  const Offer *o = &receiver.offer;
  char date[32];
  saves_date_text((time_t)o->date, date, sizeof date);
  const char *values[] = {o->name, o->signature, o->summary, date};
  return jni_strings(env, values, 4);
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
  return jni_string(env, receiver.result);
}

// Lo ultimo que paso (una oferta rechazada...), o null
JNIEXPORT jstring JNICALL TRANSFER(receiveNotice)(JNIEnv *env, jclass type) {
  (void)type;
  return jni_string(env, receiver.notice);
}
