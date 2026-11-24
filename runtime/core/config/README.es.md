# core/config

[English](README.md) | Español

Las opciones de la línea de comandos y el toml del juego: leerlos
(`args.c`, `loader.c`), mostrarlos al arrancar (`printer.c`) y
escribir un `config.toml` nuevo a partir de un juego (`gen_config.c`, un
archivo índice con sus partes en `gen_config/`).

| archivo           | de qué se encarga |
|-------------------|-------------------|
| `args.c`          | opciones de la línea de comandos (`-c`, `-b`, `--test`, `--frames`, `--script`, `--debug`, `--gen-config`, `--launcher`...) |
| `loader.c` | lee el toml del juego (`[engine]` y `[engine.pc]` / `[engine.handheld]`, `[game]` con `title`, `id` y `mods`, `[scripts]`, `[fonts]`, `[launcher]`, `[saves]` con `dir` desde `game_dir`, `[controls]`) con tomlc17; `[symbols]` y `[[achievements]]` los lee `achievements/`, `[hooks]` los lee `hooks/` |
| `printer.c`       | muestra la config cargada al arrancar |
| `gen_config.c`    | `--gen-config <carpeta o .html>`: mira el juego (`package.json` de nw.js, `index.html` o el `.html` dado, su CSS, `rpg_core.js` / `rmmz_core.js`, `pixi.js`) y escribe un `config.toml` en stdout. Es un archivo indice: sus partes estan en `gen_config/` (`manifest.c`, `html.c`, `engine.c`, `output.c`) |
