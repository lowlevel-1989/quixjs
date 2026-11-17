/*
 * quixjs: funciones nativas de quixjs dentro de la VM de JavaScriptCore.
 *
 * Las funciones de C de quixjs entraban por la API publica (un objeto con
 * callAsFunction): en cada llamada, el lock de la VM en cada conversion,
 * cada valor convertido dos veces y el resultado protegido ante el
 * recolector; ~2,7 us por llamada en una RG CubeXX. Aqui son funciones
 * nativas de JSC de verdad (JSNativeStdFunction, como las suyas): los
 * argumentos se leen de la pila de la llamada y se pasan ya convertidos al
 * formato de quixjs (QuixValue = el JSValue de runtime/js/js.h).
 */
#pragma once

#include "JSBase.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// la misma forma que JSValue en runtime/js/js.h
typedef struct QuixValue {
    const void* ref;   // JSValueRef (tag 4)
    double num;        // numero, booleano (0/1) o, con tag 4, el tipo de ref
    int32_t tag;       // 0 undefined, 1 null, 2 bool, 3 numero, 4 ref, 5 excepcion
} QuixValue;

// exception: si la funcion lanza, deja aqui el valor lanzado
typedef QuixValue (*QuixNativeCallback)(void* priv, JSContextRef ctx, QuixValue thisValue, int argc, const QuixValue* argv, JSValueRef* exception);
typedef void (*QuixNativeFinalize)(void* priv);

// Una funcion nativa de JSC que llama a cb con priv; fin(priv) cuando el
// recolector la suelta
JSObjectRef JSQuixMakeNativeFunction(JSContextRef ctx, const char* name, unsigned length, QuixNativeCallback cb, QuixNativeFinalize fin, void* priv);

#ifdef __cplusplus
}
#endif
