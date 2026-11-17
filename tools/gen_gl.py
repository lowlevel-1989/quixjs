#!/usr/bin/env python3
"""tools/gen_gl.py: genera runtime/gfx/gl_funcs.h (tabla de funciones de
OpenGL ES 2.0) y runtime/binding/webgl_constants.h (las constantes de WebGL 1)
a partir de la cabecera GLES2 que trae SDL2 (SDL_opengles2_gl2.h).

  python3 tools/gen_gl.py [/usr/include/SDL2/SDL_opengles2_gl2.h]
"""
import os
import re
import sys

header = sys.argv[1] if len(sys.argv) > 1 else "/usr/include/SDL2/SDL_opengles2_gl2.h"
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
text = open(header).read()

funcs = re.findall(r"GL_APICALL\s+[^;]*?GL_APIENTRY\s+gl(\w+)\s*\(", text)
consts = re.findall(r"#define\s+GL_(\w+)\s+(0x[0-9A-Fa-f]+|\d+)\b", text)

skip = {"ES_VERSION_2_0", "GLES_PROTOTYPES"}
# las de WebGL que no estan en GLES2
webgl_extra = [
    ("UNPACK_FLIP_Y_WEBGL", "0x9240"), ("UNPACK_PREMULTIPLY_ALPHA_WEBGL", "0x9241"),
    ("CONTEXT_LOST_WEBGL", "0x9242"), ("UNPACK_COLORSPACE_CONVERSION_WEBGL", "0x9243"),
    ("BROWSER_DEFAULT_WEBGL", "0x9244"), ("DEPTH_STENCIL", "0x84F9"),
    ("DEPTH_STENCIL_ATTACHMENT", "0x821A"),
]

lic = open(os.path.join(root, "runtime/core/main.c")).read().split("*/", 1)[0] + "*/\n"

with open(os.path.join(root, "runtime/gfx/gl_funcs.h"), "w") as f:
    f.write(lic)
    f.write("\n// Generado por tools/gen_gl.py: no editar a mano.\n")
    f.write("// Las funciones de OpenGL ES 2.0: QX_GL(TIPO_PFN, Nombre)\n\n")
    for name in funcs:
        f.write("QX_GL(PFNGL%sPROC, %s)\n" % (name.upper(), name))

with open(os.path.join(root, "runtime/binding/webgl_constants.h"), "w") as f:
    f.write(lic)
    f.write("\n// Generado por tools/gen_gl.py: no editar a mano.\n")
    f.write("// Las constantes de WebGL 1 (las de OpenGL ES 2.0 sin GL_ y las propias)\n\n")
    f.write("#pragma once\n\n#include \"js/js.h\"\n\n")
    f.write("static const JSCFunctionListEntry js_webgl_constants[] = {\n")
    seen = set()
    for name, val in consts + webgl_extra:
        if name in skip or name in seen:
            continue
        seen.add(name)
        f.write('  JS_PROP_INT32_DEF("%s", %s, JS_PROP_ENUMERABLE),\n' % (name, val))
    f.write("};\n")

print("%d functions, %d constants" % (len(funcs), len(seen)))
