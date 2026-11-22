[English](README.md) | Español

[![Buy Me A Coffee](https://raw.githubusercontent.com/lowlevel-1989/quixjs/master/docs/coffee.svg)](https://ko-fi.com/lowlevel1989)

Si quieres que [quixjs](https://github.com/lowlevel-1989/quixjs) siga recibiendo actualizaciones y
soporte, puedes apoyar el proyecto invitandome a un cafe.

## Notas

Gracias a Shiv por crear Gunfield. Este port solo ejecuta el juego, con
quixjs: https://github.com/lowlevel-1989/quixjs.

Necesita Gunfield **1.1.2.2** para Windows (Steam).

27 logros, los de Steam (opción Achievements del launcher del port).

Mods (activos): [rumble](mods/rumble/README.es.md) (vibración), [performance-mode](mods/performance-mode/README.es.md) (más rápido con muchos enemigos).

## Controles

| Botón | Acción |
|--|--|
| Cruceta / palanca izquierda | Moverse, elegir en los menús |
| A / X | Elegir |
| B | Volver |
| L1 / L2 | Pestaña anterior, en los menús |
| R1 / R2 | Pestaña siguiente, en los menús |
| Start | Pausa |
| Select + L1 | Lupa: un cuadro que amplía; la cruceta o las palancas lo mueven, R1 / L1 zoom, Y / X tamaño, A centra, B la cierra |
| Select + R1 | Captura de pantalla (en `screenshots/`) |
| Select + R2 | Volver al menú del port |
| Select + Start | Salir |

La nave dispara sola.

## Compilar

```
git clone https://github.com/lowlevel-1989/quixjs.git
cd quixjs
make PLATFORM=portmaster PORT=gunfield
```

Crea `gunfield.zip`.

## Instalar

Para usar este port necesitas el juego: Gunfield para Windows, que puedes
comprar en Steam (https://store.steampowered.com/app/2305900/).

1. **Consigue los archivos del juego.** Instala el juego con Steam y abre su
   carpeta (en la biblioteca, clic derecho en el juego, Administrar, Ver
   archivos locales). El juego es el archivo `package.nw` (es un zip): cópialo
   a tu PC tal cual, sin descomprimirlo.

2. **Instala el port.** Copia `gunfield.zip`, sin descomprimirlo, en la
   carpeta `autoinstall` de PortMaster de la SD de tu handheld:

   | Sistema | Carpeta `autoinstall` |
   |--|--|
   | ROCKNIX, AmberELEC, JELOS, uOS | `/roms/ports/PortMaster/autoinstall/` |
   | ArkOS | `/roms/tools/PortMaster/autoinstall/` |
   | muOS | `/mmc/MUOS/PortMaster/autoinstall/` |
   | Knulli | `/userdata/system/.local/share/PortMaster/autoinstall/` |

   Después abre PortMaster en la handheld: al arrancar instala el zip solo y
   lo quita de `autoinstall`. Luego puedes cerrar PortMaster.

3. **Copia el juego.** Copia el archivo `package.nw` del paso 1, tal cual (sin descomprimirlo), en `ports/gunfield/`, junto a `port.toml`, en la
   carpeta de ports de tu consola (donde está `Gunfield.sh`). Debe quedar así:

   ```
   ports/
     Gunfield.sh
     gunfield/
       port.toml
       package.nw
   ```

   En ROCKNIX la carpeta de ports es `/roms/ports/` (`/storage/roms/ports/`
   por SSH).

   Viene comprimido: la primera vez que arranca el juego se prepara (un
   momento, solo una vez).

   Si tu archivo tiene otro nombre, escríbelo en `ports/gunfield/port.toml`, en
   `rom = "package.nw"`.

4. **Juega.** Reinicia EmulationStation (o actualiza su lista de juegos) y
   abre Gunfield desde Ports.

Para actualizar el port, repite el paso 2 con el zip nuevo. El juego y tus
partidas (`gamedata/AppData/Local/Gunfield/SaveGame/`) se quedan.
