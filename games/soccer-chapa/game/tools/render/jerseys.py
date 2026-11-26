# Texturas de las franelas para la cara de la chapa (PNG 1024x1024): el
# color 1 de fondo y el color 2 con el dibujo, con un poco de grano de
# impresion. Uso: python3 jerseys.py <carpeta>
import math, random, sys, os
from PIL import Image, ImageDraw, ImageFilter

S = 1024

def rgb(h):
    h = h.lstrip('#'); return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))

def stripes(d, b, n=5, w=0.2):
    for i in range(-n, n + 1, 2):
        x = S / 2 + i * w * S / 2
        d.rectangle([x - w * S / 4, 0, x + w * S / 4, S], fill=b)

def hoops(d, b):
    for i in range(-5, 6, 2):
        y = S / 2 + i * 0.2 * S / 2
        d.rectangle([0, y - 0.05 * S, S, y + 0.05 * S], fill=b)

def sash(d, b):
    w = 0.16 * S
    d.polygon([(-w, S - w), (S - w, -w), (S + w, w), (w, S + w)], fill=b)

def halves(d, b): d.rectangle([S / 2, 0, S, S], fill=b)
def quarters(d, b): d.rectangle([S / 2, 0, S, S / 2], fill=b); d.rectangle([0, S / 2, S / 2, S], fill=b)
def chest(d, b): d.rectangle([0, S * 0.30, S, S * 0.46], fill=b)
def center(d, b): d.rectangle([S * 0.40, 0, S * 0.60, S], fill=b)
def pins(d, b):
    for i in range(-10, 11):
        x = S / 2 + i * 0.05 * S; d.rectangle([x - 0.008 * S, 0, x + 0.008 * S, S], fill=b)
def chevron(d, b):
    d.polygon([(0, S * .2), (S / 2, S * .58), (S, S * .2), (S, S * .37), (S / 2, S * .75), (0, S * .37)], fill=b)
def checker(d, b):
    c = S / 8
    for y in range(8):
        for x in range(8):
            if (x + y) & 1: d.rectangle([x * c, y * c, (x + 1) * c, (y + 1) * c], fill=b)
def collar(d, b):
    d.polygon([(S * .28, 0), (S / 2, S * .30), (S * .72, 0)], fill=b); d.rectangle([0, S * .9, S, S], fill=b)
def band(d, b, a):
    d.rectangle([0, S * .42, S, S * .58], fill=b); d.rectangle([0, S * .47, S, S * .53], fill=a)

DESIGNS = {
    'rayas': stripes, 'aros': hoops, 'banda': sash, 'mitades': halves, 'cuartos': quarters,
    'pecho': chest, 'franja': center, 'finas': pins, 'galon': chevron, 'damero': checker,
    'cuello': collar, 'faja': band,
}

def make(name, c1, c2, path, seed=1):
    random.seed(seed)
    a, b = rgb(c1), rgb(c2)
    im = Image.new('RGB', (S, S), a)
    d = ImageDraw.Draw(im)
    f = DESIGNS[name]
    if name == 'faja': f(d, b, a)
    else: f(d, b)
    # bordes de impresion suaves y grano
    im = im.filter(ImageFilter.GaussianBlur(1.2))
    px = im.load()
    for _ in range(S * S // 6):
        x, y = random.randrange(S), random.randrange(S)
        r, g, bb = px[x, y]; k = random.randint(-10, 10)
        px[x, y] = (max(0, min(255, r + k)), max(0, min(255, g + k)), max(0, min(255, bb + k)))
    # un aro fino del color 2 cerca del borde, como en las chapas impresas
    d = ImageDraw.Draw(im)
    d.ellipse([S * .035, S * .035, S * .965, S * .965], outline=b if name not in ('mitades', 'cuartos') else (240, 238, 230), width=int(S * .012))
    im.save(path)

if __name__ == '__main__':
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    TEAMS = [
        ('rayas', '#b3202b', '#f1ece2'), ('rayas', '#1c1d21', '#e9e9ea'), ('aros', '#0f4c9c', '#f3f1ea'),
        ('banda', '#f2f0ea', '#c2122a'), ('mitades', '#1b6b3a', '#f2f0ea'), ('cuartos', '#f0c419', '#1a1a1a'),
        ('pecho', '#6b1f7a', '#f2f0ea'), ('franja', '#e9e9ea', '#1c1d21'), ('galon', '#0d2a54', '#e8b021'),
        ('damero', '#d7262f', '#f3f1ea'), ('cuello', '#5fb4e6', '#f3f1ea'), ('faja', '#e2711d', '#14213d'),
    ]
    for i, (n, c1, c2) in enumerate(TEAMS):
        make(n, c1, c2, os.path.join(out, '%02d_%s.png' % (i, n)), seed=i + 1)
    print('ok', len(TEAMS))
