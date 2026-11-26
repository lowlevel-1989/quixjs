# El material de una carta segun su rareza (games/soccer-chapa/game/tools/ui/card.py): el fondo
# entero y los colores con los que layouts.py dibuja la ventana y el marco.
#   common     cartulina crema
#   rare       cromado sobre el color de la posicion, con franjas
#   epic       oro: negro con un resplandor dorado y rayos finos de oro
#   legendary  holografico: oscuro con rayos de arcoiris (la mas rara)
import math
import random
from PIL import Image, ImageDraw

W, H, S = 320, 448, 4


def mix(a, b, f):
    return tuple(int(a[i] + (b[i] - a[i]) * f) for i in range(3))


def rainbow(f):
    return tuple(int(127 + 127 * math.sin(2 * math.pi * (f + k / 3))) for k in range(3))


def vgrad(top, bottom):
    # en RGB: asi lo que se dibuja con alfa encima se mezcla (en RGBA, PIL
    # cambia el pixel por el del dibujo y deja agujeros transparentes)
    im = Image.new('RGB', (W * S, H * S))
    d = ImageDraw.Draw(im, 'RGBA')
    for y in range(H * S):
        d.line([(0, y), (W * S, y)], fill=mix(top, bottom, y / (H * S)) + (255,))
    return im


def _common(c):
    return vgrad((242, 234, 214), (230, 220, 196))


def _rare(c):
    im = vgrad(mix(c, (0, 0, 0), 0.25), mix(c, (0, 0, 0), 0.62))
    d = ImageDraw.Draw(im, 'RGBA')
    for k in range(-12, 20):
        o = k * 34 * S
        d.polygon([(o, 0), (o + 14 * S, 0), (o + 14 * S - H * S * 0.5, H * S), (o - H * S * 0.5, H * S)],
                  fill=(255, 255, 255, 22))
    return im


def _epic(c):
    im = vgrad((26, 20, 42), (10, 8, 18))
    d = ImageDraw.Draw(im, 'RGBA')
    cx, cy = W * S / 2, H * S * 0.42
    for k in range(36):
        a0, a1 = k / 36 * 2 * math.pi, (k + 0.45) / 36 * 2 * math.pi
        d.polygon([(cx, cy), (cx + math.cos(a0) * 3000, cy + math.sin(a0) * 3000),
                   (cx + math.cos(a1) * 3000, cy + math.sin(a1) * 3000)], fill=rainbow(k / 36) + (44,))
    return im


def _legendary(c):
    # negro con un resplandor dorado en el centro y rayos finos de oro
    im = Image.new('RGB', (W * S, H * S), (6, 5, 4))
    d = ImageDraw.Draw(im, 'RGBA')
    cx, cy = W * S / 2, H * S * 0.42
    for r in range(int(H * S * 0.75), 0, -6 * S):
        f = 1 - r / (H * S * 0.75)
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(int(70 * f), int(48 * f), int(12 * f), 255))
    for k in range(48):
        a0, a1 = k / 48 * 2 * math.pi, (k + 0.3) / 48 * 2 * math.pi
        d.polygon([(cx, cy), (cx + math.cos(a0) * 3000, cy + math.sin(a0) * 3000),
                   (cx + math.cos(a1) * 3000, cy + math.sin(a1) * 3000)], fill=(255, 205, 90, 30))
    return im


def material(rarity, c):
    # c: el color de la posicion. Devuelve el fondo y los colores a usar
    if rarity == 'common':
        return {'bg': _common(c), 'frame': [c], 'window': c, 'line': (30, 30, 30), 'deco': c + (60,),
                'dots': (0, 0, 0, 40), 'rainbow': False, 'pale': mix(c, (255, 255, 255), 0.62) + (255,)}
    if rarity == 'rare':
        return {'bg': _rare(c), 'frame': [(236, 240, 246)], 'inner': (90, 100, 112),
                'window': mix(c, (0, 0, 0), 0.3), 'line': (240, 244, 250),
                'deco': (230, 236, 245, 80), 'dots': None, 'rainbow': False, 'pale': (255, 255, 255, 34)}
    if rarity == 'legendary':
        return {'bg': _epic(c), 'frame': 'rainbow',
                'window': mix(c, (20, 16, 32), 0.45), 'line': None,
                'deco': (255, 255, 255, 150), 'dots': None, 'rainbow': True, 'pale': (255, 255, 255, 24)}
    return {'bg': _legendary(c), 'frame': [(255, 222, 120)],   # la epica: el oro 'inner': (120, 80, 10),
            'window': (196, 150, 48), 'line': (255, 236, 160), 'deco': (255, 220, 120, 160),
            'dots': (255, 246, 200, 70), 'rainbow': False, 'pale': (255, 205, 90, 40)}
