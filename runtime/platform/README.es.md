# platform

[English](README.md) | Español

Lo que quixjs necesita del sistema operativo, detras de una sola cabecera
(`platform.h`). Cada carpeta la implementa y el `Makefile` compila una
(`PLATFORM_OS`):

| carpeta  | para |
|----------|------|
| `linux/` | el PC y las portatiles (glibc) |
| `android/` | la app de Android (bionic, `make PLATFORM=android`); `launcher.c` es el JNI de su menu y de Transfer Save |

Otro sistema (Android...) agrega su carpeta y el resto de quixjs sigue igual.
La ventana, el GL, la entrada y la salida de audio son del backend
([backend](../backend/README.es.md)), no de aqui.

| funcion | que da |
|---------|--------|
| `platform_mem_init()` | ajustes del malloc al arrancar (glibc: los bloques grandes en su propio mmap, para que vuelvan al sistema al liberarlos) |
| `platform_mem_trim()` | devolver al sistema la memoria libre del malloc |
| `platform_mem_available_mb()` | la RAM que le queda al sistema (el guardian de memoria del loop principal) |
| `platform_malloc_stats()` | el estado del malloc de C (el comando `mem` de la consola de debug) |
| `platform_default_font()` | la fuente por defecto (Linux: `QUIXJS_FONT` o una del sistema) |
| `platform_neighbours()` | equipos de la red local con los que hablo el sistema (Linux: la tabla ARP), para Transfer Save |
| `platform_user_name()`, `platform_system_name()` | quien es este equipo, para Transfer Save |
