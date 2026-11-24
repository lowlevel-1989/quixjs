# cmake/generated.cmake: lo que se genera antes de compilar y va dentro del
# binario: los shaders, los hooks de motor, el JS de quixjs, la version y
# TRACE.
#
# Deja QUIX_GEN_DIR (la carpeta de las cabeceras generadas) y
# QUIX_GENERATED (la lista, para que se generen antes de compilar). Usa los
# scripts de tools/ (embed_shaders.py, embed_engines.py, embed_js.py).

set(QUIX_GEN_DIR ${CMAKE_BINARY_DIR}/gen)
set(QUIX_GENERATED "")
file(MAKE_DIRECTORY ${QUIX_GEN_DIR})

# quix_embed(<cabecera> <script> <args...> DEPENDS <archivos>): una cabecera
# hecha por un script de tools/, que se rehace si cambian sus archivos
function(quix_embed header script)
  cmake_parse_arguments(E "" "" "ARGS;DEPENDS" ${ARGN})
  add_custom_command(OUTPUT ${QUIX_GEN_DIR}/${header}
    COMMAND python3 ${CMAKE_SOURCE_DIR}/tools/${script} ${QUIX_GEN_DIR}/${header} ${E_ARGS}
    DEPENDS ${CMAKE_SOURCE_DIR}/tools/${script} ${E_DEPENDS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM)
  set(QUIX_GENERATED ${QUIX_GENERATED} ${QUIX_GEN_DIR}/${header} PARENT_SCOPE)
endfunction()

# los shaders (runtime/gfx/gl/shaders)
file(GLOB _shaders CONFIGURE_DEPENDS runtime/gfx/gl/shaders/*.vert runtime/gfx/gl/shaders/*.frag)
add_custom_command(OUTPUT ${QUIX_GEN_DIR}/shaders.h
  COMMAND python3 tools/embed_shaders.py runtime/gfx/gl/shaders ${QUIX_GEN_DIR}/shaders.h
  DEPENDS tools/embed_shaders.py ${_shaders}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM)
list(APPEND QUIX_GENERATED ${QUIX_GEN_DIR}/shaders.h)

# los hooks de motor y su entry.js (runtime/engines)
file(GLOB_RECURSE _engines CONFIGURE_DEPENDS runtime/engines/*.js runtime/engines/*.toml)
add_custom_command(OUTPUT ${QUIX_GEN_DIR}/engines.h
  COMMAND python3 tools/embed_engines.py runtime/engines ${QUIX_GEN_DIR}/engines.h
  DEPENDS tools/embed_engines.py ${_engines}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM)
list(APPEND QUIX_GENERATED ${QUIX_GEN_DIR}/engines.h)

# las carpetas de JS de quixjs, cada una en una tabla
foreach(_part polyfill:polyfills.h:POLYFILLS nwjs:nwjs_parts.h:NWJS_PARTS webview2:webview2_parts.h:WEBVIEW2_PARTS)
  string(REPLACE ":" ";" _p ${_part})
  list(GET _p 0 _dir)
  list(GET _p 1 _header)
  list(GET _p 2 _table)
  file(GLOB _js CONFIGURE_DEPENDS runtime/binding/${_dir}/*.js)
  quix_embed(${_header} embed_js.py ARGS --table ${_table} runtime/binding/${_dir} DEPENDS ${_js})
endforeach()

# los archivos de JS sueltos, cada uno en una cadena
quix_embed(js_webaudio.h embed_js.py ARGS WEBAUDIO_JS runtime/binding/audio/webaudio.js
  DEPENDS runtime/binding/audio/webaudio.js)
quix_embed(js_gamepads.h embed_js.py ARGS GAMEPADS_JS runtime/binding/navigator/gamepads.js
  DEPENDS runtime/binding/navigator/gamepads.js)
quix_embed(js_trace_saves.h embed_js.py ARGS TRACE_SAVES_JS runtime/binding/quix/trace-saves.js
  DEPENDS runtime/binding/quix/trace-saves.js)
quix_embed(js_profiler.h embed_js.py ARGS PROFILER_JS runtime/core/debug/profiler.js
  DEPENDS runtime/core/debug/profiler.js)
quix_embed(js_same_code.h embed_js.py ARGS SAME_CODE_JS runtime/engines/same-code.js
  DEPENDS runtime/engines/same-code.js)

# la version (archivo VERSION de la raiz) y TRACE (QUIX_TRACE): solo se
# reescriben si cambian, asi no se recompila todo sin motivo
file(READ VERSION _version)
string(STRIP "${_version}" _version)
file(CONFIGURE OUTPUT ${QUIX_GEN_DIR}/version.h
  CONTENT "// generado desde VERSION: no editar\n#define QUIX_VERSION \"${_version}\"\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS VERSION)
if(QUIX_TRACE)
  set(_trace 1)
else()
  set(_trace 0)
endif()
file(CONFIGURE OUTPUT ${QUIX_GEN_DIR}/trace_config.h CONTENT "#define QUIX_TRACE ${_trace}\n")
