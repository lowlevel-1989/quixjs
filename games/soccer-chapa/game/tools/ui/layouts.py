# La forma de cada carta segun la posicion (games/soccer-chapa/game/tools/ui/card.py): la ventana
# donde va la chapa y lo que la rodea. Las medidas que importan al juego
# (donde va la chapa y los textos) estan tambien en games/soccer-chapa/game/src/ui/card/layout.js.
#   gk   un medallon ovalado y una cinta para el nombre
#   def  un escudo y dos bloques de color
#   mid  el circulo central y las lineas del campo
#   fwd  una ventana inclinada y lineas de velocidad
from PIL import Image, ImageDraw, ImageFilter
from materials import W, H, S, rainbow


def P(pts):
    return [(x * S, y * S) for x, y in pts]


def _window(im, mat, mask_draw, outline):
    # rellena la ventana con su color (y trama) y le pone el borde
    m = Image.new('L', im.size, 0)
    mask_draw(ImageDraw.Draw(m), 255)
    fill = Image.new('RGBA', im.size, mat['window'] + (255,))
    if mat['dots']:
        d = ImageDraw.Draw(fill)
        for y in range(0, H * S, 9 * S):
            for x in range(0, W * S, 9 * S):
                o = 4 * S if (y // (9 * S)) % 2 else 0
                d.ellipse([x + o, y, x + o + 3 * S, y + 3 * S], fill=mat['dots'])
    im.paste(fill, (0, 0), m)
    d = ImageDraw.Draw(im, 'RGBA')
    if mat['rainbow']:
        for i in range(6):
            outline(d, rainbow(i / 6) + (255,), S)
    elif mat['line']:
        outline(d, mat['line'] + (255,), 3 * S)


def gk(im, mat):
    # un medallon ovalado, como el retrato de un cromo antiguo, y una cinta
    # debajo para el nombre
    d = ImageDraw.Draw(im, 'RGBA')
    oval = [(48, 70), (272, 316)]
    ring = [(38, 60), (282, 326)]
    d.ellipse(P(ring), outline=mat['deco'], width=4 * S)
    _window(im, mat, lambda dd, f: dd.ellipse(P(oval), fill=f),
            lambda dd, c, w: dd.ellipse(P(oval), outline=c, width=w))
    ribbon = [(22, 314), (298, 314), (286, 330), (298, 346), (22, 346), (34, 330)]
    d.polygon(P(ribbon), fill=mat['pale'])


def deff(im, mat):
    # como el naipe del delantero: dos bloques de color (al reves) y el
    # escudo limpio en el centro
    d = ImageDraw.Draw(im, 'RGBA')
    cy, m = 200, 18
    d.rectangle(P([(m + 2, m + 2), (W / 2, cy)]), fill=mat['pale'])
    d.rectangle(P([(W / 2, cy), (W - m - 2, H - m - 2)]), fill=mat['pale'])
    d.rounded_rectangle(P([(m, m), (W - m, H - m)]), 14 * S, outline=mat['deco'], width=2 * S)
    shield = [(96, 62), (190, 38), (284, 62), (284, 206), (190, 324), (96, 206)]
    _window(im, mat, lambda dd, f: dd.polygon(P(shield), fill=f),
            lambda dd, c, w: dd.polygon(P(shield), outline=c, width=w))


def mid(im, mat):
    d = ImageDraw.Draw(im, 'RGBA')
    cx, cy, r = 160, 180, 112
    d.line(P([(0, cy), (W, cy)]), fill=mat['deco'], width=3 * S)              # la linea de medio campo
    d.rectangle(P([(24, 24), (W - 24, H - 24)]), outline=mat['deco'], width=2 * S)
    d.arc(P([(cx - 46, H - 24 - 46), (cx + 46, H - 24 + 46)]), 180, 360, fill=mat['deco'], width=2 * S)
    box = [(cx - r, cy - r), (cx + r, cy + r)]
    _window(im, mat, lambda dd, f: dd.ellipse(P(box), fill=f),
            lambda dd, c, w: dd.ellipse(P(box), outline=c, width=w))


def fwd(im, mat):
    # el naipe de beisbol: dos cuadrantes de color y el rombo en el centro
    d = ImageDraw.Draw(im, 'RGBA')
    cx, cy, r, m = 160, 230, 118, 18
    d.rectangle(P([(cx, m + 2), (W - m - 2, cy)]), fill=mat['pale'])
    d.rectangle(P([(m + 2, cy), (cx, H - m - 2)]), fill=mat['pale'])
    d.rounded_rectangle(P([(m, m), (W - m, H - m)]), 14 * S, outline=mat['deco'], width=2 * S)
    dia = [(cx, cy - r), (cx + r, cy), (cx, cy + r), (cx - r, cy)]
    _window(im, mat, lambda dd, f: dd.polygon(P(dia), fill=f),
            lambda dd, c, w: dd.polygon(P(dia), outline=c, width=w))


LAYOUTS = {'gk': gk, 'def': deff, 'mid': mid, 'fwd': fwd}
