# game

[English](README.md) | Español

Lo que quixjs agrega encima del juego, sin tocar sus archivos. Cada carpeta
tiene su README.

| carpeta         | de qué se encarga |
|-----------------|-------------------|
| `hooks/`        | `[hooks]` del toml del juego: cambian como corre una funcion del juego (`gpu-canvas`, `skip`, `once-per-frame`, hooks de motor, `engine = "rpgmaker-mv"`) ([README](hooks/README.es.md)) |
| `symbols/`      | `[symbols]`: nombres para el estado del juego, que leen los logros y escriben los trucos ([README](symbols/README.es.md)) |
| `achievements/` | logros para los ports, definidos en el toml del juego ([README](achievements/README.es.md)) |
| `cheats/`       | trucos para los ports (`[[cheat]]`), encendidos en el launcher; una partida con trucos no consigue logros ([README](cheats/README.es.md)) |
