# games

English | [Español](README.es.md)

The ports of the repository: games packaged for PortMaster (and the PC and
Android). The test games are in [tests/games](../tests/games).

| folder                   | what it has |
|--------------------------|-------------|
| `abbaye/`                | L'Abbaye des Morts ([README](abbaye/README.md)) |
| `fearandhunger/`         | Fear & Hunger ([README](fearandhunger/README.md)) |
| `felvidek/`              | Felvidek ([README](felvidek/README.md)) |
| `firework-shiying/`      | Firework ([README](firework-shiying/README.md)) |
| `gunfield/`              | Gunfield ([README](gunfield/README.md)) |
| `hexgl/`                 | HexGL ([README](hexgl/README.md)) |
| `lookoutside/`           | Look Outside ([README](lookoutside/README.md)) |
| `millionmonstermilitia/` | Million Monster Militia ([README](millionmonstermilitia/README.md)) |
| `muffleslifesentence/`   | Muffles' Life Sentence ([README](muffleslifesentence/README.md)) |
| `supersmilingdungeon/`   | Super Smiling Dungeon ([README](supersmilingdungeon/README.md)) |

## The shape of a port

All the ports have the same shape. Only `port.toml`, the READMEs, the notice
and the two images are required; the rest, if the port needs it.

```
games/<port>/
  port.toml               the toml of the game and of the port
  README.md, README.es.md how to install and play it; they go in the zip
  COPY_THE_GAME_HERE.txt  the notice of what to copy there (the rom of the game)
  cover.jpg               the title screen, 640x480
  screenshot.jpg          a scene of the game, 640x480
  entry.js                only if it is the [scripts] entry of the port
  scripts/                the own JS of the port in [scripts] includes
  hooks/                  the own hooks of the port ([hooks])
  mods/                   the mods (Mods option of the launcher)
```

- `port.toml`: `[engine]`, `[game]`, `[scripts]`, `[saves]`, `[portmaster]`
  (data for PortMaster), the hooks, the achievements and the cheats
  ([docs/usage.md](../docs/usage.md#ports-porttoml)).
- `cover.jpg` and `screenshot.jpg`: the images shown in PortMaster and
  EmulationStation, with black bars if the game is not 4:3.
- `entry.js`: see below.
- `scripts/`: what the port adds before or along with the game: fixes of bugs
  of the game (`gunfield/scripts/levelup.js`), the gamepad as a keyboard
  (`abbaye/scripts/gamepad-keys.js`), what the game asks Steam for
  (`supersmilingdungeon/scripts/steam-achievements.js`) or the parts of a big
  `entry.js`, in their folder (`hexgl/scripts/menu/`). In the `port.toml`:
  `"../scripts/<name>.js"` in `includes`.
- `hooks/`: the hooks of the port, in `[hooks]` as `"hooks/<name>"`; for
  example the one that marks the start of a playthrough
  (`hooks/playthrough-new.js`, [docs/trace-saves.md](../docs/trace-saves.md)).
- `mods/`: one folder per mod, in two ways:
  - with `hooks.toml` and its `.js` files: the mod changes functions of the
    game (`mods/rumble/`, `millionmonstermilitia/mods/reader/`);
  - with the folders of the game (`js/`, `img/`...): the mod replaces files of
    the game (`fearandhunger/mods/no-widescreen/js/`).

## What an entry.js is

`[scripts]` of the `port.toml` says which scripts quixjs loads and in which
order: `libs`, then `includes` and last `entry`, like the `<script>` tags of
the `index.html` of the game. `entry` is the last one: the one that starts
the game. The others only leave things defined (the engine, the classes of
the game, a fix) and start nothing.

A port has its `entry.js` only when the code that starts the game is not in a
file quixjs can load. That happens in two cases:

- the game has it inside the `index.html`, in a `<script>` without a file
  (`<script>cr_createRuntime(...)</script>`). quixjs does not read the HTML:
  the port copies that code to `entry.js` (Gunfield);
- the port starts the game its own way, with something the game does not
  bring (the menu of HexGL, which in the original is an HTML page).

If the game already has a file that starts it (`js/main.js` in RPG Maker MV,
`main.js` in many games), that one is the `entry` and the port has no
`entry.js`. A script of the port that does not start the game is not an
`entry.js`, even if it has to go right before the `entry`: it goes in
`scripts/`, at the end of `includes`. For example L'Abbaye des Morts: the
game starts by itself (`window.onload`, in its `js/main.js`), and the script
of the port that presses the keys with the gamepad is
`abbaye/scripts/gamepad-keys.js`; the `entry` is the last script of the game
(`js/entities/ghost.js`).

Example: the `index.html` of Gunfield ends with this `<script>`, without a
file:

```html
<script>
  jQuery(document).ready(function () { cr_createRuntime('c2canvas'); });
</script>
```

`games/gunfield/entry.js` does the same (with what nw.js needs):

```js
(function () {
  window.c2nodewebkit = true;
  jQuery(document).ready(function () {
    cr_createRuntime('c2canvas');   // starts Construct 2 on the canvas
  });
})();
```

And the `port.toml` loads it last:

```toml
[scripts]
includes = [
  "jquery-3.4.1.min.js",       # of the game: they start nothing
  "c2runtime.js",
  "../scripts/levelup.js"      # of the port: a fix, in scripts/
]
entry    = "../entry.js"       # the last one: it starts the game
```

The paths of `[scripts]` are from `game_dir` (inside `gamedata/`); the port
is one level up, hence `../entry.js` and `../scripts/`.

`make portmaster PORT=<folder>` builds the zip of a port
([docs/build.md](../docs/build.md#portmaster)): it copies the whole folder,
except the game, what it writes (`gamedata/`, `cache/`) and what belongs to
whoever plays (the launcher settings, the gamepad mapped in Controls, the log).
