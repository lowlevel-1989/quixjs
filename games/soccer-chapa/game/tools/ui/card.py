# Las cartas de las chapas: 4 posiciones (su forma, layouts.py) por 4
# rarezas (su material, materials.py) = 16 fondos; y las de la pelota, la
# porteria, la franela y el cambio de nombre (items.py), con los mismos materiales = 16 mas
# (games/soccer-chapa/game/assets/ui/card_<posicion>_<rareza>.png, a 2x: 320 x 448, con PAD px
# transparentes alrededor para que los bordes salgan suaves al girar:
# games/soccer-chapa/game/src/ui/card/card.js). Tambien la pelota del indice (card_ball.png) y la sombra
# de la chapa sobre la carta (cap_shadow.png).
#   python3 card.py <carpeta>
import os
import sys
from PIL import Image, ImageDraw, ImageFilter
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from materials import W, H, S, material, rainbow
from layouts import LAYOUTS
import items

OUT = sys.argv[1]
PAD = 4
COLOR = {'gk': (46, 139, 87), 'def': (47, 111, 208), 'mid': (224, 160, 32), 'fwd': (208, 52, 44)}
COLOR.update(items.COLOR)                 # y lo que no es chapa: pelota, porteria, franela y nombre (items.py)
LAYOUTS = dict(LAYOUTS, **items.ITEMS)
os.makedirs(OUT, exist_ok=True)


def frame(im, colors, inner=None):
    # el marco: anillos de 3 px (uno solo en las raras: un filo cromado fino,
    # con una linea oscura muy fina por dentro)
    d = ImageDraw.Draw(im, 'RGBA')
    w = 3
    if colors == 'rainbow':                 # una sola linea con el arcoiris a lo largo
        ring = Image.new('L', im.size, 0)
        ImageDraw.Draw(ring).rounded_rectangle([4 * S, 4 * S, (W - 4) * S, (H - 4) * S], 20 * S,
                                               outline=255, width=w * S)
        grad = Image.new('RGBA', im.size)
        gd = ImageDraw.Draw(grad)
        for x in range(-int(H * S * 0.6), W * S, S):   # en diagonal: cubre toda la carta
            gd.line([(x, 0), (x + H * S * 0.6, H * S)], fill=rainbow(x / (W * S)) + (255,), width=S * 2)
        im.paste(grad, (0, 0), ring)
        return
    for i, c in enumerate(colors):
        o = 4 + i * w
        d.rounded_rectangle([o * S, o * S, (W - o) * S, (H - o) * S], (20 - i * 2) * S,
                            outline=c + (255,), width=w * S)
    if inner:
        o = 4 + len(colors) * w
        d.rounded_rectangle([o * S, o * S, (W - o) * S, (H - o) * S], (19 - len(colors) * 2) * S,
                            outline=inner + (255,), width=S)


def card(pos, rarity):
    mat = material(rarity, COLOR[pos])
    im = mat['bg']
    LAYOUTS[pos](im, mat)
    frame(im, mat['frame'], mat.get('inner'))
    im = im.convert('RGBA')                  # dibujada en RGB (materials.py: vgrad); ahora el recorte
    shape = Image.new('L', im.size, 0)
    ImageDraw.Draw(shape).rounded_rectangle([0, 0, W * S - 1, H * S - 1], 20 * S, fill=255)
    im.putalpha(Image.composite(im.getchannel('A'), Image.new('L', im.size, 0), shape))
    out = Image.new('RGBA', (W + PAD * 2, H + PAD * 2), (0, 0, 0, 0))
    out.paste(im.resize((W, H), Image.LANCZOS), (PAD, PAD))
    out.save(os.path.join(OUT, 'card_%s_%s.png' % (pos, rarity)))


for pos in COLOR:
    for rarity in ('common', 'rare', 'epic', 'legendary'):
        card(pos, rarity)

B = 64                                     # la pelota del indice (el juego la tine)
ball = Image.new('RGBA', (B * S, B * S), (0, 0, 0, 0))
ImageDraw.Draw(ball).ellipse([0, 0, B * S - 1, B * S - 1], fill=(255, 255, 255, 255))
ball.resize((B, B), Image.LANCZOS).save(os.path.join(OUT, 'card_ball.png'))
# la sombra de la chapa sobre la carta: un circulo negro difuminado
C = 160
sh = Image.new('RGBA', (C * S, C * S), (0, 0, 0, 0))
ImageDraw.Draw(sh).ellipse([34 * S, 34 * S, (C - 34) * S, (C - 34) * S], fill=(0, 0, 0, 255))
sh.filter(ImageFilter.GaussianBlur(12 * S)).resize((C, C), Image.LANCZOS).save(os.path.join(OUT, 'cap_shadow.png'))
print('cartas ok')
