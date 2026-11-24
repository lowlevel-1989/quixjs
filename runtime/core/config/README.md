# core/config

English | [Español](README.es.md)

The options of the command line and the toml of the game: read them
(`args.c`, `loader.c`), print them at start (`printer.c`) and write a
new `config.toml` from a game (`gen_config.c`, an index file with its parts
in `gen_config/`).

| file              | what it does |
|-------------------|--------------|
| `args.c`          | command line options (`-c`, `-b`, `--test`, `--frames`, `--script`, `--debug`, `--gen-config`, `--launcher`...) |
| `loader.c` | reads the toml of the game (`[engine]` and `[engine.pc]` / `[engine.handheld]`, `[game]` with `title`, `id` and `mods`, `[scripts]`, `[fonts]`, `[launcher]`, `[saves]` with `dir` from `game_dir`, `[controls]`) with tomlc17; `[symbols]` and `[[achievements]]` are read by `achievements/`, `[hooks]` by `hooks/` |
| `printer.c`       | prints the loaded config at start |
| `gen_config.c`    | `--gen-config <folder or .html>`: looks at the game (nw.js `package.json`, `index.html` or the given `.html`, its CSS, `rpg_core.js` / `rmmz_core.js`, `pixi.js`) and writes a `config.toml` to stdout. An index file: its parts are in `gen_config/` (`manifest.c`, `html.c`, `engine.c`, `output.c`) |
