# La textura del logo del menu (games/soccer-chapa/game/src/ui/menu/table.js), en games/soccer-chapa/game/assets/ui/:
#   logo        SOCCER CHAPA plano: SOCCER amarillo y CHAPA blanco
#   python3 menu.py <carpeta>
import os
import sys
from PIL import Image, ImageDraw, ImageFont

OUT = sys.argv[1]
FONTS = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'fonts')
S = 2
os.makedirs(OUT, exist_ok=True)


def font(name, size):
    return ImageFont.truetype(os.path.join(FONTS, name), size * S)


def save(im, name, w, h):
    im.resize((w, h), Image.LANCZOS).save(os.path.join(OUT, name + '.png'))


# el logo: plano, sin borde ni sombra. SOCCER en amarillo encima de CHAPA
# en blanco, en cursiva y pegados
W, H = 520, 150
logo = Image.new('RGBA', (W * S, H * S), (0, 0, 0, 0))
d = ImageDraw.Draw(logo, 'RGBA')
d.text((W / 2 * S, 34 * S), 'SOCCER', font=font('Montserrat-Black.otf', 34), fill=(255, 212, 90, 255), anchor='mm')
d.text((W / 2 * S, 92 * S), 'CHAPA', font=font('Montserrat-BlackItalic.otf', 96), fill=(255, 255, 255, 255),
       anchor='mm')
save(logo, 'logo', W, H)
print('menu ok')
