[English](README.md) | Español

## Instalar

Para usar este port necesitas el juego: Fear & Hunger 1.4.1 para Windows
(sin DRM), que puedes comprar en itch.io
(https://mirohaver.itch.io/fear-hunger).

1. **Consigue los archivos del juego.** Descarga la versión de Windows 1.4.1
   de itch.io y descomprímela en tu PC. Dentro hay `Game.exe`, varios
   archivos `.dll` y una carpeta `www/`. El juego (imágenes, sonidos, mapas,
   plugins) está todo en `www/`: es lo único que necesita el port. `Game.exe`
   y los `.dll` son el programa de Windows y no se usan.

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

## Notas

Fear & Hunger es de Miro Haverinen. Este port solo ejecuta el juego, con
quixjs (un runtime de JS en C con SDL2 y OpenGL ES 2 para juegos de RPG
Maker MV en portátiles: https://github.com/lowlevel-1989/quixjs).

`port.toml` describe todo el port (los scripts del juego, sus fuentes, su
tamaño, los hooks, los logros); el juego se lee de `gamedata/`. Lo que
cambias en el menú se guarda aparte (`quixjs-settings.toml`).

## Menú

| Opción | Qué hace |
|--|--|
| Play | Empieza el juego |
| Achievements | Los logros del port y cuáles tienes. Dentro del juego, el menú (B) también tiene Achievements |
| Transfer Save | Pasa tus partidas y logros a otro aparato de la misma red con quixjs (un PC u otra portátil): "Receive Save" en uno, "Send Save" en el otro. Los dos enseñan la misma firma de 4 caracteres para comprobarlo. Las partidas que había se guardan como copia ("Restore Backup"). |
| Controls | Asigna cada botón y palanca de tu mando |
| Mods | Activa o quita los mods copiados en `mods/` (por ejemplo una traducción). El port trae `no-widescreen`, que quita la pantalla ancha de un mod de pantalla ancha (ver su README en `mods/no-widescreen/`) |
| Options | Show FPS, Profiler, Reset achievements |
| Cheats | Activa o quita trucos (una partida con un truco no consigue logros) |
| Exit | Cierra el port |

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | Aceptar |
| B | Cancelar / menú |
| Select + Start | Salir |

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=fearandhunger
```

Construye `fearandhunger.zip` para aarch64 y armhf con clang, contra Debian
buster (glibc 2.28), y descarga lo que necesita la primera vez.
`port.json` y `gameinfo.xml` salen entonces de `port.toml`.
