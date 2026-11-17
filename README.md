# QuixJS 0.2.2

English | [Español](README.es.md)

QuixJS runs JavaScript games (RPG Maker MV, PixiJS v4, vanilla JS) on Linux
handhelds, without a browser and without X11. It uses JavaScriptCore, SDL2 and
OpenGL ES 2, and the browser APIs the games need are written in C.

[!["Buy Me A Coffee"](docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

If you want this runtime to keep getting updates and support, you can support
the project by buying me a coffee.

## Ports

A port is a game packaged for [PortMaster](https://portmaster.games), the
tool that installs games on Linux handhelds. The game files are not included:
the player copies them from their own copy.

| port | engine | how to install it |
|------|--------|-------------------|
| Fear & Hunger 1.4.1 | RPG Maker MV | [games/fearandhunger](games/fearandhunger/README.md) |

## Play a game on Linux

```
make
./quixjs --gen-config path/to/game > mygame.toml   # describe the game
./quixjs -c mygame.toml --launcher                  # play (with the menu first)
```

Each game is described by one toml file (`config.toml` by default; a port uses
`port.toml`): its folder, its size, its scripts, its hooks (changes to
functions of the game) and its achievements. The options and the toml:
[docs/usage.md](docs/usage.md).

Using a toml file to configure each game is inspired by bogodroid
(https://github.com/binarycounter/bogodroid). Thanks binarycounter.

## Documentation

| | |
|---|---|
| [Build](docs/build.md) | dependencies, `make`, the PortMaster zip |
| [Usage](docs/usage.md) | options, the toml of a game, ports, mods, controls, the `quix` object |
| [Features](docs/features.md) | what QuixJS supports |
| [Compatibility](docs/compatibility.md) | which browser APIs work and which do not |
| [Tests](docs/tests.md) | the test games, `--script` and the debug console |
| [JavaScript engine](docs/javascript-engine.md) | why JavaScriptCore |
| [Code](runtime/README.md) | what each folder and file of the runtime does |
| [Changes](CHANGELOG) | what changed in each version |

## License

GPL-3.0. JavaScriptCore (WebKit: BSD and LGPL-2.1), ICU (Unicode license),
tomlc17 (MIT), stb_vorbis (MIT / public domain), nestegg (ISC style) and PixiJS
(MIT) keep their own licenses.
