[English](README.md) | Español

## Instalar

Para usar este port necesitas el juego: Fear & Hunger 1.4.1 para Windows,
que puedes comprar en itch.io (https://mirohaver.itch.io/fear-hunger) o en
Steam (https://store.steampowered.com/app/1002300/).

1. **Consigue los archivos del juego.** Descarga la versión de Windows 1.4.1
   de itch.io y descomprímela en tu PC, o, en Steam, instala el juego y abre
   su carpeta (en la biblioteca, clic derecho en el juego, Administrar, Ver
   archivos locales). El port solo necesita la carpeta `www/`.

2. **Instala el port.** Copia `fearandhunger.zip`, sin descomprimirlo, en la
   carpeta `autoinstall` de PortMaster de la SD de tu consola:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la consola: al arrancar instala el zip solo
   (con la imagen que enseña EmulationStation) y lo quita de `autoinstall`.
   Luego puedes cerrar PortMaster.

3. **Copia el juego.** Copia la carpeta `www/` del paso 1 en
   `ports/fearandhunger/gamedata/`, en la carpeta de ports de tu consola
   (donde está `Fear and Hunger.sh`). Tiene que quedar así:

   ```
   ports/
     Fear and Hunger.sh
     fearandhunger/
       gamedata/
         www/
           data/  img/  js/  audio/ ...
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Fear & Hunger desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego, tus
partidas y tus logros (`gamedata/www/save/`) se quedan.

Sin PortMaster: descomprime el zip en la carpeta de ports. Así puede que
EmulationStation no enseñe la imagen del port.

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), por ejemplo para la versión 0.4.0:

- `quixjs-0.4.0-android-arm64.apk`: la app
- `fearandhunger-0.4.0-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `fearandhunger-0.4.0-portmaster.zip`. Sale la carpeta
   `fearandhunger/`.
3. Copia la carpeta `www/` del juego (paso 1 de Instalar) en `fearandhunger/gamedata/`. Queda así:

   ```
   fearandhunger/
     port.toml
     gamedata/
       www/
   ```

4. Comprime la carpeta `fearandhunger/` en un `.zip` y pásalo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.

## Notas

Fear & Hunger es de Miro Haverinen. Este port solo ejecuta el juego, con
quixjs: https://github.com/lowlevel-1989/quixjs.

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | OK (aceptar, hablar) |
| B | Cancel (cancelar) |
| X | Skill (habilidad) |
| Y | Menu (menú) |
| L1 / R1 | Personaje o página anterior / siguiente, en los menús |
| (lo eliges en Controls) | Torch (antorcha) |
| (lo eliges en Controls) | Phase |
| Select + Start | Salir |

Vibración: viene activada con el mod `rumble`; Options > Vibration la apaga ([mods/rumble](mods/rumble/README.es.md)).

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=fearandhunger
```

Construye `fearandhunger.zip`.
