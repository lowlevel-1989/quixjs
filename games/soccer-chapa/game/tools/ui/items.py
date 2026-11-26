# Las cartas de lo que no es una chapa (games/soccer-chapa/game/tools/ui/card.py
# las saca con los mismos materiales de rareza que las de las chapas):
#   ball  la pelota: una vitrina redonda sobre su peana y una placa
#   goal  la porteria: una ventana apaisada con el area dibujada, como el
#         fondo de un campo visto desde arriba
#   kit   la franela: sin ventana, la chapa del equipo grande en el centro
#         (la pone el juego) y la placa del nombre
#   name  el cambio de nombre (de nombre, de apellido o de los dos): muy
#         limpia, una firma grande; lo que cambia lo escribe el juego debajo
# Cada una con su color, como las posiciones. Donde va el dibujo y el
# nombre: games/soccer-chapa/game/src/ui/card/layout.js (ball, goal, kit, name).
import math
from PIL import ImageDraw
from materials import W, H, S
from layouts import P, _window

COLOR = {'ball': (40, 150, 170), 'goal': (120, 88, 200), 'kit': (224, 104, 44), 'name': (206, 64, 128)}


def _frame_in(d, mat, m=18):
    d.rounded_rectangle(P([(m, m), (W - m, H - m)]), 14 * S, outline=mat['deco'], width=2 * S)


def ball(im, mat):
    # la vitrina: un circulo grande con un aro fino alrededor, la peana debajo
    # (dos escalones) y la placa del nombre
    d = ImageDraw.Draw(im, 'RGBA')
    _frame_in(d, mat)
    cx, cy, r = 160, 178, 108
    d.ellipse(P([(cx - r - 12, cy - r - 12), (cx + r + 12, cy + r + 12)]), outline=mat['deco'], width=3 * S)
    d.polygon(P([(cx - 70, 300), (cx + 70, 300), (cx + 90, 322), (cx - 90, 322)]), fill=mat['pale'])
    d.rectangle(P([(cx - 100, 322), (cx + 100, 336)]), fill=mat['pale'])
    _window(im, mat, lambda dd, f: dd.ellipse(P([(cx - r, cy - r), (cx + r, cy + r)]), fill=f),
            lambda dd, c, w: dd.ellipse(P([(cx - r, cy - r), (cx + r, cy + r)]), outline=c, width=w))
    d.rounded_rectangle(P([(46, 350), (W - 46, 394)]), 8 * S, fill=mat['pale'])


def goal(im, mat):
    # el fondo del campo: una ventana apaisada con el area grande y la pequeña
    # dibujadas (la porteria va encima, con la boca hacia abajo), y una franja
    # para el nombre
    d = ImageDraw.Draw(im, 'RGBA')
    _frame_in(d, mat)
    box = [(28, 64), (W - 28, 316)]
    _window(im, mat, lambda dd, f: dd.rounded_rectangle(P(box), 10 * S, fill=f),
            lambda dd, c, w: dd.rounded_rectangle(P(box), 10 * S, outline=c, width=w))
    d = ImageDraw.Draw(im, 'RGBA')
    line = (255, 255, 255, 90)
    d.line(P([(40, 286), (W - 40, 286)]), fill=line, width=2 * S)                 # la linea de gol
    d.rectangle(P([(62, 196), (W - 62, 286)]), outline=line, width=2 * S)          # el area grande
    d.rectangle(P([(116, 252), (W - 116, 286)]), outline=line, width=2 * S)        # la pequeña
    d.arc(P([(124, 160), (196, 232)]), 200, 340, fill=line, width=2 * S)           # la media luna
    d.rectangle(P([(28, 340), (W - 28, 396)]), fill=mat['pale'])


def kit(im, mat):
    # sin ventana: el marco, la chapa del equipo grande en el centro (la pone
    # el juego) y la placa del nombre
    d = ImageDraw.Draw(im, 'RGBA')
    _frame_in(d, mat)
    d.rounded_rectangle(P([(46, 344), (W - 46, 394)]), 8 * S, fill=mat['pale'])


def name(im, mat):
    # muy limpia: el marco y una firma grande en el centro (tinta azul en la
    # comun; en las demas, del color de su filo); lo que cambia lo escribe el
    # juego debajo, del mismo color y sin placa
    d = ImageDraw.Draw(im, 'RGBA')
    _frame_in(d, mat)
    ink = (34, 60, 160, 255) if mat['line'] == (30, 30, 30) else (mat['line'] or (255, 255, 255)) + (255,)
    # la firma: trazo de letra ligada (bucles que avanzan, como una "l" tras
    # otra, cada uno de su tamaño) y un subrayado con su rabito
    pts, n = [], 420
    for k in range(n):
        f = k / n
        a = f * 2 * math.pi * 6.5
        r = 16 + 14 * math.sin(f * 7.3) ** 2
        pts.append((64 + f * 186 - r * 0.55 * math.sin(a), 200 - f * 24 - r * math.cos(a) * (1.2 - f * 0.5)))
    d.line(P(pts), fill=ink, width=3 * S, joint='curve')
    tail = [(70 + k * 1.9, 246 - k * 0.22 + 6 * math.sin(k / 95 * math.pi)) for k in range(95)]
    d.line(P(tail + [(256, 220), (246, 232)]), fill=ink, width=3 * S, joint='curve')   # el subrayado


ITEMS = {'ball': ball, 'goal': goal, 'kit': kit, 'name': name}
