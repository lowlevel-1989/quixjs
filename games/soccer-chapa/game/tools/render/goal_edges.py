# Limpia el borde de una porteria ya renderizada (goals.py lo llama tras
# cada render): lo casi transparente (el suelo que atrapa sombras deja un
# alfa minimo en toda la imagen) pasa a transparente del todo, y el alfa se
# apaga cerca del borde de la imagen (EDGE px): asi la sombra no acaba en
# un canto recto y no se ve el rectangulo de la imagen sobre el campo.
#   python3 goal_edges.py <png> [<png>...]
import sys
import numpy as np
from PIL import Image

LOW = 10          # alfa (0 a 255) por debajo del cual es transparente
EDGE = 60         # px del borde en los que el alfa se apaga

for path in sys.argv[1:]:
    im = Image.open(path).convert('RGBA')
    a = np.asarray(im, dtype=float).copy()
    h, w = a.shape[:2]
    y, x = np.mgrid[0:h, 0:w]
    d = np.minimum(np.minimum(x, w - 1 - x), np.minimum(y, h - 1 - y)) / EDGE
    fade = np.clip(d, 0, 1) ** 1.5
    alpha = np.where(a[..., 3] < LOW, 0, a[..., 3]) * fade
    a[..., 3] = alpha
    Image.fromarray(a.clip(0, 255).astype('uint8'), 'RGBA').save(path, optimize=True)
    print('borde limpio', path)
