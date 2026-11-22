# rom

[English](README.md) | Español

La rom es el juego: un `.zip` (tambien un `.nw`, un `.exe` con el zip
dentro o el `.asar` de un juego de Electron) o una carpeta, junto al toml (`[game] rom`). quixjs la lee en su
sitio, sin descomprimirla. `[game] game_dir` es la carpeta de dentro de la rom
donde empieza el juego (`""` el principio).

La rom se monta en `gamedata/`, la carpeta de quixjs junto al toml. Cuando el
juego pide un archivo, quixjs lo busca en este orden:

1. los mods activos;
2. `gamedata/` en disco: lo que el juego escribio (partidas, `localStorage`)
   y lo que pone ahi el port;
3. la rom.

Lo que el juego escribe va siempre a `gamedata/`. Un zip comprimido se pasa
una vez a un zip sin compresion (`zip/convert.c`), con una pantalla de
progreso (`launcher/rom.c`); despues arranca enseguida.

| archivo     | de que se encarga |
|-------------|-------------------|
| `rom.h`     | la API de la rom (leer, abrir, datos, listar, mapear) y la interfaz de cada tipo |
| `rom.c`     | montarla y pasar cada ruta del juego a su tipo, sin saber cual es |
| `rom_zip.c` | la rom en un `.zip` (`zip/`): descomprimirla una vez si hace falta, archivos y carpetas de dentro |
| `rom_dir.c` | la rom en una carpeta (los tests, o un juego descomprimido en el PC) |

Solo la llama `core/url.c`: todo archivo que lee el juego pasa por `url.c`
(scripts, imagenes, sonidos, videos, fuentes, `fs` de nw.js).
