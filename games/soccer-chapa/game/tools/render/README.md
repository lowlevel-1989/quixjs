# render

English | [Español](https://github.com/lowlevel-1989/quixjs/blob/master/games/soccer-chapa/game/tools/render/README.es.md)

The Blender scripts that make the images of the game (`../../assets/`). They
are not part of the game: run them again to change a texture. The textures
were made with AI assistance.

| file         | what it makes |
|--------------|---------------|
| `scene.py`   | the 3D scene (felt, chalk, goals, caps, ball, lights); renders the field `pitch` and the sample images |
| `passes.py`  | the white cap in separate layers (body, side light, gloss, shadow) |
| `layers.py`  | turns those renders into the layers of the game: `mask.png`, `shade.png`, `gloss.png`, `shadow.png` |
| `ball.py`    | the balls: the lit ball with its shadow (`assets/balls/<style>.png`) and its gloss (`<style>_gloss.png`) |
| `skins.py`   | the skin of each ball (what is seen rolling), for `roll.py` |
| `roll.py`    | a ball rolling: 24 frames of its skin (`assets/balls/<style>_roll.png`) |
| `jerseys.py` | the kits as images, for the sample images (the game draws them in code) |

```
python3 jerseys.py tex
blender -b -P scene.py -- pitch tex ../../assets/pitch.png 128
blender -b -P passes.py -- $PWD/sprites 256
python3 layers.py sprites ../../assets
```

`scene.py` also renders `field`, `caps` and `cap` (the sample images).
`pitch.png` goes into the game as `pitch.jpg`.
