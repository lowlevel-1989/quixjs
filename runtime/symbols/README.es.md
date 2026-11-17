# symbols

[English](README.md) | Español

Nombres para el estado del juego, en `[symbols]` del toml del juego. Los
[logros](../achievements/README.es.md) los leen y los
[trucos](../cheats/README.es.md) los escriben.

| archivo     | de qué se encarga |
|-------------|-------------------|
| `symbols.c` | lee `[symbols]`, lee cada símbolo una vez por imagen (su valor de ahora y el de la imagen anterior) y los escribe |

```
[symbols]
boss1_dead = "path:Xq.a7.$f"    una propiedad desde window (a.b.c)
ending     = "var:88"           variable 88 de RPG Maker ($gameVariables)
lever      = "switch:12"        interruptor 12 de RPG Maker ($gameSwitches)
```

- Un símbolo que aún no existe (el juego no lo ha creado) falta.
- `var:N` y `switch:N` leen `$gameVariables._data[N]` y
  `$gameSwitches._data[N]` (0 o false si el juego no los ha tocado), y se
  escriben con `setValue`, como lo hace el juego.
- `path:` se escribe asignando la última propiedad del camino.

`ach` en `tools/console.py` (con `quixjs --debug`) enseña cada símbolo, su
valor y el de la imagen anterior.
