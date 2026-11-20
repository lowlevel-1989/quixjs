# quixjs 0.4.0

[English](README.md) | Español

quixjs ejecuta juegos de JavaScript (RPG Maker MV y MZ, PixiJS v4, three.js, JS sin motor) en
portatiles con Linux, sin navegador y sin X11. Usa JavaScriptCore, SDL2 y
OpenGL ES 2, y las APIs del navegador que necesitan los juegos estan escritas
en C.

[!["Buy Me A Coffee"](docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que este runtime siga recibiendo actualizaciones y soporte, puedes
apoyar el proyecto invitandome a un cafe.

## Ports

Un port es un juego empaquetado para [PortMaster](https://portmaster.games),
la herramienta que instala juegos en portatiles con Linux. Los archivos del
juego no van incluidos: el jugador los copia de su propia copia.

| port | motor | como instalarlo |
|------|-------|-----------------|
| Fear & Hunger 1.4.1 | RPG Maker MV | [games/fearandhunger](games/fearandhunger/README.es.md) |
| Firework | RPG Maker MV | [games/firework-shiying](games/firework-shiying/README.es.md) |
| Look Outside | RPG Maker MZ | [games/lookoutside](games/lookoutside/README.es.md) |
| Felvidék | RPG Maker MV | [games/felvidek](games/felvidek/README.es.md) |
| HexGL (carreras entre handhelds de la misma red) | three.js | [games/hexgl](games/hexgl/README.es.md) |

## Jugar un juego en Linux

```
make
./quixjs --gen-config ruta/al/juego > mijuego.toml   # describir el juego
./quixjs -c mijuego.toml --launcher                   # jugar (con el menu antes)
```

Cada juego se describe con un solo toml (`config.toml` por defecto; un port
usa `port.toml`): su carpeta, su tamano, sus scripts, sus hooks (cambios a
funciones del juego) y sus logros. Las opciones y el toml:
[docs/usage.es.md](docs/usage.es.md).

Usar un archivo toml para configurar cada juego esta inspirado en bogodroid
(https://github.com/binarycounter/bogodroid). Gracias a binarycounter.

## Documentacion

| | |
|---|---|
| [Compilar](docs/build.es.md) | dependencias, `make`, el zip de PortMaster |
| [Uso](docs/usage.es.md) | opciones, el toml de un juego, ports, mods, controles, el objeto `quix` |
| [Que soporta](docs/features.es.md) | lo que soporta quixjs |
| [Compatibilidad](docs/compatibility.es.md) | que APIs del navegador funcionan y cuales no |
| [Tests](docs/tests.es.md) | los juegos de test, `--script` y la consola de debug |
| [Motor de JavaScript](docs/javascript-engine.es.md) | por que JavaScriptCore |
| [Benchmark](docs/benchmark.es.md) | h5bench (`make bench`) y como perfilar un juego |
| [Codigo](runtime/README.es.md) | que hace cada carpeta y archivo del runtime |
| [El JS dentro de quixjs](docs/embedded-js.es.md) | como van al binario los `.js` de quixjs y quien los ejecuta; agregar uno |
| [Cambios](CHANGELOG) | que cambio en cada version |

## Gracias

- [binarycounter](https://github.com/binarycounter): describir cada juego con
  un toml viene de su [bogodroid](https://github.com/binarycounter/bogodroid)
- [Knifethrower](https://github.com/Knifethrower): por compartir
  [h5bench](https://github.com/Knifethrower/h5bench), el benchmark de
  runtimes de HTML5
- KADOKAWA: el nucleo de RPG Maker MV es MIT
  ([corescript](https://github.com/rpgtkoolmv/corescript)); los hooks de RPG
  Maker MV reproducen partes de el, y de algunos plugins (Yanfly, Fallen Angel
  Olivia, Terrax, Tor Damian Design): las
  [referencias](runtime/engines/rpgmaker-mv/README.es.md#referencias)

## Licencia

GPL-3.0. JavaScriptCore (WebKit: BSD y LGPL-2.1), ICU (licencia Unicode),
tomlc17 (MIT), stb_vorbis (MIT / dominio publico), nestegg (tipo ISC) y PixiJS
(MIT) mantienen sus licencias.
