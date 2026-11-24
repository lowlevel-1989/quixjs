[English](README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a ChoKo por crear Super Smiling Dungeon. Este port solo ejecuta el
juego, con quixjs: https://github.com/lowlevel-1989/quixjs.

Necesita Super Smiling Dungeon **1.2.5** para Windows (Steam).

63 logros, los de Steam (opción Achievements del launcher del port).

Mods (activos): [controller](mods/controller/README.es.md), [big-text](mods/big-text/README.es.md), [achievements](mods/achievements/README.es.md), [fast-text](mods/fast-text/README.es.md).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Llevar el cursor al botón siguiente en esa dirección |
| A | Pulsar donde está el cursor |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / R2 acercan, L1 / L2 alejan, Y / X tamaño, A centra, Start la fija (el mando vuelve al juego) y la desfija, B la cierra |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make portmaster PORT=supersmilingdungeon
```

Construye `build/supersmilingdungeon.zip`.

## Instalar

Para usar este port necesitas el juego: Super Smiling Dungeon para Windows,
que puedes comprar en Steam (https://store.steampowered.com/app/3539970/).

1. **Consigue los archivos del juego.** Instala el juego con Steam y abre su
   carpeta (en la biblioteca, clic derecho en el juego, Administrar, Ver
   archivos locales). El juego es el archivo `resources/app.asar`: cópialo a
   tu PC tal cual.

2. **Instala el port.** Copia `supersmilingdungeon.zip`, sin descomprimirlo,
   en la carpeta `autoinstall` de PortMaster de la SD de tu handheld:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la handheld: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Copia el archivo `app.asar` del paso 1, tal cual, en
   `ports/supersmilingdungeon/`, junto a `port.toml`, en la carpeta de ports
   de tu consola (donde está `Super Smiling Dungeon.sh`). Debe quedar así:

   ```
   ports/
     Super Smiling Dungeon.sh
     supersmilingdungeon/
       port.toml
       app.asar
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Si tu archivo tiene otro nombre, escríbelo en
   `ports/supersmilingdungeon/port.toml`, en `rom = "app.asar"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Super Smiling Dungeon desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/localstorage.json`) se quedan.

## port.toml

**port.toml** le dice a quixjs dónde está el juego y dónde van las partidas. Las rutas son relativas a la carpeta que tiene port.toml.

```toml
rom      = "app.asar"
game_dir = "app"
```

- `rom`: el archivo `app.asar` del juego (en su carpeta `resources/`), copiado tal cual junto a port.toml.
- `game_dir`: la carpeta dentro de app.asar donde empieza el juego.
- Las partidas van a `gamedata/`, junto a port.toml, así `app.asar` nunca se modifica.

```
port.toml
app.asar
  app/
    gd.js
    ...
gamedata/
```
