# El publico del partido, de grabaciones de un estadio de verdad: el pack
# "Free Crowd Cheering Sounds" de Gregor Quendel (CC-BY 4.0,
# https://opengameart.org/content/free-crowd-cheering-sounds,
# www.gregorquendel.com). De cada grabacion se corta el trozo que sirve, se
# iguala su volumen (loudnorm), se pasa a mono a 22050 Hz y se guarda en
# games/soccer-chapa/game/assets/sfx/<nombre>.ogg (los usa games/soccer-chapa/game/src/audio/sfx.js):
#   crowd   el murmullo de fondo (en bucle: el final se cruza con el principio)
#   cheer   el estadio entero en un gol
#   rise    la grada que se levanta (el balon va a la porteria)
#   ooh     la reaccion a una ocasion que se va
#   chant   la grada animando a ritmo, de vez en cuando
#   python3 crowd_rec.py <carpeta del pack (los mp3)> <carpeta de salida>
import os
import subprocess
import sys

PACK, OUT = sys.argv[1], sys.argv[2]
# nombre: (numero de la grabacion, desde, hasta (s), volumen (LUFS), subida, bajada (s))
CUTS = {
    'crowd': ('10', 14.0, 31.0, -20, 0.0, 0.0),
    'cheer': ('04', 3.5, 12.5, -14, 0.2, 2.5),
    'rise':  ('05', 6.5, 10.5, -16, 0.8, 0.6),
    'ooh':   ('06', 2.5, 6.0, -18, 0.3, 1.5),
    'chant': ('08', 4.0, 14.0, -16, 0.6, 1.5),
}
LOOPS = ['crowd']       # los bucles
LOOP_X = 1.5            # lo que se cruza el final del bucle con el principio (s)


def source(num):
    return [os.path.join(PACK, f) for f in os.listdir(PACK) if f.endswith('.mp3') and ' - %s - ' % num in f][0]


for name, (num, a, b, lufs, fin, fout) in CUTS.items():
    path = os.path.join(OUT, name + '.ogg')
    if name in LOOPS:
        # el bucle: el trozo mas LOOP_X segundos; esos ultimos se mezclan,
        # subiendo, con el principio (que baja), y se quitan del final
        d = b - a
        flt = ('[0:a]atrim=%f:%f,asetpts=N/SR/TB,loudnorm=I=%d,aformat=sample_rates=22050:channel_layouts=mono,asplit[m][t];'
               '[m]atrim=%f:%f,asetpts=N/SR/TB,afade=t=in:d=%f[body];'
               '[t]atrim=%f:%f,asetpts=N/SR/TB,afade=t=out:d=%f[tail];'
               '[body][tail]amix=inputs=2:duration=first:normalize=0[o]') % (
                   a, b + LOOP_X, lufs, 0, d, LOOP_X, d, d + LOOP_X, LOOP_X)
    else:
        flt = ('[0:a]atrim=%f:%f,asetpts=N/SR/TB,loudnorm=I=%d,aformat=sample_rates=22050:channel_layouts=mono,'
               'afade=t=in:d=%f,afade=t=out:st=%f:d=%f[o]') % (a, b, lufs, max(fin, 0.01), b - a - fout, fout)
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', source(num), '-filter_complex', flt, '-map', '[o]',
                    '-c:a', 'libvorbis', '-q:a', '3', path], check=True)
    print(name, path)
