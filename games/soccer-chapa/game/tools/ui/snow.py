# La nieve del campo (games/soccer-chapa/game/src/world/weather.js: snow):
#   assets/snow.png    la capa de nieve encima del campo, a la mitad de su
#                      tamaño (el juego la estira). Es un relieve: un mapa de
#                      alturas (ruido de varias escalas, del tamaño de las
#                      chapas o poco mas) con una luz muy suave desde arriba
#                      a la izquierda: una capa fina sobre el fieltro. Solo
#                      en los bordes (el juego calienta el centro): toda
#                      alrededor del campo, con un borde que serpentea
#                      cerca de las lineas y esquinas redondas; la ventisca
#                      la amontona en una esquina (entra mas y deja montones
#                      sueltos) y en la contraria casi no llega: nada
#                      simetrico; las porterias, siempre limpias. El juego
#                      la voltea al azar en cada partido. Mas
#                      acumulada fuera de las lineas y en las esquinas, y
#                      algun cristal que brilla
#   assets/ui/flake.png  un copo: un punto blanco y suave
#   python3 snow.py <carpeta games/soccer-chapa/game/assets>
import os
import sys
import numpy as np
from PIL import Image, ImageFilter

OUT = sys.argv[1]
W, H = 1190, 680                     # la mitad de games/soccer-chapa/game/assets/pitch.jpg
rng = np.random.default_rng(11)


def noise(sx, sy):
    # ruido suave: una rejilla al azar de celdas de sx x sy px, estirada y
    # emborronada (sx > sy: alargado en horizontal, como lo deja el viento)
    n = rng.random((H // sy + 2, W // sx + 2))
    im = Image.fromarray((n * 255).astype('uint8')).resize((W, H), Image.BICUBIC)
    return np.asarray(im.filter(ImageFilter.GaussianBlur(sy / 3)), dtype=float) / 255


def smooth(a, b, v):
    t = np.clip((v - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


# el campo en la imagen (games/soccer-chapa/game/src/world/pitch.js: 14
# unidades de ancho; el campo mide 7 x 4.6, en el centro)
k = W / 14.0
fx0, fx1, fy0, fy1 = W / 2 - 3.5 * k, W / 2 + 3.5 * k, H / 2 - 2.3 * k, H / 2 + 2.3 * k
y, x = np.mgrid[0:H, 0:W]
out = np.maximum(np.maximum(fx0 - x, x - fx1), np.maximum(fy0 - y, y - fy1)) / k   # > 0 fuera del campo (unidades)
corner = np.clip(1.4 - np.minimum(np.hypot(x - fx0, y - fy0), np.minimum(np.hypot(x - fx1, y - fy0),
         np.minimum(np.hypot(x - fx0, y - fy1), np.hypot(x - fx1, y - fy1)))) / k, 0, 1)

# la altura de la nieve: manchas del tamaño de las chapas o poco mas (si son
# mucho mas grandes, las chapas parecen flotar sobre nubes) y el detalle fino
hgt = (noise(70, 40) * 0.4 + noise(26, 16) * 0.3 + noise(10, 7) * 0.2 + noise(4, 3) * 0.1)
# solo en los bordes: el juego calienta el centro. Fuera de las lineas, toda;
# dentro, se acaba a unos 0.35 unidades de la linea (algo mas en las esquinas)
# (lo que entra cambia a lo largo del borde, de 0.1 a 0.6 unidades)
bump = hgt
# la distancia a un rectangulo de esquinas muy redondas (R unidades): asi el
# borde de la nieve no sigue las lineas en angulo recto
R_ = 1.0
qx = np.abs(x - W / 2) / k - (3.5 - R_); qy = np.abs(y - H / 2) / k - (2.3 - R_)
rnd = np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - R_   # > 0 fuera
# el borde serpentea alrededor de esa linea: en unos tramos la nieve entra
# un poco (hasta unas 0.3 unidades) y en otros el fieltro sale fuera
# la ventisca: el viento la amontona arriba a la izquierda (entra hasta ~1
# unidad y deja montones sueltos dentro) y abajo a la derecha casi no llega
# a la linea: nada simetrico
gx_, gy_ = (x - W / 2) / (3.5 * k), (y - H / 2) / (2.3 * k)                         # -1 a 1 en el campo
drift = np.clip(-(gx_ * 0.6 + gy_ * 0.5), -1, 1)                                   # 1 arriba a la izquierda
wander = (noise(240, 170) - 0.5) * 0.7 + (noise(70, 50) - 0.5) * 0.22 - 0.15 + drift * 0.75
clumps = smooth(0.6, 0.66, noise(45, 35)) * smooth(0.2, 0.7, drift) * smooth(-1.8, -0.5, rnd)   # montones sueltos
near = np.maximum(np.clip(1 + (rnd + wander) / 0.12, 0, 1.6), clumps)
# las porterias siempre limpias: su boca, el area pequeña y un poco por
# detras de la red (games/soccer-chapa/game/src/world/pitch.js: goalW 1, goalD 0.32)
ax, ay = np.abs(x - W / 2) / k, np.abs(y - H / 2) / k
gd = np.hypot((ax - 3.5) / 0.95, ay / 1.35) + (noise(30, 24) - 0.5) * 0.35        # un hueco redondo e irregular
clear = smooth(1.1, 0.85, gd)
near = near * (1 - clear)
hgt = bump * 0.5 + near * 0.5
edge = hgt + (noise(3, 3) - 0.5) * 0.06                                           # el borde: irregular y nitido
cover = smooth(0.47, 0.51, edge)                # donde hay nieve (y un velo fino alrededor)
depth = smooth(0.52, 0.75, hgt)                                                    # y donde es gruesa

# la luz: la pendiente del relieve contra una luz de arriba a la izquierda
gy, gx = np.gradient(bump)                                                          # solo del relieve, no del borde
light = np.clip(0.5 - (gx + gy) * 40, 0, 1)                                        # 0 sombra, 1 luz
shadow = np.array([214, 225, 242], float); base = np.array([238, 243, 250], float)
hi = np.array([255, 255, 255], float)
col = np.where(light[..., None] < 0.5,
               shadow + (base - shadow) * (light[..., None] * 2),
               base + (hi - base) * ((light[..., None] - 0.5) * 2))
frost = (0.1 + noise(40, 30) * 0.06) * smooth(0.3, 0.9, near)                        # la escarcha: solo junto a la nieve
alpha = np.maximum(frost, cover * (0.72 + 0.25 * depth))                            # las manchas: blancas
sparkle = (rng.random((H, W)) > 0.9994) & (cover > 0.5)                             # algun cristal que brilla
col[sparkle] = 255; alpha = np.maximum(alpha, sparkle * 0.95)
img = np.dstack([col, alpha[..., None] * 255]).clip(0, 255).astype('uint8')
Image.fromarray(img, 'RGBA').save(os.path.join(OUT, 'snow.png'), optimize=True)

S = 24                                                                            # el copo
yy, xx = np.mgrid[0:S, 0:S]
d = np.hypot(xx - S / 2 + 0.5, yy - S / 2 + 0.5) / (S / 2)
a = np.clip(1 - d, 0, 1) ** 1.6
flake = np.dstack([np.full((S, S), 255), np.full((S, S), 255), np.full((S, S), 255), a * 255]).astype('uint8')
Image.fromarray(flake, 'RGBA').save(os.path.join(OUT, 'ui', 'flake.png'))
print('nieve ok')
