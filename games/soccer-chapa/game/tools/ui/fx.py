# Las texturas de los efectos de las cartas segun su rareza (games/soccer-chapa/game/src/ui/card/fx.js):
#   card_holo   bandas de arcoiris en diagonal (lamina holografica, se mueve)
#   card_glow   el resplandor alrededor de la carta (blanco: se tine)
#   card_rays   rayos de luz detras de la carta (giran)
#   python3 fx.py <carpeta>
import math
import os
import sys
from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1]
W, H = 320, 448
os.makedirs(OUT, exist_ok=True)


def save(im, name):
    # el color multiplicado por su alfa: estas texturas se suman a la
    # imagen (ADD) y lo transparente no debe sumar nada
    px = [(rr * aa // 255, gg * aa // 255, bb * aa // 255, aa) for rr, gg, bb, aa in im.get_flattened_data()]
    out = Image.new('RGBA', im.size)
    out.putdata(px)
    out.save(os.path.join(OUT, name))


# la lamina: el doble de ancha que la carta (se desplaza al inclinarla)
holo = Image.new('RGBA', (W * 2, H), (0, 0, 0, 0))
d = ImageDraw.Draw(holo)
for x in range(-H, W * 2 + H, 2):
    f = x / 180.0
    c = tuple(int(127 + 127 * math.sin(2 * math.pi * (f + k / 3))) for k in range(3))
    a = int(150 * (0.5 + 0.5 * math.sin(x / 23.0)))
    d.line([(x, H), (x + H * 0.7, 0)], fill=c + (a,), width=2)
save(holo.filter(ImageFilter.GaussianBlur(2)), 'card_holo.png')

# el resplandor: un rectangulo redondeado borroso, algo mas grande que la carta
G = 48
glow = Image.new('RGBA', (W + G * 2, H + G * 2), (0, 0, 0, 0))
ImageDraw.Draw(glow).rounded_rectangle([G - 6, G - 6, W + G + 6, H + G + 6], 28, fill=(255, 255, 255, 255))
save(glow.filter(ImageFilter.GaussianBlur(18)), 'card_glow.png')

# los rayos: cuarenta haces que se apagan hacia fuera
R = 400
rays = Image.new('RGBA', (R * 2, R * 2), (0, 0, 0, 0))
d = ImageDraw.Draw(rays)
for k in range(20):
    a0, a1 = k / 20 * 2 * math.pi, (k + 0.35) / 20 * 2 * math.pi
    d.polygon([(R, R), (R + math.cos(a0) * R, R + math.sin(a0) * R),
               (R + math.cos(a1) * R, R + math.sin(a1) * R)], fill=(255, 255, 255, 120))
fade = Image.new('L', rays.size, 0)
fd = ImageDraw.Draw(fade)
for r in range(R, 0, -4):
    fd.ellipse([R - r, R - r, R + r, R + r], fill=int(255 * (1 - r / R) ** 1.5))
rays.putalpha(Image.composite(rays.getchannel('A'), Image.new('L', rays.size, 0), fade))
save(rays.filter(ImageFilter.GaussianBlur(3)), 'card_rays.png')

print('efectos ok')
