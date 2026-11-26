[English](https://github.com/lowlevel-1989/quixjs/blob/master/games/hexgl/README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a Thibaut Despoulain (BKcore) por crear HexGL y dejarlo gratis.
Este port solo ejecuta el juego, con quixjs:
https://github.com/lowlevel-1989/quixjs.

Necesita HexGL en el commit **6addc95** (la versión enlazada en Instalar).

Mods (opción Mods del launcher del port, todos activos):
[multiplayer](mods/multiplayer/README.es.md) (carreras entre handhelds),
[volume](mods/volume/README.es.md) (Music y Effects en el menú) y
[rumble](mods/rumble/README.es.md) (vibración). Opción
Cheats: No damage (los choques no quitan escudo).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Girar |
| A | Acelerar |
| L2 / R2 | Frenos de aire |
| Select | Empezar la carrera otra vez |
| Start | Pausa: seguir, reiniciar o volver al menú |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, Start la fija (el mando vuelve al juego) y la desfija, B la cierra |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

En el menú: la cruceta para elegir, izquierda y derecha para cambiar una
opción, A para aceptar, B para volver.

## Instalar

Para usar este port necesitas HexGL, que es gratis.

1. **Descarga el juego.** HexGL es un juego gratuito de Thibaut Despoulain (BKcore), en GitHub.
   Abre su repositorio en la versión que usa este port:
   https://github.com/BKcore/HexGL/tree/6addc95a2fce3bf05f4d751823cc054c61a16d68

   y descárgalo con el botón verde **Code** y después **Download ZIP**. Sale
   `HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip`: no lo descomprimas, lo copias tal cual en el paso 3.

2. **Instala el port.** Copia `hexgl.zip`, sin descomprimirlo, en la carpeta
   `autoinstall` de PortMaster de la SD de tu handheld:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en el handheld: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Copia el zip del paso 1, tal cual (sin descomprimirlo), en `ports/hexgl/`, junto a `port.toml`, en la
   carpeta de ports de tu consola (donde está `HexGL.sh`). Debe quedar así:

   ```
   ports/
     HexGL.sh
     hexgl/
       port.toml
       HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Viene comprimido: la primera vez que arranca el juego se prepara (un
   momento, solo una vez).

   Si tu archivo tiene otro nombre, escríbelo en `ports/hexgl/port.toml`, en
   `rom = "HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre HexGL desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego se
queda.

Sin PortMaster: descomprime el zip en la carpeta de ports. Entonces
EmulationStation puede no mostrar la imagen del port.

## port.toml

**port.toml** le dice a quixjs dónde está el juego y dónde van las partidas. Las rutas son relativas a la carpeta que tiene port.toml.

```toml
rom      = "HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip"
game_dir = "HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68"
```

- `rom`: el zip descargado de GitHub (Code, Download ZIP), puesto tal cual junto a port.toml.
- `game_dir`: la carpeta dentro del zip donde empieza el juego: GitHub lo mete todo en una carpeta con el nombre del repositorio y del commit.
- Las partidas van a `gamedata/`, junto a port.toml, así `HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip` nunca se modifica.

```
port.toml
HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip
  HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68/
    bkcore/
    css/
    ...
gamedata/
```

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), la última versión
(`VERSION` en los nombres es su número):

- `quixjs-VERSION-android-arm64.apk`: la app
- `hexgl-VERSION-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `hexgl-VERSION-portmaster.zip`. Te queda la carpeta
   `hexgl/`.
3. Copia el juego (`HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip`, paso 3 de Instalar) en `hexgl/`, junto a `port.toml`. Queda así:

   ```
   hexgl/
     port.toml
     HexGL-6addc95a2fce3bf05f4d751823cc054c61a16d68.zip
   ```

4. Comprime la carpeta `hexgl/` en un `.zip` y cópialo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.
