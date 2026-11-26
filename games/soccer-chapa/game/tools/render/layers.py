# Pasa los renders de passes.py a las capas que usa el juego:
#   shade.png   gris para multiplicar (1 = la luz en el centro de la cara)
#   gloss.png   brillo para sumar (screen)
#   shadow.png  sombra: negro con alfa
#   mask.png    la silueta en blanco (el color 1 de la chapa la tine)
#   metal.png   la falda de metal sin pintar (la chapa del portero),
#               sacada de la luz de cap_lit.png
# python3 layers.py <sprites> <assets>
import sys
from PIL import Image
src, dst = sys.argv[1], sys.argv[2]
lit = Image.open(src + '/cap_lit.png').convert('RGBA')
w, h = lit.size
ref = sum(lit.getpixel((w // 2 + dx, h // 2 + dy))[1] for dx in (-8, 0, 8) for dy in (-8, 0, 8)) / 9
px = lit.load()
for y in range(h):
    for x in range(w):
        r, g, b, a = px[x, y]
        v = min(255, int(g / ref * 255))
        px[x, y] = (v, v, v, a)
lit.save(dst + '/shade.png')
mask = Image.new('RGBA', (w, h), (255, 255, 255, 0))
mask.putalpha(lit.getchannel('A'))
mask.save(dst + '/mask.png')
gl = Image.open(src + '/cap_gloss.png').convert('RGBA'); gp = gl.load()
for y in range(h):
    for x in range(w):
        r, g, b, a = gp[x, y]
        v = min(255, int(max(r, g, b) * 2.2))
        gp[x, y] = (255, 255, 255, int(v * a / 255))
gl.save(dst + '/gloss.png')


def edge(x, y):
    # la sombra se apaga antes del borde de la imagen (si no, se corta en
    # un cuadrado): 1 hasta FADE px del borde, 0 en el borde
    FADE = 28.0
    d = ((x - w / 2) ** 2 + (y - h / 2) ** 2) ** 0.5
    return max(0.0, min(1.0, (w / 2 - 2 - d) / FADE))


sh = Image.open(src + '/cap_shadow.png').convert('RGBA'); sp = sh.load()
for y in range(h):
    for x in range(w):
        r, g, b, a = sp[x, y]
        sp[x, y] = (0, 0, 0, int(a * edge(x, y) * 0.85))
sh.save(dst + '/shadow.png')
# la falda de metal del portero: la luz de cap_lit (con sus pliegues) con
# la curva del acero: sombras hondas, brillos fuertes y un toque azulado;
# solo fuera de la cara (FACE del radio R de games/soccer-chapa/game/src/caps/cap.js)
lit = Image.open(src + '/cap_lit.png').convert('RGBA'); lp = lit.load()
FACE = 102.4 * 0.86 * w / 256


def ring(x, y):
    return ((x - w / 2) ** 2 + (y - h / 2) ** 2) ** 0.5 - FACE + 0.5


vals = sorted(lp[x, y][1] for y in range(h) for x in range(w) if lp[x, y][3] and ring(x, y) > 0)
lo, hi = vals[len(vals) // 20], vals[-len(vals) // 50]
metal = Image.new('RGBA', (w, h)); mp = metal.load()
for y in range(h):
    for x in range(w):
        g, a = lp[x, y][1], lp[x, y][3]
        v = 70 + 185 * max(0.0, min(1.0, (g - lo) / float(hi - lo))) ** 1.4
        mp[x, y] = (int(v * 0.97), int(v * 0.99), int(min(255, v * 1.04)), int(a * max(0.0, min(1.0, ring(x, y)))))
metal.save(dst + '/metal.png')
print('ref', ref)
