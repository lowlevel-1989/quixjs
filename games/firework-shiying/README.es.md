[English](README.md) | Español

# Firework (Shiying Studio)

## Instalar

Para usar este port necesitas el juego: Firework para Windows, que puedes
comprar en GOG (https://www.gog.com/game/firework) o en Steam
(https://store.steampowered.com/app/1288310/).

1. **Consigue los archivos del juego.**
   - GOG: descarga el instalador de Windows y desempaquétalo en tu PC con
     innoextract (https://constexpr.org/innoextract/), o instálalo en
     Windows y abre su carpeta:

     ```
     innoextract "setup_firework_v1.0.41203_(64bit)_(52017).exe"
     ```

     La carpeta `app/` que crea tiene `www/`.
   - Steam: instala el juego y abre su carpeta (en la biblioteca, clic
     derecho en el juego, Administrar, Ver archivos locales).

   El port solo necesita la carpeta `www/`; `Game.exe` y los `.dll` no se
   usan.

2. **Instala el port.** Copia `firework-shiying.zip`, sin descomprimirlo, en
   la carpeta `autoinstall` de PortMaster de la SD de tu consola:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la consola: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Copia la carpeta `www/` del paso 1 en
   `ports/firework-shiying/gamedata/`, en la carpeta de ports de tu consola
   (donde está `Firework (Shiying Studio).sh`). Tiene que quedar así:

   ```
   ports/
     Firework (Shiying Studio).sh
     firework-shiying/
       gamedata/
         www/
           data/  img/  js/  audio/ ...
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Firework (Shiying Studio) desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/www/save/`) se quedan.

Sin PortMaster: descomprime el zip en la carpeta de ports.

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), por ejemplo para la versión 0.4.0:

- `quixjs-0.4.0-android-arm64.apk`: la app
- `firework-shiying-0.4.0-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `firework-shiying-0.4.0-portmaster.zip`. Sale la carpeta
   `firework-shiying/`.
3. Copia la carpeta `www/` del juego (ver Los archivos del juego) en `firework-shiying/gamedata/`. Queda así:

   ```
   firework-shiying/
     port.toml
     gamedata/
       www/
   ```

4. Comprime la carpeta `firework-shiying/` en un `.zip` y pásalo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.

## Mods

En Mods, en el menú del port:

- `fast-start` (activo): directo al título, sin los logos ni la pantalla de controles.
- `big-text` (activo): el texto de los mensajes más grande.

## Notas

Firework es de Shiying Studio. Este port solo ejecuta el juego, en quixjs:
https://github.com/lowlevel-1989/quixjs.

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | Select (aceptar) |
| B | Cancel (cancelar) |
| Y | Menu (menú) |
| Select + Start | Salir |
