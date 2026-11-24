# launcher

[English](README.md) | Español

Las pantallas propias de quixjs, las que no son del juego: el menú de
`--launcher` y las pantallas que abre (Transfer Save, logros). Vienen de
tamatama (https://github.com/lowlevel-1989/tamatama/tree/780b7e723f3dbe8a58bc54ed571f1518dac9191a): el menú de su opción
`--menu` (`src/shared/menu.c`) y sus pantallas (`src/shared/screen.c`).

El menú enseña el título del juego (`[game] title`, o `[engine] name`) y
debajo una firma pequeña, quixjs. Sus opciones: Play, Achievements (si hay
`[[achievements]]`), Transfer Save (si hay `[saves] files`), Controls (con
`[controls] gamepad = true`), Mods (si hay carpetas en `mods/`), Options,
Cheats (si hay `[[cheat]]`) y Exit. Con Play el juego arranca en la misma
ventana. Como en tamatama, Play, Achievements, Mods, Options y Cheats se
deslizan en su hueco (con flechas y una barra si no caben), y Transfer Save,
Controls y Exit quedan fijas debajo.

Mods enseña las carpetas de `mods/`, cada una On u Off. Las activas llevan
su número: si dos mods traen el mismo archivo, gana el número más alto. Lo
elegido vale al pulsar Play; hasta que se cambia, los mods activos son los
de `[game] mods`.

Options tiene Show FPS, Profiler, Vibration (si el juego vibra: `[controls] vibration`), Quality (si el port trae `[engine] quality`: los pixeles que dibuja un juego 3D, con `devicePixelRatio`; low dibuja la cuarta parte), Scaling (Sharp, Smooth o Pixel perfect: como se amplia el juego), Fullscreen (solo en el PC) y Reset
achievements. No hay opción de fps: el port fija `[engine] fps`. Lo que se
cambia en Options, Mods y Cheats se guarda enseguida en `quixjs-settings.toml`,
junto al config. Controls asigna todos los botones y palancas del mando y lo
guarda en `controller.map`, el formato de PortMaster (`backend/sdl/mapper.c`).
Pide siempre todos los botones, en el orden del mando estándar (Start, Select,
la cruceta, A, B, X, Y, L1, R1, L2, R2...), y las dos palancas: los combos de
quixjs (Select + Start, L1, R1, R2, X, Y, izquierda y derecha) los usan, y en
algunas consolas están en otro orden. Con `[controls.buttons]` un botón sale
con el nombre que le da el port ("Confirm (A)"). Las acciones
de `[controls.keys]` van al final, por su nombre, y se guardan en
`controller.keys`; con `[controls] screenshot = true` y `magnifier = true`, tambien
Screenshot y Magnifier (un botón suelto además de sus combos).

| archivo      | de qué se encarga |
|--------------|-------------------|
| `launcher.h` | lo que usa el resto de quixjs: el menú, Controls, abrir las pantallas y montar la rom |
| `common.h` | lo que comparten las partes del launcher: las funciones de cada pantalla |
| `launcher.c` | el menú: sus opciones, con los colores de `[launcher]`, y lo que se elige; después de Transfer Save vuelve a leer los logros |
| `screens.c` | las pantallas del menú: Achievements, Options (con Reset achievements), Cheats y Mods |
| `controls.c` | Controls: asignar el mando entrada a entrada y guardarlo en `controller.map` |
| `rom.c` | montar la rom (`[game] rom`) al empezar el juego; si el zip va comprimido, la pantalla "preparing the game" con el porcentaje mientras se descomprime una vez |
| `settings.c` | `quixjs-settings.toml`: `show_fps`, `profile`, `fullscreen`, `vibration`, `cheats` y `mods`, y lo que cambia elegir un truco o un mod (lo usa también el menú de Android, `platform/android/menu.c`) |
| `ui/`         | las pantallas propias (heredado de tamatama): `ui.c` las abre, las monta y las corre (`ui_run`), `draw.c` las dibuja (logo, título, líneas, la lista que se desliza, la leyenda, el aviso y la ayuda) e `input.c` lee lo que se pulsa (teclado, cruceta y stick del mando, `--script`); lo comparten en `common.h` |

## Cómo se dibuja

Una pantalla se dibuja en una superficie del tamaño del juego, con
coordenadas de una pantalla de 480 de alto, y se muestra con
`backend_present`: tiene las mismas barras negras y el mismo escalado que el
juego. El texto es SDL_ttf con la fuente por defecto (`QUIXJS_FONT`), y cada
texto se guarda hasta que deja de usarse, porque volver a dibujarlo en cada
imagen cuesta en una portátil.

## Teclas

| tecla / botón | acción |
|---------------|--------|
| flechas, cruceta, stick | moverse |
| Enter, Espacio, Z / A, X, Y, Start | elegir |
| Escape, Retroceso, tecla X / B | volver |
| Select + Start | salir |
| Select + R2 (en el juego) | volver a este menú |

`--script` también funciona aquí: sus frames cuentan desde el primero del
menú, y los del juego van detrás.

## Colores

```
[launcher]
background = "#101014"   # colores CSS
text       = "#e8e8e8"
accent     = "#2c4a7a"   # la barra de la opción elegida
```
