# cmake/jsc.cmake: JavaScriptCore, el motor de JS, desde
# runtime/thirdparty/webkit (con los cambios de quixjs), estatico.
#
# Deja el target jsc (make <plataforma>-jsc) y QUIX_JSC_DIR (sus cabeceras y
# sus librerias). En el PC se compila con clang y usa el ICU del sistema. En
# las portatiles y en Android, con su toolchain, y lleva su propio ICU (y en
# las portatiles su libc++): cmake/jsc/, que deja todo en obj/deps/jsc-<arch>.
# QUIX_JSC_JOBS: cuantos compiladores a la vez (3: la RAM manda).

include(ExternalProject)

set(QUIX_WEBKIT ${CMAKE_SOURCE_DIR}/runtime/thirdparty/webkit)
set(QUIX_JSC_JOBS 3 CACHE STRING "compiladores a la vez para JavaScriptCore")

# el codigo esta en el proyecto: comprobar que esta y que lleva los cambios
if(NOT EXISTS ${QUIX_WEBKIT}/JavaScriptCore/CMakeLists.txt)
  message(FATAL_ERROR "falta runtime/thirdparty/webkit (el codigo de JavaScriptCore)")
endif()
file(STRINGS ${QUIX_WEBKIT}/cmake/OptionsJSCOnly.cmake _quix_change REGEX "ENABLE_API_TESTS OFF\\)   # quixjs")
if(NOT _quix_change)
  message(FATAL_ERROR "runtime/thirdparty/webkit: falta el cambio de OptionsJSCOnly.cmake")
endif()

# las opciones de JavaScriptCore, las mismas en todas las plataformas
set(QUIX_JSC_OPTIONS -DPORT=JSCOnly -DENABLE_STATIC_JSC=ON -DCMAKE_BUILD_TYPE=Release
  -DDEVELOPER_MODE=OFF -DUSE_LIBBACKTRACE=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON)

# quix_jsc(<carpeta> [DEPENDS <targets>] [INSTALL <comando>] [ARGS <opciones>]):
# compila JavaScriptCore en esa carpeta; ninja no repite lo que ya esta hecho
function(quix_jsc dir)
  cmake_parse_arguments(_jsc "" "" "DEPENDS;INSTALL;ARGS;BYPRODUCTS" ${ARGN})
  ExternalProject_Add(jsc
    SOURCE_DIR ${QUIX_WEBKIT}
    BINARY_DIR ${dir}
    DEPENDS ${_jsc_DEPENDS}
    CMAKE_GENERATOR Ninja
    CMAKE_ARGS ${QUIX_JSC_OPTIONS} ${_jsc_ARGS}
    BUILD_COMMAND nice -n 10 ninja -C ${dir} -j ${QUIX_JSC_JOBS} JavaScriptCore
    BUILD_ALWAYS ON
    INSTALL_COMMAND "${_jsc_INSTALL}"
    BUILD_BYPRODUCTS ${_jsc_BYPRODUCTS})
endfunction()

if(QUIX_PLATFORM STREQUAL "pc")
  set(QUIX_JSC_DIR ${CMAKE_SOURCE_DIR}/obj/deps/jsc-build/${CMAKE_SYSTEM_PROCESSOR}-host/jsc)
  quix_jsc(${QUIX_JSC_DIR} ARGS -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
    BYPRODUCTS ${QUIX_JSC_DIR}/lib/libJavaScriptCore.a ${QUIX_JSC_DIR}/lib/libWTF.a ${QUIX_JSC_DIR}/lib/libbmalloc.a)
  return()
endif()

# cruzado: aarch64 (PortMaster), android-arm64 o android-x86_64
if(QUIX_PLATFORM STREQUAL "android")
  set(QUIX_JSC_ARCH android-${_arch})
else()
  set(QUIX_JSC_ARCH aarch64)
endif()
set(QUIX_JSC_WORK ${CMAKE_SOURCE_DIR}/obj/deps/jsc-build/${QUIX_JSC_ARCH})
set(QUIX_JSC_PREFIX ${QUIX_JSC_WORK}/prefix)   # libc++ e ICU instalados
set(QUIX_JSC_DIR ${CMAKE_SOURCE_DIR}/obj/deps/jsc-${QUIX_JSC_ARCH})
include(cmake/jsc/sources.cmake)
if(QUIX_PLATFORM STREQUAL "portmaster")
  include(cmake/jsc/libcxx.cmake)
else()
  # Android: la libc++ estatica del NDK
  set(_ndk_lib ${CMAKE_SYSROOT}/usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-android)
  set(QUIX_LIBCXX_A ${_ndk_lib}/libc++_static.a)
  set(QUIX_LIBCXXABI_A ${_ndk_lib}/libc++abi.a)
  set(QUIX_LIBCXX_LICENSES ${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/NOTICE=libc++-NDK.txt)
  set(QUIX_JSC_ABOUT "libc++ del NDK, para ${CMAKE_SYSTEM_PROCESSOR}-linux-android (Android API 28).")
endif()
include(cmake/jsc/icu.cmake)

if(QUIX_PLATFORM STREQUAL "android")
  set(_toolchain -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/toolchains/android.cmake
    -DQUIX_ANDROID_ABI=${QUIX_ANDROID_ABI} -DCMAKE_FIND_ROOT_PATH=${QUIX_JSC_PREFIX})
else()
  set(_toolchain -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/toolchains/portmaster.cmake
    -DQUIX_LIBCXX=${QUIX_JSC_PREFIX})
endif()
string(JOIN "|" _licenses ${QUIX_LIBCXX_LICENSES})   # una lista en un solo argumento
set(_libs JavaScriptCore WTF bmalloc icui18n icuuc icudata c++ c++abi)
list(TRANSFORM _libs REPLACE "(.+)" "${QUIX_JSC_DIR}/lib/lib\\1.a")
quix_jsc(${QUIX_JSC_WORK}/jsc DEPENDS icu
  ARGS ${_toolchain} -DICU_ROOT=${QUIX_JSC_PREFIX}
  INSTALL ${CMAKE_COMMAND} -DAR=${CMAKE_AR} -DBUILD=${QUIX_JSC_WORK}/jsc -DPREFIX=${QUIX_JSC_PREFIX}
    -DOUT=${QUIX_JSC_DIR} -DWEBKIT=${QUIX_WEBKIT} -DICU_SRC=${QUIX_ICU_SRC}
    -DLIBCXX_A=${QUIX_LIBCXX_A} -DLIBCXXABI_A=${QUIX_LIBCXXABI_A} -DLIBCXX_LICENSES=${_licenses}
    -DABOUT=${QUIX_JSC_ABOUT} -P ${CMAKE_SOURCE_DIR}/cmake/jsc/install.cmake
  BYPRODUCTS ${_libs})
