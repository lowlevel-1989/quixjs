[English](README.md) | Español

## Instalar

Para usar este port necesitas HexGL, que es gratis.

1. **Descarga el juego.** Baja este archivo en tu PC:
   https://github.com/BKcore/HexGL/archive/refs/heads/master.zip

   Descomprímelo. Te queda la carpeta `HexGL-master/`, con estos archivos
   dentro (son los que copias en el paso 3):

   ```
   HexGL-master/
     audio/  bkcore/  css/  libs/  textures/ ...
   ```

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

3. **Copia el juego.** Copia lo que hay dentro de `HexGL-master/` (no la
   carpeta en sí) en `ports/hexgl/gamedata/`, en la carpeta de ports de tu
   handheld (donde está `HexGL.sh`). Tiene que quedar así:

   ```
   ports/
     HexGL.sh
     hexgl/
       gamedata/
         audio/  bkcore/  css/  libs/  textures/ ...
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre HexGL desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego se
queda.

Sin PortMaster: descomprime el zip en la carpeta de ports. Entonces
EmulationStation puede no mostrar la imagen del port.

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), por ejemplo para la versión 0.4.0:

- `quixjs-0.4.0-android-arm64.apk`: la app
- `hexgl-0.4.0-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `hexgl-0.4.0-portmaster.zip`. Te queda la carpeta
   `hexgl/`.
3. Copia lo que hay dentro de `HexGL-master/` (el paso 1 de Instalar) en
   `hexgl/gamedata/`. Queda así:

   ```
   hexgl/
     port.toml  menu.js  mods/ ...
     gamedata/
       audio/  bkcore/  css/  libs/  textures/ ...
   ```

4. Comprime la carpeta `hexgl/` en un `.zip` y cópialo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.

## Notas

HexGL es de Thibaut Despoulain (BKcore). Este port solo ejecuta el juego, en
quixjs: https://github.com/lowlevel-1989/quixjs.

Mods (opción Mods del launcher del port, los dos activos):
[multiplayer](mods/multiplayer/README.es.md) (carreras entre handhelds) y
[volume](mods/volume/README.es.md) (Music y Effects en el menú). Opción
Cheats: No damage (los choques no quitan escudo).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Girar |
| A | Acelerar |
| L2 / R2 | Frenos de aire |
| Select | Empezar la carrera otra vez |
| Start | Pausa: seguir, reiniciar o volver al menú |
| Select + Start | Salir |

En el menú: la cruceta para elegir, izquierda y derecha para cambiar una
opción, A para aceptar, B para volver.

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=hexgl
```

Genera `hexgl.zip`.
