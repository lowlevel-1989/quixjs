[English](README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a Francis Coulombe por crear Look Outside, y a Raw Fury por
publicarlo. Este port solo ejecuta el juego, con quixjs:
https://github.com/lowlevel-1989/quixjs.

Necesita Look Outside **2.3** para Windows (GOG o Steam).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | OK (aceptar) |
| B | Cancel (cancelar) |
| X | Dash (correr) |
| Y | Menu (menú) |
| L1 / R1 | Next / Prev: personaje o página siguiente / anterior, en los menús |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, B la cierra |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=lookoutside
```

Crea `lookoutside.zip`.

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

   La carpeta del juego es `Look Outside`: dentro están `Game.exe`, varios
   `.dll` y las carpetas `audio/`, `data/`, `img/`, `js/`... (`Game.exe` y
   los `.dll` son el programa de Windows y no se usan). Con innoextract la
   carpeta sale con otro nombre: renómbrala a `Look Outside`.

2. **Instala el port.** Copia `lookoutside.zip`, sin descomprimirlo, en la
   carpeta `autoinstall` de PortMaster de la SD de tu handheld:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la handheld: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Comprime la carpeta del paso 1 (`Look Outside`, la que
   tiene `data/` y `js/`) en un zip (en Windows: botón derecho sobre ella,
   Enviar a, Carpeta comprimida): sale `Look Outside.zip`. Cópialo en
   `ports/lookoutside/`, junto a `port.toml`, en la carpeta de ports de tu
   consola (donde está `Look Outside.sh`). Debe quedar así:

   ```
   ports/
     Look Outside.sh
     lookoutside/
       port.toml
       Look Outside.zip
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Mejor un zip sin compresión (con 7-Zip: Nivel de compresión, Almacenar; en
   Linux o macOS: `zip -0 -r`). Uno comprimido también vale: la primera vez
   que arranca el juego se prepara (un momento, solo una vez).

   Si tu archivo tiene otro nombre, escríbelo en `ports/lookoutside/port.toml`, en
   `rom = "Look Outside.zip"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Look Outside desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/save/`) se quedan.

Sin PortMaster: descomprime el zip en la carpeta de ports.

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), la última versión
(`VERSION` en los nombres es su número):

- `quixjs-VERSION-android-arm64.apk`: la app
- `lookoutside-VERSION-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `lookoutside-VERSION-portmaster.zip`. Sale la carpeta
   `lookoutside/`.
3. Copia el juego (`Look Outside.zip`, paso 3 de Instalar) en `lookoutside/`, junto a `port.toml`. Queda así:

   ```
   lookoutside/
     port.toml
     Look Outside.zip
   ```

4. Comprime la carpeta `lookoutside/` en un `.zip` y pásalo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.
