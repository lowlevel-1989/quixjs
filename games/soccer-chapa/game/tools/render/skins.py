# Las pieles de las bolas: lo que se ve rodar de cada estilo, en un mapa 2:1
# (1024x512) con alfa (lo transparente deja ver la bola). roll.py las pone
# en una esfera y renderiza el giro.
#   python3 skins.py <carpeta>
import math
import os
import random
import sys
from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1]
W, H = 1024, 512
os.makedirs(OUT, exist_ok=True)


def wave(d, y, amp, color, width, phase=0.0):
    pts = [(x, y + math.sin(x / W * 2 * math.pi + phase) * amp) for x in range(0, W + 1, 4)]
    d.line(pts, fill=color, width=width)


def stars(d, color, n, size, seed):
    random.seed(seed)
    for _ in range(n):
        cx, cy = random.randrange(W), random.randrange(int(H * 0.2), int(H * 0.8))
        pts = []
        for k in range(10):
            a = -math.pi / 2 + k * math.pi / 5
            r = size if k % 2 == 0 else size * 0.45
            pts.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
        d.polygon(pts, fill=color)


def marble():
    # vetas: lineas onduladas que se cruzan, suavizadas
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    random.seed(3)
    for _ in range(9):
        y = random.randrange(H)
        amp = random.uniform(30, 120)
        ph = random.uniform(0, 6.28)
        c = random.choice([(30, 90, 190, 230), (60, 130, 220, 200), (20, 50, 140, 230)])
        wave(d, y, amp, c, random.randint(10, 26), ph)
    return im.filter(ImageFilter.GaussianBlur(5))


def soccer():
    # el balon: 12 pentagonos negros (en los vertices de un icosaedro) y 20
    # hexagonos blancos (en sus caras), con costuras finas entre paneles.
    # Los paneles van girados un angulo cualquiera: desde arriba no se ve
    # simetrico (como cae un balon de verdad)
    g = (1 + 5 ** 0.5) / 2
    pent = [(0, a, b * g) for a in (-1, 1) for b in (-1, 1)]
    pent += [(a, b * g, 0) for a in (-1, 1) for b in (-1, 1)]
    pent += [(b * g, 0, a) for a in (-1, 1) for b in (-1, 1)]
    # los hexagonos: en el centro de cada cara del icosaedro (tres vertices
    # vecinos entre si, a distancia 2)
    near = lambda u, v: abs(sum((u[k] - v[k]) ** 2 for k in range(3)) - 4) < 1e-6
    hexa = []
    for i in range(12):
        for j in range(i + 1, 12):
            for k in range(j + 1, 12):
                a, b, c = pent[i], pent[j], pent[k]
                if near(a, b) and near(b, c) and near(a, c):
                    hexa.append(tuple((a[n] + b[n] + c[n]) / 3 for n in range(3)))

    def unit(v):
        n = math.sqrt(sum(c * c for c in v))
        return tuple(c / n for c in v)

    def turn(v, ax, ay, az):
        x, y, z = v
        y, z = y * math.cos(ax) - z * math.sin(ax), y * math.sin(ax) + z * math.cos(ax)
        x, z = x * math.cos(ay) + z * math.sin(ay), -x * math.sin(ay) + z * math.cos(ay)
        x, y = x * math.cos(az) - y * math.sin(az), x * math.sin(az) + y * math.cos(az)
        return (x, y, z)

    rot = (0.61, 0.37, 0.23)
    AP, AH = 0.688, 0.866               # las apotemas del pentagono y del hexagono
    SEAM = 0.022                        # el ancho de las costuras
    centers = [(unit(turn(unit(v), *rot)), True) for v in pent]
    centers += [(unit(turn(unit(v), *rot)), False) for v in hexa]
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    px = im.load()
    for y in range(H):
        lat = math.pi / 2 - (y + 0.5) / H * math.pi
        cl, sl = math.cos(lat), math.sin(lat)
        for x in range(W):
            lon = (x + 0.5) / W * 2 * math.pi
            p = (cl * math.cos(lon), cl * math.sin(lon), sl)
            # la distancia a cada centro, medida en apotemas de su panel: el
            # borde entre dos paneles esta donde valen lo mismo (lados rectos)
            dp = max(p[0] * c[0] + p[1] * c[1] + p[2] * c[2] for c, is_p in centers if is_p)
            hs = sorted((p[0] * c[0] + p[1] * c[1] + p[2] * c[2] for c, is_p in centers if not is_p), reverse=True)
            np_ = math.acos(min(1, dp)) / AP
            nh = [math.acos(min(1, h)) / AH for h in hs[:2]]
            if abs(np_ - nh[0]) < SEAM:
                px[x, y] = (60, 62, 66, 235)    # costura del pentagono
            elif np_ < nh[0]:
                px[x, y] = (16, 17, 20, 255)    # pentagono negro
            elif nh[1] - nh[0] < SEAM:
                px[x, y] = (120, 122, 126, 210)  # costura entre hexagonos
    return im.filter(ImageFilter.GaussianBlur(0.7))


def build(style):
    im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    if style == 'steel':        # grabados: dos estrellas pequenas y motas finas (sin lineas)
        stars(d, (40, 46, 52, 170), 2, 30, 1)
        random.seed(4)
        for _ in range(60):
            x, y = random.randrange(W), random.randrange(int(H * 0.1), int(H * 0.9))
            d.ellipse([x - 4, y - 4, x + 4, y + 4], fill=(40, 46, 52, 110))
    elif style == 'gold':       # estrellas grabadas, mas oscuras
        stars(d, (120, 70, 10, 200), 7, 34, 2)
    elif style == 'onyx':       # una franja dorada que da la vuelta
        wave(d, H / 2, H * 0.18, (224, 176, 70, 255), 16)
    elif style == 'pearl':      # un remolino nacarado, muy suave
        for i in range(5):
            wave(d, H * (0.25 + i * 0.12), 50, (200, 190, 230, 90), 20, i * 1.3)
        im = im.filter(ImageFilter.GaussianBlur(8))
    elif style == 'marble':
        im = marble()
    elif style == 'soccer':
        im = soccer()
    elif style == 'glass':      # las aspas de color de una canica (ojo de gato), dentro
        for i, c in enumerate([(230, 40, 60, 235), (255, 196, 40, 235), (40, 120, 230, 235)]):
            ph = i * 2 * math.pi / 3
            pts = [(x, H / 2 + math.sin(x / W * 4 * math.pi + ph) * H * 0.3) for x in range(0, W + 1, 4)]
            d.line(pts, fill=c, width=34)
        im = im.filter(ImageFilter.GaussianBlur(3))
    return im


for s in ('steel', 'gold', 'onyx', 'pearl', 'marble', 'glass', 'soccer'):
    build(s).save(os.path.join(OUT, s + '.png'))
print('skins ok')
