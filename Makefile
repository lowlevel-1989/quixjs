# Makefile: los comandos de quixjs. Por debajo es CMake (CMakeLists.txt y
# cmake/, docs/build.md): este archivo solo los deja cortos e iguales en
# todas las plataformas.
#
#   make pc                      quixjs para el PC (build/quixjs); make, lo mismo
#   make portmaster PORT=hexgl   el zip de un port (build/hexgl.zip); PORT=all, todos
#   make android                 la app de Android (build/quixjs.apk)
#   make <plataforma>-jsc        solo JavaScriptCore
#   make <plataforma>-clean      limpiar una plataforma; make clean, todas
#   make test                    los tests (en el PC; sin el de Transfer Save, que pide UDP)
#   make bench                   el benchmark h5bench (en el PC)
#   make release                 los archivos de una release (release/)
#
# Opciones: TRACE=1 (las medidas [trace]), VIDEO=0 (sin video),
# ABI=x86_64 (Android para el emulador del PC).

PORT  ?= fearandhunger
ABI   ?= arm64-v8a
TRACE ?= 0
VIDEO ?= AUTO

ANDROID := android-$(if $(filter x86_64,$(ABI)),x86_64,arm64)
OPTIONS := -DQUIX_TRACE=$(if $(filter 1,$(TRACE)),ON,OFF) \
           -DQUIX_VIDEO=$(if $(filter 0,$(VIDEO)),OFF,$(if $(filter 1,$(VIDEO)),ON,$(VIDEO)))

# configurar un preset (rapido si ya esta) y compilar un target suyo
configure = cmake --preset $(1) $(OPTIONS) > /dev/null
build     = cmake --build --preset $(1) $(if $(2),--target $(2))

.DEFAULT_GOAL := pc
.PHONY: pc portmaster android pc-jsc portmaster-jsc android-jsc \
        pc-clean portmaster-clean android-clean clean test bench release

pc:
	$(call configure,pc) && $(call build,pc)

portmaster:
	$(call configure,portmaster) && $(call build,portmaster,$(if $(filter all,$(PORT)),ports,port-$(PORT)))

android:
	$(call configure,$(ANDROID)) && $(call build,$(ANDROID),apk)

pc-jsc:
	$(call configure,pc) && $(call build,pc,jsc)

portmaster-jsc:
	$(call configure,portmaster) && $(call build,portmaster,jsc)

android-jsc:
	$(call configure,$(ANDROID)) && $(call build,$(ANDROID),jsc)

pc-clean:
	rm -rf obj/cmake/pc build/quixjs

portmaster-clean:
	rm -rf obj/cmake/portmaster build/*.zip

android-clean:
	rm -rf obj/cmake/android-* build/quixjs*.apk build/quixjs*.apk.idsig

clean: pc-clean portmaster-clean android-clean

test: pc
	ctest --preset pc --label-exclude network

bench: pc
	tools/h5bench.sh

release:
	tools/release.sh
