[English](README.md) | Español

## Instalar

Para usar este port necesitas el juego: Felvidék para Windows, que puedes
comprar en GOG (https://www.gog.com/game/felvidek).

1. **Consigue los archivos del juego.** Instala Felvidék en tu PC desde GOG
   y abre su carpeta. El port solo necesita la carpeta `www/`.

2. **Instala el port.** Copia `felvidek.zip`, sin descomprimirlo, en la
   carpeta `autoinstall` de PortMaster de la SD de tu consola:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la consola: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Copia la carpeta `www/` del paso 1 en
   `ports/felvidek/gamedata/`, en la carpeta de ports de tu consola (donde
   está `Felvidek.sh`). Tiene que quedar así:

   ```
   ports/
     Felvidek.sh
     felvidek/
       gamedata/
         www/
           data/  img/  js/  audio/  movies/ ...
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Felvidék desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/www/save/`) se quedan.

Sin PortMaster: descomprime el zip en la carpeta de ports. Así puede que
EmulationStation no enseñe la imagen del port.

## Notas

Felvidék es de Jan Malík. Este port solo ejecuta el juego, con quixjs:
https://github.com/lowlevel-1989/quixjs.

Texto más grande: viene activado con el mod [big-text](mods/big-text/README.es.md).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | OK (aceptar, hablar) |
| B | Cancel (cancelar) |
| X | Dash (correr) |
| Y | Menu (menú) |
| L1 / R1 | Anterior / siguiente, en los menús |
| Select + Start | Salir |

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=felvidek
```

Construye `felvidek.zip`.
