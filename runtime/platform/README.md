# platform

English | [Español](README.es.md)

What quixjs needs from the operating system, behind one header
(`platform.h`). Each folder implements it and the `Makefile` builds one
(`PLATFORM_OS`):

| folder   | for |
|----------|-----|
| `linux/` | the PC and the handhelds (glibc) |
| `android/` | the Android app (bionic, `make PLATFORM=android`); `launcher.c` is the JNI of its menu and Transfer Save screens |

Another system (Android...) adds its folder and the rest of quixjs stays the
same. The window, GL, input and audio output belong to the backend
([backend](../backend/README.md)), not here.

| function | what it gives |
|----------|---------------|
| `platform_mem_init()` | malloc settings at start (glibc: big blocks in their own mmap, so they go back to the system when freed) |
| `platform_mem_trim()` | give the free memory of malloc back to the system |
| `platform_mem_available_mb()` | the RAM left in the system (the low memory guard of the main loop) |
| `platform_malloc_stats()` | the state of the C malloc (the `mem` command of the debug console) |
| `platform_default_font()` | the default font (Linux: `QUIXJS_FONT` or one of the system) |
| `platform_neighbours()` | machines of the local network the system talked to (Linux: the ARP table), for Transfer Save |
| `platform_user_name()`, `platform_system_name()` | who this machine is, for Transfer Save |
