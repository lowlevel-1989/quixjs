# launcher

[English](README.md) | Español

Las pantallas propias de quixjs, las que no son del juego: el menú de
`--launcher` y las pantallas que abre (Transfer Save, logros). Vienen de
tamatama (https://github.com/lowlevel-1989/tamatama): el menú de su opción
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

Options tiene Show FPS, Profiler, Vibration (si el juego vibra: `[controls] vibration`), Fullscreen (solo en el PC) y Reset
achievements. No hay opción de fps: el port fija `[engine] fps`. Lo que se
cambia en Options, Mods y Cheats se guarda enseguida en `quixjs-settings.toml`,
junto al config. Controls asigna todos los botones y palancas del mando y lo
guarda en `controller.map`, el formato de PortMaster (`backend/sdl/mapper.c`).
Con `[controls.buttons]` pide Start, esos botones por el nombre que les da el
port ("Confirm (A)") y siempre las dos palancas; los que no pide siguen como
estaban. Select se pide siempre (Select + Start cierra el juego). Las acciones
de `[controls.keys]` van al final, por su nombre, y se guardan en
`controller.keys`.

| archivo      | de qué se encarga |
|--------------|-------------------|
| `launcher.c` | el menú: sus opciones, las pantallas de logros, opciones y trucos, y lo que se elige; después de Transfer Save vuelve a leer los logros |
| `settings.c` | `quixjs-settings.toml`: `show_fps`, `profile`, `fullscreen`, `vibration`, `cheats` y `mods`, y lo que cambia elegir un truco o un mod (lo usa también el menú de Android, `platform/android/launcher.c`) |
| `ui.c`       | dibuja una pantalla (logo, título, líneas, una lista que se desliza si no cabe, un aviso y la ayuda) y espera a que se elija: teclado, la cruceta y el stick del mando, y `--script` |

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

`--script` también funciona aquí: sus frames cuentan desde el primero del
menú, y los del juego van detrás.

## Colores

```
[launcher]
background = "#101014"   # colores CSS
text       = "#e8e8e8"
accent     = "#2c4a7a"   # la barra de la opción elegida
```
