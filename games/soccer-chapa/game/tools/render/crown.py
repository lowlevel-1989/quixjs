# La corona del torneo, en 3D (como la moneda: coin.py): un aro de oro
# pulido con cinco puntas, una bola en cada punta y joyas de cristal en el
# aro (rojo, azul y verde), vista de frente y un poco desde arriba, con el
# estudio que reflejan las bolas. Sin fondo: games/soccer-chapa/game/assets/ui/crown.png.
#   blender -b -P crown.py -- <salida.png>
import bpy, sys, os, math, bmesh
argv = sys.argv[sys.argv.index('--') + 1:]
CROWN_OUT = argv[0]
CROWN_SIZE = 384

here = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(here, 'scene.py')).read()
src = src[:src.index('# ----------------------------------------------------------------- montaje')]
sys.argv = ['x', '--', 'ball', '.', '/dev/null', '96']
exec(compile(src, 'scene.py', 'exec'))
for ob in list(bpy.data.objects):
    bpy.data.objects.remove(ob)
sc = bpy.context.scene
sc.render.resolution_x = sc.render.resolution_y = CROWN_SIZE
sc.render.film_transparent = True


def metal(name, color, rough):
    m = bpy.data.materials.new(name); m.use_nodes = True; p = principled(m.node_tree)
    p.inputs['Base Color'].default_value = color; p.inputs['Metallic'].default_value = 1
    p.inputs['Roughness'].default_value = rough
    return m


def gem(name, color):
    m = bpy.data.materials.new(name); m.use_nodes = True; p = principled(m.node_tree)
    p.inputs['Base Color'].default_value = color; p.inputs['Roughness'].default_value = 0.02
    setin(p, ['Transmission Weight', 'Transmission'], 1.0); p.inputs['IOR'].default_value = 1.9
    return m


GOLD = metal('gold', (1.0, 0.71, 0.29, 1), 0.2)
VELVET = bpy.data.materials.new('velvet'); VELVET.use_nodes = True
pv = principled(VELVET.node_tree); pv.inputs['Base Color'].default_value = (0.09, 0.004, 0.012, 1); pv.inputs['Roughness'].default_value = 1.0; setin(pv, ['Specular IOR Level', 'Specular'], 0.05)

R, H, T = 1.0, 0.42, 0.07          # radio del aro, alto de la banda, grosor
# el aro: un tubo con borde redondeado
bpy.ops.mesh.primitive_cylinder_add(vertices=128, radius=R, depth=H, location=(0, 0, H / 2))
band = bpy.context.object
sol = band.modifiers.new('s', 'SOLIDIFY'); sol.thickness = T
bev = band.modifiers.new('b', 'BEVEL'); bev.width = 0.02; bev.segments = 3
bpy.ops.object.modifier_apply(modifier='s')
bm = bmesh.new(); bm.from_mesh(band.data)
for f in [f for f in bm.faces if abs(f.normal.z) > 0.9]:
    bm.faces.remove(f)                                       # sin tapas: es un aro
bm.to_mesh(band.data); bm.free()
bpy.ops.object.shade_smooth(); band.data.materials.append(GOLD)
# el terciopelo de dentro (se ve por arriba)
bpy.ops.mesh.primitive_uv_sphere_add(radius=R * 0.93, location=(0, 0, H * 0.6), segments=48, ring_count=24)
cap = bpy.context.object; cap.scale.z = 0.55; bpy.ops.object.shade_smooth(); cap.data.materials.append(VELVET)
# las cinco puntas: placas triangulares curvadas siguiendo el aro, con una bola
for i in range(5):
    a = i * 2 * math.pi / 5 - math.pi / 2
    bpy.ops.mesh.primitive_cone_add(vertices=4, radius1=0.42, depth=0.85, location=(0, 0, 0))
    pt = bpy.context.object
    pt.scale = (1, 0.12, 1)
    pt.rotation_euler = (0, 0, a + math.pi / 2)
    pt.location = (math.cos(a) * R, math.sin(a) * R, H + 0.36)
    b = pt.modifiers.new('b', 'BEVEL'); b.width = 0.03; b.segments = 3
    bpy.ops.object.shade_smooth(); pt.data.materials.append(GOLD)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.085, location=(math.cos(a) * R, math.sin(a) * R, H + 0.82))
    bpy.ops.object.shade_smooth(); bpy.context.object.data.materials.append(GOLD)
# las joyas del aro, de frente
for i, col in enumerate([(0.8, 0.02, 0.05, 1), (0.02, 0.15, 0.9, 1), (0.02, 0.6, 0.15, 1), (0.02, 0.15, 0.9, 1), (0.8, 0.02, 0.05, 1)]):
    a = -math.pi / 2 + (i - 2) * 0.42
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.09, location=(math.cos(a) * (R + T + 0.02), math.sin(a) * (R + T + 0.02), H / 2), segments=8, ring_count=6)
    g = bpy.context.object; g.scale = (0.6, 0.6, 1) if False else (1, 0.55, 1)
    g.rotation_euler = (0, 0, a); g.data.materials.append(gem('gem%d' % i, col))
    bpy.ops.mesh.primitive_torus_add(major_radius=0.1, minor_radius=0.02, location=g.location, rotation=(math.pi / 2, 0, a + math.pi / 2))
    bpy.ops.object.shade_smooth(); bpy.context.object.data.materials.append(GOLD)

# estudio y camara: de frente, algo desde arriba
wt = sc.world.node_tree; bg = wt.nodes['Background']
bg.inputs[0].default_value = (0.3, 0.28, 0.26, 1); bg.inputs[1].default_value = 0.7
import mathutils
for loc, size, energy in (((-3, -4, 5), 3.0, 1600), ((4, -2, 3), 2.0, 700), ((0, 4, 4), 3.0, 500)):
    bpy.ops.object.light_add(type='AREA', location=loc); l = bpy.context.object
    l.data.size = size; l.data.energy = energy
    l.rotation_euler = mathutils.Vector([-v for v in loc]).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.object.camera_add(location=(0, -6.2, 2.9))
cam = bpy.context.object; cam.data.type = 'ORTHO'; cam.data.ortho_scale = 2.9
cam.rotation_euler = mathutils.Vector((0, 6.2, -2.3)).to_track_quat('-Z', 'Y').to_euler()
sc.camera = cam
sc.render.filepath = CROWN_OUT
bpy.ops.render.render(write_still=True)
print('corona ok')
