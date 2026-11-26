# Los efectos del partido, sintetizados (sin grabaciones, asistidos por IA):
# numpy hace la onda y ffmpeg la pasa a ogg vorbis (games/soccer-chapa/game/assets/sfx/<nombre>.ogg,
# mono, 22050 Hz). Los usa games/soccer-chapa/game/src/audio/sfx.js.
# Estilo arcade (los juegos de SEGA como Monkey Ball): tonos alegres y con
# rebote, no grabaciones realistas.
#   flick      el golpe del dedo: "pok" (el de los menus)
#   flick1..3  el golpe del dedo al tirar: tres distintos (mas o menos
#              grave, con mas o menos chasquido de la uña)
#   cap        chapa contra chapa: "bonk" elastico
#   tick       moverse por las listas: un "tic" corto y suave, de plastico
#   flip       una carta que se da la vuelta: el roce del carton ("fsst")
#   tear       el sobre que se rasga: aluminio que cruje y se rompe
#   ball       el "tin" de campanita de los menus
#   clink1..3  chapa contra el balon, fuerte: acero contra metal, seco y
#              redondo, sin cola aguda (tres distintos: el juego elige uno)
#   tap1..3    chapa contra el balon, suave: mas grave y apagado por el fieltro
#   thump      el golpe grave que se suma en los choques muy fuertes
#   roll       el balon rodando (en bucle; el juego le sube el volumen con
#              la velocidad)
#   pass       una campanilla (el juego la sube de tono con cada pase)
#   go         el saque: tres pitidos y el "ya"
#   goal       la fanfarria del gol
#   whistle    el silbato del arbitro (el saque y el gol)
#   whistle_end  el final del partido: dos pitidos cortos y uno largo
#   clap       una tanda de aplausos (los pases seguidos)
# El publico (crowd, ooh, cheer, rise, chant) no sale de aqui: es grabado
# (crowd_rec.py).
#   python3 sfx.py <carpeta>
import os
import subprocess
import sys
import numpy as np

OUT = sys.argv[1]
SR = 22050
rng = np.random.default_rng(7)
os.makedirs(OUT, exist_ok=True)


def t(sec):
    return np.arange(int(SR * sec)) / SR


def band(x, lo, hi):
    # filtro paso banda en frecuencia (FFT): deja de lo a hi Hz
    f = np.fft.rfftfreq(len(x), 1 / SR)
    X = np.fft.rfft(x)
    X[(f < lo) | (f > hi)] = 0
    return np.fft.irfft(X, len(x))


def noise(sec):
    return rng.standard_normal(int(SR * sec))


def env(n, attack, decay):
    # subida lineal de attack s y caida exponencial con constante decay s
    x = np.arange(n) / SR
    a = np.clip(x / max(attack, 1e-4), 0, 1)
    return a * np.exp(-np.maximum(0, x - attack) / decay)


def norm(x, peak=0.9):
    return x / (np.max(np.abs(x)) + 1e-9) * peak


def loud(x, rms=0.16, peak=0.95):
    # al mismo volumen medio (rms), sin pasar de peak: los golpes cortos se
    # oyen igual de fuertes que los largos
    x = x / (np.sqrt(np.mean(x ** 2)) + 1e-9) * rms
    return np.clip(x, -peak, peak)


def save(x, name, loop=False):
    if loop:                                   # cruza el final con el principio: sin salto
        n = int(SR * 0.4)
        x = x.copy()
        fade = np.linspace(0, 1, n)
        x[:n] = x[:n] * fade + x[-n:] * (1 - fade)
        x = x[:-n]
    pcm = (np.clip(x, -1, 1) * 32767).astype('<i2').tobytes()
    path = os.path.join(OUT, name + '.ogg')
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-f', 's16le', '-ar', str(SR), '-ac', '1',
                    '-i', '-', '-c:a', 'libvorbis', '-q:a', '3', path], input=pcm, check=True)


def ring(sec, freqs, decay, click=0.6):
    # un golpe de metal: parciales inarmonicos que se apagan, mas un chasquido
    x = t(sec)
    s = sum(a * np.sin(2 * np.pi * f * x) * np.exp(-x / (decay * d)) for f, a, d in freqs)
    tick = band(noise(sec), 2000, 9000) * env(len(x), 0.0005, 0.004)
    return s + tick * click


def sweep(sec, f0, f1, shape=1.0):
    # un tono que va de f0 a f1 Hz (shape > 1: cae rapido al principio)
    x = t(sec)
    f = f1 + (f0 - f1) * (1 - x / sec) ** shape
    return np.sin(2 * np.pi * np.cumsum(f) / SR)


def bell(sec, f, ratio=3.5, index=2.5, decay=0.25):
    # una campanita FM: brillante al principio y que se apaga
    x = t(sec)
    e = np.exp(-x / decay)
    return np.sin(2 * np.pi * f * x + index * e * np.sin(2 * np.pi * f * ratio * x)) * e


def square(sec, f):
    return np.sign(np.sin(2 * np.pi * f * t(sec)))


# el golpe del dedo: "pok"
pok = sweep(0.09, 900, 260, 3) * env(int(SR * 0.09), 0.001, 0.03)
pok += band(noise(0.09), 2000, 8000) * env(int(SR * 0.09), 0.0005, 0.004) * 0.4
save(norm(pok), 'flick')
for i, (f0, f1, dec, nail) in enumerate(((820, 230, 0.028, 0.5), (980, 300, 0.024, 0.35), (720, 200, 0.034, 0.6))):
    n = int(SR * 0.1)
    x = sweep(0.1, f0, f1, 3) * env(n, 0.001, dec)
    x += band(noise(0.1), 1800, 7500) * env(n, 0.0005, 0.004) * nail          # la uña
    x += sweep(0.1, 180, 90, 2) * env(n, 0.001, 0.02) * 0.35                 # el golpe sobre el fieltro
    save(loud(x, 0.17), 'flick%d' % (i + 1))

# chapa contra chapa: "bonk" elastico (dos tonos que caen y un poco de cuadrada)
n = int(SR * 0.16)
bonk = (sweep(0.16, 620, 330, 2.5) + 0.5 * sweep(0.16, 1240, 660, 2.5)) * env(n, 0.001, 0.05)
bonk += square(0.16, 330) * env(n, 0.001, 0.02) * 0.15
save(norm(bonk), 'cap')

# moverse por las listas: "tic" corto, suave y sin cola
n = int(SR * 0.035)
tk = np.sin(2 * np.pi * 1900 * t(0.035)) * env(n, 0.0005, 0.006) + band(noise(0.035), 2500, 7000) * env(n, 0.0003, 0.002) * 0.4
save(loud(tk, 0.1, 0.5), 'tick')

# una carta que se da la vuelta: el roce del carton, que sube y baja
n = int(SR * 0.14)
x = t(0.14)
sw = band(noise(0.14), 1500, 6500) * np.sin(np.pi * np.clip(x / 0.12, 0, 1)) ** 2
sw += band(noise(0.14), 300, 1200) * env(n, 0.001, 0.01) * 0.3                 # el toque del borde
save(loud(sw, 0.11, 0.6), 'flip')

# el sobre que se rasga: crujidos de aluminio (chasquidos al azar, cada vez
# mas seguidos) y el tiron final
n = int(SR * 0.45)
crk = np.zeros(n)
for k in range(70):
    at = int(n * (1 - (1 - rng.uniform()) ** 1.6) * 0.85); ln = int(SR * rng.uniform(0.002, 0.008))
    crk[at:at + ln] += rng.standard_normal(min(ln, n - at)) * rng.uniform(0.3, 1.0)
crk = band(crk, 1500, 9000)
rip = band(noise(0.45), 800, 6000) * np.clip((t(0.45) - 0.3) / 0.05, 0, 1) * env(n, 0.0, 0.09)
save(loud(crk + rip * 0.8, 0.16, 0.9), 'tear')

# la campanita de los menus: "tin"
save(norm(bell(0.4, 1760, 3.5, 2.0, 0.09) + bell(0.4, 2640, 2.0, 1.0, 0.06) * 0.4, 0.8), 'ball')


# chapa contra el balon: parciales de metal inarmonicos al azar alrededor de
# base (Hz), que se apagan en decay s, con su chasquido y un cuerpo grave
def clink(base, decay, bright, click=0.6, body=0.6):
    freqs = [(base * r * (1 + rng.uniform(-0.04, 0.04)), a, d)
             for r, a, d in ((1.0, 1.0, 1.0), (1.59, 0.6, 0.7), (2.33, 0.45 * bright, 0.5), (3.12, 0.3 * bright, 0.35))]
    x = ring(0.35, freqs, decay, click)
    return x + sweep(0.35, 320, 180, 2) * env(len(x), 0.0008, 0.018) * body


# fuertes: un toque de metal delicado, no un "tin" agudo (grave, corto, casi
# sin chasquido ni brillo, y algo de cuerpo)
for i, (base, decay) in enumerate(((1550, 0.045), (1700, 0.04), (1850, 0.05))):
    save(loud(band(clink(base, decay, 0.25, 0.12, 0.7), 80, 5000), 0.12), 'clink%d' % (i + 1))
for i, base in enumerate((1250, 1450, 1650)):              # suaves: graves, cortos y sin brillo
    x = clink(base, 0.05, 0.2)
    x += band(noise(0.35), 150, 900) * env(len(x), 0.001, 0.02) * 0.5   # el fieltro
    save(loud(band(x, 80, 4000), 0.15), 'tap%d' % (i + 1))

# el golpe grave de los choques fuertes: "tok" sobre el fieltro
n = int(SR * 0.14)
tok = sweep(0.14, 170, 70, 2) * env(n, 0.001, 0.04) + band(noise(0.14), 80, 500) * env(n, 0.0005, 0.02) * 0.6
save(norm(tok, 0.8), 'thump')

# el balon rodando: un zumbido suave (en bucle)
r = band(noise(4), 120, 900) * (1 + 0.3 * np.sin(2 * np.pi * 7 * t(4)))
save(norm(r, 0.5), 'roll', loop=True)

# el pase: una campanilla (el juego la sube de tono con cada pase seguido)
save(norm(bell(0.6, 1318, 2.0, 1.5, 0.18) + bell(0.6, 1976, 2.0, 1.0, 0.12) * 0.5, 0.8), 'pass')

# el saque: tres pitidos y el "ya", mas agudo y largo (cuenta atras)
def beep(sec, f):
    n = int(SR * sec)
    return (square(sec, f) * 0.35 + np.sin(2 * np.pi * f * t(sec))) * env(n, 0.002, sec * 0.5)


gap = np.zeros(int(SR * 0.28))
save(norm(np.concatenate([beep(0.12, 880), gap, beep(0.12, 880), gap, beep(0.12, 880), gap,
                          beep(0.5, 1760)]), 0.6), 'go')

# el gol: fanfarria que sube (do mi sol do) con campanas y cuadrada
notes = [523, 659, 784, 1046]
fan = np.concatenate([(bell(0.14, f, 2.0, 1.2, 0.12) + square(0.14, f) * 0.25 * env(int(SR * 0.14), 0.002, 0.08))
                      for f in notes[:3]])
last = bell(1.2, 1046, 2.0, 1.5, 0.5) + bell(1.2, 1318, 2.0, 1.0, 0.45) * 0.6 + bell(1.2, 1568, 2.0, 1.0, 0.4) * 0.5
last += square(1.2, 523) * env(int(SR * 1.2), 0.005, 0.35) * 0.2
save(norm(np.concatenate([fan, last]), 0.75), 'goal')

# el silbato de arbitro: un tono agudo y brillante (3 kHz) que la bolita de
# dentro hace trinar (el tono y el volumen tiemblan unas 30 veces por
# segundo), con el soplido de aire; entra rapido y se apaga suave (no se
# corta). whistle: un pitido; whistle_end: el final, dos cortos y uno largo
def whistle(sec):
    x = t(sec)
    trill = np.sin(2 * np.pi * 29 * x)
    f = 3000 + 140 * trill
    ph = 2 * np.pi * np.cumsum(f) / SR
    s = np.sin(ph) + 0.35 * np.sin(2 * ph) + 0.12 * np.sin(3 * ph)
    s *= 0.72 + 0.28 * trill
    s += band(noise(sec), 2400, 4200) * 0.18
    rel = 0.12
    e = np.clip(x / 0.012, 0, 1) * np.where(x > sec - rel, np.exp(-(x - (sec - rel)) / 0.035), 1.0)
    return s * e


save(norm(whistle(0.55), 0.6), 'whistle')
gap = np.zeros(int(SR * 0.12))
save(norm(np.concatenate([whistle(0.22), gap, whistle(0.22), gap, whistle(1.0)]), 0.6), 'whistle_end')


# los aplausos: muchas palmas sueltas (chasquidos cortos al azar) que se apagan
def claps(sec, n):
    x = np.zeros(int(SR * sec))
    one = band(noise(0.03), 900, 5000) * env(int(SR * 0.03), 0.0005, 0.008)
    for k in range(n):
        i = int(rng.uniform(0, sec * 0.85) * SR)
        x[i:i + len(one)] += one * rng.uniform(0.3, 1)
    return x * env(len(x), 0.08, sec * 0.45)


save(norm(claps(1.8, 260), 0.5), 'clap')

print('sonidos ok')
