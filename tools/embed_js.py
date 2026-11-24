#!/usr/bin/env python3
"""tools/embed_js.py: mete archivos .js de runtime/ en el binario, como cadenas
de C. Lo usa el Makefile, antes de compilar.

Por que: parte de quixjs esta escrita en JS (el polyfill, WebAudio, el modo
nw.js, getGamepads, el resumen del profiler). Antes ese JS estaba dentro del C,
como una cadena con comillas y \\n en cada linea: dificil de leer y de
editar. Ahora es un .js normal junto a su .c, y esta herramienta lo convierte
en la cadena al compilar. quixjs sigue siendo un solo programa: no busca
archivos al arrancar (en una consola no se pueden perder ni quedar viejos).

Un archivo: una constante con su codigo.

  python3 tools/embed_js.py obj/cmake/pc/gen/js_nwjs.h NWJS_JS runtime/binding/nwjs.js

  -> static const char NWJS_JS[] = "...";   (el .c hace #include "js_nwjs.h")

Una carpeta: una tabla {nombre, codigo} con sus .js en orden alfabetico (la
usa binding/polyfill.c, que los ejecuta todos).

  python3 tools/embed_js.py obj/cmake/pc/gen/polyfills.h --table POLYFILLS runtime/binding/polyfill

  -> static const struct { const char *name; const char *src; } POLYFILLS[] = {...};

El .h solo se reescribe si cambia: si no, make recompilaria todo cada vez.
"""
import os
import sys


def c_string(text, indent):
    out = []
    for line in text.splitlines():
        out.append(indent + '"' + line.replace("\\", "\\\\").replace('"', '\\"') + '\\n"')
    return "\n".join(out) if out else indent + '""'


def main():
    args = sys.argv[1:]
    if len(args) == 3:
        out, ident, path = args
        with open(path) as f:
            body = "static const char " + ident + "[] =\n" + c_string(f.read(), "  ") + ";"
        source = path
    elif len(args) == 4 and args[1] == "--table":
        out, _, ident, folder = args
        rows = []
        for name in sorted(os.listdir(folder)):
            if name.endswith(".js"):
                with open(os.path.join(folder, name)) as f:
                    rows.append('  { "' + name + '",\n' + c_string(f.read(), "    ") + " },")
        body = ("static const struct { const char *name; const char *src; } " + ident + "[] = {\n"
                + "\n".join(rows) + "\n};")
        source = folder
    else:
        sys.exit(__doc__)
    data = "// generado por tools/embed_js.py desde " + source + ": no editar\n#pragma once\n\n" + body + "\n"
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    if os.path.exists(out) and open(out).read() == data:
        return
    with open(out, "w") as f:
        f.write(data)


if __name__ == "__main__":
    main()
