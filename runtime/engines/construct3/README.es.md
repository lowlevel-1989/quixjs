# construct3

[English](README.md) | Español

Lo que quixjs necesita para los juegos hechos con Construct 3
(`c3runtime.js`), exportados para NW.js o para WebView2.

Los hooks estan en `hooks/`, en una carpeta por el trabajo que hacen
(`load`, `render`, `memory`, `logic`, `port`); los que no estan en
`hooks.toml` van en `hooks/extra/`, con las mismas carpetas. El nombre de un
hook es su ruta: `hooks/render/tint-native.js` es
`"<motor>/render/tint-native"` en `[hooks]`.

| archivo | ruta | qué hace |
|--|--|--|
| `entry.js` | (antes del juego) | `C3_IsSupported` y `C3_Is_Supported`: Construct 3 no arranca si su `supportcheck.js` no lo deja, y en quixjs solo falla por WebAssembly, que solo usan algunos plugins |

### hooks/render/

Dibujar menos, o en C.

| archivo | ruta | qué hace |
|--|--|--|
| `hooks/render/render-scale.js` | `RuntimeInterface.prototype._GetWindowInnerWidth` y `_GetWindowInnerHeight` | del motor: con `[game] render_scale`, la ventana que ve el juego es la del tamaño con que se ve, y Construct 3 dibuja a ese tamaño (720x405 en vez de 1440x810 en una pantalla de 720x720) |

### hooks/memory/

Liberar memoria y el GC.

| archivo | ruta | qué hace |
|--|--|--|
| `hooks/memory/keep-textures.js` | `C3.Layout.prototype._Unload` | del motor: las texturas de una pantalla se quedan al salir de ella (hasta 96 MB, se van las más antiguas), así volver no las sube otra vez a la GPU (~100 ms cada hoja de 2048x2048 en la RG CubeXX) |

### hooks/port/

Lo que quixjs añade al juego.

| archivo | ruta | qué hace |
|--|--|--|
| `hooks/port/playthrough-save.js`, `playthrough-load.js` | `C3.Runtime.prototype._SaveToJsonString`, `_DoLoadFromJsonString` | del motor: la partida para los trucos (`quix.playthrough`), en la clave `quix` del guardado de Construct 3 |
| `hooks/extra/port/savestate.js` | `C3.Runtime.prototype.Start` | no esta en el preset; un port que los ha probado pone `construct3/extra/port/savestate` en su `[hooks]`: los savestates (`quix.state.register`) con el guardado del propio Construct 3 (`_SaveToJsonString`; para cargar, `LoadFromJsonString`, al final del siguiente tick, como su acción Load); solo con una partida en marcha |

### hooks/extra/render/

No estan en `hooks.toml`: cada port los enciende en su propio `[hooks]`. Dibujar menos, o en C.

| archivo | ruta | qué hace |
|--|--|--|
| `hooks/extra/render/flat-2d.js` | `C3.Runtime.prototype.Uses3DFeatures` | no es del motor: el juego se dibuja como 2D, sin buffer de profundidad. Para un proyecto con el 3D activado que no usa 3D: en la Mali-G31 (Panfrost) ese buffer pierde lo que tenía tras una capa con textura propia y lo que va detrás no se dibuja |

`[engine] name = "construct3"` ejecuta `entry.js`, y `[hooks] engine =
"construct3"` pone los hooks de `hooks.toml` (los del motor). Un juego añade
`extra/render/flat-2d` en su `[hooks]`:

```
[hooks]
engine = "construct3"
"C3.Runtime.prototype.Uses3DFeatures" = "construct3/extra/render/flat-2d"
```
