# Soccer Chapa: render en Blender (Cycles) de la chapa y el campo (sin
# porterias: cada equipo lleva la suya, goals.py), vistos
# desde arriba (camara ortografica). La chapa esta boca abajo: la cara
# impresa arriba y la falda de 21 pliegues abierta alrededor.
#   blender -b -P scene.py -- <field|caps|cap> <texturas> <salida.png> [muestras]
import bpy, bmesh, math, sys, os, random

argv = sys.argv[sys.argv.index('--') + 1:]
MODE, TEX, OUT = argv[0], argv[1], argv[2]
SAMPLES = int(argv[3]) if len(argv) > 3 else 96
random.seed(4)

R = 0.16      # radio de la chapa (unidades x10: 32 mm de diametro)
H = 0.065     # alto
FLUTES = 21

# ------------------------------------------------------------------ escena
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.render.engine = 'CYCLES'
sc.cycles.device = 'CPU'
sc.cycles.samples = SAMPLES
sc.cycles.use_denoising = True
sc.render.resolution_x, sc.render.resolution_y = (1280, 720) if MODE != 'cap' else (1024, 1024)
sc.render.film_transparent = False
try:
    sc.view_settings.view_transform = 'AgX'
    sc.view_settings.look = 'AgX - Medium High Contrast'
except Exception:
    pass
world = bpy.data.worlds.new('w'); sc.world = world
world.use_nodes = True
world.node_tree.nodes['Background'].inputs[0].default_value = (0.02, 0.022, 0.02, 1)
world.node_tree.nodes['Background'].inputs[1].default_value = 0.25

def node(tree, kind, **kw):
    n = tree.nodes.new(kind)
    for k, v in kw.items():
        if k in n.inputs: n.inputs[k].default_value = v
        else: setattr(n, k, v)
    return n

def principled(tree):
    for n in tree.nodes:
        if n.type == 'BSDF_PRINCIPLED': return n
    return node(tree, 'ShaderNodeBsdfPrincipled')

def setin(n, names, v):
    for name in names:
        if name in n.inputs: n.inputs[name].default_value = v; return

# ----------------------------------------------------------------- fieltro
def felt_material():
    m = bpy.data.materials.new('felt'); m.use_nodes = True; t = m.node_tree
    p = principled(t)
    coord = node(t, 'ShaderNodeTexCoord')
    # fibras: ruido muy fino estirado
    mapf = node(t, 'ShaderNodeMapping'); mapf.inputs['Scale'].default_value = (1, 3, 1)
    t.links.new(coord.outputs['Object'], mapf.inputs['Vector'])
    fib = node(t, 'ShaderNodeTexNoise'); fib.inputs['Scale'].default_value = 420; fib.inputs['Detail'].default_value = 8; fib.inputs['Roughness'].default_value = 0.75
    t.links.new(mapf.outputs['Vector'], fib.inputs['Vector'])
    blot = node(t, 'ShaderNodeTexNoise'); blot.inputs['Scale'].default_value = 2.2; blot.inputs['Detail'].default_value = 4
    t.links.new(coord.outputs['Object'], blot.inputs['Vector'])
    ramp = node(t, 'ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].color = (0.005, 0.016, 0.009, 1)
    ramp.color_ramp.elements[1].color = (0.022, 0.055, 0.032, 1)
    mix = node(t, 'ShaderNodeMath', operation='MULTIPLY_ADD')
    mix.inputs[1].default_value = 0.65; mix.inputs[2].default_value = 0.0
    t.links.new(fib.outputs['Fac'], mix.inputs[0])
    add = node(t, 'ShaderNodeMath', operation='ADD')
    t.links.new(mix.outputs[0], add.inputs[0])
    b2 = node(t, 'ShaderNodeMath', operation='MULTIPLY'); b2.inputs[1].default_value = 0.35
    t.links.new(blot.outputs['Fac'], b2.inputs[0]); t.links.new(b2.outputs[0], add.inputs[1])
    t.links.new(add.outputs[0], ramp.inputs['Fac'])
    t.links.new(ramp.outputs['Color'], p.inputs['Base Color'])
    p.inputs['Roughness'].default_value = 1.0
    setin(p, ['Sheen Weight', 'Sheen'], 0.35)
    setin(p, ['Sheen Roughness'], 0.6)
    bump = node(t, 'ShaderNodeBump'); bump.inputs['Strength'].default_value = 0.9; bump.inputs['Distance'].default_value = 0.006
    t.links.new(fib.outputs['Fac'], bump.inputs['Height'])
    t.links.new(bump.outputs['Normal'], p.inputs['Normal'])
    return m

def plane(name, w, h, z, mat):
    bpy.ops.mesh.primitive_plane_add(size=1, location=(0, 0, z))
    o = bpy.context.object; o.name = name; o.scale = (w, h, 1)
    o.data.materials.append(mat)
    return o

# ------------------------------------------------------------------- tiza
def chalk_material():
    m = bpy.data.materials.new('chalk'); m.use_nodes = True; t = m.node_tree
    out = t.nodes['Material Output']; p = principled(t)
    p.inputs['Base Color'].default_value = (0.70, 0.72, 0.68, 1); p.inputs['Roughness'].default_value = 1
    tr = node(t, 'ShaderNodeBsdfTransparent'); mx = node(t, 'ShaderNodeMixShader')
    coord = node(t, 'ShaderNodeTexCoord')
    n = node(t, 'ShaderNodeTexNoise'); n.inputs['Scale'].default_value = 260; n.inputs['Detail'].default_value = 6
    t.links.new(coord.outputs['Object'], n.inputs['Vector'])
    ramp = node(t, 'ShaderNodeValToRGB')
    ramp.color_ramp.elements[0].position = 0.42; ramp.color_ramp.elements[1].position = 0.58
    n.inputs['Roughness'].default_value = 0.8
    t.links.new(n.outputs['Fac'], ramp.inputs['Fac'])
    t.links.new(ramp.outputs['Color'], mx.inputs['Fac'])
    t.links.new(tr.outputs[0], mx.inputs[1]); t.links.new(p.outputs[0], mx.inputs[2])
    t.links.new(mx.outputs[0], out.inputs['Surface'])
    return m

def ring_mesh(name, r0, r1, segs, mat, z=0.0006):
    me = bpy.data.meshes.new(name); bm = bmesh.new()
    inner = [bm.verts.new((math.cos(2 * math.pi * i / segs) * r0, math.sin(2 * math.pi * i / segs) * r0, z)) for i in range(segs)]
    outer = [bm.verts.new((math.cos(2 * math.pi * i / segs) * r1, math.sin(2 * math.pi * i / segs) * r1, z)) for i in range(segs)]
    for i in range(segs):
        j = (i + 1) % segs; bm.faces.new((inner[i], inner[j], outer[j], outer[i]))
    bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(o); o.data.materials.append(mat)
    return o

# ------------------------------------------------------------------- chapa
def cap_mesh():
    # anillos (radio, alto, pliegue): la cara plana, el hombro y la falda que se abre
    rings = [
        (0.000, H, 0), (0.30, H, 0), (0.60, H * 0.995, 0), (0.78, H * 0.985, 0),
        (0.82, H * 0.975, 0), (0.86, H * 0.955, 0),              # borde de la cara
        (0.90, H * 0.90, 0.004), (0.935, H * 0.78, 0.012),       # hombro
        (0.955, H * 0.60, 0.030), (0.985, H * 0.32, 0.055),      # falda
        (1.010, H * 0.10, 0.070), (1.020, H * 0.02, 0.072), (1.000, 0.0, 0.070)
    ]
    segs = FLUTES * 12
    me = bpy.data.meshes.new('cap'); bm = bmesh.new()
    uv = bm.loops.layers.uv.new()
    verts = []
    for ri, (rr, z, amp) in enumerate(rings):
        row = []
        n = 1 if ri == 0 else segs
        for i in range(n):
            a = 2 * math.pi * i / segs
            # pliegue: hacia fuera en las crestas, redondeado
            k = 1 + amp * (0.5 + 0.5 * math.cos(FLUTES * a)) ** 2.4
            row.append(bm.verts.new((math.cos(a) * rr * R * k, math.sin(a) * rr * R * k, z)))
        verts.append(row)
    TOP = 5  # caras hasta el anillo 5 llevan la franela
    for ri in range(1, len(rings)):
        for i in range(segs):
            j = (i + 1) % segs
            if ri == 1:
                f = bm.faces.new((verts[0][0], verts[1][i], verts[1][j]))
            else:
                f = bm.faces.new((verts[ri - 1][i], verts[ri][i], verts[ri][j], verts[ri - 1][j]))
            f.material_index = 0 if ri <= TOP else 1
            f.smooth = True
            for l in f.loops:
                co = l.vert.co
                l[uv].uv = (co.x / (0.86 * R) * 0.5 + 0.5, co.y / (0.86 * R) * 0.5 + 0.5)
    bm.normal_update(); bm.to_mesh(me); bm.free()
    return me

_cap_mesh = None
def cap(x, y, tex, c1, rot=0.0):
    global _cap_mesh
    if _cap_mesh is None: _cap_mesh = cap_mesh()
    me = _cap_mesh.copy()
    o = bpy.data.objects.new('cap', me); bpy.context.collection.objects.link(o)
    o.location = (x, y, 0); o.rotation_euler = (0, 0, rot)
    # cara: la franela con barniz
    mt = bpy.data.materials.new('top'); mt.use_nodes = True; t = mt.node_tree; p = principled(t)
    im = node(t, 'ShaderNodeTexImage'); im.image = bpy.data.images.load(tex)
    t.links.new(im.outputs['Color'], p.inputs['Base Color'])
    p.inputs['Roughness'].default_value = 0.32
    setin(p, ['Coat Weight', 'Clearcoat'], 0.6); setin(p, ['Coat Roughness', 'Clearcoat Roughness'], 0.12)
    # un poco de abolladura en la chapa
    nz = node(t, 'ShaderNodeTexNoise'); nz.inputs['Scale'].default_value = 9; bump = node(t, 'ShaderNodeBump')
    bump.inputs['Strength'].default_value = 0.08; t.links.new(nz.outputs['Fac'], bump.inputs['Height']); t.links.new(bump.outputs['Normal'], p.inputs['Normal'])
    # falda: metal pintado del color 1
    ms = bpy.data.materials.new('skirt'); ms.use_nodes = True; t2 = ms.node_tree; p2 = principled(t2)
    p2.inputs['Base Color'].default_value = c1 + (1,)
    p2.inputs['Metallic'].default_value = 0.55; p2.inputs['Roughness'].default_value = 0.28
    setin(p2, ['Coat Weight', 'Clearcoat'], 0.5)
    o.data.materials.append(mt); o.data.materials.append(ms)
    return o

def ball(x, y, r=0.055):
    bpy.ops.mesh.primitive_uv_sphere_add(radius=r, location=(x, y, r), segments=64, ring_count=32)
    o = bpy.context.object; bpy.ops.object.shade_smooth()
    m = bpy.data.materials.new('ball'); m.use_nodes = True; p = principled(m.node_tree)
    p.inputs['Base Color'].default_value = (0.025, 0.027, 0.03, 1); p.inputs['Roughness'].default_value = 0.35
    setin(p, ['Coat Weight', 'Clearcoat'], 0.6)
    o.data.materials.append(m)

def srgb(h):
    h = h.lstrip('#'); v = [int(h[i:i + 2], 16) / 255 for i in (0, 2, 4)]
    return tuple(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in v)

# ------------------------------------------------------------------- luces
def lights():
    # la principal de lado (45 grados) para que la camara cenital no vea su
    # reflejo en el barniz; la de relleno, suave y del lado contrario
    def area(loc, size, energy, color):
        bpy.ops.object.light_add(type='AREA', location=loc)
        l = bpy.context.object; l.data.size = size; l.data.energy = energy; l.data.color = color
        d = [-v for v in loc]; l.rotation_euler = __import__('mathutils').Vector(d).to_track_quat('-Z', 'Y').to_euler()
        return l
    area((-5.0, 4.0, 5.5), 5.0, 2400, (1.0, 0.95, 0.88))
    area((5.0, -4.0, 6.0), 8.0, 380, (0.85, 0.92, 1.0))

def camera(scale, x=0, y=0):
    bpy.ops.object.camera_add(location=(x, y, 10), rotation=(0, 0, 0))
    c = bpy.context.object; c.data.type = 'ORTHO'; c.data.ortho_scale = scale
    sc.camera = c


def strip(x0, y0, x1, y1, w, mat):
    me = bpy.data.meshes.new('line'); bm = bmesh.new()
    dx, dy = x1 - x0, y1 - y0; l = math.hypot(dx, dy); nx, ny = -dy / l * w / 2, dx / l * w / 2
    v = [bm.verts.new((x0 + nx, y0 + ny, 0.0006)), bm.verts.new((x1 + nx, y1 + ny, 0.0006)),
         bm.verts.new((x1 - nx, y1 - ny, 0.0006)), bm.verts.new((x0 - nx, y0 - ny, 0.0006))]
    bm.faces.new(v); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new('line', me); bpy.context.collection.objects.link(o); o.data.materials.append(mat)

def arc(cx, cy, r, a0, a1, w, mat, segs=160):
    for i in range(segs):
        b0 = a0 + (a1 - a0) * i / segs; b1 = a0 + (a1 - a0) * (i + 1) / segs
        strip(cx + math.cos(b0) * r, cy + math.sin(b0) * r, cx + math.cos(b1) * r, cy + math.sin(b1) * r, w, mat)

# ----------------------------------------------------------------- montaje
texs = sorted(f for f in os.listdir(TEX) if f.endswith('.png'))
T = lambda i: os.path.join(TEX, texs[i])
COL = ['#b3202b', '#1c1d21', '#0f4c9c', '#f2f0ea', '#1b6b3a', '#f0c419', '#6b1f7a', '#e9e9ea', '#0d2a54', '#d7262f', '#5fb4e6', '#e2711d']

FELT = felt_material()
plane('felt', 30, 30, 0, FELT)
lights()

if MODE == 'field':
    chalk = chalk_material()
    CR = 1.05
    ring_mesh('circle', CR - 0.012, CR + 0.012, 512, chalk)
    plane('half', 0.024, 30, 0.0006, chalk)
    ring_mesh('dot', 0.0, 0.03, 64, chalk)
    # los dos equipos formados junto al circulo, como en la foto
    home = [(-0.32, 1.55), (-0.55, 1.28), (-0.95, 0.98), (-1.25, 0.60), (-1.38, 0.18), (-1.30, -0.28), (-1.05, -0.70), (-0.62, -1.05), (-0.30, -1.38)]
    away = [(0.34, 1.50), (0.72, 1.20), (0.40, 0.42), (1.18, 0.78), (1.40, 0.25), (1.30, -0.30), (1.00, -0.80), (0.58, -1.12), (0.30, -1.45)]
    for i, (x, y) in enumerate(home): cap(x, y, T(0), srgb(COL[0]), rot=random.uniform(-0.2, 0.2))
    for i, (x, y) in enumerate(away): cap(x, y, T(1), srgb(COL[1]), rot=random.uniform(-0.2, 0.2))
    ball(-0.18, 0.06)
    camera(5.6)
elif MODE == 'caps':
    for i in range(12):
        col, row = i % 6, i // 6
        cap(-1.15 + col * 0.46, 0.27 - row * 0.56, T(i), srgb(COL[i]), rot=random.uniform(-0.12, 0.12))
    camera(2.9, 0, 0)
elif MODE == 'pitch':
    # el fieltro de sobra alrededor: llena una pantalla de hasta 21:9 a todo
    # el alto sin que se vea el final de la imagen (14 x 8 unidades)
    sc.render.resolution_x, sc.render.resolution_y = 2380, 1360
    chalk = chalk_material(); LW = 0.024
    L, Wd = 7.0, 4.6; hx, hy = L / 2, Wd / 2
    for a, b in (((-hx, -hy), (hx, -hy)), ((hx, -hy), (hx, hy)), ((hx, hy), (-hx, hy)), ((-hx, hy), (-hx, -hy)), ((0, -hy), (0, hy))):
        strip(a[0], a[1], b[0], b[1], LW, chalk)
    arc(0, 0, 0.9, 0, 2 * math.pi, LW, chalk, 320)
    ring_mesh('dot', 0.0, 0.03, 64, chalk)
    for s in (-1, 1):
        gx = s * hx
        for a, b in (((gx, -1.3), (gx - s * 1.1, -1.3)), ((gx - s * 1.1, -1.3), (gx - s * 1.1, 1.3)), ((gx - s * 1.1, 1.3), (gx, 1.3)),
                     ((gx, -0.6), (gx - s * 0.4, -0.6)), ((gx - s * 0.4, -0.6), (gx - s * 0.4, 0.6)), ((gx - s * 0.4, 0.6), (gx, 0.6))):
            strip(a[0], a[1], b[0], b[1], LW, chalk)
        px = gx - s * 0.75
        o = ring_mesh('spot', 0.0, 0.028, 48, chalk); o.location.x = px
        # la media luna fuera del area
        d = 1.1 - 0.75; a0 = math.acos(d / 0.9)
        if s > 0: arc(px, 0, 0.9, math.pi - a0, math.pi + a0, LW, chalk)
        else: arc(px, 0, 0.9, -a0, a0, LW, chalk)
        for cy in (-hy, hy):  # banderines: cuartos de circulo en las esquinas
            ang0 = math.atan2(-cy, -gx); arc(gx, cy, 0.14, ang0 - math.pi / 4, ang0 + math.pi / 4, LW, chalk, 24)
    # luz pareja en todo el fieltro (sin el degradado de la luz de lado) y
    # una lateral flojita (la misma que las porterias, goals.py: van aparte)
    for ob in list(bpy.data.objects):
        if ob.type == 'LIGHT': bpy.data.objects.remove(ob)
    bpy.ops.object.light_add(type='AREA', location=(0, 0, 12)); top = bpy.context.object
    top.data.size = 40; top.data.energy = 9000; top.data.color = (1.0, 0.97, 0.92)
    bpy.ops.object.light_add(type='AREA', location=(-6, 5, 6)); side = bpy.context.object
    side.data.size = 6; side.data.energy = 900
    side.rotation_euler = __import__('mathutils').Vector((6, -5, -6)).to_track_quat('-Z', 'Y').to_euler()
    camera(14.0)
else:  # una chapa de cerca
    cap(0, 0, T(0), srgb(COL[0]), rot=0.1)
    camera(0.46)

sc.render.filepath = OUT
bpy.ops.render.render(write_still=True)
print('render', OUT)
