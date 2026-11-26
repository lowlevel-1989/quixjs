# El relieve de las dos caras de la moneda del sorteo: mapas de alturas en
# gris (blanco, lo mas alto) que usa games/soccer-chapa/game/tools/render/coin.py para la moneda 3D.
# Cada uno: el borde estriado, un aro levantado, el fondo hundido y el motivo.
#   heads.png  cara: una chapa (la corona con sus dientes y la franela)
#   tails.png  cruz: un balon y una corona de laurel
#   python3 coin.py <carpeta>
import math
import os
import sys
from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1]
N = 512
os.makedirs(OUT, exist_ok=True)
C = N / 2


def base():
    # el disco: el canto estriado, un aro levantado y el fondo hundido
    h = Image.new('L', (N, N), 0)
    d = ImageDraw.Draw(h)
    d.ellipse([4, 4, N - 4, N - 4], fill=150)
    for k in range(120):                                       # las estrias del canto
        a = k / 120 * 2 * math.pi
        x0, y0 = C + math.cos(a) * (C - 22), C + math.sin(a) * (C - 22)
        x1, y1 = C + math.cos(a) * (C - 5), C + math.sin(a) * (C - 5)
        d.line([(x0, y0), (x1, y1)], fill=120, width=3)
    d.ellipse([28, 28, N - 28, N - 28], fill=210)               # el aro
    d.ellipse([44, 44, N - 44, N - 44], fill=110)               # el fondo
    return h, d


def cap(d, r):
    pts = []
    for i in range(42):
        a = i * math.pi / 21
        rr = r if i % 2 == 0 else r * 0.86
        pts.append((C + math.cos(a) * rr, C + math.sin(a) * rr))
    d.polygon(pts, fill=220)
    d.ellipse([C - r * 0.7, C - r * 0.7, C + r * 0.7, C + r * 0.7], fill=180)
    for k in range(-3, 4, 2):                                   # las rayas de la franela
        d.rectangle([C + k * r * 0.14 - r * 0.06, C - r * 0.6, C + k * r * 0.14 + r * 0.06, C + r * 0.6],
                    fill=205)


def ball(d, r):
    d.ellipse([C - r, C - r, C + r, C + r], fill=215)
    for k in range(5):                                          # los pentagonos del balon
        a = -math.pi / 2 + k * 2 * math.pi / 5
        px, py = C + math.cos(a) * r * 0.62, C + math.sin(a) * r * 0.62
        pts = [(px + math.cos(a + j * 2 * math.pi / 5) * r * 0.22, py + math.sin(a + j * 2 * math.pi / 5) * r * 0.22)
               for j in range(5)]
        d.polygon(pts, fill=165)
    pts = [(C + math.cos(-math.pi / 2 + j * 2 * math.pi / 5) * r * 0.26, C + math.sin(-math.pi / 2 + j * 2 * math.pi / 5) * r * 0.26)
           for j in range(5)]
    d.polygon(pts, fill=165)


def laurel(d):
    for s in (-1, 1):                                           # las hojas, a cada lado, abajo
        for k in range(9):
            a = math.pi / 2 + s * (0.35 + k * 0.17)
            x, y = C + math.cos(a) * 178, C + math.sin(a) * 178
            d.ellipse([x - 13, y - 7, x + 13, y + 7], fill=200)


h, d = base(); cap(d, 150)
h.filter(ImageFilter.GaussianBlur(1.5)).save(os.path.join(OUT, 'heads.png'))
h, d = base(); ball(d, 110); laurel(d)
h.filter(ImageFilter.GaussianBlur(1.5)).save(os.path.join(OUT, 'tails.png'))
print('relieve ok')
