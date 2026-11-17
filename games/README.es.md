# games

[English](README.md) | Español

Los juegos del repositorio: los juegos de test y los ports (juegos empaquetados
para PortMaster).

| carpeta         | que tiene |
|-----------------|-----------|
| `tests/`        | los juegos de test: una carpeta por test con su `config.toml`; las librerias compartidas en `lib/` (`assert.js`, `webgl.js`, PixiJS v4 en `lib/pixi_v4/pixi.js`); los archivos que cargan (imagenes, sonidos, datos) en `assets/`; el benchmark en `bench/canvas-lights` ([docs/tests.es.md](../docs/tests.es.md)) |
| `fearandhunger/` | el port de PortMaster de Fear & Hunger ([README](fearandhunger/README.es.md)) |

Un port es una carpeta con:

- `port.toml`: el toml que describe el juego y el port: `[engine]`, `[game]`,
  `[scripts]`, `[saves]`, `[portmaster]` (datos para PortMaster), los hooks,
  los logros y los trucos ([docs/usage.es.md](../docs/usage.es.md#ports-porttoml));
- `README.md` y `README.es.md`: como instalarlo; van en el zip;
- `screenshot.jpg`: la imagen que se ve en PortMaster y EmulationStation.

`make PLATFORM=portmaster PORT=<carpeta>` construye el zip de un port
([docs/build.es.md](../docs/build.es.md#portmaster)).
