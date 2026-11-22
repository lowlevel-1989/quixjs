#pragma once

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
