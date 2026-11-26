# La bola rodando: 24 imagenes de la piel de un estilo (skins.py) vista
# desde arriba mientras gira hacia la derecha (eje y). Sin luz: la piel con
# su color y su alfa; el juego la orienta hacia donde va la bola, avanza de
# imagen con lo que rueda y pone el brillo fijo encima (games/soccer-chapa/game/src/match/ball.js).
# Sale en una tira: games/soccer-chapa/game/assets/balls/<estilo>_roll.png
#   blender -b -P roll.py -- <piel.png> <salida.png>
import bpy, sys, math, os
argv = sys.argv[sys.argv.index('--') + 1:]
SKIN, OUT = argv[0], argv[1]
FRAMES, SIZE = 24, 128

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.engine = 'CYCLES'; sc.cycles.samples = 16; sc.cycles.use_denoising = False
sc.render.resolution_x = sc.render.resolution_y = SIZE
sc.render.film_transparent = True
sc.view_settings.view_transform = 'Standard'
sc.world = bpy.data.worlds.new('w')

bpy.ops.mesh.primitive_uv_sphere_add(radius=1, segments=128, ring_count=64)
ball = bpy.context.object
bpy.ops.object.shade_smooth()
m = bpy.data.materials.new('skin'); m.use_nodes = True; t = m.node_tree
for n in list(t.nodes): t.nodes.remove(n)
out = t.nodes.new('ShaderNodeOutputMaterial')
img = t.nodes.new('ShaderNodeTexImage'); img.image = bpy.data.images.load(SKIN)
emit = t.nodes.new('ShaderNodeEmission')
t.links.new(img.outputs['Color'], emit.inputs['Color'])
clear = t.nodes.new('ShaderNodeBsdfTransparent'); mix = t.nodes.new('ShaderNodeMixShader')
# solo la cara de delante: la de atras no se ve a traves de la bola
geo = t.nodes.new('ShaderNodeNewGeometry'); front = t.nodes.new('ShaderNodeMath'); front.operation = 'SUBTRACT'
front.inputs[0].default_value = 1.0
t.links.new(geo.outputs['Backfacing'], front.inputs[1])
fac = t.nodes.new('ShaderNodeMath'); fac.operation = 'MULTIPLY'
t.links.new(img.outputs['Alpha'], fac.inputs[0]); t.links.new(front.outputs[0], fac.inputs[1])
t.links.new(fac.outputs[0], mix.inputs['Fac'])
t.links.new(clear.outputs[0], mix.inputs[1]); t.links.new(emit.outputs[0], mix.inputs[2])
t.links.new(mix.outputs[0], out.inputs['Surface'])
m.blend_method = 'BLEND' if hasattr(m, 'blend_method') else None
ball.data.materials.append(m)

bpy.ops.object.camera_add(location=(0, 0, 10))
cam = bpy.context.object; cam.data.type = 'ORTHO'; cam.data.ortho_scale = 2.0
sc.camera = cam

frames = []
for i in range(FRAMES):
    ball.rotation_euler = (0, i / FRAMES * 2 * math.pi, 0)
    path = os.path.join(os.path.dirname(OUT), '_roll_%02d.png' % i)
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)
    frames.append(path)

strip = bpy.data.images.new('strip', SIZE * FRAMES, SIZE, alpha=True)
pix = [0.0] * (SIZE * FRAMES * SIZE * 4)
for i, f in enumerate(frames):
    im = bpy.data.images.load(f); p = list(im.pixels)
    for y in range(SIZE):
        row = p[y * SIZE * 4:(y + 1) * SIZE * 4]
        start = (y * SIZE * FRAMES + i * SIZE) * 4
        pix[start:start + SIZE * 4] = row
    os.remove(f)
strip.pixels = pix
strip.filepath_raw = OUT; strip.file_format = 'PNG'; strip.save()
print('roll ok')
