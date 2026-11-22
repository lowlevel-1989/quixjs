[English](README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a Locomalito y Gryzor87 por crear L'Abbaye des Morts y dejarlo
gratis, y a Lucera Project por su versión HTML5. Este port solo ejecuta el
juego, con quixjs: https://github.com/lowlevel-1989/quixjs.

Necesita la versión HTML5 en el commit **21e1fc4** (la versión enlazada en
Instalar).

12 logros (opción Achievements del launcher del port).

Mods (activos): [achievements](mods/achievements/README.es.md) (la pausa muestra los logros), [rumble](mods/rumble/README.es.md) (vibra al morir).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse, subir y bajar |
| A | Saltar; empezar, en el título |
| Select | Información, en el título |
| Start | Pausa: muestra los logros (mod `achievements`); Start o B siguen |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, B la cierra |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=abbaye
```

Genera `abbaye.zip`.

## Instalar

Para usar este port necesitas L'Abbaye des Morts (versión HTML5), que es gratis.

1. **Descarga el juego.** L'Abbaye des Morts (HTML5 version) es un juego gratuito de Lucera Project, en GitHub.
   Abre su repositorio en la versión que usa este port:
   https://github.com/luceraproject/abbaye-des-morts/tree/21e1fc42694a2c69078c6451ae8b6f8fb22dff99

   y descárgalo con el botón verde **Code** y después **Download ZIP**. Sale
   `abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip`: no lo descomprimas, lo copias tal cual en el paso 3.

2. **Instala el port.** Copia `abbaye.zip`, sin descomprimirlo, en la carpeta
   `autoinstall` de PortMaster de la SD de tu handheld:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en el handheld: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Copia el zip del paso 1, tal cual (sin descomprimirlo), en `ports/abbaye/`, junto a `port.toml`, en la
   carpeta de ports de tu consola (donde está `Abbaye.sh`). Debe quedar así:

   ```
   ports/
     Abbaye.sh
     abbaye/
       port.toml
       abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Viene comprimido: la primera vez que arranca el juego se prepara (un
   momento, solo una vez).

   Si tu archivo tiene otro nombre, escríbelo en `ports/abbaye/port.toml`, en
   `rom = "abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre L'Abbaye des Morts desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego se
queda.

Sin PortMaster: descomprime el zip en la carpeta de ports. Entonces
EmulationStation puede no mostrar la imagen del port.

## port.toml

**port.toml** le dice a quixjs dónde está el juego y dónde van las partidas. Las rutas son relativas a la carpeta que tiene port.toml.

```toml
rom      = "abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip"
game_dir = "abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99"
```

- `rom`: el zip descargado de GitHub (Code, Download ZIP), puesto tal cual junto a port.toml.
- `game_dir`: la carpeta dentro del zip donde empieza el juego: GitHub lo mete todo en una carpeta con el nombre del repositorio y del commit.
- Las partidas van a `gamedata/`, junto a port.toml, así `abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip` nunca se modifica.

```
port.toml
abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip
  abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99/
    js/
    ...
gamedata/
```

## Android

Necesitas el juego y dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), la última versión
(`VERSION` en los nombres es su número):

- `quixjs-VERSION-android-arm64.apk`: la app
- `abbaye-VERSION-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `abbaye-VERSION-portmaster.zip`. Te queda la carpeta
   `abbaye/`.
3. Copia el juego (`abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip`, paso 3 de Instalar) en `abbaye/`, junto a `port.toml`. Queda así:

   ```
   abbaye/
     port.toml
     abbaye-des-morts-21e1fc42694a2c69078c6451ae8b6f8fb22dff99.zip
   ```

4. Comprime la carpeta `abbaye/` en un `.zip` y cópialo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.
