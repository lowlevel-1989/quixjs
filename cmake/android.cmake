# cmake/android.cmake: la app de Android (make android). Indice: solo incluye
# sus partes, en orden.
#
#   android/native.cmake  SDL2, SDL2_ttf, SDL2_image y libquixjs.so, con el NDK
#   android/apk.cmake     el Java, los recursos, los assets y el .apk firmado

include(cmake/android/native.cmake)
include(cmake/android/apk.cmake)
