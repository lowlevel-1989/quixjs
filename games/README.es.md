# games

[English](README.md) | Español

Los ports del repositorio: juegos empaquetados para PortMaster (y para el PC y
Android). Los juegos de test estan en [tests/games](../tests/games).

| carpeta                  | que tiene |
|--------------------------|-----------|
| `abbaye/`                | L'Abbaye des Morts ([README](abbaye/README.es.md)) |
| `fearandhunger/`         | Fear & Hunger ([README](fearandhunger/README.es.md)) |
| `felvidek/`              | Felvidek ([README](felvidek/README.es.md)) |
| `firework-shiying/`      | Firework ([README](firework-shiying/README.es.md)) |
| `gunfield/`              | Gunfield ([README](gunfield/README.es.md)) |
| `hexgl/`                 | HexGL ([README](hexgl/README.es.md)) |
| `lookoutside/`           | Look Outside ([README](lookoutside/README.es.md)) |
| `millionmonstermilitia/` | Million Monster Militia ([README](millionmonstermilitia/README.es.md)) |
| `muffleslifesentence/`   | Muffles' Life Sentence ([README](muffleslifesentence/README.es.md)) |
| `supersmilingdungeon/`   | Super Smiling Dungeon ([README](supersmilingdungeon/README.es.md)) |

## La forma de un port

Todos los ports tienen la misma forma. Solo `port.toml`, los README, el
aviso y las dos imagenes son obligatorios; el resto, si el port lo necesita.

```
games/<port>/
  port.toml               el toml del juego y del port
  README.md, README.es.md como instalarlo y jugarlo; van en el zip
  COPY_THE_GAME_HERE.txt  el aviso de que copiar ahi (la rom del juego)
  cover.jpg               la pantalla de titulo, 640x480
  screenshot.jpg          una escena del juego, 640x480
  entry.js                solo si es el [scripts] entry del port
  scripts/                el JS propio del port en [scripts] includes
  hooks/                  los hooks propios del port ([hooks])
  mods/                   los mods (opcion Mods del launcher)
```

- `port.toml`: `[engine]`, `[game]`, `[scripts]`, `[saves]`, `[portmaster]`
  (datos para PortMaster), los hooks, los logros y los trucos
  ([docs/usage.es.md](../docs/usage.es.md#ports-porttoml)).
- `cover.jpg` y `screenshot.jpg`: las imagenes que se ven en PortMaster y
  EmulationStation, con bandas negras si el juego no es 4:3.
- `entry.js`: ver abajo.
- `scripts/`: lo que el port agrega antes o junto al juego: arreglos de
  fallos del juego (`gunfield/scripts/levelup.js`), el mando como teclado
  (`abbaye/scripts/gamepad-keys.js`), lo que el juego pide a
  Steam (`supersmilingdungeon/scripts/steam-achievements.js`) o las partes
  de un `entry.js` grande, en su carpeta (`hexgl/scripts/menu/`). En el
  `port.toml`: `"../scripts/<nombre>.js"` en `includes`.
- `hooks/`: los hooks del port, en `[hooks]` como `"hooks/<nombre>"`; por
  ejemplo el que marca el comienzo de una partida
  (`hooks/playthrough-new.js`, [docs/trace-saves.es.md](../docs/trace-saves.es.md)).
- `mods/`: una carpeta por mod, de dos formas:
  - con `hooks.toml` y sus `.js`: el mod cambia funciones del juego
    (`mods/rumble/`, `millionmonstermilitia/mods/reader/`);
  - con las carpetas del juego (`js/`, `img/`...): el mod sustituye
    archivos del juego (`fearandhunger/mods/no-widescreen/js/`).

## Que es un entry.js

`[scripts]` del `port.toml` dice que scripts carga quixjs y en que orden:
`libs`, despues `includes` y al final `entry`, como los `<script>` del
`index.html` del juego. `entry` es el ultimo: el que arranca el juego. Los
demas solo dejan cosas definidas (el motor, las clases del juego, un
arreglo) y no arrancan nada.

Un port tiene su `entry.js` solo cuando el codigo que arranca el juego no
esta en un archivo que quixjs pueda cargar. Pasa en dos casos:

- el juego lo trae dentro del `index.html`, en un `<script>` sin archivo
  (`<script>cr_createRuntime(...)</script>`). quixjs no lee el HTML: el port
  copia ese codigo a `entry.js` (Gunfield);
- el port arranca el juego a su manera, con algo que el juego no trae (el
  menu de HexGL, que en el original es una pagina HTML).

Si el juego ya tiene un archivo que lo arranca (`js/main.js` en RPG Maker
MV, `main.js` en muchos juegos), ese es el `entry` y el port no tiene
`entry.js`. Un script del port que no arranca el juego no es un `entry.js`,
aunque tenga que ir justo antes del `entry`: va en `scripts/`, al final de
`includes`. Por ejemplo L'Abbaye des Morts: el juego arranca solo
(`window.onload`, en su `js/main.js`), y el script del port que pulsa las
teclas con el mando es `abbaye/scripts/gamepad-keys.js`; el `entry` es el
ultimo script del juego (`js/entities/ghost.js`).

Ejemplo: el `index.html` de Gunfield termina con este `<script>`, sin
archivo:

```html
<script>
  jQuery(document).ready(function () { cr_createRuntime('c2canvas'); });
</script>
```

`games/gunfield/entry.js` hace lo mismo (con lo que necesita nw.js):

```js
(function () {
  window.c2nodewebkit = true;
  jQuery(document).ready(function () {
    cr_createRuntime('c2canvas');   // arranca Construct 2 en el canvas
  });
})();
```

Y el `port.toml` lo carga el ultimo:

```toml
[scripts]
includes = [
  "jquery-3.4.1.min.js",       # del juego: no arrancan nada
  "c2runtime.js",
  "../scripts/levelup.js"      # del port: un arreglo, en scripts/
]
entry    = "../entry.js"       # el ultimo: arranca el juego
```

Las rutas de `[scripts]` son desde `game_dir` (dentro de `gamedata/`); el
port esta un nivel arriba, por eso `../entry.js` y `../scripts/`.

`make portmaster PORT=<carpeta>` construye el zip de un port
([docs/build.es.md](../docs/build.es.md#portmaster)): copia la carpeta entera,
menos el juego, lo que escribe (`gamedata/`, `cache/`) y lo de quien juega
(los ajustes del launcher, el mando asignado en Controls, el log).
