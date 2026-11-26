# El sobre y el dorso de las cartas (games/soccer-chapa/game/src/ui/pack-view.js), en games/soccer-chapa/game/assets/ui/:
#   pack_body   el sobre de aluminio sin la tira de arriba (plateado: el
#               juego lo tine con el color de cada sobre)
#   pack_top    la tira de arriba con la linea de puntos: se rasga al abrir
#   pack_label  el nombre arriba, los rayos y la sombra de la chapa y la
#               linea de abajo (sin tenir; la chapa y "4 CARTAS" los pone
#               el juego)
#   card_back   el dorso de las cartas: el aluminio del sobre (se tine con
#               su color)
#   card_back_logo  encima del dorso, sin tenir: SOCCER CHAPA y el filo
# Todo a 1x con PAD px transparentes alrededor (bordes suaves al girar).
#   python3 pack.py <carpeta>
import math
import os
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

OUT = sys.argv[1]
FONTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'fonts')
FONT = os.path.join(FONTS, 'Montserrat-Black.otf')
FONT_I = os.path.join(FONTS, 'Montserrat-BlackItalic.otf')
S, PAD = 4, 4
PW, PH, TOP = 300, 440, 44          # el sobre y donde se rasga
CW, CH = 320, 448                   # la carta
os.makedirs(OUT, exist_ok=True)


def P(pts):
    return [(x * S, y * S) for x, y in pts]


def font(size, italic=False):
    return ImageFont.truetype(FONT_I if italic else FONT, size * S)


def save(im, name, w, h):
    out = Image.new('RGBA', (w + PAD * 2, h + PAD * 2), (0, 0, 0, 0))
    out.paste(im.resize((w, h), Image.LANCZOS), (PAD, PAD))
    out.save(os.path.join(OUT, name + '.png'))


def outline():
    # el contorno del sobre: dientes arriba y abajo (el sellado)
    pts, T, n = [], 7, 25
    for i in range(n + 1):
        pts.append((i * PW / n, 0 if i % 2 else T))
    for i in range(n, -1, -1):
        pts.append((i * PW / n, PH if i % 2 else PH - T))
    return pts


def foil():
    # plateado: mas claro en el centro (el sobre abombado), un reflejo en
    # vertical, rayas finas en diagonal y el sellado con sus estrias
    im = Image.new('RGBA', (PW * S, PH * S))
    d = ImageDraw.Draw(im, 'RGBA')
    for x in range(PW * S):
        f = x / (PW * S)
        v = 150 + 85 * math.sin(math.pi * f) ** 0.7 + 30 * math.exp(-((f - 0.3) / 0.06) ** 2)
        v = int(min(255, v))
        d.line([(x, 0), (x, PH * S)], fill=(v, v, v, 255))
    for k in range(-30, 40):
        x = k * 22
        d.line(P([(x, 0), (x + PH * 0.5, PH)]), fill=(255, 255, 255, 18), width=S)
    for y0, y1 in ((0, 30), (PH - 30, PH)):
        for y in range(y0, y1, 3):
            d.line(P([(0, y), (PW, y)]), fill=(0, 0, 0, 40) if (y // 3) % 2 else (255, 255, 255, 50), width=S)
    m = Image.new('L', im.size, 0)
    ImageDraw.Draw(m).polygon(P(outline()), fill=255)
    im.putalpha(m)
    return im


def wordmark(im, cx, y, k=1):
    # SOCCER pequeno y espaciado, CHAPA grande en cursiva con su sombra
    d = ImageDraw.Draw(im, 'RGBA')
    d.text((cx * S, y * S), 'S O C C E R', font=font(int(17 * k), True), fill=(255, 255, 255, 235), anchor='mm')
    for dx, dy, c in ((3, 4, (0, 0, 0, 110)), (0, 0, (255, 255, 255, 255))):
        d.text(((cx + dx * k) * S, (y + (38 + dy) * k) * S), 'CHAPA', font=font(int(56 * k), True),
               fill=c, anchor='mm')


body = foil()
d = ImageDraw.Draw(body, 'RGBA')
for x in range(6, PW - 6, 10):                                  # la linea de puntos
    d.line(P([(x, TOP), (x + 5, TOP)]), fill=(0, 0, 0, 110), width=S)
save(body.crop((0, 0, PW * S, TOP * S)), 'pack_top', PW, TOP)
save(body.crop((0, TOP * S, PW * S, PH * S)), 'pack_body', PW, PH - TOP)

label = Image.new('RGBA', (PW * S, PH * S), (0, 0, 0, 0))
d = ImageDraw.Draw(label, 'RGBA')
cx, cy = PW / 2, 262                                           # los rayos detras de la chapa
for k in range(28):
    a0, a1 = k / 28 * 2 * math.pi, (k + 0.45) / 28 * 2 * math.pi
    d.polygon(P([(cx, cy), (cx + math.cos(a0) * 190, cy + math.sin(a0) * 190),
                 (cx + math.cos(a1) * 190, cy + math.sin(a1) * 190)]), fill=(255, 255, 255, 34))
fade = Image.new('L', label.size, 0)
fd = ImageDraw.Draw(fade)
for r in range(150, 0, -3):
    fd.ellipse(P([(cx - r, cy - r), (cx + r, cy + r)]), fill=int(255 * (1 - r / 150) ** 0.8))
label.putalpha(Image.composite(label.getchannel('A'), Image.new('L', label.size, 0), fade))
glow = Image.new('RGBA', label.size, (0, 0, 0, 0))             # la sombra de la chapa
ImageDraw.Draw(glow).ellipse(P([(cx - 84, cy - 70), (cx + 90, cy + 98)]), fill=(0, 0, 0, 90))
label.alpha_composite(glow.filter(ImageFilter.GaussianBlur(10 * S)))
wordmark(label, PW / 2, 84)
d = ImageDraw.Draw(label, 'RGBA')
d.line(P([(70, 372), (PW - 70, 372)]), fill=(255, 255, 255, 150), width=S)
save(label, 'pack_label', PW, PH)

# el dorso: el mismo aluminio del sobre (plateado: el juego lo tine con el
# color del sobre) y, aparte y sin tenir, el nombre en blanco y un filo
back = Image.new('RGBA', (CW * S, CH * S))
d = ImageDraw.Draw(back, 'RGBA')
for x in range(CW * S):
    f = x / (CW * S)
    v = int(min(255, 165 + 80 * math.sin(math.pi * f) ** 0.7 + 25 * math.exp(-((f - 0.3) / 0.06) ** 2)))
    d.line([(x, 0), (x, CH * S)], fill=(v, v, v, 255))
for k in range(-30, 40):
    x = k * 22
    d.line(P([(x, 0), (x + CH * 0.5, CH)]), fill=(255, 255, 255, 18), width=S)
m = Image.new('L', back.size, 0)
ImageDraw.Draw(m).rounded_rectangle([0, 0, CW * S - 1, CH * S - 1], 20 * S, fill=255)
back.putalpha(m)
save(back, 'card_back', CW, CH)
logo = Image.new('RGBA', (CW * S, CH * S), (0, 0, 0, 0))
ImageDraw.Draw(logo).rounded_rectangle(P([(2, 2), (CW - 2, CH - 2)]), 18 * S,
                                       outline=(255, 255, 255, 230), width=3 * S)
ImageDraw.Draw(logo).rounded_rectangle(P([(14, 14), (CW - 14, CH - 14)]), 10 * S,
                                       outline=(255, 255, 255, 110), width=S)
wordmark(logo, CW / 2, CH / 2 - 24)
save(logo, 'card_back_logo', CW, CH)
print('sobre ok')
