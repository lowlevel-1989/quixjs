# La textura de las chispas de un choque fuerte (games/soccer-chapa/game/src/ui/impact.js), en
# games/soccer-chapa/game/assets/ui/: blanca y premultiplicada (se suma a la imagen, ADD).
#   impact_streak  una chispa alargada (va en la direccion del golpe)
#   python3 impact.py <carpeta>
import os
import sys
from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)


def save(im, name):
    px = [(r * a // 255, g * a // 255, b * a // 255, a) for r, g, b, a in im.get_flattened_data()]
    out = Image.new('RGBA', im.size)
    out.putdata(px)
    out.save(os.path.join(OUT, name + '.png'))


streak = Image.new('RGBA', (64, 12), (255, 255, 255, 0))
ImageDraw.Draw(streak).polygon([(0, 6), (48, 2), (64, 6), (48, 10)], fill=(255, 255, 255, 255))
save(streak.filter(ImageFilter.GaussianBlur(1)), 'impact_streak')
print('golpe ok')
