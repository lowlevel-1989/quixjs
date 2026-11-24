#pragma once

#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>

// El sandbox: el juego solo ve archivos dentro de la carpeta de su port (ver
// runtime/core/sandbox.c)

// Leer: la carpeta del toml del juego. Escribir: solo write_dir (la carpeta
// de las partidas, gamedata; sin ella, la del toml). 0 si va bien.
int sandbox_init(const char *config_path, const char *write_dir);
// 1 si path (relativa a la carpeta actual o absoluta) se puede leer
int sandbox_allows(const char *path);
// 1 si en path se puede escribir (crear, borrar, renombrar)
int sandbox_allows_write(const char *path);
// Como sandbox_allows / _write, pero si no, lo dice en el log (what: que se pidio)
int sandbox_check(const char *path, const char *what);
int sandbox_check_write(const char *path, const char *what);

// Abrir y comprobar lo que de verdad se abrio (su ruta en /proc/self/fd):
// entre comprobar una ruta y abrirla, un enlace simbolico podria cambiar;
// asi lo que se lee o se escribe es lo comprobado. Escribir (O_WRONLY,
// O_RDWR, O_CREAT...) solo en la carpeta de las partidas. -1 / NULL si no
// (errno EACCES), y lo dice en el log (what)
int   sandbox_open(const char *path, int flags, int mode, const char *what);
FILE *sandbox_fopen(const char *path, const char *mode, const char *what);
DIR  *sandbox_opendir(const char *path, const char *what);
int   sandbox_stat(const char *path, struct stat *st, const char *what);
// Crear, borrar y renombrar dentro de la carpeta de las partidas: con la
// carpeta abierta y comprobada (mkdirat, unlinkat, renameat). 0 si va bien
int   sandbox_mkdir(const char *path, int mode, const char *what);
int   sandbox_unlink(const char *path, const char *what);
int   sandbox_rmdir(const char *path, const char *what);
int   sandbox_rename(const char *from, const char *to, const char *what);
