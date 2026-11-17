[English](README.md) | Español

# no-widescreen

Quita la pantalla ancha de un mod de pantalla ancha (como "Widescreen y
Español", Nexus 205), para que el juego quede a su tamaño original, 816x624.
En las pantallas cuadradas de muchas portátiles el juego se ve más grande.

## Orden

Tiene que tener un número más alto que el mod de pantalla ancha en la opción
Mods del menú del port. Activa primero el mod de pantalla ancha y después
este:

```
es205: On (1)
no-widescreen: On (2)
```

Si tiene el número más bajo, gana el mod de pantalla ancha y no cambia nada.
Sin un mod de pantalla ancha, este mod no hace nada.

## Cómo funciona

Los mods de pantalla ancha añaden dos plugins: `js/plugins/fullscreen.js`
(el juego a 1280x720) y `js/plugins/ShiftEnemies.js` (los enemigos movidos
para ese tamaño). Este mod trae dos archivos con los mismos nombres que no
hacen nada.
