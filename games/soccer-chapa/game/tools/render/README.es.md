# render

[English](https://github.com/lowlevel-1989/quixjs/blob/master/games/soccer-chapa/game/tools/render/README.md) | Español

Los scripts de Blender que hacen las imagenes del juego (`../../assets/`). No
son parte del juego: se vuelven a ejecutar para cambiar una textura. Las
texturas se generaron con asistencia de IA.

| archivo      | que hace |
|--------------|----------|
| `scene.py`   | la escena 3D (fieltro, tiza, porterias, chapas, balon, luces); renderiza el campo `pitch` y las imagenes de muestra |
| `passes.py`  | la chapa en blanco por capas (volumen, luz lateral, brillo, sombra) |
| `layers.py`  | pasa esos renders a las capas del juego: `mask.png`, `shade.png`, `gloss.png`, `shadow.png` |
| `ball.py`    | las bolas: la bola iluminada con su sombra (`assets/balls/<estilo>.png`) y su brillo (`<estilo>_gloss.png`) |
| `skins.py`   | la piel de cada bola (lo que se ve rodar), para `roll.py` |
| `roll.py`    | una bola rodando: 24 imagenes de su piel (`assets/balls/<estilo>_roll.png`) |
| `jerseys.py` | las franelas como imagenes, para las imagenes de muestra (el juego las dibuja por codigo) |

```
python3 jerseys.py tex
blender -b -P scene.py -- pitch tex ../../assets/pitch.png 128
blender -b -P passes.py -- $PWD/sprites 256
python3 layers.py sprites ../../assets
```

`scene.py` tambien renderiza `field`, `caps` y `cap` (las imagenes de
muestra). `pitch.png` entra en el juego como `pitch.jpg`.
