/*
 * quixjs: muestreo de asignaciones (ver QuixAllocProfile.h).
 */
#include "config.h"
#include "QuixAllocProfile.h"

#include "CallFrame.h"
#include "CodeBlock.h"
#include "JSCInlines.h"
#include "StackVisitor.h"
#include <stdlib.h>
#include <string.h>
#include <wtf/HashMap.h>
#include <wtf/text/MakeString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace JSC {

bool g_quixAllocSampling = false;
static HashMap<String, uint64_t>* s_bytes;
static uint64_t s_total;

// El nombre sin crear nada en el monton de JS (estamos dentro de un reparto
// de memoria): del bloque de codigo ya compilado
static String frameName(StackVisitor& visitor)
{
    CodeBlock* cb = visitor->codeBlock();
    CString inferred = cb->inferredName();
    String name = inferred.length() ? String::fromUTF8(inferred.span()) : "(anonymous)"_s;
    String url = cb->ownerExecutable()->sourceURL();
    size_t slash = url.reverseFind('/');
    if (slash != notFound)
        url = url.substring(slash + 1);
    // la linea donde empieza la funcion (fija): la de la instruccion en curso
    // puede estar a medio cambiar si el JIT esta recompilando
    return makeString(name, " ("_s, url, ':', cb->ownerExecutable()->firstLine(), ')');
}

static bool s_inSample;   // si algo de aqui dentro vuelve a repartir memoria

void quixAllocSample(VM& vm, size_t bytes)
{
    if (!bytes || s_inSample)
        return;
    s_inSample = true;
    if (!s_bytes)
        s_bytes = new HashMap<String, uint64_t>();
    s_total += bytes;
    String key;
    int found = 0;
    CallFrame* top = vm.topCallFrame;
    if (top) {
        StackVisitor::visit(top, vm, [&](StackVisitor& visitor) -> IterationStatus {
            if (!visitor->codeBlock())
                return IterationStatus::Continue;   // nativas y marcos sin codigo
            String f = frameName(visitor);
            key = found ? makeString(key, " < "_s, f) : f;
            return ++found == 2 ? IterationStatus::Done : IterationStatus::Continue;
        });
    }
    if (!found)
        key = "(no JS on the stack: quixjs, timers, GC)"_s;
    auto result = s_bytes->add(key, 0);
    result.iterator->value += bytes;
    s_inSample = false;
}

} // namespace JSC

void JSQuixAllocProfileEnable(bool on)
{
    if (on) {
        if (JSC::s_bytes)
            JSC::s_bytes->clear();
        JSC::s_total = 0;
    }
    JSC::g_quixAllocSampling = on;
}

char* JSQuixAllocProfileTake(void)
{
    StringBuilder out;
    out.append("total\t"_s, JSC::s_total, '\n');
    if (JSC::s_bytes) {
        for (auto& entry : *JSC::s_bytes)
            out.append(entry.value, '\t', entry.key, '\n');
        JSC::s_bytes->clear();
    }
    JSC::s_total = 0;
    CString utf8 = out.toString().utf8();
    char* copy = static_cast<char*>(malloc(utf8.length() + 1));
    if (copy)
        memcpy(copy, utf8.data(), utf8.length() + 1);
    return copy;
}
