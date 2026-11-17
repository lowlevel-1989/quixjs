/*
 * quixjs: funciones nativas de quixjs dentro de la VM (ver QuixNativeFunction.h).
 */
#include "config.h"
#include "QuixNativeFunction.h"

#include "APICast.h"
#include "JSCInlines.h"
#include "JSNativeStdFunction.h"
#include "JSLock.h"
#include <memory>

using namespace JSC;

namespace {

enum { TagUndefined = 0, TagNull, TagBool, TagNumber, TagRef, TagException };
enum { RefObject = 1, RefString, RefSymbol, RefOther };

struct Owner {
    void* priv;
    QuixNativeFinalize fin;
    Owner(void* p, QuixNativeFinalize f) : priv(p), fin(f) { }
    ~Owner()
    {
        if (fin)
            fin(priv);
    }
};

// un valor de JSC al formato de quixjs, sin pasar por la API (sin lock)
inline QuixValue decode(VM& vm, JSValue v)
{
    QuixValue q { nullptr, 0, TagUndefined };
    if (!v || v.isUndefined())
        return q;
    if (v.isNull()) {
        q.tag = TagNull;
        return q;
    }
    if (v.isBoolean()) {
        q.tag = TagBool;
        q.num = v.asBoolean() ? 1 : 0;
        return q;
    }
    if (v.isNumber()) {
        q.tag = TagNumber;
        q.num = v.asNumber();
        return q;
    }
    q.tag = TagRef;
    q.ref = toRef(vm, v);   // celdas: sin crear nada, tambien en 32 bits
    q.num = v.isString() ? RefString : v.isSymbol() ? RefSymbol : v.isObject() ? RefObject : RefOther;
    return q;
}

inline JSValue encode(JSGlobalObject* globalObject, const QuixValue& q)
{
    switch (q.tag) {
    case TagNull:
        return jsNull();
    case TagBool:
        return jsBoolean(q.num != 0);
    case TagNumber:
        return jsNumber(q.num);
    case TagRef:
        return toJS(globalObject, static_cast<JSValueRef>(q.ref));
    default:
        return jsUndefined();
    }
}

} // namespace

JSObjectRef JSQuixMakeNativeFunction(JSContextRef ctx, const char* name, unsigned length, QuixNativeCallback cb, QuixNativeFinalize fin, void* priv)
{
    JSGlobalObject* globalObject = toJS(ctx);
    VM& vm = globalObject->vm();
    JSLockHolder lock(vm);
    std::shared_ptr<Owner> owner = std::make_shared<Owner>(priv, fin);
    NativeStdFunction function = [cb, owner](JSGlobalObject* globalObject, CallFrame* callFrame) -> EncodedJSValue {
        VM& vm = globalObject->vm();
        auto scope = DECLARE_THROW_SCOPE(vm);
        size_t argc = callFrame->argumentCount();
        QuixValue stackArgs[16];
        std::unique_ptr<QuixValue[]> heapArgs;
        QuixValue* args = stackArgs;
        if (argc > 16) {
            heapArgs = std::make_unique<QuixValue[]>(argc);
            args = heapArgs.get();
        }
        for (size_t i = 0; i < argc; ++i)
            args[i] = decode(vm, callFrame->uncheckedArgument(i));
        QuixValue thisValue = decode(vm, callFrame->thisValue());
        JSValueRef exception = nullptr;
        QuixValue result = cb(owner->priv, toRef(globalObject), thisValue, static_cast<int>(argc), args, &exception);
        if (exception) {
            throwException(globalObject, scope, toJS(globalObject, exception));
            return { };
        }
        RELEASE_AND_RETURN(scope, JSValue::encode(encode(globalObject, result)));
    };
    JSNativeStdFunction* f = JSNativeStdFunction::create(vm, globalObject, length, String::fromUTF8(name ? name : ""), WTFMove(function));
    return toRef(f);
}
