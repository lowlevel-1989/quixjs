# construct2

[English](README.md) | Español

Hooks para juegos hechos con Construct 2 (`c2runtime.js`). Cada uno comprueba
que la función es la de Construct 2 y, si no, la deja. Medidos con Gunfield
en una partida automática, A-B-B-A.

Los hooks estan en `hooks/`, en una carpeta por el trabajo que hacen
(`load`, `render`, `memory`, `logic`, `port`); los que no estan en
`hooks.toml` van en `hooks/extra/`, con las mismas carpetas. El nombre de un
hook es su ruta: `hooks/render/tint-native.js` es
`"<motor>/render/tint-native"` en `[hooks]`.

### hooks/extra/render/

No estan en `hooks.toml`: cada port los enciende en su propio `[hooks]`. Dibujar menos, o en C.

| hook | para | qué hace |
|------|------|----------|
| `extra/render/effects-direct` | `cr.layer.prototype.drawInstanceWithShadersGL` | un objeto cuyo plugin "pre-dibuja" (Spritefont2, NinePatch) con un solo efecto que solo cambia el color (Tint, Brightness...) se dibuja directo con el shader, no en una textura aparte (cada cambio de framebuffer son ~0,5 ms en Mali). RGB30: de 19,8 a 27,2 fps; RX6S: de 7,4 a 11,4 |
| `extra/render/pixellate-1` | `cr.layer.prototype.drawGL` | una capa cuyo unico efecto es Pixellate de tamano 1 o menos (opacidad 1, mezcla normal) se dibuja directa, como una capa sin efecto: el efecto deja la imagen igual, y se ahorran dos cambios de framebuffer y un quad de pantalla entera por capa (~0,3-0,5 ms cada uno en Mali); Gunfield lo pone en sus capas de balas |

### hooks/extra/memory/

No estan en `hooks.toml`: cada port los enciende en su propio `[hooks]`. Liberar memoria y el GC.

| hook | para | qué hace |
|------|------|----------|
| `extra/memory/clear-array` | `cr.truncateArray`, `cr.clearArray` | las listas de los eventos que ya están vacías no se tocan (el 99% de ~12800 llamadas por imagen), y las cortas se vacían con `pop()`, que el JIT compila. RGB30: de 28,1 a 31,1 fps; RX6S: de 11,4 a 12,5 |

### hooks/extra/logic/

No estan en `hooks.toml`: cada port los enciende en su propio `[hooks]`. Menos CPU en eventos, el interprete y las batallas.

| hook | para | qué hace |
|------|------|----------|
| `extra/logic/include-sol` | `cr.eventinclude.prototype.run` | una hoja de eventos incluida dentro de un grupo (sin condiciones por encima) limpia la selección solo de los tipos que puede elegir, no de los ~530 del juego (23 inclusiones por imagen en Gunfield), y de esos solo los que no tienen la selección limpia al entrar (casi nunca hay ninguno: de ~4300 a ~9 por imagen con 160 balas). RGB30: de 29,8 a 35,6 fps; RX6S: de 11,9 a 17,4 |

```
[hooks]
"cr.layer.prototype.drawInstanceWithShadersGL" = "construct2/extra/render/effects-direct"
"cr.truncateArray" = "construct2/extra/memory/clear-array"
"cr.clearArray"    = "construct2/extra/memory/clear-array"
"cr.eventinclude.prototype.run" = "construct2/extra/logic/include-sol"
"cr.layer.prototype.drawGL" = "construct2/extra/render/pixellate-1"
```

Test: `tests/games/core/construct2-hooks`.
