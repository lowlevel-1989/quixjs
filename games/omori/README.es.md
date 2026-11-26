[English](https://github.com/lowlevel-1989/quixjs/blob/master/games/omori/README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a OMOCAT por crear OMORI. Este port solo ejecuta el juego, con
quixjs: https://github.com/lowlevel-1989/quixjs.

Necesita OMORI **1.0.0** para Windows (Steam, la rama `v1.0.0`).

Gracias a [bbbreaddd](https://github.com/bbbreaddd), pancito, por avisar de
que OMORI 1.0.0 no tiene DRM y se puede portar. Él trabaja en
[pmjs](https://github.com/bbbreaddd/pmjs/tree/575917a030daff8df7af47da83156a903310e419), otro runtime nativo para
juegos de RPG Maker MV y MZ: pásate a probarlo.

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | OK (Z) |
| B | Menú / volver (X) |
| X | Correr (Shift) |
| Y | Etiqueta (A) |
| L1 / R1 | Página anterior / siguiente (Q / W) |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, Start la fija (el mando vuelve al juego) y la desfija, B la cierra |
| Select + X / Y | Savestate: guardar / cargar (en el mapa; no en hardcore) |
| Select + izquierda / derecha | Ranura del savestate (1 a 4) |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

Mods (activos): [controller-mode](mods/controller-mode/README.es.md) (el modo mando de OMORI: los avisos enseñan los botones del mando).

Los savestates (Select + X / Y) son experimentales, en fase temprana: sigue
guardando también con el juego.

Los 85 logros de Steam están en TROPHIES, en el menú del juego.

## Instalar

Para usar este port necesitas el juego: OMORI para Windows, que puedes
comprar en Steam (https://store.steampowered.com/app/1150690/). El port
necesita su versión 1.0.0, que Steam guarda como una rama.

1. **Consigue los archivos del juego.** En la biblioteca de Steam, clic
   derecho en OMORI, Propiedades, Betas, y elige `v1.0.0` en Participación
   en betas. Espera a que Steam la descargue y abre su carpeta (clic derecho
   en OMORI, Administrar, Ver archivos locales). El port solo necesita la
   carpeta `www/`.

2. **Instala el port.** Copia `omori.zip`, sin descomprimirlo, en la
   carpeta `autoinstall` de PortMaster de la SD de tu handheld:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la handheld: al arrancar instala el zip solo
   (con la imagen que enseña EmulationStation) y lo quita de `autoinstall`.
   Luego puedes cerrar PortMaster.

3. **Copia el juego.** Comprime la carpeta `www/` del paso 1 en un zip llamado `www.zip` (en
   Windows: botón derecho sobre `www`, Enviar a, Carpeta comprimida) y cópialo en `ports/omori/`, junto a `port.toml`, en la
   carpeta de ports de tu consola (donde está `OMORI.sh`). Debe quedar así:

   ```
   ports/
     OMORI.sh
     omori/
       port.toml
       www.zip
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Mejor un zip sin compresión (con 7-Zip: Nivel de compresión, Almacenar; en
   Linux o macOS: `zip -0 -r`). Uno comprimido también vale: la primera vez
   que arranca el juego se prepara (un momento, solo una vez).

   Si tu archivo tiene otro nombre, escríbelo en `ports/omori/port.toml`, en
   `rom = "www.zip"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre OMORI desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego, tus
partidas y tus logros (`gamedata/www/save/`) se quedan.

Sin PortMaster: descomprime el zip en la carpeta de ports. Así puede que
EmulationStation no enseñe la imagen del port.

## port.toml

**port.toml** le dice a quixjs dónde está el juego y dónde van las partidas. Las rutas son relativas a la carpeta que tiene port.toml.

```toml
rom      = "www.zip"
game_dir = "www"
```

- `rom`: la carpeta `www/` del juego (la que tiene `data/`, `js/` ...) comprimida en un zip y puesta junto a port.toml.
- `game_dir`: la carpeta dentro del zip donde empieza el juego. Al comprimir una carpeta en Windows, la propia carpeta queda dentro del zip, así que esto le dice a quixjs dónde mirar.
- Las partidas van a `gamedata/`, junto a port.toml, así `www.zip` nunca se modifica.

```
port.toml
www.zip
  www/
    data/
    js/
    ...
gamedata/
```

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), la última versión
(`VERSION` en los nombres es su número):

- `quixjs-VERSION-android-arm64.apk`: la app
- `omori-VERSION-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `omori-VERSION-portmaster.zip`. Sale la carpeta
   `omori/`.
3. Copia el juego (`www.zip`, paso 3 de Instalar) en `omori/`, junto a `port.toml`. Queda así:

   ```
   omori/
     port.toml
     www.zip
   ```

4. Comprime la carpeta `omori/` en un `.zip` y pásalo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.
