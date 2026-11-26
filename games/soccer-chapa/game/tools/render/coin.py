# La moneda del sorteo del saque, en 3D: oro pulido con el relieve de cada
# cara (coin_relief.py) y el canto estriado, vista desde arriba mientras da
# una vuelta entera sobre el eje x (COIN_FRAMES imagenes: la 0, cara arriba; la
# mitad, cruz). Se refleja un estudio como las bolas (ball.py). Sale en una
# tira: games/soccer-chapa/game/assets/ui/coin_flip.png (games/soccer-chapa/game/src/ui/toss.js elige la imagen del giro).
#   python3 coin_relief.py <relieve>
#   blender -b -P coin.py -- <relieve> <salida.png>
import bpy, sys, os, math
argv = sys.argv[sys.argv.index('--') + 1:]
COIN_RELIEF, COIN_OUT = argv[0], argv[1]   # otros nombres: scene.py (abajo) pone los suyos
COIN_FRAMES, COIN_SIZE = int(os.environ.get('COIN_FRAMES', 32)), 192

here = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(here, 'scene.py')).read()
src = src[:src.index('# ----------------------------------------------------------------- montaje')]
sys.argv = ['x', '--', 'ball', '.', '/dev/null', '64']
exec(compile(src, 'scene.py', 'exec'))
for ob in list(bpy.data.objects):
    bpy.data.objects.remove(ob)
sc = bpy.context.scene
sc.render.resolution_x = sc.render.resolution_y = COIN_SIZE
sc.render.film_transparent = True
sc.cycles.samples = 64


def gold(name, relief=None, flip=False):
    # oro pulido; con relief, su imagen como relieve (bump) sobre la cara
    m = bpy.data.materials.new(name); m.use_nodes = True; t = m.node_tree; p = principled(t)
    p.inputs['Base Color'].default_value = (1.0, 0.6, 0.16, 1)
    p.inputs['Metallic'].default_value = 1; p.inputs['Roughness'].default_value = 0.22
    if relief:
        co = node(t, 'ShaderNodeTexCoord'); mp = node(t, 'ShaderNodeMapping')
        mp.inputs['Location'].default_value = (0.5, 0.5, 0)
        mp.inputs['Scale'].default_value = (0.5, 0.5 * (-1 if flip else 1), 1)   # la cruz se ve tras girar en x
        img = node(t, 'ShaderNodeTexImage'); img.image = bpy.data.images.load(relief)
        img.extension = 'CLIP'
        bump = node(t, 'ShaderNodeBump'); bump.inputs['Strength'].default_value = 0.9
        bump.inputs['Distance'].default_value = 0.06
        t.links.new(co.outputs['Object'], mp.inputs['Vector']); t.links.new(mp.outputs['Vector'], img.inputs['Vector'])
        t.links.new(img.outputs['Color'], bump.inputs['Height']); t.links.new(bump.outputs['Normal'], p.inputs['Normal'])
    return m


def studio():
    # cielo claro arriba y oscuro abajo (el oro lo refleja) y dos paneles de luz
    wt = sc.world.node_tree; bg = wt.nodes['Background']
    bg.inputs[0].default_value = (0.25, 0.22, 0.2, 1); bg.inputs[1].default_value = 0.5
    for loc, size, energy in (((-1.6, 1.4, 3), (2.4, 1.6), 900), ((2, -0.8, 2), (0.6, 2.4), 400)):
        bpy.ops.object.light_add(type='AREA', location=loc); l = bpy.context.object
        l.data.size = size[0]; l.data.size_y = size[1]; l.data.shape = 'RECTANGLE'; l.data.energy = energy
        l.rotation_euler = __import__('mathutils').Vector([-v for v in loc]).to_track_quat('-Z', 'Y').to_euler()


studio()
bpy.ops.mesh.primitive_cylinder_add(vertices=160, radius=1, depth=0.14)
coin = bpy.context.object
bev = coin.modifiers.new('bevel', 'BEVEL'); bev.width = 0.03; bev.segments = 4
bpy.ops.object.shade_smooth()
coin.data.materials.append(gold('heads', os.path.join(COIN_RELIEF, 'heads.png')))
coin.data.materials.append(gold('tails', os.path.join(COIN_RELIEF, 'tails.png'), flip=True))
coin.data.materials.append(gold('edge'))
for f in coin.data.polygons:
    f.material_index = 0 if f.normal.z > 0.9 else 1 if f.normal.z < -0.9 else 2
camera(2.4)

frames = []
for i in range(COIN_FRAMES):
    coin.rotation_euler = (i / COIN_FRAMES * 2 * math.pi, 0, 0)
    path = COIN_OUT + '.%02d.png' % i
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)
    frames.append(path)
# la tira, con python dentro de blender (sin PIL): una imagen al lado de otra
strip_img = bpy.data.images.new('strip', COIN_SIZE * COIN_FRAMES, COIN_SIZE, alpha=True)
px = [0.0] * (COIN_SIZE * COIN_FRAMES * COIN_SIZE * 4)
for i, path in enumerate(frames):
    im = bpy.data.images.load(path); src_px = list(im.pixels)
    for y in range(COIN_SIZE):
        row = src_px[y * COIN_SIZE * 4:(y + 1) * COIN_SIZE * 4]
        o = (y * COIN_SIZE * COIN_FRAMES + i * COIN_SIZE) * 4
        px[o:o + COIN_SIZE * 4] = row
    os.remove(path)
strip_img.pixels = px
strip_img.filepath_raw = COIN_OUT; strip_img.file_format = 'PNG'; strip_img.save()
print('moneda ok')
