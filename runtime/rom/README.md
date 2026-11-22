# rom

English | [Español](README.es.md)

The rom is the game: a `.zip` (also a `.nw`, an `.exe` with the zip
inside or the `.asar` of an Electron game) or a folder, next to the toml (`[game] rom`). quixjs reads it in
place, without unzipping it. `[game] game_dir` is the folder inside the rom
where the game starts (`""` the top).

The rom is mounted on `gamedata/`, the folder of quixjs next to the toml.
When the game asks for a file, quixjs looks in this order:

1. the active mods;
2. `gamedata/` on disk: what the game wrote (saves, `localStorage`) and what
   the port puts there;
3. the rom.

What the game writes always goes to `gamedata/`. A compressed zip is passed
once to a zip without compression (`zip/convert.c`), with a progress screen
(`launcher/rom.c`); after that it starts at once.

| file        | what it does |
|-------------|--------------|
| `rom.h`     | the rom API (read, open, stat, list, map) and the interface of each kind |
| `rom.c`     | mount it and pass each path of the game to its kind, without knowing which |
| `rom_zip.c` | the rom in a `.zip` (`zip/`): uncompress it once if needed, files and folders inside |
| `rom_dir.c` | the rom in a folder (the tests, or a game unzipped on the PC) |

`core/url.c` is the only one that calls it: every file the game reads goes
through `url.c` (scripts, images, sounds, videos, fonts, `fs` of nw.js).
