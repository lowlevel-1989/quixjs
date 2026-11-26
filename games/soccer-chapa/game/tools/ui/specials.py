# Las 5 cartas miticas (cualquier posicion, muy dificiles de sacar):
# games/soccer-chapa/game/assets/ui/card_mythic_<n>.png, a 2x: 320 x 448, con PAD px transparentes.
# Donde va la chapa y los textos: games/soccer-chapa/game/src/ui/card/layout.js (mythic0..4).
#   0  tabaco 1910   sepia y papel viejo (los cromos T206)
#   1  retrato       a sangre, con el nombre gigante en vertical
#   2  autografo     holografica clara con una firma en tinta
#   3  ficha         horizontal, con la tabla de stats del dorso
#   4  topps 1971    borde negro, la foto y la banda negra abajo
#   python3 specials.py <carpeta>
import math
import os
import random
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from materials import W, H, S, mix, rainbow
from materials import vgrad as _vgrad


def vgrad(top, bottom):
    return _vgrad(top, bottom).convert('RGBA')

OUT = sys.argv[1]
PAD = 4
FONT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'fonts', 'Montserrat-Black.otf')


def P(pts):
    return [(x * S, y * S) for x, y in pts]


def font(size):
    return ImageFont.truetype(FONT, size * S)


def dots(im, box, color):
    d = ImageDraw.Draw(im, 'RGBA')
    for y in range(box[1], box[3], 9):
        for x in range(box[0] + (4 if (y // 9) % 2 else 0), box[2], 9):
            d.ellipse(P([(x, y), (x + 3, y + 3)]), fill=color)


def tobacco():
    random.seed(1)
    im = vgrad((236, 222, 190), (214, 196, 160))
    d = ImageDraw.Draw(im, 'RGBA')
    stains = Image.new('RGBA', im.size, (0, 0, 0, 0))        # manchas del papel viejo
    sd = ImageDraw.Draw(stains)
    for _ in range(14):
        x, y, r = random.uniform(0, W), random.uniform(0, H), random.uniform(10, 40)
        sd.ellipse(P([(x - r, y - r), (x + r, y + r)]), fill=(150, 110, 60, 22))
    im.alpha_composite(stains.filter(ImageFilter.GaussianBlur(8 * S)))
    d.rectangle(P([(14, 14), (W - 14, H - 14)]), outline=(110, 80, 46, 255), width=2 * S)
    d.rectangle(P([(20, 20), (W - 20, H - 20)]), outline=(110, 80, 46, 255), width=S)
    d.rectangle(P([(40, 70), (W - 40, 318)]), outline=(110, 80, 46, 255), width=S)   # el filete de la foto
    d.rectangle(P([(30, 346), (W - 30, 392)]), fill=(222, 204, 166, 255), outline=(110, 80, 46, 255), width=S)
    return im


def rookie():
    # retrato a sangre: un fondo de color intenso con rayas finas, sin
    # ventana; la chapa grande arriba y a la izquierda el nombre gigante en
    # vertical (lo escribe el juego), con una franja oscura detras
    im = _vgrad((24, 120, 200), (12, 60, 140))       # en RGB: las rayas se mezclan
    d = ImageDraw.Draw(im, 'RGBA')
    for k in range(-20, 40):
        x = k * 14
        d.line(P([(x, 0), (x - 120, H)]), fill=(255, 255, 255, 16), width=2 * S)
    d.rectangle(P([(0, 0), (70, H)]), fill=(8, 22, 54, 230))
    d.line(P([(70, 0), (70, H)]), fill=(250, 210, 70, 255), width=3 * S)
    return im.convert('RGBA')

def autograph():
    im = vgrad((246, 244, 250), (226, 230, 244))
    holo = Image.new('RGBA', im.size, (0, 0, 0, 0))
    hd = ImageDraw.Draw(holo)
    for x in range(-H, W + H, 3):
        hd.line(P([(x, H), (x + H * 0.7, 0)]), fill=rainbow(x / 160.0) + (40,), width=2 * S)
    im.alpha_composite(holo)
    d = ImageDraw.Draw(im, 'RGBA')
    d.ellipse(P([(55, 65), (265, 275)]), fill=(40, 46, 70, 255), outline=(255, 255, 255, 255), width=4 * S)
    random.seed(7)                                             # una firma: una curva de tinta azul
    pts, x, y = [], 50.0, 318.0
    for k in range(120):
        x += 1.9
        y = 318 + math.sin(k * 0.33) * 14 + math.sin(k * 0.11) * 8
        pts.append((x, y))
    d.line(P(pts), fill=(30, 50, 150, 255), width=3 * S, joint='curve')
    d.line(P([(70, 342), (250, 334)]), fill=(30, 50, 150, 255), width=2 * S)
    return im

def plate():
    # ficha horizontal: la carta se lee girada. La chapa a la izquierda y a
    # la derecha una tabla de stats con su cabecera, como el dorso de un cromo
    im = vgrad((242, 234, 212), (230, 218, 190))
    d = ImageDraw.Draw(im, 'RGBA')
    d.rectangle(P([(0, 0), (W, 150)]), fill=(26, 90, 60, 255))
    d.rectangle(P([(0, 150), (W, 158)]), fill=(240, 200, 80, 255))
    d.rectangle(P([(20, 190), (W - 20, 236)]), fill=(26, 90, 60, 255))
    for k in range(4):
        y = 236 + k * 44
        d.rectangle(P([(20, y), (W - 20, y + 44)]), fill=(255, 250, 236, 255) if k % 2 else (236, 226, 200, 255))
    d.rectangle(P([(20, 190), (W - 20, 412)]), outline=(26, 90, 60, 255), width=2 * S)
    return im

def deco():
    # el Topps de 1971: borde negro, la foto con un filo blanco fino y una
    # banda negra abajo para el nombre y las stats
    im = Image.new('RGBA', (W * S, H * S), (16, 16, 18, 255))
    d = ImageDraw.Draw(im, 'RGBA')
    sky = vgrad((120, 170, 140), (60, 110, 84))
    m = Image.new('L', im.size, 0)
    ImageDraw.Draw(m).rectangle(P([(20, 20), (W - 20, 340)]), fill=255)
    im.paste(sky, (0, 0), m)
    d.rectangle(P([(20, 20), (W - 20, 340)]), outline=(250, 250, 250, 255), width=S)
    return im


def save(im, n):
    d = ImageDraw.Draw(im, 'RGBA')
    d.rounded_rectangle([4 * S, 4 * S, (W - 4) * S, (H - 4) * S], 20 * S, outline=(40, 34, 24, 120), width=S)
    shape = Image.new('L', im.size, 0)
    ImageDraw.Draw(shape).rounded_rectangle([0, 0, W * S - 1, H * S - 1], 20 * S, fill=255)
    im.putalpha(Image.composite(im.getchannel('A'), Image.new('L', im.size, 0), shape))
    out = Image.new('RGBA', (W + PAD * 2, H + PAD * 2), (0, 0, 0, 0))
    out.paste(im.resize((W, H), Image.LANCZOS), (PAD, PAD))
    out.save(os.path.join(OUT, 'card_mythic_%d.png' % n))


for n, fn in enumerate((tobacco, rookie, autograph, plate, deco)):
    save(fn(), n)
print('miticas ok')
