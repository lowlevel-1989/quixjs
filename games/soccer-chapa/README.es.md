[English](https://github.com/lowlevel-1989/quixjs/blob/master/games/soccer-chapa/README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Soccer Chapa es un juego de lowlevel.1989, hecho para quixjs: el juego, su
diseño, sus reglas y su estilo. Las texturas, los efectos y parte del
contenido se han hecho con ayuda de IA.

Fútbol de chapas por turnos: cada uno da un golpe y pasa el turno. Partido
rápido, Modo Leyenda, torneos de hasta 16 equipos entre handhelds de la misma
red, historial con repeticiones y práctica. Las chapas, franelas, bolas y
porterías salen en sobres de cartas.

El juego viene con el port: no hay que copiar nada. Empieza en inglés; el
español está en Options.

## Controles

| Botón | Acción |
|--|--|
| Izquierda / derecha | Apuntar |
| Palanca izquierda | Apuntar hacia donde la empujas |
| Arriba / abajo, L1 / R1 | Chapa anterior / siguiente |
| A | Golpear: la primera sube la fuerza, la segunda la fija y la tercera da el golpe (en el centro de la zona sale perfecto); con izquierda o derecha pulsada, con efecto |
| B (mantenido) | Apuntar despacio, para afinar |
| X | Avisar el tiro a gol (sin avisar, el gol no vale) |
| Y (mantenido) | La carta de la chapa |
| L2 / R2 (mantenido) | Ver el campo entero |
| Start | Pausa |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, Start la fija (el mando vuelve al juego) y la desfija, B la cierra |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

En la práctica: X coloca las chapas y el balón (L1 / R1 eligen, la cruceta o
la palanca mueven, X termina), L2 repite el último tiro y Start cambia la
jugada, el viento y el terreno.

En los menús: izquierda y derecha para elegir, arriba y abajo para cambiar
una opción, A para aceptar, B para volver.

## Instalar

1. **Instala el port.** Copia `soccer-chapa.zip`, sin descomprimirlo, en la
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

2. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Soccer Chapa desde Ports.

Para actualizar el port, repite el paso 1 con el zip nuevo. Tu partida
(`gamedata/save/`) se queda.

Sin PortMaster: descomprime el zip en la carpeta de ports. Así puede que
EmulationStation no enseñe la imagen del port.

## port.toml

**port.toml** le dice a quixjs dónde está el juego y dónde va la partida. Las
rutas son relativas a la carpeta que tiene port.toml.

```toml
rom      = "game"
game_dir = ""
```

- `rom`: la carpeta `game/`, que viene con el port.
- La partida (perfiles, cartas y opciones) va a `gamedata/`, junto a
  port.toml.

```
port.toml
game/
  entry.js
  src/
  assets/
  ...
gamedata/
```

## Android

Necesitas dos archivos de las releases de quixjs
(https://github.com/lowlevel-1989/quixjs/releases), la última versión
(`VERSION` en los nombres es su número):

- `quixjs-VERSION-android-arm64.apk`: la app
- `soccer-chapa-VERSION-portmaster.zip`: el port

1. Instala el `.apk` en el teléfono.
2. Copia `soccer-chapa-VERSION-portmaster.zip` al teléfono.
3. En quixjs, toca "Import game" y elige ese `.zip`. Después toca el juego
   y Play.
