# cmake/android/apk.cmake: la app de Android, quixjs.apk, sin Gradle: los assets (la
# fuente y las licencias), el Java (el de SDL2 y android/java), los recursos
# y las .so de cmake/android/native.cmake, alineado y firmado con la clave de
# depuracion de siempre (~/.android/debug.keystore).
#
# Deja el target apk (sale en build/quixjs.apk, o quixjs-x86_64.apk). Usa el
# SDK de Android (QUIX_ANDROID_HOME: build-tools y una plataforma) y un JDK.

file(GLOB _tools ${QUIX_ANDROID_HOME}/build-tools/*)
list(SORT _tools COMPARE NATURAL)
list(POP_BACK _tools _bt)
file(GLOB _jars ${QUIX_ANDROID_HOME}/platforms/android-*/android.jar)
list(SORT _jars COMPARE NATURAL)
list(POP_BACK _jars _android_jar)
if(NOT _bt OR NOT _android_jar)
  message(FATAL_ERROR "faltan build-tools o una plataforma en ${QUIX_ANDROID_HOME}: instalalos con sdkmanager")
endif()

set(_out ${CMAKE_BINARY_DIR}/apk)
file(MAKE_DIRECTORY ${_out})
set(_keystore $ENV{HOME}/.android/debug.keystore)
if(QUIX_ANDROID_ABI STREQUAL "x86_64")
  set(_apk ${QUIX_OUTPUT_DIR}/quixjs-x86_64.apk)
else()
  set(_apk ${QUIX_OUTPUT_DIR}/quixjs.apk)
endif()

# los assets: la fuente de las pantallas de quixjs (DejaVu Sans Bold, la del
# port de PortMaster si esta su sysroot; si no, la del PC) y las licencias de
# todo lo que lleva la app (MainActivity las ensena)
set(_font ${CMAKE_SOURCE_DIR}/obj/deps/sysroot-arm64/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf)
if(NOT EXISTS ${_font})
  execute_process(COMMAND fc-match -f %{file} "DejaVu Sans:bold" OUTPUT_VARIABLE _font)
endif()
set(_lic ${_out}/assets/licenses)
set(_licenses
  ${CMAKE_SOURCE_DIR}/LICENSE=quixjs.txt
  ${sdl2_SOURCE_DIR}/LICENSE.txt=SDL2.txt
  ${sdl2_ttf_SOURCE_DIR}/LICENSE.txt=SDL2_ttf.txt
  ${sdl2_image_SOURCE_DIR}/LICENSE.txt=SDL2_image.txt
  ${CMAKE_SOURCE_DIR}/runtime/thirdparty/tomlc17/LICENSE=tomlc17.txt
  ${CMAKE_SOURCE_DIR}/runtime/thirdparty/stb/LICENSE=stb_vorbis.txt
  ${CMAKE_SOURCE_DIR}/runtime/thirdparty/minimp4/LICENSE=minimp4.txt
  ${CMAKE_SOURCE_DIR}/runtime/thirdparty/ffmpeg/LICENSE=ffmpeg.txt)
set(_copy_licenses "")
foreach(_item ${_licenses})
  string(REPLACE "=" ";" _pair ${_item})
  list(GET _pair 0 _from)
  list(GET _pair 1 _to)
  list(APPEND _copy_licenses COMMAND ${CMAKE_COMMAND} -E copy ${_from} ${_lic}/${_to})
endforeach()
file(GLOB _jsc_licenses ${QUIX_JSC_DIR}/licenses/*.txt)

# el Java: el de SDL2 y el de la app; R.java lo escribe aapt2
file(GLOB _java CONFIGURE_DEPENDS ${QUIX_SDL_DIR}/android-project/app/src/main/java/org/libsdl/app/*.java
  ${CMAKE_SOURCE_DIR}/android/java/*/*/*/*.java)
file(GLOB_RECURSE _res CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/android/res/*)
file(READ ${CMAKE_SOURCE_DIR}/VERSION _version)
string(STRIP "${_version}" _version)

add_custom_command(OUTPUT ${_apk}
  # los assets
  COMMAND ${CMAKE_COMMAND} -E rm -rf assets gen classes lib classes.dex
  COMMAND ${CMAKE_COMMAND} -E make_directory ${_lic} ${_out}/assets/fonts ${_out}/gen ${_out}/classes
    ${_out}/lib/${QUIX_ANDROID_ABI}
  COMMAND ${CMAKE_COMMAND} -E copy ${_font} ${_out}/assets/fonts/DejaVuSans-Bold.ttf
  ${_copy_licenses}
  COMMAND ${CMAKE_COMMAND} -E copy ${_jsc_licenses} ${_lic}
  # los recursos y el manifiesto (--debug-mode: depurable, run-as lee los logs)
  COMMAND ${_bt}/aapt2 compile --dir ${CMAKE_SOURCE_DIR}/android/res -o ${_out}/res.zip
  COMMAND ${_bt}/aapt2 link -o ${_out}/unsigned.apk -I ${_android_jar}
    --manifest ${CMAKE_SOURCE_DIR}/android/AndroidManifest.xml --min-sdk-version 28 --target-sdk-version 34
    --version-name ${_version} --version-code 1 --debug-mode --java ${_out}/gen -A ${_out}/assets ${_out}/res.zip
  # el Java, en classes.dex
  COMMAND javac -nowarn --release 11 -encoding UTF-8 -cp ${_android_jar} -d ${_out}/classes
    ${_java} ${_out}/gen/com/lowlevel1989/quixjs/R.java
  COMMAND jar cf ${_out}/classes.jar -C ${_out}/classes .
  COMMAND ${_bt}/d8 --release --lib ${_android_jar} --min-api 28 --output ${_out} ${_out}/classes.jar
  # las .so
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:quixjs> $<TARGET_FILE:SDL2> $<TARGET_FILE:SDL2_ttf>
    $<TARGET_FILE:SDL2_image> ${_out}/lib/${QUIX_ANDROID_ABI}
  # todo en el .apk, alineado y firmado
  COMMAND zip -q -X ${_out}/unsigned.apk classes.dex
  COMMAND zip -q -X -r ${_out}/unsigned.apk lib
  COMMAND ${_bt}/zipalign -p -f 4 ${_out}/unsigned.apk ${_out}/aligned.apk
  COMMAND ${_bt}/apksigner sign --ks ${_keystore} --ks-pass pass:android --out ${_apk} ${_out}/aligned.apk
  WORKING_DIRECTORY ${_out}
  DEPENDS quixjs SDL2 SDL2_ttf SDL2_image ${_java} ${_res} ${CMAKE_SOURCE_DIR}/android/AndroidManifest.xml
  VERBATIM)
add_custom_target(apk ALL DEPENDS ${_apk})

# la clave de depuracion, si aun no esta (la misma que usa Android Studio)
if(NOT EXISTS ${_keystore})
  get_filename_component(_dir ${_keystore} DIRECTORY)
  file(MAKE_DIRECTORY ${_dir})
  execute_process(COMMAND keytool -genkeypair -keystore ${_keystore} -storepass android -keypass android
    -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US"
    OUTPUT_QUIET)
endif()
