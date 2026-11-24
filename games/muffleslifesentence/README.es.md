[English](README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a BossyPino por crear Muffles' Life Sentence, y a Black Lantern
Collective por publicarlo. Este port solo ejecuta el juego,
con quixjs: https://github.com/lowlevel-1989/quixjs.

Necesita Muffles' Life Sentence para Windows (Steam).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse |
| A | OK (aceptar, hablar) |
| B | Cancel (cancelar) |
| X | Dash (correr) |
| Y | Menu (menú) |
| L1 / R1 | Anterior / siguiente, en los menús |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, Start la fija (el mando vuelve al juego) y la desfija, B la cierra |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make portmaster PORT=muffleslifesentence
```

Construye `build/muffleslifesentence.zip`.

## Instalar

Para usar este port necesitas el juego: Muffles' Life Sentence para
Windows, que puedes comprar en Steam
(https://store.steampowered.com/app/3145370/Muffles_Life_Sentence/).

1. **Consigue los archivos del juego.** Instala Muffles' Life Sentence en tu
   PC desde Steam y abre su carpeta (en Steam: botón derecho sobre el juego,
   Administrar, Ver archivos locales). El port solo necesita la carpeta `www/`.

2. **Instala el port.** Copia `muffleslifesentence.zip`, sin descomprimirlo, en la
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
   Windows: botón derecho sobre `www`, Enviar a, Carpeta comprimida) y cópialo en `ports/muffleslifesentence/`, junto a `port.toml`, en la
   carpeta de ports de tu consola (donde está `Muffles Life Sentence.sh`). Debe quedar así:

   ```
   ports/
     Muffles Life Sentence.sh
     muffleslifesentence/
       port.toml
       www.zip
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Mejor un zip sin compresión (con 7-Zip: Nivel de compresión, Almacenar; en
   Linux o macOS: `zip -0 -r`). Uno comprimido también vale: la primera vez
   que arranca el juego se prepara (un momento, solo una vez).

   Si tu archivo tiene otro nombre, escríbelo en `ports/muffleslifesentence/port.toml`, en
   `rom = "www.zip"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Muffles' Life Sentence desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/www/save/`) se quedan.

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
- `muffleslifesentence-VERSION-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. En tu PC, descomprime `muffleslifesentence-VERSION-portmaster.zip`. Sale la carpeta
   `muffleslifesentence/`.
3. Copia el juego (`www.zip`, paso 3 de Instalar) en `muffleslifesentence/`, junto a `port.toml`. Queda así:

   ```
   muffleslifesentence/
     port.toml
     www.zip
   ```

4. Comprime la carpeta `muffleslifesentence/` en un `.zip` y pásalo al teléfono.
5. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.
