[English](README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a Space Capsule Games y Dejobaan Games por crear Million Monster
Militia. Este port solo ejecuta el juego, con quixjs:
https://github.com/lowlevel-1989/quixjs.

Necesita Million Monster Militia **1.026** para Windows (Steam).

77 logros, los de Steam (opción Achievements del launcher del port).

Mods (activos): [controller](mods/controller/README.es.md) (jugar con mando), [achievements](mods/achievements/README.es.md) (Start: la lista de logros), [reader](mods/reader/README.es.md) (R1: los textos con letra grande).
Mods (apagados): [battle-music](mods/battle-music/README.es.md) (música en la batalla).

## Controles

| Botón | Acción |
|--|--|
| Cruceta | Mover el cursor entre lo que se puede pulsar; en el tablero, de casilla en casilla |
| A | Pulsar |
| B | Volver (cierra la ventana que abrió A) |
| X | Lleva el cursor al menú de arriba a la izquierda (el engranaje) |
| Y | Lleva el cursor al botón de la esquina de abajo a la derecha (ATTACK, desplegar…) |
| R1 | Lector: los textos de la pantalla (o de la carta del cursor) con letra grande |
| Start | Logros: la lista, en cualquier momento (Start o B vuelven) |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, Start la fija (el mando vuelve al juego) y la desfija, B la cierra |
| Select + X / Y | Savestate: guardar / cargar (no en hardcore) |
| Select + izquierda / derecha | Ranura del savestate (1 a 4) |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

Los savestates (Select + X / Y) son experimentales, en fase temprana: sigue
guardando también con el juego.

En el tablero la cruceta va girada 45 grados: arriba sube a la derecha,
derecha baja a la derecha, abajo baja a la izquierda e izquierda sube a la
izquierda.

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make portmaster PORT=millionmonstermilitia
```

Construye `build/millionmonstermilitia.zip`.

## Instalar

Para usar este port necesitas el juego: Million Monster Militia para
Windows, que puedes comprar en Steam
(https://store.steampowered.com/app/2358770/).

1. **Consigue los archivos del juego.** Instala el juego con Steam y abre su
   carpeta (en la biblioteca, clic derecho en el juego, Administrar, Ver
   archivos locales). El juego es el archivo `package.nw` (es un zip): cópialo
   a tu PC tal cual, sin descomprimirlo.

2. **Instala el port.** Copia `millionmonstermilitia.zip`, sin
   descomprimirlo, en la carpeta `autoinstall` de PortMaster de la SD de tu
   handheld:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la handheld: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Copia el archivo `package.nw` del paso 1, tal cual
   (sin descomprimirlo), en `ports/millionmonstermilitia/`, junto a
   `port.toml`, en la carpeta de ports de tu consola (donde está
   `Million Monster Militia.sh`). Debe quedar así:

   ```
   ports/
     Million Monster Militia.sh
     millionmonstermilitia/
       port.toml
       package.nw
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Viene comprimido: la primera vez que arranca el juego se prepara (un
   momento, solo una vez).

   Si tu archivo tiene otro nombre, escríbelo en
   `ports/millionmonstermilitia/port.toml`, en `rom = "package.nw"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Million Monster Militia desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/localstorage.json`) se quedan.

## port.toml

**port.toml** le dice a quixjs dónde está el juego y dónde van las partidas. Las rutas son relativas a la carpeta que tiene port.toml.

```toml
rom      = "package.nw"
game_dir = ""
player   = "Master"
```

- `rom`: el archivo `package.nw` del juego (un zip con sus archivos), copiado tal cual junto a port.toml.
- `game_dir`: vacío: el juego empieza en la raíz de package.nw, no hay ninguna carpeta dentro.
- `player`: tu nombre en el juego (el que daría Steam).
- Las partidas van a `gamedata/`, junto a port.toml, así `package.nw` nunca se modifica.
