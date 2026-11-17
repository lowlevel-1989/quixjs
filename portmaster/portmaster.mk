# portmaster.mk: make PLATFORM=portmaster construye PORT.zip, un
# port de PortMaster (portatiles con Linux) con quixjs. Lo incluye el Makefile.
#   1. obj/deps: Debian buster arm64 y armhf (glibc 2.28, SDL 2.0.9: lo bastante
#      viejo para cualquier CFW, el sistema de la portatil: ROCKNIX,
#      ArkOS...), descargado una vez. Sin root ni contenedor.
#   2. JavaScriptCore, ICU y libc++ estaticos para cada arquitectura
#      (tools/build-jsc.sh, en obj/deps/jsc-ARCH).
#   3. quixjs.aarch64 y quixjs.armhf con clang y lld contra esos sysroots. Usan
#      el SDL2 del dispositivo; SDL2_image y SDL2_ttf (y lo que necesitan) van
#      en libs.ARCH/ porque no todos los CFW los traen.
#   4. PORT.zip: el script .sh del port y su carpeta (port.json,
#      README.md, gameinfo.xml, port.toml, gamedata/, quixjs.ARCH, fonts/,
#      libs.ARCH/, licenses/), como espera PortMaster. El script .sh usa el
#      quixjs de la arquitectura del dispositivo. El port es games/PORT/: su port.toml
#      (tools/portmaster.py saca de el el .sh, port.json y gameinfo.xml) y su
#      README.md. El juego NO va dentro: el usuario copia su copia en
#      gamedata/ y el script .sh ejecuta quixjs -c port.toml, que lo lee de ahi.
# Uso: make PLATFORM=portmaster [PORT=fearandhunger] [PM_ARCHES="aarch64"]
PORT        ?= fearandhunger
PM          := obj/portmaster
PM_DEPS     := obj/deps
PM_ARCHIVE  := http://archive.debian.org/debian
PM_SUITE    := buster
# lo que se enlaza (headers + .so) y lo que se lleva en libs/
PM_PACKAGES := libc6 libc6-dev linux-libc-dev libgcc1 libgcc-8-dev \
               libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev \
               libsdl2-ttf-2.0-0 libsdl2-ttf-dev libfreetype6 libpng16-16 libjpeg62-turbo \
               libtiff5 libwebp6 libjbig0 libzstd1 liblzma5 zlib1g fonts-dejavu-core \
               libgles2-mesa-dev mesa-common-dev libegl1-mesa-dev \
               libvpx5 libvpx-dev libopus0 libopus-dev
# las que no traen todos los CFW: el video (libvpx, libopus) y SDL2_ttf. SDL2,
# SDL2_image, freetype y libpng son las del sistema (casi todos los CFW las
# traen, y el comprobador de PortMaster pide no llevarlas)
PM_BUNDLE   := libvpx.so.5 libopus.so.0 libSDL2_ttf-2.0.so.0
# paquete de Debian de cada libreria que se lleva (para su licencia)
PM_BUNDLE_PKGS := libvpx5 libopus0 libsdl2-ttf-2.0-0 fonts-dejavu-core
PM_VERSION  := $(shell sed -n 's/^\#define QUIX_VERSION "\(.*\)"/\1/p' runtime/binding/quix.h)
PM_ZIP      := $(PORT).zip
PM_PORTDIR  := games/$(PORT)

# solo aarch64: armhf se puede compilar con PM_ARCHES="aarch64 armhf", pero no
# va en el port (no se ha probado en una consola)
PM_ARCHES ?= aarch64

# Por arquitectura: el sysroot de Debian, el triplete y las opciones de clang
DEB_aarch64     := arm64
DEB_armhf       := armhf
TRIPLE_aarch64  := aarch64-linux-gnu
TRIPLE_armhf    := arm-linux-gnueabihf
# afinado para los Cortex-A53/A35 de las portatiles (el mismo ARMv8: corre en
# todas); sin outline-atomics: el libgcc de GCC 8 no las trae
TARGET_aarch64  := --target=aarch64-linux-gnu -mtune=cortex-a53 -mno-outline-atomics
TARGET_armhf    := --target=arm-linux-gnueabihf -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard
SYSROOT_aarch64 := $(CURDIR)/$(PM_DEPS)/sysroot-arm64
SYSROOT_armhf   := $(CURDIR)/$(PM_DEPS)/sysroot-armhf
# armhf: los atomicos de 64 bits van en libatomic
JSC_EXTRA_aarch64 :=
JSC_EXTRA_armhf   := -l:libatomic.a

pm_cc = clang --no-default-config $(TARGET_$(1)) --sysroot=$(SYSROOT_$(1))
pm_cflags = -O2 -D_GNU_SOURCE -Iruntime -Iruntime/thirdparty -Iruntime/thirdparty/tomlc17 -Iobj/gen \
            -I$(SYSROOT_$(1))/usr/include/SDL2 -I$(PM_DEPS)/jsc-$(1)/include \
            -DQUIX_VIDEO -DQUIX_HANDHELD -w
# JavaScriptCore, ICU y libc++ van dentro del binario (estaticos)
pm_libs = -L$(PM_DEPS)/jsc-$(1)/lib -lJavaScriptCore -lWTF -lbmalloc $(JSC_EXTRA_$(1)) \
          -licui18n -licuuc -licudata -l:libc++.a -l:libc++abi.a \
          -lvpx -lopus -lSDL2_ttf -lSDL2_image -lSDL2 -lm -lpthread -ldl -Wl,--gc-sections -Wl,--strip-all \
          -Wl,--allow-shlib-undefined -Wl,--disable-new-dtags -Wl,-rpath,'$$$$ORIGIN/libs.$(1)' -static-libgcc

# con video siempre (nestegg + libvpx + libopus, estas dos en libs.ARCH/)
PM_SRCS := $(filter-out runtime/thirdparty/nestegg/nestegg.c,$(SRCS)) runtime/thirdparty/nestegg/nestegg.c
PM_BINS := $(PM_ARCHES:%=$(PM)/quixjs.%)

all: $(PM_ZIP)

# 1. El sysroot: los paquetes desempaquetados. Sus symlinks absolutos
#    (libm.so -> /lib/...) apuntarian a este PC: se hacen relativos.
$(PM_DEPS)/sysroot-%/.ready:
	@echo "Downloading Debian $(PM_SUITE) for $* into $(@D) (only the first time)"
	rm -rf $(@D).tmp && mkdir -p $(@D).tmp
	curl -fsSL $(PM_ARCHIVE)/dists/$(PM_SUITE)/main/binary-$*/Packages.xz | xz -d > $(@D).tmp/Packages
	set -e; for p in $(PM_PACKAGES); do \
	  f=$$(awk -v p=$$p '$$1=="Package:"{ok=$$2==p} ok&&$$1=="Filename:"{print $$2;exit}' $(@D).tmp/Packages); \
	  test -n "$$f" || { echo "no package $$p for $*"; exit 1; }; \
	  curl -fsSL $(PM_ARCHIVE)/$$f -o $(@D).tmp/package.deb; \
	  (cd $(@D).tmp && ar x package.deb && tar -xf data.tar.* && rm -f package.deb data.tar.* control.tar.* debian-binary); \
	done
	cd $(@D).tmp && find . -type l | while read link; do \
	  target=$$(readlink "$$link"); case "$$target" in /*) \
	    ln -sfn "$$(realpath -m --relative-to="$$(dirname "$$link")" ".$$target")" "$$link";; esac; done
	rm -f $(@D).tmp/Packages && touch $(@D).tmp/.ready && rm -rf $(@D) && mv $(@D).tmp $(@D)

# 2. JavaScriptCore de cada arquitectura (tools/build-jsc.sh: tarda, una vez)
$(PM_DEPS)/jsc-%/lib/libJavaScriptCore.a:
	tools/build-jsc.sh $*

# 3. quixjs.ARCH
define PM_ARCH_RULES
# si cambian las flags (por ejemplo -mtune), se recompila todo
$(PM)/$(1)/cflags: FORCE
	@mkdir -p $$(@D); echo '$(call pm_cc,$(1)) $(call pm_cflags,$(1))' | cmp -s - $$@ || \
	  echo '$(call pm_cc,$(1)) $(call pm_cflags,$(1))' > $$@

$(PM)/$(1)/%.o: %.c $(HEADERS) $(PM)/$(1)/cflags | $(PM_DEPS)/sysroot-$(DEB_$(1))/.ready \
                $(PM_DEPS)/jsc-$(1)/lib/libJavaScriptCore.a
	@mkdir -p $$(@D) && $(call pm_cc,$(1)) $(call pm_cflags,$(1)) -c $$< -o $$@

$(PM)/quixjs.$(1): $(PM_SRCS:%.c=$(PM)/$(1)/%.o) $(PM_DEPS)/jsc-$(1)/lib/libJavaScriptCore.a
	$(call pm_cc,$(1)) -fuse-ld=lld $(PM_SRCS:%.c=$(PM)/$(1)/%.o) -o $$@ $(call pm_libs,$(1))
endef
$(foreach a,$(PM_ARCHES),$(eval $(call PM_ARCH_RULES,$(a))))
.PHONY: FORCE

# 4. El .zip
PM_STAGE := $(PM)/zip
$(PM_ZIP): $(PM_BINS) portmaster/portmaster.mk portmaster/portmaster.sh tools/portmaster.py $(shell find $(PM_PORTDIR) -type f 2>/dev/null)
	rm -rf $(PM_STAGE) && mkdir -p $(PM_STAGE)
	# el script .sh, port.json y gameinfo.xml salen del port.toml del port
	PM_ARCHES="$(PM_ARCHES)" python3 tools/portmaster.py $(PM_PORTDIR)/port.toml $(PM_STAGE)
	cp $(PM_PORTDIR)/README.md $(PM_STAGE)/$(PORT)/
	if [ -f $(PM_PORTDIR)/README.es.md ]; then cp $(PM_PORTDIR)/README.es.md $(PM_STAGE)/$(PORT)/; fi
	# los mods que trae el port (mods/<nombre>/, se activan en la opcion Mods)
	if [ -d $(PM_PORTDIR)/mods ]; then cp -r $(PM_PORTDIR)/mods $(PM_STAGE)/$(PORT)/; fi
	mkdir -p $(PM_STAGE)/$(PORT)/licenses $(PM_STAGE)/$(PORT)/gamedata
	# cada arquitectura: su quixjs y sus librerias en libs.ARCH/
	set -e; for a in $(PM_ARCHES); do \
	  cp $(PM)/quixjs.$$a $(PM_STAGE)/$(PORT)/; \
	  case $$a in aarch64) sr=$(SYSROOT_aarch64); tr=$(TRIPLE_aarch64);; armhf) sr=$(SYSROOT_armhf); tr=$(TRIPLE_armhf);; esac; \
	  mkdir -p $(PM_STAGE)/$(PORT)/libs.$$a; \
	  for l in $(PM_BUNDLE); do \
	    f=$$(find $$sr/usr/lib/$$tr $$sr/lib/$$tr -name "$$l" | head -1); \
	    test -n "$$f" || { echo "missing $$l in the $$a sysroot"; exit 1; }; \
	    cp -L "$$f" $(PM_STAGE)/$(PORT)/libs.$$a/$$l; \
	  done; \
	  cp $(PM_DEPS)/jsc-$$a/licenses/* $(PM_STAGE)/$(PORT)/licenses/; \
	done
	# la fuente de las pantallas de quixjs (el lanzador, los avisos): no todos los CFW traen una
	mkdir -p $(PM_STAGE)/$(PORT)/fonts
	cp $(SYSROOT_aarch64)/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf $(PM_STAGE)/$(PORT)/fonts/
	cp LICENSE $(PM_STAGE)/$(PORT)/licenses/quixjs.txt
	cp runtime/thirdparty/tomlc17/LICENSE $(PM_STAGE)/$(PORT)/licenses/tomlc17.txt
	cp runtime/thirdparty/stb/LICENSE $(PM_STAGE)/$(PORT)/licenses/stb_vorbis.txt
	cp runtime/thirdparty/nestegg/LICENSE $(PM_STAGE)/$(PORT)/licenses/nestegg.txt
	set -e; for p in $(PM_BUNDLE_PKGS); do \
	  cp $(SYSROOT_aarch64)/usr/share/doc/$$p/copyright $(PM_STAGE)/$(PORT)/licenses/$$p.txt; \
	done
	echo "$(PM_VERSION)" > $(PM_STAGE)/$(PORT)/VERSION
	chmod +x $(PM_STAGE)/*.sh $(PM_STAGE)/$(PORT)/quixjs.*
	rm -f $@ && cd $(PM_STAGE) && zip -q -r -9 $(CURDIR)/$@ .
	@echo "Built $@ (quixjs $(PM_VERSION), $(PM_ARCHES)): unzip it into the ports folder of the device"

clean: ; rm -rf $(PM) $(PORT).zip
distclean: clean ; rm -rf $(PM_DEPS)

.PHONY: all clean distclean
