#!/usr/bin/env python3
"""tools/rpgmaker_mv_model.py: saca una funcion original de un juego de RPG Maker MV,
tal cual, para el MODEL de un hook (runtime/engines/rpgmaker-mv/).

  python3 tools/rpgmaker_mv_model.py <carpeta js del juego> <funcion> [<funcion>...]

  python3 tools/rpgmaker_mv_model.py "juego/www/js" Game_Event.prototype.isStarting

Busca la funcion en los archivos de RPG Maker (rpg_*.js; en MZ rmmz_*.js y
libs/pixi.js) y en los plugins (plugins/*.js). Sirve tambien para MZ
(runtime/engines/rpgmaker-mz/). Si esta en varios, la que cuenta es la ultima en cargarse: la
de un plugin cambia la de RPG Maker (el orden de los plugins es el de
plugins.js). Escribe de que archivo sale y la entrada para MODEL:

  'Game_Event.isStarting': function() {
      return this._starting;
  },

Por que: un hook solo cambia una funcion si es la que espera, y lo mira con
__quixEngine.sameCode (runtime/engines/same-code.js), que compara el texto sin
espacios. El modelo tiene que ser el original exacto: copiado del juego, no
escrito a mano.
"""
import json
import os
import re
import sys


def functions(path):
    """Las asignaciones 'Nombre = function (...) {...};' de un archivo, con su texto."""
    src = open(path, encoding="utf-8", errors="replace").read()
    for m in re.finditer(r"(?m)^[ \t]*([\w$.]+)\s*=\s*(function\s*\([^)]*\)\s*\{)", src):
        start, depth, i = m.start(2), 0, m.start(2)
        quote = None
        while i < len(src):
            c = src[i]
            if quote:
                if c == "\\":
                    i += 1
                elif c == quote:
                    quote = None
            elif c in "'\"":
                quote = c
            elif c == "/" and src[i + 1:i + 2] == "/":
                i = src.find("\n", i)
                if i < 0:
                    break
            elif c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        line = src.count("\n", 0, m.start(1)) + 1
        yield m.group(1), src[start:i + 1], line


def plugin_order(js):
    """Los plugins activos de plugins.js, en orden de carga."""
    try:
        text = open(os.path.join(js, "plugins.js"), encoding="utf-8").read()
        data = json.loads(text[text.index("["):text.rindex("]") + 1])
        return [p["name"] + ".js" for p in data if p.get("status")]
    except (OSError, ValueError):
        return []


def short(name):
    """Game_Event.prototype.isStarting -> Game_Event.isStarting"""
    return name.replace(".prototype.", ".")


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    js, wanted = sys.argv[1], sys.argv[2:]
    # MV: rpg_*.js; MZ: rmmz_*.js, y antes PixiJS (libs/pixi.js)
    files = sorted(f for f in os.listdir(js) if f.startswith(("rpg_", "rmmz_")) and f.endswith(".js"))
    files = [os.path.join(js, f) for f in files]
    if os.path.exists(os.path.join(js, "libs", "pixi.js")) and files and "rmmz_" in files[0]:
        files.insert(0, os.path.join(js, "libs", "pixi.js"))
    files += [os.path.join(js, "plugins", p) for p in plugin_order(js)
              if os.path.exists(os.path.join(js, "plugins", p))]
    for want in wanted:
        found = []
        for path in files:
            for name, text, line in functions(path):
                if name == want:
                    found.append((path, line, text))
        if not found:
            print("%s: no esta en %s" % (want, js))
            continue
        path, line, text = found[-1]   # la ultima en cargarse es la que se usa
        rel = os.path.relpath(path, js)
        print("%s: %s:%d" % (want, rel, line))
        if len(found) > 1:
            print("  (tambien en %s: la de %s la cambia)" %
                  (", ".join("%s:%d" % (os.path.relpath(p, js), l) for p, l, _ in found[:-1]), rel))
        body = text.split("\n")
        body[-1] += ","
        print("    '%s': %s" % (short(want), body[0]))
        for l in body[1:]:
            print("    " + l)


if __name__ == "__main__":
    main()
