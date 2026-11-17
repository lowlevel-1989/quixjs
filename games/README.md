# games

English | [Español](README.es.md)

The games of the repository: the test games and the ports (games packaged for
PortMaster).

| folder          | what it has |
|-----------------|-------------|
| `tests/`        | the test games: one folder per test with its `config.toml`; the shared libraries in `lib/` (`assert.js`, `webgl.js`, PixiJS v4 in `lib/pixi_v4/pixi.js`); the files they load (images, sounds, data) in `assets/`; the benchmark in `bench/canvas-lights` ([docs/tests.md](../docs/tests.md)) |
| `fearandhunger/` | the PortMaster port of Fear & Hunger ([README](fearandhunger/README.md)) |

A port is a folder with:

- `port.toml`: the toml that describes the game and the port: `[engine]`,
  `[game]`, `[scripts]`, `[saves]`, `[portmaster]` (data for PortMaster), the
  hooks, the achievements and the cheats ([docs/usage.md](../docs/usage.md#ports-porttoml));
- `README.md` and `README.es.md`: how to install it; they go in the zip;
- `screenshot.jpg`: the image shown in PortMaster and EmulationStation.

`make PLATFORM=portmaster PORT=<folder>` builds the zip of a port
([docs/build.md](../docs/build.md#portmaster)).
