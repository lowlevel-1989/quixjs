# cmake/jsc/install.cmake: deja JavaScriptCore cruzado listo para enlazar,
# en OUT (obj/deps/jsc-<arch>):
#   lib/*.a                     JavaScriptCore, WTF, bmalloc, ICU, libc++
#   include/JavaScriptCore/*.h  la API en C
#   licenses/                   las licencias de todo lo anterior
# Solo copia lo que cambio: quixjs no se vuelve a enlazar por nada.
#
#   cmake -DAR=... -DBUILD=<build de JSC> -DPREFIX=<ICU instalada> -DOUT=...
#     -DWEBKIT=... -DICU_SRC=... -DLIBCXX_A=... -DLIBCXXABI_A=...
#     -DLIBCXX_LICENSES=<de>=<a>|... -DABOUT=<texto> -P cmake/jsc/install.cmake

file(MAKE_DIRECTORY ${OUT}/lib ${OUT}/include/JavaScriptCore ${OUT}/licenses)

# copy(<de> <a>): solo si cambio
function(copy from to)
  file(COPY_FILE ${from} ${to} ONLY_IF_DIFFERENT)
endfunction()

# las .a de WebKit: WebKit las deja "thin" (solo rutas a los .o del build); se
# rehacen como archivos normales, con los objetos dentro
file(GLOB _archives ${BUILD}/lib/*.a)
foreach(_a ${_archives})
  get_filename_component(_name ${_a} NAME)
  file(READ ${_a} _magic LIMIT 8 HEX)
  if(_magic STREQUAL "213c7468696e3e0a")   # "!<thin>\n"
    execute_process(COMMAND ${AR} t ${_a} OUTPUT_VARIABLE _objs WORKING_DIRECTORY ${BUILD}/lib)
    string(REPLACE "\n" ";" _objs "${_objs}")
    list(FILTER _objs EXCLUDE REGEX "^$")
    file(REMOVE ${OUT}/${_name}.tmp)
    execute_process(COMMAND ${AR} qcs ${OUT}/${_name}.tmp ${_objs} WORKING_DIRECTORY ${BUILD}/lib
      COMMAND_ERROR_IS_FATAL ANY)
    copy(${OUT}/${_name}.tmp ${OUT}/lib/${_name})
    file(REMOVE ${OUT}/${_name}.tmp)
  else()
    copy(${_a} ${OUT}/lib/${_name})
  endif()
endforeach()
foreach(_lib icudata icuuc icui18n)
  copy(${PREFIX}/lib/lib${_lib}.a ${OUT}/lib/lib${_lib}.a)
endforeach()
copy(${LIBCXX_A} ${OUT}/lib/libc++.a)
copy(${LIBCXXABI_A} ${OUT}/lib/libc++abi.a)

# la API publica en C
set(_api ${WEBKIT}/JavaScriptCore/API)
foreach(_h JavaScript JSBase JSContextRef JSStringRef JSObjectRef JSTypedArray JSValueRef
    WebKitAvailability JSBasePrivate JSContextRefPrivate JSObjectRefPrivate JSStringRefPrivate
    JSWeakObjectMapRefPrivate)
  copy(${_api}/${_h}.h ${OUT}/include/JavaScriptCore/${_h}.h)
endforeach()

# licencias: JSC es LGPL-2 (y BSD en buena parte), WTF y bmalloc BSD, mas el
# codigo de terceros que llevan dentro. La BSD, el comentario del principio
# de un archivo.
set(_l ${OUT}/licenses)
function(header_license from to)
  file(READ ${from} _text)
  string(FIND "${_text}" "*/" _end)
  math(EXPR _end "${_end} + 3")
  string(SUBSTRING "${_text}" 0 ${_end} _text)
  file(WRITE ${to} "${_text}")
endfunction()
copy(${WEBKIT}/JavaScriptCore/COPYING.LIB ${_l}/JavaScriptCore-LGPL-2.txt)
header_license(${_api}/JSBase.h ${_l}/JavaScriptCore-BSD.txt)
header_license(${WEBKIT}/WTF/wtf/Assertions.h ${_l}/WTF-BSD.txt)
header_license(${WEBKIT}/bmalloc/bmalloc/bmalloc.h ${_l}/bmalloc-BSD.txt)
file(GLOB _wtf ${WEBKIT}/WTF/LICENSE-*.txt)
foreach(_f ${_wtf})
  get_filename_component(_name ${_f} NAME_WE)
  string(REPLACE "LICENSE-" "" _name ${_name})
  copy(${_f} ${_l}/WTF-${_name}.txt)
endforeach()
copy(${WEBKIT}/WTF/wtf/fast_float/LICENSE ${_l}/WTF-fast_float.txt)
file(READ ${WEBKIT}/WTF/wtf/dtoa/LICENSE _a)
file(READ ${WEBKIT}/WTF/wtf/dtoa/COPYING _b)
file(WRITE ${_l}/WTF-dtoa.txt "${_a}${_b}")
copy(${WEBKIT}/WTF/wtf/simdutf/LICENSE-simdutf.txt ${_l}/WTF-simdutf.txt)
copy(${ICU_SRC}/LICENSE ${_l}/ICU.txt)
string(REPLACE "|" ";" LIBCXX_LICENSES "${LIBCXX_LICENSES}")
foreach(_item ${LIBCXX_LICENSES})
  string(REPLACE "=" ";" _pair ${_item})
  list(GET _pair 0 _from)
  list(GET _pair 1 _to)
  copy(${_from} ${_l}/${_to})
endforeach()

file(WRITE ${OUT}/README.txt "\
JavaScriptCore de WebKit webkitgtk-2.50.6 (port JSCOnly, estatico), ICU 77.1 y
${ABOUT}
Construido con cmake/jsc.cmake de quixjs.

Enlazar (C, con clang y lld): -I${OUT}/include y
  -L${OUT}/lib -lJavaScriptCore -lWTF -lbmalloc
  -licui18n -licuuc -licudata -l:libc++.a -l:libc++abi.a
  -static-libgcc -lpthread -ldl -lm -Wl,--gc-sections
")
