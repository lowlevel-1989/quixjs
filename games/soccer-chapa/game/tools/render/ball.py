# Las bolas del juego, vistas desde arriba, con su sombra y fondo
# transparente (games/soccer-chapa/game/assets/balls/<estilo>.png).
#   blender -b -P ball.py -- <estilo> <salida.png> [base|gloss]
# base: la bola iluminada, lisa (sin su piel) y con sombra; gloss: solo el
# brillo, para ponerlo encima de la piel que rueda (games/soccer-chapa/game/src/match/ball.js)
# Estilos: steel, onyx, pearl, gold, marble, glass. Se refleja un estudio:
# paneles de luz que se ven como rectangulos nitidos en la bola (lo que la
# hace elegante) y que la camara no ve.
import bpy, sys, os, math

argv = sys.argv[sys.argv.index('--') + 1:]
STYLE, PNG = argv[0], argv[1]
PASS = argv[2] if len(argv) > 2 else 'base'

# las ayudas de scene.py (materiales, camara) sin montar su escena
here = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(here, 'scene.py')).read()
src = src[:src.index('# ----------------------------------------------------------------- montaje')]
sys.argv = ['x', '--', 'ball', '.', '/dev/null', '128']
exec(compile(src, 'scene.py', 'exec'))

R = 0.08
sc = bpy.context.scene
sc.render.resolution_x = sc.render.resolution_y = 256
sc.render.film_transparent = True
sc.cycles.film_transparent_glass = True   # el cristal deja ver lo de detras (el campo del juego)
sc.cycles.samples = 128


def material(name):
    m = bpy.data.materials.new(name); m.use_nodes = True
    return m, principled(m.node_tree)


def panel(loc, size, strength):
    # un panel de luz: lo refleja la bola, la camara no lo ve
    bpy.ops.mesh.primitive_plane_add(size=1, location=loc)
    p = bpy.context.object; p.scale = (size[0], size[1], 1)
    p.rotation_euler = __import__('mathutils').Vector([-v for v in loc]).to_track_quat('Z', 'Y').to_euler()
    m, b = material('panel')
    setin(b, ['Emission Color', 'Emission'], (1, 0.98, 0.95, 1)); setin(b, ['Emission Strength'], strength)
    b.inputs['Base Color'].default_value = (0, 0, 0, 1)
    p.data.materials.append(m)
    p.visible_camera = False; p.visible_shadow = False


def studio():
    # el estudio: claro arriba, gris en el horizonte y el verde del fieltro
    # abajo (el metal refleja este degradado)
    wt = sc.world.node_tree; bg = wt.nodes['Background']
    tc = node(wt, 'ShaderNodeTexCoord'); sep = node(wt, 'ShaderNodeSeparateXYZ')
    ramp = node(wt, 'ShaderNodeValToRGB'); els = ramp.color_ramp.elements
    els[0].position = 0.0; els[0].color = (0.02, 0.05, 0.03, 1)
    els[1].position = 1.0; els[1].color = (0.85, 0.86, 0.85, 1)
    els.new(0.5).color = (0.12, 0.14, 0.14, 1)
    els.new(0.62).color = (0.35, 0.37, 0.37, 1)
    mapz = node(wt, 'ShaderNodeMapRange'); mapz.inputs['From Min'].default_value = -1.0
    wt.links.new(tc.outputs['Generated'], sep.inputs[0]); wt.links.new(sep.outputs['Z'], mapz.inputs['Value'])
    wt.links.new(mapz.outputs['Result'], ramp.inputs['Fac']); wt.links.new(ramp.outputs['Color'], bg.inputs['Color'])
    bg.inputs[1].default_value = 1.0
    panel((-0.55, 0.5, 0.8), (0.9, 0.6), 10)       # la caja de luz grande, arriba a la izquierda
    panel((0.75, -0.25, 0.55), (0.18, 0.9), 5)     # una tira alargada a la derecha
    if STYLE == 'gold':
        panel((0.2, -0.7, 0.35), (1.2, 0.5), 6)    # el oro refleja el fieltro oscuro: luz de frente
    # la luz que da la sombra: casi desde arriba (la bola no flota)
    bpy.ops.object.light_add(type='AREA', location=(-0.4, 0.35, 3)); l = bpy.context.object
    l.data.size = 1.0; l.data.energy = 110
    l.visible_glossy = False   # da la sombra, pero no se refleja
    # el fieltro solo recoge la sombra
    bpy.ops.mesh.primitive_plane_add(size=4); c = bpy.context.object
    c.is_shadow_catcher = True; c.visible_glossy = False


def styled(style):
    m, b = material(style)
    if style == 'steel':
        b.inputs['Base Color'].default_value = (0.9, 0.91, 0.93, 1)
        b.inputs['Metallic'].default_value = 1; b.inputs['Roughness'].default_value = 0.16
    elif style == 'gold':       # oro pulido: con un panel mas que lo aclara (studio)
        b.inputs['Base Color'].default_value = (1.0, 0.8, 0.36, 1)
        b.inputs['Metallic'].default_value = 1; b.inputs['Roughness'].default_value = 0.1
    elif style == 'onyx':
        b.inputs['Base Color'].default_value = (0.008, 0.009, 0.01, 1)
        b.inputs['Roughness'].default_value = 0.15
        setin(b, ['Coat Weight', 'Clearcoat'], 1.0); setin(b, ['Coat Roughness', 'Clearcoat Roughness'], 0.12)
    elif style == 'pearl':
        b.inputs['Base Color'].default_value = (0.9, 0.87, 0.82, 1)
        b.inputs['Roughness'].default_value = 0.3
        setin(b, ['Coat Weight', 'Clearcoat'], 1.0); setin(b, ['Coat Roughness', 'Clearcoat Roughness'], 0.04)
        setin(b, ['Thin Film Thickness'], 420.0)
    elif style == 'marble':     # piedra clara pulida (las vetas ruedan encima)
        b.inputs['Base Color'].default_value = (0.9, 0.92, 0.95, 1)
        b.inputs['Roughness'].default_value = 0.12
        setin(b, ['Coat Weight', 'Clearcoat'], 1.0)
    elif style == 'soccer':     # cuero blanco satinado (los paneles ruedan encima)
        b.inputs['Base Color'].default_value = (0.93, 0.93, 0.91, 1)
        b.inputs['Roughness'].default_value = 0.38
        setin(b, ['Coat Weight', 'Clearcoat'], 0.25); setin(b, ['Coat Roughness', 'Clearcoat Roughness'], 0.2)
    elif style == 'glass':      # cristal de verdad: deja ver el campo (film_transparent_glass);
        b.inputs['Base Color'].default_value = (0.9, 0.97, 1.0, 1)   # las aspas de color van en su piel
        b.inputs['Roughness'].default_value = 0.0
        setin(b, ['Transmission Weight', 'Transmission'], 1.0); setin(b, ['IOR'], 1.45)
    if PASS == 'gloss':
        # solo el brillo: la base en negro (sumado en pantalla, el negro no cambia nada)
        b.inputs['Base Color'].default_value = (0, 0, 0, 1)
        setin(b, ['Transmission Weight', 'Transmission'], 0.0)
    return m


for ob in list(bpy.data.objects):
    bpy.data.objects.remove(ob)
studio()
bpy.ops.mesh.primitive_uv_sphere_add(radius=R, location=(0, 0, R), segments=128, ring_count=64)
bpy.ops.object.shade_smooth()
bpy.context.object.data.materials.append(styled(STYLE))
camera(R * 2 * 2.4)   # con sitio para la sombra (games/soccer-chapa/game/src/match/ball.js: RENDER)
sc.render.filepath = PNG
bpy.ops.render.render(write_still=True)
print('ball', STYLE)
