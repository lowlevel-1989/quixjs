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

# ---- OpenGL ES 3.0 (WebGL 2): python3 tools/gen_gl.py ES2.h /usr/include/GLES3/gl3.h
# runtime/gfx/gl3_funcs.h (las funciones que ES 3.0 agrega, sus tipos y
# constantes, sin necesitar GLES3 en el sistema) y
# runtime/binding/webgl2_constants.h (las constantes que WebGL 2 agrega)
if len(sys.argv) > 2:
    t3 = open(sys.argv[2]).read()
    es3 = t3[t3.index("#ifndef GL_ES_VERSION_3_0"):]
    es3 = es3[:es3.index("#endif /* GL_ES_VERSION_3_0 */")]
    typedefs = re.findall(r"typedef [^;]*\(GL_APIENTRYP (PFNGL\w+PROC)\)[^;]*;", es3)
    lines = re.findall(r"(typedef [^;]*\(GL_APIENTRYP PFNGL\w+PROC\)[^;]*;)", es3)
    consts3 = re.findall(r"#define\s+GL_(\w+)\s+(0x[0-9A-Fa-f]+u?|\d+u?)\b", es3)
    have = set(n for n, _ in consts)
    with open(os.path.join(root, "runtime/gfx/gl3_funcs.h"), "w") as f:
        f.write(lic)
        f.write("\n// Generado por tools/gen_gl.py: no editar a mano.\n")
        f.write("// Lo que OpenGL ES 3.0 agrega a ES 2.0 (de GLES3/gl3.h de Khronos): sus\n")
        f.write("// tipos y constantes, y sus funciones como QX_GL3(TIPO_PFN, Nombre).\n\n")
        f.write("#ifndef QX_GL3\n#ifndef QX_GL3_TYPES\n#define QX_GL3_TYPES\n#include <stdint.h>\n")
        f.write("typedef struct __GLsync *GLsync;\ntypedef int64_t GLint64;\ntypedef uint64_t GLuint64;\ntypedef uint16_t GLhalf;\n\n")
        for n, v in consts3:
            if n in have or n == "ES_VERSION_3_0":
                continue
            f.write("#ifndef GL_%s\n#define GL_%s %s\n#endif\n" % (n, n, v))
        f.write("\n")
        for l in lines:
            f.write(l.replace("GL_APIENTRYP", "GL_APIENTRY *") + "\n")
        f.write("#endif\n#else\n")
        protos = {n.upper(): n for n in re.findall(r"GL_APICALL\s+[^;]*?GL_APIENTRY\s+gl(\w+)\s*\(", es3)}
        for name in typedefs:
            f.write("QX_GL3(%s, %s)\n" % (name, protos[name[5:-4]]))
        f.write("#endif\n")
    with open(os.path.join(root, "runtime/binding/webgl2_constants.h"), "w") as f:
        f.write(lic)
        f.write("\n// Generado por tools/gen_gl.py: no editar a mano.\n")
        f.write("// Las constantes que WebGL 2 agrega a WebGL 1 (las de OpenGL ES 3.0)\n\n")
        f.write("#pragma once\n\n#include \"js/js.h\"\n\n")
        f.write("static const JSCFunctionListEntry js_webgl2_constants[] = {\n")
        n3 = 0
        for n, v in consts3 + [("MAX_CLIENT_WAIT_TIMEOUT_WEBGL", "0x9247")]:
            if n in have or n == "ES_VERSION_3_0" or n in seen:
                continue
            seen.add(n)
            v = v.rstrip("u")
            if v.startswith("0x") and int(v, 16) > 0x7fffffff:
                continue   # TIMEOUT_IGNORED: no cabe en un int32
            f.write('  JS_PROP_INT32_DEF("%s", %s, JS_PROP_ENUMERABLE),\n' % (n, v))
            n3 += 1
        f.write("};\n")
    print("ES 3.0: %d functions, %d constants" % (len(typedefs), n3))
