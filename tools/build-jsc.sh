#!/usr/bin/env bash
# tools/build-jsc.sh: JavaScriptCore estatico para las portatiles (PortMaster).
#
# Construye, contra Debian buster (glibc 2.28, para que corra en firmwares
# viejos y nuevos: ArkOS, ROCKNIX, muOS...), para aarch64 y armhf:
#   1. el sysroot de buster (make PLATFORM=portmaster obj/deps/sysroot-*/.ready)
#   2. libc++ y libc++abi de LLVM (la misma version que el clang del PC),
#      estaticas: el libstdc++ de buster (GCC 8) es viejo para WebKit (C++23)
#   3. ICU, estatica y con los datos recortados (un filtro de datos): primero
#      para el PC (sus herramientas generan los datos) y luego cruzada
#   4. JavaScriptCore, WTF y bmalloc (runtime/thirdparty/webkit, port
#      JSCOnly, ENABLE_STATIC_JSC)
#   5. el resultado en obj/deps/jsc-ARCH:
#        lib/*.a                     JavaScriptCore, WTF, bmalloc, ICU, libc++
#        include/JavaScriptCore/*.h  la API en C
#        licenses/                   las licencias de todo lo anterior
#   6. una prueba, solo si JSC_TEST_SRC da un programa en C que use la API:
#      se enlaza contra lib/ y se comprueba que no pide nada mas nuevo que
#      GLIBC_2.28
#
# Uso: tools/build-jsc.sh [aarch64|armhf|all] [etapa...]
#        tools/build-jsc.sh host
#   etapas: sysroot libcxx icu jsc install test (por defecto, todas en orden)
#   host: JavaScriptCore para este PC (para make y make test), con el clang y
#         el ICU del sistema, en obj/deps/jsc-build/$(uname -m)-host/jsc; el Makefile
#         lo enlaza desde ahi
#   variables: JOBS (3 por defecto: la RAM manda), LLVM_VERSION, ICU_VERSION,
#              JSC_EXTRA_CMAKE (mas opciones para el cmake de WebKit)
# Lo que ya esta hecho no se repite (marcas .done en obj/deps/jsc-build).
# Los logs quedan en obj/deps/logs.
#
# Necesita en el PC: clang, clang++ y lld (de la misma version que
# LLVM_VERSION), cmake, ninja, ruby, perl, python3, curl, xz, unzip,
# y los ar/ranlib de binutils para aarch64-linux-gnu y arm-linux-gnu.
set -euo pipefail

ROOT=${QUIX_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}
DEPS=$ROOT/obj/deps
SRC=$DEPS/src
LOGS=$DEPS/logs
WORK=$DEPS/jsc-build
# el codigo de JavaScriptCore es parte de quixjs (runtime/thirdparty/webkit)
WEBKIT=$ROOT/runtime/thirdparty/webkit
JOBS=${JOBS:-3}
NICE="nice -n 10"

# LLVM: libc++ de la misma version que el clang que compila
LLVM_VERSION=${LLVM_VERSION:-$(clang --version | sed -n 's/.*clang version \([0-9.]*\).*/\1/p' | head -1)}
LLVM_TARBALL=llvm-project-$LLVM_VERSION.src.tar.xz
LLVM_URL=https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVM_VERSION/$LLVM_TARBALL
LLVM_SRC=$SRC/llvm-project-$LLVM_VERSION

ICU_VERSION=${ICU_VERSION:-77.1}
ICU_U=${ICU_VERSION/./_}
ICU_URL=https://github.com/unicode-org/icu/releases/download/release-${ICU_VERSION/./-}
ICU_SRC=$SRC/icu-$ICU_VERSION

# De que version de WebKit salio (runtime/thirdparty/webkit)
WEBKIT_TAG=webkitgtk-2.50.6

mkdir -p "$SRC" "$LOGS" "$WORK"

log() { printf '%s\n' "== $*"; }
die() { printf '%s\n' "error: $*" >&2; exit 1; }

# run LOG cmd...: el comando con su salida en obj/deps/logs/LOG.log; si falla,
# el final del log
run() {
  local name=$1; shift
  if ! "$@" > "$LOGS/$name.log" 2>&1; then
    tail -n 40 "$LOGS/$name.log" >&2
    die "fallo $name (log completo: $LOGS/$name.log)"
  fi
}

# Lo que depende de la arquitectura.
setup_arch() {
  ARCH=$1
  case $ARCH in
    aarch64)
      DEB=arm64; TRIPLE=aarch64-linux-gnu; BINUTILS=aarch64-linux-gnu; CPU=aarch64
      # sin outline atomics: el libgcc de GCC 8 no trae __aarch64_ldadd4...
      ARCHFLAGS="--target=aarch64-linux-gnu -mtune=cortex-a53 -mno-outline-atomics";;
    armhf)
      DEB=armhf; TRIPLE=arm-linux-gnueabihf; BINUTILS=arm-linux-gnu; CPU=armv7l
      ARCHFLAGS="--target=arm-linux-gnueabihf -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard";;
    *) die "arquitectura desconocida: $ARCH (aarch64 o armhf)";;
  esac
  SYSROOT=$DEPS/sysroot-$DEB
  B=$WORK/$ARCH
  PREFIX=$B/prefix          # libc++ e ICU instalados (headers + .a)
  OUT=$DEPS/jsc-$ARCH
  JSC_BUILD=$B/jsc
  AR=$(command -v "$BINUTILS-ar") || die "falta $BINUTILS-ar (binutils cruzado)"
  RANLIB=$(command -v "$BINUTILS-ranlib") || die "falta $BINUTILS-ranlib"
  CC_FLAGS="--no-default-config $ARCHFLAGS --sysroot=$SYSROOT"
  # C++ contra el libc++ propio, y todo enlazado con lld y estatico salvo glibc
  CXX_FLAGS="$CC_FLAGS -stdlib=libc++ -nostdinc++ -isystem $PREFIX/include/c++/v1"
  LD_FLAGS="-fuse-ld=lld -stdlib=libc++ -L$PREFIX/lib -static-libstdc++ -static-libgcc -pthread"
  mkdir -p "$B"
}

done_mark() { touch "$B/$1.done"; }
is_done() { [ -f "$B/$1.done" ]; }

# 1. El sysroot de buster, con la regla de portmaster/portmaster.mk (la misma que usa
#    quixjs.aarch64, con SDL y demas). Si make falla, solo lo minimo.
stage_sysroot() {
  [ -f "$SYSROOT/.ready" ] && return
  log "$ARCH: sysroot de Debian buster en $SYSROOT"
  if ! (cd "$ROOT" && make PLATFORM=portmaster "obj/deps/sysroot-$DEB/.ready") > "$LOGS/sysroot-$DEB.log" 2>&1; then
    log "make fallo (ver $LOGS/sysroot-$DEB.log): se descarga lo minimo"
    local tmp=$SYSROOT.tmp p f
    rm -rf "$tmp" && mkdir -p "$tmp"
    curl -fsSL "http://archive.debian.org/debian/dists/buster/main/binary-$DEB/Packages.xz" | xz -d > "$tmp/Packages"
    for p in libc6 libc6-dev linux-libc-dev libgcc1 libgcc-8-dev libatomic1; do
      f=$(awk -v p=$p '$1=="Package:"{ok=$2==p} ok&&$1=="Filename:"{print $2;exit}' "$tmp/Packages")
      [ -n "$f" ] || die "no hay paquete $p para $DEB"
      curl -fsSL "http://archive.debian.org/debian/$f" -o "$tmp/package.deb"
      (cd "$tmp" && ar x package.deb && tar -xf data.tar.* && rm -f package.deb data.tar.* control.tar.* debian-binary)
    done
    # los symlinks absolutos (libm.so -> /lib/...) se hacen relativos
    (cd "$tmp" && find . -type l | while read -r link; do
      target=$(readlink "$link"); case "$target" in /*)
        ln -sfn "$(realpath -m --relative-to="$(dirname "$link")" ".$target")" "$link";; esac; done)
    rm -f "$tmp/Packages" && touch "$tmp/.ready" && rm -rf "$SYSROOT" && mv "$tmp" "$SYSROOT"
  fi
}

# Fuentes: LLVM (solo lo que hace falta para los runtimes), ICU (fuente y
# datos en bruto, para poder filtrarlos). JavaScriptCore no se descarga: esta en
# runtime/thirdparty/webkit.
fetch() { # fetch URL FICHERO
  [ -f "$SRC/$2" ] && return
  log "descargando $2"
  curl -fL --retry 3 -o "$SRC/$2.part" "$1" && mv "$SRC/$2.part" "$SRC/$2"
}

sources_llvm() {
  [ -f "$LLVM_SRC/.ready" ] && return
  fetch "$LLVM_URL" "$LLVM_TARBALL"
  # libc: libc++ usa sus cabeceras compartidas (from_chars de coma flotante)
  log "extrayendo los runtimes de LLVM $LLVM_VERSION"
  rm -rf "$LLVM_SRC" && mkdir -p "$LLVM_SRC"
  tar -xJf "$SRC/$LLVM_TARBALL" -C "$LLVM_SRC" --strip-components=1 \
    "llvm-project-$LLVM_VERSION.src/"{runtimes,libcxx,libcxxabi,libunwind,libc,cmake,llvm/cmake,llvm/utils,third-party,LICENSE.TXT}
  touch "$LLVM_SRC/.ready"
}

sources_icu() {
  [ -f "$ICU_SRC/.ready" ] && return
  fetch "$ICU_URL/icu4c-$ICU_U-src.tgz" "icu4c-$ICU_U-src.tgz"
  fetch "$ICU_URL/icu4c-$ICU_U-data.zip" "icu4c-$ICU_U-data.zip"
  log "extrayendo ICU $ICU_VERSION"
  rm -rf "$ICU_SRC" && mkdir -p "$ICU_SRC"
  tar -xzf "$SRC/icu4c-$ICU_U-src.tgz" -C "$ICU_SRC" --strip-components=1
  # los datos en bruto (el .tgz solo trae el .dat ya hecho): sin ellos no
  # hay filtro
  rm -rf "$ICU_SRC/source/data" && unzip -q "$SRC/icu4c-$ICU_U-data.zip" -d "$ICU_SRC/source"
  [ -d "$ICU_SRC/source/data/locales" ] || die "icu4c-$ICU_U-data.zip no trae data/locales"
  touch "$ICU_SRC/.ready"
}

sources_webkit() {
  # el codigo esta en el proyecto (con los cambios de quixjs ya hechos)
  [ -f "$WEBKIT/JavaScriptCore/CMakeLists.txt" ] ||
    die "falta runtime/thirdparty/webkit (el codigo de JavaScriptCore)"
  grep -q 'set(ENABLE_API_TESTS OFF)   # quixjs' "$WEBKIT/cmake/OptionsJSCOnly.cmake" ||
    die "runtime/thirdparty/webkit: falta el cambio de OptionsJSCOnly.cmake"
}

# toolchain NOMBRE CXXFLAGS LDFLAGS: un fichero de toolchain de CMake para
# ARCH en $B/NOMBRE.cmake (libc++ se compila sin -stdlib=libc++; WebKit, con).
toolchain() {
  cat > "$B/$1.cmake" <<EOF
# generado por tools/build-jsc.sh
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR $CPU)
set(CMAKE_SYSROOT $SYSROOT)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_AR $AR)
set(CMAKE_RANLIB $RANLIB)
set(CMAKE_C_FLAGS_INIT "$CC_FLAGS")
set(CMAKE_ASM_FLAGS_INIT "$CC_FLAGS")
set(CMAKE_CXX_FLAGS_INIT "$2")
set(CMAKE_EXE_LINKER_FLAGS_INIT "$3")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "$3")
set(CMAKE_FIND_ROOT_PATH $PREFIX)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
EOF
}

# 2. libc++ + libc++abi, estaticas. El desenrollado de pila es el de libgcc
#    (libgcc_eh.a de buster): sin libunwind. libc++.a lleva dentro libc++abi.
stage_libcxx() {
  is_done libcxx && return
  sources_llvm
  log "$ARCH: libc++ $LLVM_VERSION"
  # En ARM (EHABI) libc++abi llama a __gnu_unwind_frame y usa los nombres
  # de libunwind (_URC_FATAL_PHASE1_ERROR...), que el unwind.h de clang no
  # trae: se compila con las cabeceras de libunwind, pero el desenrollado
  # sigue siendo el de libgcc_eh (el ABI EHABI es el mismo).
  local unwind=""
  [ "$ARCH" = armhf ] && unwind="-isystem $LLVM_SRC/libunwind/include"
  toolchain toolchain-libcxx "$CC_FLAGS $unwind" "-fuse-ld=lld"
  rm -rf "$B/libcxx"
  run "libcxx-$ARCH-configure" cmake -G Ninja -S "$LLVM_SRC/runtimes" -B "$B/libcxx" \
    -DCMAKE_TOOLCHAIN_FILE="$B/toolchain-libcxx.cmake" \
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
    -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXXABI_ENABLE_SHARED=OFF \
    -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXXABI_USE_LLVM_UNWINDER=OFF \
    -DLIBCXX_USE_COMPILER_RT=OFF -DLIBCXXABI_USE_COMPILER_RT=OFF \
    -DLIBCXX_INCLUDE_TESTS=OFF -DLIBCXX_INCLUDE_BENCHMARKS=OFF -DLIBCXXABI_INCLUDE_TESTS=OFF \
    -DLIBCXX_INSTALL_MODULES=OFF
  run "libcxx-$ARCH-build" $NICE ninja -C "$B/libcxx" -j "$JOBS" install
  done_mark libcxx
}

# 3. ICU. Filtro de datos: unos pocos idiomas y fuera lo que JSC no usa
#    (conversores de juegos de caracteres, transliteracion, nombres de
#    caracteres: ni ucnv_, ni utrans_, ni u_charName en JSC/WTF). La tabla de
#    alias de conversores (cnvalias) se queda: u_init la carga.
ICU_FILTER_JSON='{
  "localeFilter": {
    "filterType": "language",
    "includelist": ["en", "es", "pt", "fr", "de", "it", "ru", "pl", "ja", "zh", "ko"]
  },
  "featureFilters": {
    "conversion_mappings": "exclude",
    "translit": "exclude",
    "unames": "exclude"
  }
}'

icu_host() {
  [ -f "$WORK/icu-host/.done" ] && return
  sources_icu
  log "ICU $ICU_VERSION para el PC (herramientas y datos)"
  printf '%s\n' "$ICU_FILTER_JSON" > "$WORK/icu-filter.json"
  rm -rf "$WORK/icu-host" && mkdir -p "$WORK/icu-host"
  (cd "$WORK/icu-host" && ICU_DATA_FILTER_FILE="$WORK/icu-filter.json" CC=clang CXX=clang++ \
    run icu-host-configure "$ICU_SRC/source/configure" --disable-tests --disable-samples \
      --enable-static --disable-shared)
  run icu-host-build $NICE make -C "$WORK/icu-host" -j "$JOBS"
  touch "$WORK/icu-host/.done"
}

stage_icu() {
  is_done icu && return
  icu_host
  log "$ARCH: ICU $ICU_VERSION"
  rm -rf "$B/icu" && mkdir -p "$B/icu"
  (cd "$B/icu" && ICU_DATA_FILTER_FILE="$WORK/icu-filter.json" \
    CC=clang CXX=clang++ AR="$AR" RANLIB="$RANLIB" \
    CFLAGS="$CC_FLAGS -O2 -fPIC" CXXFLAGS="$CXX_FLAGS -O2 -fPIC" LDFLAGS="$LD_FLAGS" \
    run "icu-$ARCH-configure" "$ICU_SRC/source/configure" --host="$TRIPLE" \
      --with-cross-build="$WORK/icu-host" --prefix="$PREFIX" \
      --enable-static --disable-shared --with-data-packaging=static \
      --disable-tools --disable-tests --disable-samples --disable-extras --disable-layoutex)
  run "icu-$ARCH-build" $NICE make -C "$B/icu" -j "$JOBS"
  run "icu-$ARCH-install" make -C "$B/icu" install
  done_mark icu
}

# 4. JavaScriptCore: la misma configuracion de cmake que el build para el PC,
#    pero cruzada (con el toolchain de ARCH).
stage_jsc() {
  is_done jsc && return
  sources_webkit
  toolchain toolchain "$CXX_FLAGS" "$LD_FLAGS"
  log "$ARCH: JavaScriptCore (WebKit $WEBKIT_TAG) en $JSC_BUILD"
  # shellcheck disable=SC2086
  run "jsc-$ARCH-configure" cmake -G Ninja -S "$WEBKIT" -B "$JSC_BUILD" \
    -DCMAKE_TOOLCHAIN_FILE="$B/toolchain.cmake" \
    -DPORT=JSCOnly -DENABLE_STATIC_JSC=ON -DCMAKE_BUILD_TYPE=Release -DDEVELOPER_MODE=OFF \
    -DUSE_LIBBACKTRACE=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DICU_ROOT="$PREFIX" ${JSC_EXTRA_CMAKE:-}
  run "jsc-$ARCH-build" $NICE ninja -C "$JSC_BUILD" -j "$JOBS" JavaScriptCore
  done_mark jsc
}

# 5. obj/deps/jsc-ARCH
stage_install() {
  log "$ARCH: instalando en $OUT"
  rm -rf "$OUT" && mkdir -p "$OUT/lib" "$OUT/include/JavaScriptCore" "$OUT/licenses"
  # las .a de WebKit (JavaScriptCore, WTF, bmalloc)
  # WebKit las deja como archivos "thin" (solo rutas a los .o del build): se
  # rehacen como archivos normales, con los objetos dentro
  local a
  for a in "$JSC_BUILD"/lib/*.a; do
    if [ "$(head -c 7 "$a")" = '!<thin>' ]; then
      (cd "$(dirname "$a")" && "$AR" t "$a" | xargs "$AR" qcs "$OUT/lib/$(basename "$a")")
    else
      cp "$a" "$OUT/lib/"
    fi
  done
  cp "$PREFIX"/lib/libicudata.a "$PREFIX"/lib/libicuuc.a "$PREFIX"/lib/libicui18n.a \
     "$PREFIX"/lib/libc++.a "$PREFIX"/lib/libc++abi.a "$OUT/lib/"
  # la API publica en C
  local h api=$WEBKIT/JavaScriptCore/API
  for h in JavaScript.h JSBase.h JSContextRef.h JSStringRef.h JSObjectRef.h JSTypedArray.h \
           JSValueRef.h WebKitAvailability.h JSBasePrivate.h JSContextRefPrivate.h \
           JSObjectRefPrivate.h JSStringRefPrivate.h JSWeakObjectMapRefPrivate.h; do
    cp "$api/$h" "$OUT/include/JavaScriptCore/"
  done
  # licencias: JSC es LGPL-2 (y BSD en buena parte), WTF y bmalloc BSD, mas
  # el codigo de terceros que llevan dentro
  local L=$OUT/licenses W=$WEBKIT
  cp "$W/JavaScriptCore/COPYING.LIB" "$L/JavaScriptCore-LGPL-2.txt"
  sed -n '1,/\*\//p' "$api/JSBase.h" > "$L/JavaScriptCore-BSD.txt"
  sed -n '1,/\*\//p' "$W/WTF/wtf/Assertions.h" > "$L/WTF-BSD.txt"
  sed -n '1,/\*\//p' "$W/bmalloc/bmalloc/bmalloc.h" > "$L/bmalloc-BSD.txt"
  for f in "$W"/WTF/LICENSE-*.txt; do cp "$f" "$L/WTF-$(basename "$f" .txt | sed 's/^LICENSE-//').txt"; done
  cp "$W/WTF/wtf/fast_float/LICENSE" "$L/WTF-fast_float.txt"
  cat "$W/WTF/wtf/dtoa/LICENSE" "$W/WTF/wtf/dtoa/COPYING" > "$L/WTF-dtoa.txt"
  cp "$W/WTF/wtf/simdutf/LICENSE-simdutf.txt" "$L/WTF-simdutf.txt"
  cp "$ICU_SRC/LICENSE" "$L/ICU.txt"
  cp "$LLVM_SRC/libcxx/LICENSE.TXT" "$L/libc++.txt"
  cp "$LLVM_SRC/libcxxabi/LICENSE.TXT" "$L/libc++abi.txt"
  {
    echo "JavaScriptCore de WebKit $WEBKIT_TAG (port JSCOnly, estatico), ICU $ICU_VERSION y"
    echo "libc++/libc++abi de LLVM $LLVM_VERSION, para $TRIPLE (Debian buster, glibc 2.28)."
    echo "Construido con tools/build-jsc.sh de quixjs."
    echo
    echo "Enlazar (C, con clang y lld): -I$OUT/include y"
    echo "  -L$OUT/lib -lJavaScriptCore -lWTF -lbmalloc"
    echo "  -licui18n -licuuc -licudata -l:libc++.a -l:libc++abi.a"
    echo "  -static-libgcc -lpthread -ldl -lm $(link_atomic) -Wl,--gc-sections"
  } > "$OUT/README.txt"
}

# armhf: los atomicos de 64 bits que no son lock-free van en libatomic (estatica)
link_atomic() { [ "$ARCH" = armhf ] && echo "-l:libatomic.a"; true; }

# 6. La prueba: el programa de JSC_TEST_SRC (sin el, no hay prueba),
#    enlazado como lo hara quixjs, y las versiones de glibc que pide.
stage_test() {
  local src=${JSC_TEST_SRC:-}
  [ -n "$src" ] || { log "$ARCH: sin JSC_TEST_SRC, no hay prueba"; return; }
  local exe=$B/jsc-run
  log "$ARCH: prueba $exe"
  # shellcheck disable=SC2046
  run "test-$ARCH" clang $CC_FLAGS -O2 -fuse-ld=lld -Wl,--gc-sections -I"$OUT/include" "$src" -o "$exe" \
    -L"$OUT/lib" -lJavaScriptCore -lWTF -lbmalloc -licui18n -licuuc -licudata \
    -l:libc++.a -l:libc++abi.a -static-libgcc -lpthread -ldl -lm $(link_atomic)
  local readelf=$BINUTILS-readelf
  echo "versiones de simbolos pedidas por $exe:"
  "$readelf" -V "$exe" | grep -o 'Name: [A-Z_]*[0-9.]*' | sort -u | sed 's/^/  /'
  if "$readelf" -V "$exe" | grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -uV | tail -1 |
     awk -F. '{exit !($1>2 || ($1==2 && $2>28))}'; then
    die "$exe pide una version de glibc posterior a 2.28"
  fi
  if "$readelf" -V "$exe" | grep -q 'GLIBCXX\|CXXABI'; then die "$exe pide libstdc++"; fi
  "$readelf" -d "$exe" | grep NEEDED | sed 's/^/  /'
  ls -l "$exe"
}

# JavaScriptCore para el PC: el clang, libstdc++ y el ICU del sistema
build_host() {
  local build="$WORK/$(uname -m)-host/jsc"
  sources_webkit
  if [ ! -f "$build/build.ninja" ]; then
    log "host: JavaScriptCore (WebKit $WEBKIT_TAG) en $build"
    # shellcheck disable=SC2086
    run "jsc-host-configure" cmake -G Ninja -S "$WEBKIT" -B "$build" \
      -DPORT=JSCOnly -DENABLE_STATIC_JSC=ON -DCMAKE_BUILD_TYPE=Release -DDEVELOPER_MODE=OFF \
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
      -DUSE_LIBBACKTRACE=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON ${JSC_EXTRA_CMAKE:-}
  fi
  # ninja no repite lo que ya esta hecho
  run "jsc-host-build" $NICE ninja -C "$build" -j "$JOBS" JavaScriptCore
  log "host: listo ($build/lib)"
}

main() {
  local arches=all stages=""
  if [ $# -gt 0 ]; then arches=$1; shift; fi
  if [ "$arches" = host ]; then
    build_host
    return
  fi
  stages=${*:-"sysroot libcxx icu jsc install test"}
  [ "$arches" = all ] && arches="aarch64 armhf"
  local a s
  for a in $arches; do
    setup_arch "$a"
    for s in $stages; do
      case $s in
        sysroot|libcxx|icu|jsc|install|test) "stage_$s";;
        *) die "etapa desconocida: $s";;
      esac
    done
  done
}

main "$@"
