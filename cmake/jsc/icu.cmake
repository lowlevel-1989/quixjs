# cmake/jsc/icu.cmake: ICU (Unicode, para Intl de JavaScriptCore), estatica
# y con los datos recortados por cmake/jsc/icu-filter.json: unos pocos
# idiomas y fuera lo que JSC no usa (conversores de juegos de caracteres,
# transliteracion, nombres de caracteres).
#
# Primero para el PC (icu_host: sus herramientas generan los datos; una sola
# vez para todas las arquitecturas) y luego cruzada (icu, instalada en
# QUIX_JSC_PREFIX). Usa el compilador del toolchain.

set(_filter ICU_DATA_FILTER_FILE=${CMAKE_CURRENT_LIST_DIR}/icu-filter.json)
set(_host ${CMAKE_SOURCE_DIR}/obj/deps/jsc-build/icu-host)
set(_common --enable-static --disable-shared --disable-tests --disable-samples)

ExternalProject_Add(icu_host
  SOURCE_DIR ${QUIX_ICU_SRC}
  BINARY_DIR ${_host}
  # compartida entre arquitecturas: no se vuelve a configurar si ya esta
  CONFIGURE_COMMAND sh -c "test -f config.status || exec \"$@\"" sh
    ${CMAKE_COMMAND} -E env ${_filter} CC=clang CXX=clang++ <SOURCE_DIR>/source/configure ${_common}
  BUILD_COMMAND nice -n 10 make -j ${QUIX_JSC_JOBS}
  INSTALL_COMMAND "")

# las opciones del compilador cruzado; en las portatiles, con la libc++ de
# cmake/jsc/libcxx.cmake
set(_cflags "--target=${CMAKE_C_COMPILER_TARGET} --sysroot=${CMAKE_SYSROOT} ${CMAKE_C_FLAGS} -O2 -fPIC")
if(QUIX_PLATFORM STREQUAL "portmaster")
  set(_cxx clang++)
  set(_cxxflags "${_cflags} -stdlib=libc++ -nostdinc++ -isystem ${QUIX_JSC_PREFIX}/include/c++/v1")
  set(_ldflags "-fuse-ld=lld -stdlib=libc++ -L${QUIX_JSC_PREFIX}/lib -static-libstdc++ -static-libgcc -pthread")
  set(_triple aarch64-linux-gnu)
  set(_depends icu_host libcxx)
else()
  set(_cxx ${CMAKE_CXX_COMPILER})
  set(_cxxflags "${_cflags}")
  set(_ldflags "-static-libstdc++")
  set(_triple ${CMAKE_SYSTEM_PROCESSOR}-linux-android)
  set(_depends icu_host)
endif()

ExternalProject_Add(icu
  SOURCE_DIR ${QUIX_ICU_SRC}
  BINARY_DIR ${QUIX_JSC_WORK}/icu
  DEPENDS ${_depends}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env ${_filter} CC=${CMAKE_C_COMPILER} CXX=${_cxx}
    AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CFLAGS=${_cflags} CXXFLAGS=${_cxxflags} LDFLAGS=${_ldflags}
    <SOURCE_DIR>/source/configure --host=${_triple} --with-cross-build=${_host} --prefix=${QUIX_JSC_PREFIX}
    ${_common} --with-data-packaging=static --disable-tools --disable-extras --disable-layoutex
  BUILD_COMMAND nice -n 10 make -j ${QUIX_JSC_JOBS}
  INSTALL_COMMAND make install)
