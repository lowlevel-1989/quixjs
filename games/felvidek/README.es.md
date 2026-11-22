[English](README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a Jan Malík por crear Felvidék. Este port solo ejecuta el juego,
con quixjs: https://github.com/lowlevel-1989/quixjs.

Necesita Felvidék **1.04** para Windows (GOG).

Mods (activos): [big-text](mods/big-text/README.es.md) (texto más grande).
Mods (apagados): [readable-font](mods/readable-font/README.es.md) (fuente más legible).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | OK (aceptar, hablar) |
| B | Cancel (cancelar) |
| X | Dash (correr) |
| Y | Menu (menú) |
| L1 / R1 | Anterior / siguiente, en los menús |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, B la cierra |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=felvidek
```

Construye `felvidek.zip`.

## Instalar

Para usar este port necesitas el juego: Felvidék para Windows, que puedes
comprar en GOG (https://www.gog.com/game/felvidek).

1. **Consigue los archivos del juego.** Instala Felvidék en tu PC desde GOG
   y abre su carpeta. El port solo necesita la carpeta `www/`.

2. **Instala el port.** Copia `felvidek.zip`, sin descomprimirlo, en la
   carpeta `autoinstall` de PortMaster de la SD de tu handheld:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la handheld: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Comprime la carpeta `www/` del paso 1 en un zip llamado `www.zip` (en
   Windows: botón derecho sobre `www`, Enviar a, Carpeta comprimida) y cópialo en `ports/felvidek/`, junto a `port.toml`, en la
   carpeta de ports de tu consola (donde está `Felvidek.sh`). Debe quedar así:

   ```
   ports/
     Felvidek.sh
     felvidek/
       port.toml
       www.zip
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Mejor un zip sin compresión (con 7-Zip: Nivel de compresión, Almacenar; en
   Linux o macOS: `zip -0 -r`). Uno comprimido también vale: la primera vez
   que arranca el juego se prepara (un momento, solo una vez).

   Si tu archivo tiene otro nombre, escríbelo en `ports/felvidek/port.toml`, en
   `rom = "www.zip"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Felvidék desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/www/save/`) se quedan.

Sin PortMaster: descomprime el zip en la carpeta de ports. Así puede que
EmulationStation no enseñe la imagen del port.

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), la última versión
(`VERSION` en los nombres es su número):

- `quixjs-VERSION-android-arm64.apk`: la app
- `felvidek-VERSION-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `felvidek-VERSION-portmaster.zip`. Sale la carpeta
   `felvidek/`.
3. Copia el juego (`www.zip`, paso 3 de Instalar) en `felvidek/`, junto a `port.toml`. Queda así:

   ```
   felvidek/
     port.toml
     www.zip
   ```

4. Comprime la carpeta `felvidek/` en un `.zip` y pásalo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.
