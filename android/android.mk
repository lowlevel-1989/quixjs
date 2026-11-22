# android/android.mk: make PLATFORM=android construye quixjs.apk, la app de
# Android. La incluye el Makefile. Adaptado de tamatama (android/android.mk,
# MIT):
# https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/android.mk
#   1. android/deps: SDL2, SDL2_ttf y SDL2_image, descargados una vez
#   2. JavaScriptCore para Android (tools/build-jsc.sh android-arm64), si no
#      esta en obj/deps/jsc-android-arm64
#   3. CMake + NDK, para ABI (arm64-v8a; ABI=x86_64 para el emulador del
#      PC, con tools/build-jsc.sh android-x86_64): libSDL2.so, libSDL2_ttf.so,
#      libSDL2_image.so y libquixjs.so (android/CMakeLists.txt), con las
#      mismas fuentes que el Makefile (SRCS) y la plataforma android
#   4. javac + d8: el Java de SDL y android/java, en classes.dex
#   5. aapt2: el manifiesto, android/res y los assets (la fuente y las
#      licencias) en el .apk. Los ports no van: cada juego se importa en la
#      app con la carpeta completa de su port, en un .zip
#   6. zipalign + apksigner, con la clave de depuracion de siempre
#      (~/.android/debug.keystore)
# Necesita el SDK de Android con un NDK, build-tools y una plataforma
# (ANDROID_HOME, ~/Android/Sdk por defecto) y un JDK. Sin Gradle.
ANDROID_HOME ?= $(HOME)/Android/Sdk
NDK          ?= $(lastword $(sort $(wildcard $(ANDROID_HOME)/ndk/*)))
BUILD_TOOLS  ?= $(lastword $(sort $(wildcard $(ANDROID_HOME)/build-tools/*)))
ANDROID_JAR  ?= $(lastword $(sort $(wildcard $(ANDROID_HOME)/platforms/android-*/android.jar)))
# 28 (Android 9): el minimo de JavaScriptCore (bionic con aligned_alloc)
MIN_SDK      := 28
TARGET_SDK   := 34
APP_VERSION  := $(shell tr -d ' \n' < VERSION)
OUT          := obj/android
APK          := quixjs$(if $(filter x86_64,$(ABI)),-x86_64).apk
KEYSTORE     := $(HOME)/.android/debug.keystore
# ABI=x86_64: un .apk para el emulador de Android del PC, solo para probar
# (necesita tools/build-jsc.sh android-x86_64). Si no, arm64-v8a
ABI          ?= arm64-v8a
JSC_ARCH     := android-$(if $(filter x86_64,$(ABI)),x86_64,arm64)
JSC_ANDROID  := obj/deps/jsc-$(JSC_ARCH)

SDL2_URL  := https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz
TTF_URL   := https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.24.0/SDL2_ttf-2.24.0.tar.gz
IMAGE_URL := https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.8/SDL2_image-2.8.8.tar.gz
DEPS      := android/deps/src

.PHONY: all android-native android-java android-assets clean distclean
all: $(APK)

# 1. Las fuentes, en android/deps/src/{SDL2,SDL2_ttf,SDL2_image}
$(DEPS)/.ready:
	@echo "Descargando SDL2, SDL2_ttf y SDL2_image en android/deps/ (solo la primera vez)"
	rm -rf $(DEPS) && mkdir -p $(DEPS)
	curl -fsSL $(SDL2_URL) | tar xz -C $(DEPS) && mv $(DEPS)/SDL2-* $(DEPS)/SDL2
	curl -fsSL $(TTF_URL) | tar xz -C $(DEPS) && mv $(DEPS)/SDL2_ttf-* $(DEPS)/SDL2_ttf
	curl -fsSL $(IMAGE_URL) | tar xz -C $(DEPS) && mv $(DEPS)/SDL2_image-* $(DEPS)/SDL2_image
	touch $@

# 2. JavaScriptCore (tarda: una vez)
$(JSC_ANDROID)/lib/libJavaScriptCore.a:
	tools/build-jsc.sh $(JSC_ARCH)

# 3. Las librerias nativas. CMake decide que recompilar. Los archivos que el
# Makefile genera (obj/gen) primero. CMAKE_POLICY_VERSION_MINIMUM: el freetype
# de SDL2_ttf pide un CMake anterior a 3.5, que CMake 4 ya no acepta
android-native: $(DEPS)/.ready $(JSC_ANDROID)/lib/libJavaScriptCore.a $(GEN_SHADERS) $(GEN_ENGINES) \
                $(GEN_POLYFILLS) $(GEN_JS) $(GEN_VERSION) $(GEN_TRACE)
	@test -d "$(NDK)" || { echo "No hay NDK en $(ANDROID_HOME)/ndk: instala uno con sdkmanager"; exit 1; }
	cmake -S android -B $(OUT)/native-$(ABI) -G Ninja -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_TOOLCHAIN_FILE=$(NDK)/build/cmake/android.toolchain.cmake \
	  -DANDROID_ABI=$(ABI) -DANDROID_PLATFORM=android-$(MIN_SDK) -DANDROID_STL=c++_static \
	  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
	  -DQUIX_SRCS="$(SRCS)" -DJSC_DIR=$(CURDIR)/$(JSC_ANDROID) \
	  -DFFMPEG_SRCS="$(FFMPEG_SRCS) $(FFMPEG_SRCS_aarch64)" >/dev/null
	cmake --build $(OUT)/native-$(ABI)
	rm -rf $(OUT)/apk/lib && mkdir -p $(OUT)/apk/lib/$(ABI)
	cp $(OUT)/native-$(ABI)/libquixjs.so $(OUT)/native-$(ABI)/SDL2/libSDL2.so \
	  $(OUT)/native-$(ABI)/SDL2_ttf/libSDL2_ttf.so $(OUT)/native-$(ABI)/SDL2_image/libSDL2_image.so \
	  $(OUT)/apk/lib/$(ABI)/

# Los assets: la fuente de las pantallas de quixjs y las licencias de todo lo que lleva la app (la
# pantalla de inicio las ensena: MainActivity, "Licenses")
android-assets: $(DEPS)/.ready $(JSC_ANDROID)/lib/libJavaScriptCore.a
	rm -rf $(OUT)/assets && mkdir -p $(OUT)/assets/licenses $(OUT)/assets/fonts
	# la misma fuente que el port de PortMaster (DejaVu Sans Bold, del sysroot de
	# Debian; si no esta, la del PC)
	cp obj/deps/sysroot-arm64/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf $(OUT)/assets/fonts/ 2>/dev/null || \
	  cp $$(fc-match -f '%{file}' 'DejaVu Sans:bold') $(OUT)/assets/fonts/DejaVuSans-Bold.ttf
	cp LICENSE $(OUT)/assets/licenses/quixjs.txt
	cp $(DEPS)/SDL2/LICENSE.txt $(OUT)/assets/licenses/SDL2.txt
	cp $(DEPS)/SDL2_ttf/LICENSE.txt $(OUT)/assets/licenses/SDL2_ttf.txt
	cp $(DEPS)/SDL2_image/LICENSE.txt $(OUT)/assets/licenses/SDL2_image.txt
	for f in $(JSC_ANDROID)/licenses/*.txt; do cp "$$f" $(OUT)/assets/licenses/; done
	# lo de runtime/thirdparty que va dentro (como en el port de PortMaster)
	cp runtime/thirdparty/tomlc17/LICENSE $(OUT)/assets/licenses/tomlc17.txt
	cp runtime/thirdparty/stb/LICENSE $(OUT)/assets/licenses/stb_vorbis.txt
	cp runtime/thirdparty/minimp4/LICENSE $(OUT)/assets/licenses/minimp4.txt
	cp runtime/thirdparty/ffmpeg/LICENSE $(OUT)/assets/licenses/ffmpeg.txt

# 4 y 5. Los recursos primero: aapt2 escribe R.java, que usa el Java.
# --debug-mode: el .apk es para probar, depurable (adb shell run-as
# com.lowlevel1989.quixjs lee log.txt y las partidas de la app)
JAVA := $(wildcard $(DEPS)/SDL2/android-project/app/src/main/java/org/libsdl/app/*.java) \
        $(wildcard android/java/*/*/*/*.java)
android-java: $(DEPS)/.ready android-assets
	@test -f "$(ANDROID_JAR)" || { echo "No hay plataforma en $(ANDROID_HOME)/platforms: instala una con sdkmanager"; exit 1; }
	rm -rf $(OUT)/res $(OUT)/gen $(OUT)/classes $(OUT)/dex && mkdir -p $(OUT)/res $(OUT)/gen $(OUT)/classes $(OUT)/dex
	$(BUILD_TOOLS)/aapt2 compile --dir android/res -o $(OUT)/res/res.zip
	$(BUILD_TOOLS)/aapt2 link -o $(OUT)/base.apk -I $(ANDROID_JAR) --manifest android/AndroidManifest.xml \
	  --min-sdk-version $(MIN_SDK) --target-sdk-version $(TARGET_SDK) \
	  --version-name $(APP_VERSION) --version-code 1 --debug-mode --java $(OUT)/gen -A $(OUT)/assets $(OUT)/res/res.zip
	javac -nowarn --release 11 -encoding UTF-8 -cp $(ANDROID_JAR) -d $(OUT)/classes \
	  $(JAVA) $$(find $(OUT)/gen -name '*.java')
	$(BUILD_TOOLS)/d8 --release --lib $(ANDROID_JAR) --min-api $(MIN_SDK) --output $(OUT)/dex \
	  $$(find $(OUT)/classes -name '*.class')

# 6. Todo en el .apk, alineado y firmado
$(APK): android-native android-java
	@test -f $(KEYSTORE) || { mkdir -p $(dir $(KEYSTORE)) && keytool -genkeypair -keystore $(KEYSTORE) \
	  -storepass android -keypass android -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 \
	  -dname "CN=Android Debug,O=Android,C=US" >/dev/null; }
	cp $(OUT)/base.apk $(OUT)/unsigned.apk
	cd $(OUT)/dex && zip -q -X ../unsigned.apk classes.dex
	cd $(OUT)/apk && zip -q -X -r ../unsigned.apk lib
	$(BUILD_TOOLS)/zipalign -p -f 4 $(OUT)/unsigned.apk $(OUT)/aligned.apk
	$(BUILD_TOOLS)/apksigner sign --ks $(KEYSTORE) --ks-pass pass:android --out $@ $(OUT)/aligned.apk
	@echo "Listo: $@ (version $(APP_VERSION)). Instalar con: adb install -r $@"

clean: ; rm -rf $(OUT) $(APK) $(APK).idsig
distclean: clean ; rm -rf android/deps
