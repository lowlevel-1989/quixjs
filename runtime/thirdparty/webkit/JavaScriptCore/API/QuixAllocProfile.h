/*
 * quixjs: muestreo de asignaciones del monton de JavaScriptCore.
 *
 * Cada vez que un asignador agota su lista libre (LocalAllocator::
 * allocateSlowCase) se apuntan los bytes que repartio a la funcion JS que
 * estaba corriendo (la de arriba de la pila y la que la llamo). Cuanto mas
 * memoria crea una funcion, mas veces cae aqui: un muestreo proporcional a
 * los bytes, sin coste si esta apagado.
 */
#pragma once

#include <stddef.h>

namespace JSC {
class VM;
extern bool g_quixAllocSampling;
void quixAllocSample(VM&, size_t bytes);
}

extern "C" {
// Encender (borra lo anterior) o apagar
void JSQuixAllocProfileEnable(bool on);
// Lo apuntado, en texto: "bytes\tfuncion (archivo:linea) < quien la llamo\n"
// por linea, sin ordenar, y una primera linea "total\t<bytes>". El que llama
// libera el texto con free().
char* JSQuixAllocProfileTake(void);
}
