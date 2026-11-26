# Las texturas de la interfaz (PNG a 2x, en games/soccer-chapa/game/assets/ui/). Lo que lleva el
# color de un equipo sale en blanco: el juego lo tine.
#   python3 ui.py ../../assets/ui
import math
import os
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

OUT = sys.argv[1]
FONTS = os.path.join(os.path.dirname(__file__), '..', '..', 'fonts')
os.makedirs(OUT, exist_ok=True)

DARK = (14, 18, 22)
DARK2 = (30, 37, 45)
PAPER = (244, 241, 234)


def font(name, size):
    return ImageFont.truetype(os.path.join(FONTS, name), size)


def vgrad(w, h, top, bottom):
    im = Image.new('RGBA', (w, h))
    d = ImageDraw.Draw(im)
    for y in range(h):
        f = y / max(1, h - 1)
        c = tuple(int(top[i] + (bottom[i] - top[i]) * f) for i in range(3))
        d.line([(0, y), (w, y)], fill=c + (255,))
    return im


def rounded(w, h, r):
    m = Image.new('L', (w, h), 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, w - 1, h - 1], r, fill=255)
    return m


def panel(w, h, r, top=DARK2, bottom=DARK, alpha=235):
    im = vgrad(w, h, top, bottom)
    m = rounded(w, h, r).point(lambda v: v * alpha // 255)
    im.putalpha(m)
    d = ImageDraw.Draw(im)
    # un filo de luz arriba
    d.rounded_rectangle([1, 1, w - 2, h - 2], r, outline=(255, 255, 255, 38))
    return im


def shadow(im, blur=10, off=4, alpha=140):
    w, h = im.size
    pad = blur * 2
    out = Image.new('RGBA', (w + pad * 2, h + pad * 2), (0, 0, 0, 0))
    sh = Image.new('RGBA', out.size, (0, 0, 0, 0))
    a = im.getchannel('A').point(lambda v: v * alpha // 255)
    sh.paste((0, 0, 0, 255), (pad, pad + off), a)
    out = Image.alpha_composite(out, sh.filter(ImageFilter.GaussianBlur(blur)))
    out.alpha_composite(im, (pad, pad))
    return out


def save(im, name):
    im.save(os.path.join(OUT, name))


# --------------------------------------------------------------- marcador
# placa: [color][ NOMBRE ][ 0 | 0 ][ NOMBRE ][color]
SB_W, SB_H = 600, 84
sb = panel(SB_W, SB_H, 14)
score = vgrad(150, SB_H - 16, (250, 248, 243), (220, 216, 207))
score.putalpha(rounded(150, SB_H - 16, 9))
sb.alpha_composite(score, ((SB_W - 150) // 2, 8))
ImageDraw.Draw(sb).line([(SB_W // 2, 20), (SB_W // 2, SB_H - 20)],
                        fill=(30, 30, 30, 120), width=2)
save(shadow(sb), 'scoreboard.png')

# franja de color de un equipo (dos mitades: se tinen por separado)
chip = Image.new('RGBA', (14, 34), (255, 255, 255, 255))
chip.putalpha(rounded(14, 34, 4))
save(chip, 'chip.png')

# la barra del turno
bar = Image.new('RGBA', (160, 8), (255, 255, 255, 255))
bar.putalpha(rounded(160, 8, 4))
save(bar, 'turn.png')

# ------------------------------------------------- medidor (estilo golf)
# como en un juego de golf: una barra de cristal claro con una pista
# hundida; la fuerza crece hacia la izquierda (X1 es el 0 %, X0 el 100 %)
# con relleno amarillo, y la zona de precision esta a la derecha del 0 %,
# en rosa. Se dibuja a 4x y se reduce (bordes suaves). Si cambia, cambiar
# tambien games/soccer-chapa/game/src/ui/meter.js y games/soccer-chapa/game/src/match/aim.js (IMPACT, ZONE)
MW, MH = 920, 64
X0, X1 = 60, 760              # 100 % y 0 %
ZC, ZH = 834, 70              # centro y media anchura de la zona de precision
PH = 13                       # media anchura del centro perfecto
FY = 16                       # donde empieza el relleno (arriba)
SS = 4                        # dibujar a 4x


def big(w, h):
    return Image.new('RGBA', (w * SS, h * SS), (0, 0, 0, 0))


def small(im, w, h):
    return im.resize((w, h), Image.LANCZOS)


def rr(d, box, r, **kw):
    d.rounded_rectangle([v * SS for v in box], r * SS, **kw)


# el marco de cristal
frame = big(MW, MH)
glass = vgrad(MW * SS, MH * SS, (255, 255, 255), (214, 219, 214))
m = Image.new('L', glass.size, 0)
rr(ImageDraw.Draw(m), [0, 0, MW - 1, MH - 1], 14, fill=200)
glass.putalpha(m)
frame.alpha_composite(glass)
d = ImageDraw.Draw(frame)
rr(d, [0, 0, MW - 1, MH - 1], 14, outline=(40, 46, 50, 120), width=2 * SS)
rr(d, [3, 2, MW - 4, MH // 2], 12, outline=(255, 255, 255, 200), width=SS)
# la pista hundida: oscura, con sombra interior arriba; llega hasta el
# final de la zona de precision
TX0, TX1 = X0 - 8, ZC + ZH + 4
track = vgrad((TX1 - TX0) * SS, (MH - FY * 2 + 8) * SS, (34, 40, 44), (70, 78, 82))
tm = Image.new('L', track.size, 0)
rr(ImageDraw.Draw(tm), [0, 0, TX1 - TX0 - 1, MH - FY * 2 + 7], 7, fill=150)
track.putalpha(tm)
frame.alpha_composite(track, (TX0 * SS, (FY - 4) * SS))
# las marcas, arriba y abajo de la pista
for i in range(11):
    x = (X1 - (X1 - X0) * i // 10) * SS
    big_tick = i % 5 == 0
    l = (9 if big_tick else 5) * SS
    w = (3 if big_tick else 2) * SS
    c = (30, 35, 38, 230 if big_tick else 150)
    d.line([(x, (FY - 5) * SS - l), (x, (FY - 5) * SS)], fill=c, width=w)
    d.line([(x, (MH - FY + 5) * SS), (x, (MH - FY + 5) * SS + l)], fill=c, width=w)
save(shadow(small(frame, MW, MH), blur=8, off=3, alpha=110), 'meter.png')

# el relleno: amarillo con volumen (claro arriba, intenso abajo) y brillo
FH = MH - FY * 2
fill = vgrad((X1 - X0) * SS, FH * SS, (255, 236, 120), (246, 168, 30))
hl = big(X1 - X0, FH)
ImageDraw.Draw(hl).rectangle([0, 2 * SS, (X1 - X0) * SS, FH * SS * 0.42], fill=(255, 255, 255, 70))
fill.alpha_composite(hl)
save(small(fill, X1 - X0, FH), 'meter_fill.png')

# la aguja: una barra fina con un triangulo arriba y otro abajo
NW, NH = 30, MH + 18
nd = big(NW, NH)
d = ImageDraw.Draw(nd)
dark = (28, 32, 36, 255)
d.rectangle([12 * SS, 6 * SS, 18 * SS, (NH - 6) * SS], fill=dark)
d.polygon([(3 * SS, 0), (27 * SS, 0), (15 * SS, 10 * SS)], fill=dark)
d.polygon([(3 * SS, NH * SS), (27 * SS, NH * SS), (15 * SS, (NH - 10) * SS)], fill=dark)
save(shadow(small(nd, NW, NH), blur=3, off=2, alpha=100), 'needle.png')
# la marca de la fuerza fijada y la flecha amarilla de la zona
mk = big(30, 22)
ImageDraw.Draw(mk).polygon([(0, 0), (30 * SS, 0), (15 * SS, 20 * SS)], fill=(255, 255, 255, 255))
save(shadow(small(mk, 30, 22), blur=3, off=2, alpha=110), 'mark.png')
# la precision, como en un juego de golf: una linea bajo la barra (va
# aparte: games/soccer-chapa/game/src/ui/meter.js la pone debajo), con los lados en rosa apagado y
# el centro perfecto en blanco con borde
ZL = big(ZH * 2 + 4, 20)
d = ImageDraw.Draw(ZL)
rr(d, [2, 4, ZH * 2 + 1, 15], 4, fill=(214, 92, 128, 255), outline=(30, 35, 38, 200), width=SS)
rr(d, [ZH + 2 - PH, 1, ZH + 2 + PH, 18], 4, fill=(255, 255, 255, 255), outline=(30, 35, 38, 255), width=SS)
save(shadow(small(ZL, ZH * 2 + 4, 20), blur=3, off=2, alpha=110), 'zone_line.png')
up = big(36, 28)
ImageDraw.Draw(up).polygon([(18 * SS, 0), (36 * SS, 26 * SS), (0, 26 * SS)], fill=(255, 214, 64, 255))
save(shadow(small(up, 36, 28), blur=3, off=2), 'zone_arrow.png')

# ----------------------------------------------------------------- botones
def button(kind):
    s = 64
    im = Image.new('RGBA', (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.ellipse([2, 2, s - 3, s - 3], fill=DARK + (230,), outline=PAPER + (230,), width=3)
    c = s // 2
    if kind in ('a', 'b', 'x', 'y'):
        f = font('Montserrat-Black.otf', 30)
        d.text((c, c + 1), kind.upper(), font=f, fill=PAPER + (255,), anchor='mm')
    if kind in ('r2', 'l2'):
        d.text((c, c + 1), kind.upper(), font=font('Montserrat-Black.otf', 22), fill=PAPER + (255,), anchor='mm')
    if kind == 'lr':
        d.polygon([(c - 20, c), (c - 6, c - 11), (c - 6, c + 11)], fill=PAPER)
        d.polygon([(c + 20, c), (c + 6, c - 11), (c + 6, c + 11)], fill=PAPER)
    if kind == 'ud':
        d.polygon([(c, c - 20), (c - 11, c - 6), (c + 11, c - 6)], fill=PAPER)
        d.polygon([(c, c + 20), (c - 11, c + 6), (c + 11, c + 6)], fill=PAPER)
    if kind == 'move':                     # mover: las cuatro flechas (cruceta o palanca)
        for dx, dy in ((0, -1), (0, 1), (-1, 0), (1, 0)):
            tip, base, side = (c + dx * 21, c + dy * 21), (c + dx * 9, c + dy * 9), (dy * 8, dx * 8)
            d.polygon([tip, (base[0] + side[0], base[1] + side[1]), (base[0] - side[0], base[1] - side[1])], fill=PAPER)
    if kind == 'l1r1':                     # elegir: L1 / R1
        f = font('Montserrat-Black.otf', 15)
        d.text((c, c - 9), 'L1', font=f, fill=PAPER + (255,), anchor='mm')
        d.text((c, c + 10), 'R1', font=f, fill=PAPER + (255,), anchor='mm')
    if kind == 'start':                    # las opciones: el menu (tres rayas)
        for y in (c - 10, c, c + 10):
            d.rounded_rectangle([c - 14, y - 3, c + 14, y + 3], 3, fill=PAPER)
    return im


for k in ('a', 'b', 'x', 'y', 'lr', 'ud', 'r2', 'l2', 'move', 'l1r1', 'start'):
    save(button(k), 'btn_' + k + '.png')

# ---------------------------------------------------------------- gol
# la franja oscura inclinada y las dos lineas de color (se tinen)
GW, GH = 1600, 260
band = Image.new('RGBA', (GW, GH), (0, 0, 0, 0))
ImageDraw.Draw(band).polygon([(0, 40), (GW, 0), (GW, GH - 40), (0, GH)],
                             fill=DARK + (235,))
save(band, 'goal_band.png')
line = Image.new('RGBA', (GW, 40), (0, 0, 0, 0))
ImageDraw.Draw(line).polygon([(0, 30), (GW, 0), (GW, 10), (0, 40)],
                             fill=(255, 255, 255, 255))
save(line, 'goal_line.png')

# ------------------------------------------------------------------ viento
# la placa del viento, con el estilo del marcador: a la izquierda un
# circulo (la flecha va dentro, centrada en x 42, y 42); a la derecha el
# juego escribe la fuerza
WP_W, WP_H = 230, 84
wp = panel(WP_W, WP_H, 14)
d = ImageDraw.Draw(wp)
d.ellipse([12, 12, 72, 72], fill=(255, 255, 255, 14), outline=(255, 255, 255, 70), width=2)
for i in range(8):
    a = i / 8 * 2 * math.pi
    c, r0, r1 = 42, 26, 30
    d.line([(c + math.cos(a) * r0, c + math.sin(a) * r0),
            (c + math.cos(a) * r1, c + math.sin(a) * r1)],
           fill=(255, 255, 255, 120), width=2)
save(shadow(wp), 'wind_panel.png')

# la flecha: fina, con la punta en chevron (mira a la derecha; se gira)
arrow = Image.new('RGBA', (96, 40), (0, 0, 0, 0))
d = ImageDraw.Draw(arrow)
d.rounded_rectangle([6, 17, 70, 23], 3, fill=(255, 255, 255, 255))
d.polygon([(56, 4), (92, 20), (56, 36), (64, 20)], fill=(255, 255, 255, 255))
save(arrow, 'wind_arrow.png')

# ------------------------------------------------------------- punteria
dot = Image.new('RGBA', (24, 24), (0, 0, 0, 0))
ImageDraw.Draw(dot).ellipse([2, 2, 21, 21], fill=(255, 255, 255, 255))
save(shadow(dot, blur=3, off=2, alpha=120), 'dot.png')
tip = Image.new('RGBA', (40, 40), (0, 0, 0, 0))
ImageDraw.Draw(tip).polygon([(4, 4), (38, 20), (4, 36), (12, 20)], fill=(255, 255, 255, 255))
save(shadow(tip, blur=3, off=2, alpha=120), 'tip.png')

# el halo bajo la chapa elegida: un anillo suave (se tine)
HS = 256
halo = Image.new('RGBA', (HS, HS), (0, 0, 0, 0))
ImageDraw.Draw(halo).ellipse([40, 40, HS - 41, HS - 41], outline=(255, 255, 255, 255), width=14)
halo = halo.filter(ImageFilter.GaussianBlur(9))
ring = Image.new('RGBA', (HS, HS), (0, 0, 0, 0))
ImageDraw.Draw(ring).ellipse([44, 44, HS - 45, HS - 45], outline=(255, 255, 255, 255), width=5)
halo.alpha_composite(ring)
save(halo, 'halo.png')

# ---------------------------------------------------------- nombre de chapa
# la etiqueta bajo la chapa: una pastilla oscura (el juego la estira al
# nombre) y su linea de color de equipo debajo (se tine)
tag = panel(240, 44, 22, alpha=225)
save(tag, 'name_tag.png')
tline = Image.new('RGBA', (200, 6), (255, 255, 255, 255))
tline.putalpha(rounded(200, 6, 3))
save(tline, 'name_line.png')

# ---------------------------------------------------------------- terreno
# la gota de lluvia: un trazo fino que se desvanece arriba (cae hacia abajo)
drop = Image.new('RGBA', (8, 64), (0, 0, 0, 0))
dd = ImageDraw.Draw(drop)
for y in range(64):
    dd.line([(3, y), (4, y)], fill=(210, 225, 235, int(200 * y / 63)))
save(drop, 'drop.png')

# ------------------------------------------------------------------- carta
# el brillo que recorre la carta (games/soccer-chapa/game/src/ui/card/card.js); el resto de la carta lo
# hace games/soccer-chapa/game/tools/ui/card.py
# La franja va en diagonal: la imagen es mas ancha que la carta (SHINE_PAD a
# la izquierda y mas a la derecha) para que la franja quepa entera y no se
# vea su corte; el juego la pone SHINE_PAD px a la izquierda (games/soccer-chapa/game/src/ui/card/card.js)
CW, CH, SHINE_PAD = 320, 448, 100
shine = Image.new('RGBA', (CW + SHINE_PAD + 200, CH), (0, 0, 0, 0))
sd = ImageDraw.Draw(shine)
for i in range(60):
    a = int(110 * (1 - abs(i - 30) / 30) ** 2)
    x = SHINE_PAD + i * 3 - 60
    sd.line([(x, CH), (x + CH * 0.6, 0)], fill=(255, 255, 255, a), width=3)
shine = shine.filter(ImageFilter.GaussianBlur(4))
shine.putdata([(r * a // 255, g * a // 255, b * a // 255, a) for r, g, b, a in shine.get_flattened_data()])  # se suma (ADD)
save(shine, 'card_shine.png')
print('ui ok')
