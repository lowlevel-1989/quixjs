#!/usr/bin/env python3
"""tools/embed_shaders.py: mete los shaders de runtime/gfx/gl/shaders/ en el binario.
Lo usa el Makefile, antes de compilar.

  python3 tools/embed_shaders.py runtime/gfx/gl/shaders obj/cmake/pc/gen/shaders.h

Cada archivo pasa a ser una cadena de C: runtime/gfx/gl/shaders/gpu2d.frag es
SHADER_GPU2D_FRAG. Asi los shaders se editan como GLSL y quixjs sigue siendo
un solo programa, sin archivos que buscar al arrancar.
"""
import os
import sys


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    folder, out = sys.argv[1], sys.argv[2]
    lines = ["// generado por tools/embed_shaders.py desde " + folder + ": no editar", "#pragma once", ""]
    for name in sorted(os.listdir(folder)):
        if not name.endswith((".vert", ".frag")):
            continue
        with open(os.path.join(folder, name)) as f:
            text = f.read()
        ident = "SHADER_" + name.replace(".", "_").replace("-", "_").upper()
        lines.append("static const char " + ident + "[] =")
        for line in text.splitlines():
            escaped = line.replace("\\", "\\\\").replace('"', '\\"')
            lines.append('  "' + escaped + '\\n"')
        lines[-1] += ";"
        lines.append("")
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    data = "\n".join(lines)
    # solo si cambia: si no, make recompilaria todo cada vez
    if os.path.exists(out) and open(out).read() == data:
        return
    with open(out, "w") as f:
        f.write(data)


if __name__ == "__main__":
    main()
