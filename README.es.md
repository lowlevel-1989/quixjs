# quixjs 0.7.0

[English](README.md) | Español

quixjs ejecuta juegos de JavaScript (RPG Maker MV y MZ, Construct 2 y 3,
GDevelop 5, Phaser 2, PixiJS v4, three.js, JS sin motor; tambien juegos
empaquetados con nw.js o Electron) en portatiles con Linux, el PC y Android,
sin navegador y sin X11. Usa JavaScriptCore, SDL2 y OpenGL ES 2, y las APIs
del navegador que necesitan los juegos estan escritas en C.

[!["Buy Me A Coffee"](docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que este runtime siga recibiendo actualizaciones y soporte, puedes
apoyar el proyecto invitandome a un cafe.

## Ports

Un port es un juego empaquetado para [PortMaster](https://portmaster.games),
la herramienta que instala juegos en portatiles con Linux. Los archivos del
juego no van incluidos: el jugador los copia de su propia copia.

| port | donde conseguir el juego | motor | savestates | logros | como instalarlo |
|------|--------------------------|-------|------------|--------|-----------------|
| Fear & Hunger 1.4.1 | [itch.io](https://mirohaver.itch.io/fear-hunger) | RPG Maker MV | experimentales | sí | [games/fearandhunger](games/fearandhunger/README.es.md) |
| Firework | [GOG](https://www.gog.com/game/firework) | RPG Maker MV | experimentales | sí | [games/firework-shiying](games/firework-shiying/README.es.md) |
| Look Outside | [GOG](https://www.gog.com/game/look_outside) | RPG Maker MZ | experimentales | sí | [games/lookoutside](games/lookoutside/README.es.md) |
| Felvidék | [GOG](https://www.gog.com/game/felvidek) | RPG Maker MV | experimentales | no | [games/felvidek](games/felvidek/README.es.md) |
| Muffles' Life Sentence | [Steam](https://store.steampowered.com/app/3145370/Muffles_Life_Sentence/) | RPG Maker MV | experimentales | no | [games/muffleslifesentence](games/muffleslifesentence/README.es.md) |
| OMORI 1.0.0 | [Steam](https://store.steampowered.com/app/1150690/) (rama `v1.0.0`) | RPG Maker MV | experimentales | sí | [games/omori](games/omori/README.es.md) |
| HexGL (carreras entre handhelds de la misma red) | gratis, [GitHub](https://github.com/BKcore/HexGL) | three.js | no | no | [games/hexgl](games/hexgl/README.es.md) |
| Soccer Chapa (fútbol de chapas; torneos entre handhelds de la misma red) | viene con el port | PixiJS | no | no | [games/soccer-chapa](games/soccer-chapa/README.es.md) |
| Gunfield | [Steam](https://store.steampowered.com/app/2305900/) | Construct 2 | no | sí | [games/gunfield](games/gunfield/README.es.md) |
| Million Monster Militia | [Steam](https://store.steampowered.com/app/2358770/) | Construct 3 | experimentales | sí | [games/millionmonstermilitia](games/millionmonstermilitia/README.es.md) |
| L'Abbaye des Morts | gratis, [GitHub](https://github.com/luceraproject/abbaye-des-morts) | Phaser 2 | experimentales | sí | [games/abbaye](games/abbaye/README.es.md) |
| Super Smiling Dungeon | [Steam](https://store.steampowered.com/app/3539970/) | GDevelop 5 | no | sí | [games/supersmilingdungeon](games/supersmilingdungeon/README.es.md) |

Los savestates (Select + X / Y) son experimentales, en fase temprana: sigue
guardando también con el juego.

## Jugar un juego en Linux

```
make
./build/quixjs --gen-config ruta/al/juego > mijuego.toml   # describir el juego
./build/quixjs -c mijuego.toml --launcher                   # jugar (con el menu antes)
```

Cada juego se describe con un solo toml, que se pasa con `-c` (un port
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
| [Encontrar el guardado](docs/trace-saves.es.md) | `--trace-saves`: dónde guarda un juego y dónde van los hooks de playthrough |
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
- [bbbreaddd](https://github.com/bbbreaddd), pancito: por avisar de que OMORI
  1.0.0 no tiene DRM y se puede portar. Hace
  [pmjs](https://github.com/bbbreaddd/pmjs/tree/575917a030daff8df7af47da83156a903310e419), otro runtime nativo
  para juegos de RPG Maker MV y MZ: pruebalo
- KADOKAWA: el nucleo de RPG Maker MV es MIT
  ([corescript](https://github.com/rpgtkoolmv/corescript)); los hooks de RPG
  Maker MV reproducen partes de el, y de algunos plugins (Yanfly, Fallen Angel
  Olivia, Terrax, Tor Damian Design): las
  [referencias](runtime/engines/rpgmaker-mv/README.es.md#referencias)

## Licencia

GPL-3.0. JavaScriptCore (WebKit: BSD y LGPL-2.1), ICU (licencia Unicode),
tomlc17 (MIT), stb_vorbis (MIT / dominio publico), nestegg (tipo ISC) y PixiJS
(MIT) mantienen sus licencias.
