# Capas de la chapa para componerla en el juego (PNG con alfa, vista cenital):
#   cap_body.png    el volumen en blanco con luz cenital (gira con la chapa)
#   cap_light.png   la luz lateral fija: gris 128 = sin cambio (overlay)
#   cap_gloss.png   el brillo del barniz (screen), fijo
#   cap_shadow.png  la sombra en el fieltro (negro con alfa), fija
#   cap_top.png     la mascara de la cara (donde va la franela)
#   blender -b -P passes.py -- <carpeta> <tamano>
import bpy, sys, os, math
argv = sys.argv[sys.argv.index('--') + 1:]
SPR, SIZE = argv[0], int(argv[1])
os.makedirs(SPR, exist_ok=True)

# reutiliza la malla y las ayudas de scene.py sin montar la escena
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'scene.py')).read()
src = src[:src.index('# ----------------------------------------------------------------- montaje')]
sys.argv = ['x', '--', 'cap', '.', '/dev/null', '1']
exec(compile(src, 'scene.py', 'exec'))

sc = bpy.context.scene
sc.render.resolution_x = sc.render.resolution_y = SIZE
sc.render.film_transparent = True
sc.cycles.samples = 64
try: sc.view_settings.view_transform = 'Standard'; sc.view_settings.look = 'None'; sc.view_settings.exposure = -1.2
except Exception: pass

def mat(name, base, rough=0.5, metal=0.0, coat=0.0, emit=None):
    m = bpy.data.materials.new(name); m.use_nodes = True; t = m.node_tree; p = principled(t)
    p.inputs['Base Color'].default_value = base + (1,)
    p.inputs['Roughness'].default_value = rough; p.inputs['Metallic'].default_value = metal
    setin(p, ['Coat Weight', 'Clearcoat'], coat)
    if emit is not None:
        setin(p, ['Emission Color', 'Emission'], emit + (1,)); setin(p, ['Emission Strength'], 1.0)
    return m

me = cap_mesh()
o = bpy.data.objects.new('cap', me); bpy.context.collection.objects.link(o)
camera(2 * R * 1.25)   # la chapa con un poco de margen para la sombra

def setmats(top, skirt):
    o.data.materials.clear(); o.data.materials.append(top); o.data.materials.append(skirt)

def clear_lights():
    for ob in list(bpy.data.objects):
        if ob.type == 'LIGHT': bpy.data.objects.remove(ob)

def render(name):
    sc.render.filepath = os.path.join(SPR, name)
    bpy.ops.render.render(write_still=True)

world = sc.world.node_tree.nodes['Background']

# 1. volumen: blanco, luz cenital grande y cielo uniforme (no cambia al girar)
clear_lights(); world.inputs[1].default_value = 0.45; world.inputs[0].default_value = (1, 1, 1, 1)
white = mat('w', (0.8, 0.8, 0.8), rough=0.6)
setmats(white, mat('ws', (0.62, 0.62, 0.62), rough=0.45, metal=0.3))
bpy.ops.object.light_add(type='AREA', location=(0, 0, 4)); bpy.context.object.data.size = 3; bpy.context.object.data.energy = 18
render('cap_body.png')

# 2. luz lateral: la misma chapa con la luz del campo; el juego la usa en overlay
clear_lights(); world.inputs[1].default_value = 0.25; world.inputs[0].default_value = (0.02, 0.022, 0.02, 1)
lights()
render('cap_lit.png')

# 3. brillo: solo el reflejo del barniz (base negra)
setmats(mat('g', (0, 0, 0), rough=0.3, coat=0.6), mat('gs', (0, 0, 0), rough=0.28, metal=0.55, coat=0.5))
render('cap_gloss.png')

# 5. sombra: el fieltro como receptor de sombras, la chapa invisible a la camara
setmats(white, white)
lights(); world.inputs[1].default_value = 0.25
bpy.ops.mesh.primitive_plane_add(size=4, location=(0, 0, 0)); pl = bpy.context.object
pl.is_shadow_catcher = True
o.visible_camera = False
render('cap_shadow.png')
print('passes ok')
