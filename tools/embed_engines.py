#!/usr/bin/env python3
"""tools/embed_engines.py: mete los hooks de runtime/engines/<motor>/*.js en el binario.
Lo usa el Makefile, antes de compilar.

  python3 tools/embed_engines.py runtime/engines obj/gen/engines.h

Cada archivo pasa a ser un hook con nombre "<motor>/<archivo sin .js>", por
ejemplo runtime/engines/rpgmaker-mv/request-images-once.js es
"rpgmaker-mv/request-images-once". El archivo es una funcion de JS que
recibe la funcion original del juego y devuelve la que la sustituye.

El hooks.toml de cada motor (sus hooks de siempre, [hooks] como en el
config.toml) va tambien: lo usa un juego con [hooks] engine = "<motor>".

runtime/engines/lib/*.js no son hooks: son modulos que cualquier hook puede
usar con __quixEngine.lib("<archivo sin .js>") (ENGINE_LIBS).
"""
import os
import sys


def c_string(text):
    out = []
    for line in text.splitlines():
        out.append('  "' + line.replace("\\", "\\\\").replace('"', '\\"') + '\\n"')
    return "\n".join(out) if out else '  ""'


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    root, out = sys.argv[1], sys.argv[2]
    hooks = []
    presets = []
    libs = []
    for engine in sorted(os.listdir(root)):
        folder = os.path.join(root, engine)
        if not os.path.isdir(folder):
            continue
        if engine == "lib":
            for name in sorted(os.listdir(folder)):
                if name.endswith(".js"):
                    with open(os.path.join(folder, name)) as f:
                        libs.append((name[:-3], f.read()))
            continue
        preset = os.path.join(folder, "hooks.toml")
        if os.path.exists(preset):
            with open(preset) as f:
                presets.append((engine, f.read()))
        for name in sorted(os.listdir(folder)):
            if name.endswith(".js"):
                with open(os.path.join(folder, name)) as f:
                    hooks.append((engine + "/" + name[:-3], f.read()))
    lines = ["// generado por tools/embed_engines.py desde " + root + ": no editar", "#pragma once", "",
             "typedef struct {", "  const char *name;", "  const char *source;", "} EngineHook;", "",
             "static const EngineHook ENGINE_HOOKS[] = {"]
    for name, src in hooks:
        lines.append('  { "' + name + '",')
        lines.append(c_string(src) + " },")
    lines.append("  { 0, 0 }")
    lines.append("};")
    lines += ["", "typedef struct {", "  const char *engine;", "  const char *hooks;   // su hooks.toml", "} EnginePreset;", "",
              "static const EnginePreset ENGINE_PRESETS[] = {"]
    for engine, text in presets:
        lines.append('  { "' + engine + '",')
        lines.append(c_string(text) + " },")
    lines.append("  { 0, 0 }")
    lines.append("};")
    lines += ["", "// runtime/engines/lib: los modulos de __quixEngine.lib (nombre y codigo)",
              "static const EngineHook ENGINE_LIBS[] = {"]
    for name, src in libs:
        lines.append('  { "' + name + '",')
        lines.append(c_string(src) + " },")
    lines.append("  { 0, 0 }")
    lines.append("};")
    data = "\n".join(lines) + "\n"
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    if os.path.exists(out) and open(out).read() == data:
        return
    with open(out, "w") as f:
        f.write(data)


if __name__ == "__main__":
    main()
