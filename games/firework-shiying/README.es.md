[English](README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a Shiying Studio por crear Firework. Este port solo ejecuta el
juego, con quixjs: https://github.com/lowlevel-1989/quixjs.

Necesita Firework **1.0.4** para Windows (GOG o Steam).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | Select (aceptar) |
| B | Cancel (cancelar) |
| X | Correr (mod `run`); sobre una partida, borrarla (mod `delete-save`) |
| Y | Menu (menú) |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, B la cierra |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=firework-shiying
```

Crea `firework-shiying.zip`.

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
   la carpeta `autoinstall` de PortMaster de la SD de tu handheld:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la handheld: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Comprime la carpeta `www/` del paso 1 en un zip llamado `www.zip` (en
   Windows: botón derecho sobre `www`, Enviar a, Carpeta comprimida) y cópialo en `ports/firework-shiying/`, junto a `port.toml`, en la
   carpeta de ports de tu consola (donde está `Firework (Shiying Studio).sh`). Debe quedar así:

   ```
   ports/
     Firework (Shiying Studio).sh
     firework-shiying/
       port.toml
       www.zip
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Mejor un zip sin compresión (con 7-Zip: Nivel de compresión, Almacenar; en
   Linux o macOS: `zip -0 -r`). Uno comprimido también vale: la primera vez
   que arranca el juego se prepara (un momento, solo una vez).

   Si tu archivo tiene otro nombre, escríbelo en `ports/firework-shiying/port.toml`, en
   `rom = "www.zip"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Firework (Shiying Studio) desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/www/save/`) se quedan.

Sin PortMaster: descomprime el zip en la carpeta de ports.

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), por ejemplo para la
versión 0.4.0:

- `quixjs-0.4.0-android-arm64.apk`: la app
- `firework-shiying-0.4.0-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `firework-shiying-0.4.0-portmaster.zip`. Sale la carpeta
   `firework-shiying/`.
3. Copia el juego (`www.zip`, paso 3 de Instalar) en `firework-shiying/`, junto a `port.toml`. Queda así:

   ```
   firework-shiying/
     port.toml
     www.zip
   ```

4. Comprime la carpeta `firework-shiying/` en un `.zip` y pásalo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.

## Mods

En Mods, en el menú del port:

- `fast-start` (activo): directo al título, sin los logos ni la pantalla de controles.
- `big-text` (activo): el texto de los mensajes más grande.
- `run` (activo): mantén X para correr, en todos los mapas (el juego solo deja en 3).
- `delete-save` (activo): X dos veces sobre una partida, en guardar o cargar, la borra.
- `achievements` (activo): la opción "Achievements" en el menú del juego, con los logros.
