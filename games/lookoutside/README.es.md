[English](README.md) | Español

## Instalar

Para usar este port necesitas el juego: Look Outside para Windows, que
puedes comprar en GOG (https://www.gog.com/game/look_outside) o en Steam
(https://store.steampowered.com/app/3373660/).

1. **Consigue los archivos del juego.**
   - GOG: descarga el instalador de Windows (`setup_look_outside_*.exe`) y
     descomprímelo en tu PC con innoextract
     (`innoextract setup_look_outside_*.exe`), o instálalo en Windows y abre
     la carpeta donde quedó.
   - Steam: instala el juego y abre su carpeta (en la biblioteca, clic
     derecho en el juego, Administrar, Ver archivos locales).

   Dentro están `Game.exe`, varios `.dll`, `index.html` y las carpetas
   `audio/`, `data/`, `img/`, `js/`... El juego es esa carpeta: `Game.exe`
   y los `.dll` son el programa de Windows y no se usan.

2. **Instala el port.** Copia `lookoutside.zip`, sin descomprimirlo, en la
   carpeta `autoinstall` de PortMaster de la SD de tu consola:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la consola: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Copia el contenido de la carpeta del paso 1 (la que
   tiene `index.html`) en `ports/lookoutside/gamedata/`, en la carpeta de
   ports de tu consola (donde está `Look Outside.sh`). Tiene que quedar así:

   ```
   ports/
     Look Outside.sh
     lookoutside/
       gamedata/
         index.html
         audio/  data/  img/  js/ ...
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH). Las carpetas `__redist/`, `locales/`, `lib/`, `swiftshader/` y
   los `.exe` y `.dll` no hacen falta y puedes dejarlos fuera.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Look Outside desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/save/`) se quedan.

Sin PortMaster: descomprime el zip en la carpeta de ports.

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), por ejemplo para la versión 0.4.0:

- `quixjs-0.4.0-android-arm64.apk`: la app
- `lookoutside-0.4.0-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `lookoutside-0.4.0-portmaster.zip`. Sale la carpeta
   `lookoutside/`.
3. Copia lo que hay dentro de la carpeta del juego, la que tiene `index.html` (paso 1 de Instalar) en `lookoutside/gamedata/`. Queda así:

   ```
   lookoutside/
     port.toml
     gamedata/
       index.html
       audio/  data/  img/  js/ ...
   ```

4. Comprime la carpeta `lookoutside/` en un `.zip` y pásalo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.

## Notas

Look Outside es de Francis Coulombe, publicado por Raw Fury. Este port solo
corre el juego, en quixjs (https://github.com/lowlevel-1989/quixjs).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | OK (aceptar) |
| B | Cancel (cancelar) |
| X | Dash (correr) |
| Y | Menu (menú) |
| L1 / R1 | Next / Prev: personaje o página siguiente / anterior, en los menús |
| Select + Start | Salir |
