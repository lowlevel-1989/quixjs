# Las porterias, cada una aparte del campo (el campo, games/soccer-chapa/game/assets/pitch.jpg, ya
# no las lleva): vistas desde arriba con la misma luz que el campo (scene.py,
# modo pitch), sin fondo y con su sombra (el suelo atrapa sombras). La boca
# es siempre la misma (1.0 de ancho y 0.32 de fondo, games/soccer-chapa/game/src/world/pitch.js: los
# choques no cambian); lo de alrededor es de cada una y puede salirse por
# detras y por los lados, fuera del campo.
# La linea de gol esta en x = 0; la de la derecha mira hacia -x. Cada una
# sale en dos imagenes, la de la derecha y la de la izquierda (en espejo,
# con la luz en su sitio), del mismo marco: FRAME en unidades del campo.
#   blender -b -P goals.py -- <porteria|all> <carpeta> [muestras]
#   games/soccer-chapa/game/assets/goals/<porteria>_r.png y <porteria>_l.png
# Tras cada render, goal_edges.py le limpia el borde (sin rectangulo).
import bpy, sys, os, math, random

argv = sys.argv[sys.argv.index('--') + 1:]
GOAL_ID, GOAL_OUT = argv[0], argv[1]
GOAL_SAMPLES = int(argv[2]) if len(argv) > 2 else 64
FRAME = (-0.3, 1.5, -1.25, 1.25)       # x0, x1, y0, y1 (games/soccer-chapa/game/src/data/goals.js: FRAME)
PX = 340                               # pixeles por unidad (el doble que el campo)
GW, GD, GH = 1.0, 0.32, 0.22           # la boca: ancho, fondo y alto

here = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(here, 'scene.py')).read()
src = src[:src.index('# ----------------------------------------------------------------- montaje')]
sys.argv = ['x', '--', 'pitch', '.', '/dev/null', str(GOAL_SAMPLES)]
exec(compile(src, 'scene.py', 'exec'))
sc = bpy.context.scene


# ------------------------------------------------------------- materiales
def mat(name, color, metal=0.0, rough=0.5, bump=0.0, scale=60.0, voronoi=False, emit=0.0):
    m = bpy.data.materials.new(name); m.use_nodes = True; t = m.node_tree; p = principled(t)
    p.inputs['Base Color'].default_value = srgb(color) + (1,)
    p.inputs['Metallic'].default_value = metal; p.inputs['Roughness'].default_value = rough
    if emit:
        setin(p, ['Emission Color', 'Emission'], srgb(color) + (1,)); p.inputs['Emission Strength'].default_value = emit
    if bump:
        co = node(t, 'ShaderNodeTexCoord')
        tex = node(t, 'ShaderNodeTexVoronoi' if voronoi else 'ShaderNodeTexNoise')
        tex.inputs['Scale'].default_value = scale
        if not voronoi: tex.inputs['Detail'].default_value = 6
        b = node(t, 'ShaderNodeBump'); b.inputs['Strength'].default_value = bump; b.inputs['Distance'].default_value = 0.01
        t.links.new(co.outputs['Object'], tex.inputs['Vector'])
        t.links.new(tex.outputs['Distance' if voronoi else 'Fac'], b.inputs['Height'])
        t.links.new(b.outputs['Normal'], p.inputs['Normal'])
    return m


def put(o, m, smooth=True):
    o.data.materials.append(m)
    if smooth: bpy.ops.object.shade_smooth()
    return o


def box(x, y, z, sx, sy, sz, m, bevel=0.006, rot=0.0):
    bpy.ops.mesh.primitive_cube_add(size=1, location=(x, y, z), rotation=(0, 0, rot))
    o = bpy.context.object; o.scale = (sx, sy, sz); bpy.ops.object.transform_apply(scale=True)
    if bevel:
        b = o.modifiers.new('b', 'BEVEL'); b.width = bevel; b.segments = 2
    return put(o, m, smooth=False)


def cyl(x, y, z, r, depth, m, rot=(0, 0, 0), verts=32):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=r, depth=depth, location=(x, y, z), rotation=rot)
    return put(bpy.context.object, m)


def cone(x, y, z, r, depth, m, rot=(0, 0, 0), verts=16, r2=0.0):
    bpy.ops.mesh.primitive_cone_add(vertices=verts, radius1=r, radius2=r2, depth=depth, location=(x, y, z), rotation=rot)
    return put(bpy.context.object, m)


def ball_(x, y, z, sx, sy, sz, m, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=1, location=(x, y, z), rotation=rot)
    o = bpy.context.object; o.scale = (sx, sy, sz)
    return put(o, m)


def torus(x, y, z, R, r, m):
    bpy.ops.mesh.primitive_torus_add(major_radius=R, minor_radius=r, location=(x, y, z), major_segments=48, minor_segments=16)
    return put(bpy.context.object, m)


def tube(points, m, radius=1.0, res=12):
    # un tubo por puntos (x, y, z, grosor)
    cu = bpy.data.curves.new('tube', 'CURVE'); cu.dimensions = '3D'
    cu.bevel_depth = radius; cu.bevel_resolution = res; cu.use_fill_caps = True
    sp = cu.splines.new('NURBS'); sp.points.add(len(points) - 1); sp.use_endpoint_u = True; sp.order_u = 3
    for p, (x, y, z, r) in zip(sp.points, points):
        p.co = (x, y, z, 1); p.radius = r
    o = bpy.data.objects.new('tube', cu); bpy.context.collection.objects.link(o)
    o.data.materials.append(m)
    return o


def brick(name, c1, c2, mortar, scale=12.0, rough=0.85, rows=0.5):
    # piedra o ladrillo: el dibujo de bloques (con su junta hundida) y un
    # ruido fino de color, para que no se vea liso
    m = bpy.data.materials.new(name); m.use_nodes = True; t = m.node_tree; p = principled(t)
    co = node(t, 'ShaderNodeTexCoord')
    b = node(t, 'ShaderNodeTexBrick'); b.inputs['Scale'].default_value = scale
    b.inputs['Color1'].default_value = srgb(c1) + (1,); b.inputs['Color2'].default_value = srgb(c2) + (1,)
    b.inputs['Mortar'].default_value = srgb(mortar) + (1,); b.inputs['Mortar Size'].default_value = 0.025
    b.inputs['Row Height'].default_value = rows; b.offset = 0.5
    t.links.new(co.outputs['Object'], b.inputs['Vector'])
    n = node(t, 'ShaderNodeTexNoise'); n.inputs['Scale'].default_value = 120; n.inputs['Detail'].default_value = 8
    t.links.new(co.outputs['Object'], n.inputs['Vector'])
    mix = node(t, 'ShaderNodeMix'); mix.data_type = 'RGBA'; mix.blend_type = 'MULTIPLY'
    mix.inputs['Factor'].default_value = 0.35
    t.links.new(b.outputs['Color'], mix.inputs[6]); t.links.new(n.outputs['Color'], mix.inputs[7])
    t.links.new(mix.outputs[2], p.inputs['Base Color'])
    bump = node(t, 'ShaderNodeBump'); bump.inputs['Strength'].default_value = 0.8; bump.inputs['Distance'].default_value = 0.006
    inv = node(t, 'ShaderNodeMath'); inv.operation = 'SUBTRACT'; inv.inputs[0].default_value = 1.0
    t.links.new(b.outputs['Fac'], inv.inputs[1]); t.links.new(inv.outputs[0], bump.inputs['Height'])
    t.links.new(bump.outputs['Normal'], p.inputs['Normal'])
    p.inputs['Roughness'].default_value = rough
    return m


def mesh_obj(name, verts, faces, m):
    me = bpy.data.meshes.new(name); me.from_pydata(verts, [], faces); me.update()
    o = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(o); o.data.materials.append(m)
    return o


def grid(name, f, nu, nv, m):
    # una red: la superficie f(u, v) -> (x, y, z) en una rejilla de nu x nv
    # cuadros, de hilo (wireframe)
    verts = [f(i / nu, j / nv) for i in range(nu + 1) for j in range(nv + 1)]
    faces = [(i * (nv + 1) + j, (i + 1) * (nv + 1) + j, (i + 1) * (nv + 1) + j + 1, i * (nv + 1) + j + 1)
             for i in range(nu) for j in range(nv)]
    return mesh_obj(name, verts, faces, m)


def net(m, thick=0.0035, cell=0.03, sag=0.035, back=GD, x0=0.02):
    # la red de verdad: la de arriba cae del larguero al suelo del fondo,
    # combada en el centro; las de los lados cierran del poste a la de arriba
    nu = max(4, round((back - x0) / cell)); nv = round(GW / cell)
    def top(u, v):
        x = x0 + (back - x0) * u
        z = GH * (1 - u) ** 1.25 - sag * math.sin(math.pi * u) * (0.5 + 0.5 * math.sin(math.pi * v))
        return (x + sag * 0.4 * math.sin(math.pi * u), -GW / 2 + GW * v, max(0.0, z))
    parts = [grid('net_top', top, nu, nv, m)]
    for s in (-1, 1):
        def side(u, w, s=s):
            x, y, z = top(u, 0 if s < 0 else 1)
            return (x, s * GW / 2, z * w)
        parts.append(grid('net_side', side, nu, max(2, round(GH / cell)), m))
    for o in parts:
        w = o.modifiers.new('wire', 'WIREFRAME'); w.thickness = thick; w.use_even_offset = False
    return parts


def frame(post, netm, r=0.022, thick=0.0035, bars=True, cell=0.03, sag=0.035):
    # postes, larguero, las barras del suelo que sujetan la red y la red
    for y in (-GW / 2, GW / 2):
        cyl(0.02, y, GH / 2, r, GH, post)
    cyl(0.02, 0, GH, r, GW + 2 * r, post, rot=(math.pi / 2, 0, 0))
    if bars:
        for y in (-GW / 2, GW / 2):
            cyl(0.02 + GD / 2, y, r * 0.6, r * 0.6, GD, post, rot=(0, math.pi / 2, 0))
        cyl(GD + 0.02, 0, r * 0.6, r * 0.6, GW, post, rot=(math.pi / 2, 0, 0))
    net(netm, thick, cell, sag, GD + 0.02)


# ------------------------------------------------------------- porterias
def classic():
    # la de siempre: aluminio blanco y red blanca de hilo fino
    frame(mat('post', '#eeeeea', metal=0.2, rough=0.3), mat('net', '#e6e6e2', rough=0.9))


def wood():
    # potrero: troncos sin pulir, algo torcidos, atados con cuerda, y una red
    # de cuerda gruesa y vieja, mas combada
    bark = mat('bark', '#5d4029', rough=0.95, bump=1.2, scale=35)
    rope = mat('rope', '#b8a582', rough=1.0, bump=0.5, scale=200)
    for s in (-1, 1):
        o = cyl(0.02, s * GW / 2, GH / 2 + 0.01, 0.03, GH + 0.02, bark, rot=(s * 0.05, 0.04, 0), verts=10)
        d = o.modifiers.new('d', 'DISPLACE'); d.strength = 0.006
        d.texture = bpy.data.textures.new('lump', 'CLOUDS'); d.texture.noise_scale = 0.05
        torus(0.02, s * GW / 2, GH - 0.015, 0.033, 0.006, rope)                    # el atado
    o = cyl(0.02, 0, GH + 0.01, 0.026, GW + 0.16, bark, rot=(math.pi / 2, 0, 0.04), verts=10)
    d = o.modifiers.new('d', 'DISPLACE'); d.strength = 0.006
    d.texture = bpy.data.textures.new('lump2', 'CLOUDS'); d.texture.noise_scale = 0.05
    for s in (-1, 1):                                                              # estacas que tensan la red
        cyl(GD + 0.06, s * GW / 2, 0.02, 0.008, 0.06, bark, rot=(0, 0.5, 0), verts=6)
    net(rope, 0.0065, 0.045, 0.06, GD + 0.04)


def castle():
    # la puerta de un castillo: dos torres de piedra con almenas y suelo de
    # tablones arriba, los muros de la boca, el muro del fondo con almenas y
    # estandartes, y el paso empedrado
    stone = brick('stone', '#a29a8c', '#8a8377', '#5e5951', scale=9)
    cobble = brick('cobble', '#7d776d', '#6b665d', '#3e3b36', scale=26, rows=1.0)
    planks = mat('planks', '#7a5532', rough=0.8, bump=0.6, scale=18)
    banner = mat('banner', '#a3242f', rough=0.7, bump=0.2, scale=60)
    gold_m = mat('trim', '#e0b04a', metal=1, rough=0.3)
    box(GD / 2 + 0.01, 0, 0.004, GD + 0.06, GW, 0.008, cobble, bevel=0)
    for s in (-1, 1):
        y = s * (GW / 2 + 0.17)
        cyl(0.14, y, 0.17, 0.18, 0.34, stone, verts=48)
        cyl(0.14, y, 0.341, 0.15, 0.004, planks, verts=48)                       # el suelo de arriba
        for k in range(14):                                                       # las almenas
            a = k * 2 * math.pi / 14
            box(0.14 + math.cos(a) * 0.165, y + math.sin(a) * 0.165, 0.37, 0.04, 0.05, 0.06, stone, rot=a, bevel=0.003)
        box(0.14, y, 0.35, 0.05, 0.05, 0.02, mat('hatch', '#4a3220'))              # la trampilla
        box(0.17, s * (GW / 2 + 0.035), 0.12, 0.3, 0.07, 0.24, stone)              # los muros de la boca
    box(GD + 0.09, 0, 0.15, 0.12, GW + 0.5, 0.3, stone)                           # el muro del fondo
    for k in range(10):
        box(GD + 0.09, -0.68 + k * 0.151, 0.33, 0.12, 0.07, 0.06, stone, bevel=0.003)
    for y in (-0.25, 0.25):                                                       # los estandartes, colgando hacia el campo
        box(GD + 0.028, y, 0.16, 0.004, 0.14, 0.26, banner, bevel=0)
        box(GD + 0.026, y, 0.2, 0.004, 0.05, 0.05, gold_m, bevel=0)
    for k in range(7):                                                            # el rastrillo, subido en el muro
        cyl(GD + 0.03, -0.42 + k * 0.14, 0.29, 0.008, 0.02, mat('iron', '#2a2a2c', metal=1, rough=0.5))


def swirl(name, inner, outer, twist=14.0, arms=3, power=4.0, deep=False):
    # un remolino de energia que brilla: brazos en espiral (el angulo mas el
    # radio por twist) con ruido, de inner (el centro) a outer (los bordes)
    m = bpy.data.materials.new(name); m.use_nodes = True; t = m.node_tree; p = principled(t)
    co = node(t, 'ShaderNodeTexCoord'); sep = node(t, 'ShaderNodeSeparateXYZ')
    t.links.new(co.outputs['Object'], sep.inputs['Vector'])
    ang = node(t, 'ShaderNodeMath', operation='ARCTAN2'); t.links.new(sep.outputs['Y'], ang.inputs[0]); t.links.new(sep.outputs['X'], ang.inputs[1])
    rad = node(t, 'ShaderNodeVectorMath', operation='LENGTH'); t.links.new(co.outputs['Object'], rad.inputs[0])
    tw = node(t, 'ShaderNodeMath', operation='MULTIPLY'); tw.inputs[1].default_value = twist; t.links.new(rad.outputs['Value'], tw.inputs[0])
    ar = node(t, 'ShaderNodeMath', operation='MULTIPLY'); ar.inputs[1].default_value = arms; t.links.new(ang.outputs[0], ar.inputs[0])
    add = node(t, 'ShaderNodeMath', operation='ADD'); t.links.new(ar.outputs[0], add.inputs[0]); t.links.new(tw.outputs[0], add.inputs[1])
    sn = node(t, 'ShaderNodeMath', operation='SINE'); t.links.new(add.outputs[0], sn.inputs[0])
    nz = node(t, 'ShaderNodeTexNoise'); nz.inputs['Scale'].default_value = 9; nz.inputs['Detail'].default_value = 6
    t.links.new(co.outputs['Object'], nz.inputs['Vector'])
    nzk = node(t, 'ShaderNodeMath', operation='MULTIPLY'); nzk.inputs[1].default_value = 0.3
    t.links.new(nz.outputs['Fac'], nzk.inputs[0])
    mix = node(t, 'ShaderNodeMath', operation='MULTIPLY_ADD'); mix.inputs[1].default_value = 0.25
    t.links.new(sn.outputs[0], mix.inputs[0]); t.links.new(nzk.outputs[0], mix.inputs[2])
    edge = node(t, 'ShaderNodeMath', operation='SUBTRACT'); edge.inputs[0].default_value = 1.0
    t.links.new(rad.outputs['Value'], edge.inputs[1])                         # mas claro en el centro
    tot = node(t, 'ShaderNodeMath', operation='MULTIPLY_ADD'); tot.inputs[1].default_value = 0.8 if deep else 0.35
    t.links.new((rad.outputs['Value'] if deep else edge.outputs[0]), tot.inputs[0]); t.links.new(mix.outputs[0], tot.inputs[2])
    ramp = node(t, 'ShaderNodeValToRGB'); r = ramp.color_ramp
    r.elements[0].position = 0.12; r.elements[0].color = srgb('#0d0418') + (1,)
    mid = r.elements.new(0.45); mid.color = srgb(outer) + (1,)
    r.elements[-1].position = 0.82; r.elements[-1].color = srgb(inner) + (1,)
    if deep:                                                   # un pozo: negro al centro, la luz hacia el borde
        r.elements[0].position = 0.15; r.elements[0].color = srgb('#030008') + (1,)
        mid.position = 0.6
        r.elements[-1].position = 0.95
    t.links.new(tot.outputs[0], ramp.inputs['Fac'])
    t.links.new(ramp.outputs['Color'], p.inputs['Base Color'])
    setin(p, ['Emission Color', 'Emission'], (1, 1, 1, 1))
    t.links.new(ramp.outputs['Color'], p.inputs['Emission Color'] if 'Emission Color' in p.inputs else p.inputs['Emission'])
    p.inputs['Emission Strength'].default_value = power; p.inputs['Roughness'].default_value = 0.3
    return m


def portal():
    # un portal: dos obeliscos de piedra oscura con runas que brillan, un arco
    # de piedra encima de la boca, dentro un remolino de energia (violeta y
    # cian) y detras una plataforma de piedra con un anillo de runas y
    # cristales que flotan
    rock = brick('rock', '#4a4a55', '#3d3d47', '#24242a', scale=7)
    slab = brick('slab', '#5b5a63', '#4f4e57', '#2c2b31', scale=14, rows=1.0)
    rune = mat('rune', '#6fe3ff', rough=0.4, emit=6.0)
    crystal = mat('crystal', '#b58cff', metal=0.1, rough=0.1, emit=2.5)
    P = 0.62                                                                   # el centro de la plataforma, detras de la linea
    cyl(P, 0, 0.006, 0.6, 0.012, slab, verts=96)                              # la plataforma
    torus(P, 0, 0.014, 0.54, 0.008, rune)                                     # su anillo de runas
    for k in range(24):                                                        # las runas del anillo
        a = k * 2 * math.pi / 24
        box(P + math.cos(a) * 0.47, math.sin(a) * 0.47, 0.014, 0.03, 0.012, 0.004, rune, rot=a, bevel=0)
    cx, rx, ry = GD / 2 + 0.02, GD / 2 + 0.02, GW / 2 - 0.02       # el pozo: en la boca
    o = cyl(cx, 0, 0.03, 1, 0.004, swirl('vortex', '#38c6ff', '#6a2ee0', twist=10.0, power=1.1, deep=True), verts=96)
    o.scale = (rx, ry, 1)
    torus(cx, 0, 0.034, 1, 0.016, rock).scale = (rx + 0.03, ry + 0.03, 1)                     # el brocal de piedra
    torus(cx, 0, 0.04, 1, 0.006, rune).scale = (rx + 0.005, ry + 0.005, 1)                    # y su filo de luz
    spark = mat('spark', '#e6fbff', rough=0.3, emit=8.0)
    rs = random.Random(5)
    for k in range(46):                                                       # chispas que caen en espiral
        f = k / 46.0; a = f * 5 * math.pi + rs.uniform(-0.2, 0.2); d = 0.95 - f * 0.8
        ball_(cx + math.cos(a) * rx * d, math.sin(a) * ry * d, 0.036, 0.004 + 0.006 * (1 - f), 0.004 + 0.006 * (1 - f), 0.002, spark)
    bpy.ops.object.light_add(type='POINT', location=(cx, 0, 0.3))          # su luz tine la piedra de alrededor
    l = bpy.context.object; l.data.energy = 2.5; l.data.color = (0.55, 0.35, 1.0); l.data.shadow_soft_size = 0.3
    for s in (-1, 1):                                                          # los obeliscos
        y = s * (GW / 2 + 0.06)
        box(0.05, y, 0.16, 0.12, 0.12, 0.32, rock, bevel=0.01)
        cone(0.05, y, 0.36, 0.085, 0.08, rock, verts=4).rotation_euler.z = math.pi / 4
        for z in (0.08, 0.16, 0.24):                                           # sus runas
            box(0.05 - 0.061, y, z, 0.004, 0.05, 0.03, rune, bevel=0)
    box(0.03, 0, 0.34, 0.05, GW + 0.12, 0.05, rock, bevel=0.01)               # el arco, encima de la boca
    box(0.03, 0, 0.366, 0.014, 0.4, 0.004, rune, bevel=0)
    rng = random.Random(3)
    for k in range(5):                                                         # los cristales, flotando
        a = -math.pi / 2 + k * math.pi / 4
        x, y = P + math.cos(a) * 0.66, math.sin(a) * 0.66
        cone(x, y, 0.2 + rng.uniform(0, 0.08), 0.035, 0.12, crystal, verts=6, rot=(rng.uniform(-0.4, 0.4), rng.uniform(-0.4, 0.4), 0))


def dragon():
    # un dragon acostado alrededor de la porteria: la cabeza junto a un poste,
    # mirando al campo; el cuerpo hace el fondo, con las alas plegadas, y la
    # cola cierra el otro lado y se enrosca. Dentro, una red de cuerda oscura
    scales = mat('scales', '#2e6b3a', rough=0.45, bump=0.9, scale=70, voronoi=True)
    belly = mat('belly', '#c9a65a', rough=0.6, bump=0.4, scale=40)
    horn = mat('horn', '#efe6cf', rough=0.4)
    wing = mat('wing', '#7a2a2a', rough=0.6, bump=0.2, scale=30)
    eye = mat('eye', '#ffc21a', emit=2.5)
    net(mat('dnet', '#3b3226', rough=1.0), 0.0055, 0.04, 0.05, GD + 0.02)
    body = [(0.30, -0.70, 0.07, 0.06), (0.50, -0.66, 0.09, 0.09), (0.66, -0.42, 0.11, 0.13),
            (0.70, 0.0, 0.12, 0.15), (0.66, 0.42, 0.11, 0.13), (0.48, 0.66, 0.08, 0.09),
            (0.18, 0.70, 0.05, 0.06), (0.0, 0.82, 0.04, 0.04), (0.02, 1.0, 0.03, 0.025), (0.18, 1.08, 0.02, 0.012)]
    tube(body, scales, 1.0)
    tube([(p[0] + 0.02, p[1], p[2] - 0.05, p[3] * 0.8) for p in body[1:7]], belly, 1.0)
    cone(0.24, 1.08, 0.02, 0.05, 0.1, scales, rot=(0, math.pi / 2, 0.3), verts=3)   # la punta de la cola
    for i in range(1, 8):                                                          # las puas del lomo
        a, b = body[i], body[i + 1]
        for f in (0.0, 0.5):
            x, y = a[0] + (b[0] - a[0]) * f, a[1] + (b[1] - a[1]) * f
            r = a[3] + (b[3] - a[3]) * f
            cone(x, y, a[2] + r * 0.9, r * 0.35, r * 0.8, horn, verts=8)
    # la cabeza: una pieza que sale del cuello y se estrecha hasta el hocico,
    # con las escamas del cuerpo; ojos pequenos con pupila de reptil y ceja,
    # cuernos echados hacia atras y los orificios en la punta
    tube([(0.36, -0.70, 0.08, 0.07), (0.24, -0.71, 0.095, 0.085), (0.12, -0.72, 0.085, 0.065),
          (0.02, -0.72, 0.07, 0.045), (-0.05, -0.72, 0.062, 0.032)], scales, 1.0)
    tube([(0.2, -0.72, 0.04, 0.05), (0.06, -0.72, 0.035, 0.04), (-0.03, -0.72, 0.035, 0.026)], belly, 1.0)   # la mandibula
    pupil = mat('pupil', '#120c05', rough=0.3)
    for s in (-1, 1):
        y = -0.71 + s * 0.055
        ball_(0.19, y, 0.15, 0.022, 0.016, 0.012, eye)
        ball_(0.19, y, 0.158, 0.016, 0.004, 0.006, pupil)                         # la pupila, en raya
        cone(0.17, y + s * 0.01, 0.17, 0.016, 0.06, scales, rot=(0, math.pi / 2 + 0.6, 0), verts=8)   # la ceja
        cone(0.32, -0.71 + s * 0.045, 0.15, 0.022, 0.2, horn, rot=(0, math.pi / 2 - 0.35, s * 0.35), verts=12)   # el cuerno
        ball_(-0.06, -0.72 + s * 0.018, 0.088, 0.008, 0.006, 0.004, mat('nostril', '#101010'))
        for k in range(3):                                                         # los dientes, asomando
            cone(0.02 + k * 0.035, -0.72 + s * 0.05, 0.05, 0.007, 0.03, horn, rot=(s * -1.2, 0, 0), verts=6)
    for y in (-0.5, 0.5):                                                          # las patas delanteras
        ball_(0.56, y * 1.05, 0.04, 0.09, 0.05, 0.04, scales)
        for k in range(3):
            cone(0.48, y * 1.05 + (k - 1) * 0.025, 0.02, 0.008, 0.05, horn, rot=(0, -1.6, 0), verts=6)
    for s in (-1, 1):                                                              # las alas plegadas
        bpy.ops.mesh.primitive_cone_add(vertices=3, radius1=0.34, depth=0.02, location=(0.86, s * 0.28, 0.2),
                                        rotation=(0, 0, s * 0.5))
        o = bpy.context.object; o.scale = (1.2, 0.55, 1); put(o, wing, smooth=False)
        tube([(0.62, s * 0.1, 0.22, 1), (0.9, s * 0.3, 0.24, 0.8), (1.18, s * 0.42, 0.2, 0.5)], scales, 0.018)


def soft(o, level=2):
    # redondea un bloque (subdivision): piedra tallada, no un mueble
    s = o.modifiers.new('s', 'SUBSURF'); s.levels = level; s.render_levels = level
    bpy.ops.object.shade_smooth()
    return o


def moai():
    # dos moais de la isla de Pascua, uno junto a cada poste, de pie y
    # mirando al campo y algo caidos hacia delante (TILT; BIG: su tamaño;
    # SINK: lo que se hunden en la tierra, como los de verdad): desde arriba se
    # les ve la cara (la frente, los ojos hundidos, la nariz larga, los
    # labios, las orejas largas) y el pukao (el sombrero de piedra roja).
    # Entre ellos, una red de cuerda
    TILT, BIG, SINK = -0.4, 1.3, 0.12
    tuff = mat('tuff', '#76705f', rough=0.95, bump=1.6, scale=40)
    dark = mat('socket', '#3a372f', rough=1.0, bump=0.8, scale=40)
    scoria = mat('scoria', '#934c36', rough=0.95, bump=1.4, scale=55)
    net(mat('mnet', '#a8946c', rough=1.0), 0.005, 0.04, 0.05, GD + 0.02)
    for s in (-1, 1):
        y, x0 = s * (GW / 2 + 0.19), 0.4                                       # detras de la linea: no pisan el campo
        before = set(bpy.data.objects)
        soft(box(x0, y, 0.12, 0.2, 0.28, 0.24, tuff, bevel=0.05))                 # los hombros
        soft(box(x0, y, 0.4, 0.17, 0.21, 0.34, tuff, bevel=0.05))                 # la cabeza, larga
        soft(box(x0 - 0.085, y, 0.52, 0.05, 0.22, 0.05, tuff, bevel=0.015))       # la frente, salida
        for e in (-1, 1):
            soft(box(x0 - 0.088, y + e * 0.05, 0.47, 0.02, 0.055, 0.04, dark, bevel=0.006), 1)   # los ojos
            soft(box(x0, y + e * 0.11, 0.42, 0.07, 0.035, 0.2, tuff, bevel=0.012))  # las orejas largas
        soft(box(x0 - 0.1, y, 0.4, 0.05, 0.05, 0.16, tuff, bevel=0.015))          # la nariz, larga y recta
        soft(box(x0 - 0.1, y, 0.32, 0.06, 0.08, 0.04, tuff, bevel=0.015))         # su punta, ancha
        soft(box(x0 - 0.088, y, 0.27, 0.03, 0.11, 0.025, tuff, bevel=0.008), 1)   # los labios
        soft(box(x0 - 0.075, y, 0.22, 0.04, 0.15, 0.04, tuff, bevel=0.012))       # la barbilla
        cyl(x0 + 0.01, y, 0.61, 0.1, 0.09, scoria, verts=40)                       # el pukao
        cyl(x0 + 0.01, y, 0.665, 0.065, 0.03, scoria, verts=40)
        bpy.ops.object.empty_add(location=(x0 - 0.1, y, 0)); pivot = bpy.context.object   # gira desde la base, delante
        for o in set(bpy.data.objects) - before - {pivot}:
            o.parent = pivot; o.matrix_parent_inverse = pivot.matrix_world.inverted()
        pivot.rotation_euler.y = TILT; pivot.scale = (BIG, BIG, BIG)
        pivot.location.z = -SINK                                               # algo enterrados: apoyados, no flotan
        bpy.context.view_layer.update()                                       # el giro, aplicado a cada pieza (el
        for o in set(bpy.data.objects) - before - {pivot}:                    # espejo de la porteria de la izquierda
            mw = o.matrix_world.copy(); o.parent = None; o.matrix_world = mw  # mueve cada objeto por su cuenta)
        bpy.data.objects.remove(pivot)

GOALS = {'classic': classic, 'wood': wood, 'castle': castle, 'portal': portal, 'dragon': dragon, 'moai': moai}


# ------------------------------------------------------------------ montaje
def clear():
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob)


def setup(mirror):
    # la luz del campo (scene.py, modo pitch), el suelo que solo atrapa la
    # sombra y la camara sobre el marco
    for loc, size, energy in (((0, 0, 12), 40, 9000), ((-6, 5, 6), 6, 900)):
        bpy.ops.object.light_add(type='AREA', location=loc); l = bpy.context.object
        l.data.size = size; l.data.energy = energy
        if loc[2] == 12: l.data.color = (1.0, 0.97, 0.92)
        else: l.rotation_euler = __import__('mathutils').Vector([-v for v in loc]).to_track_quat('-Z', 'Y').to_euler()
    floor = plane('floor', 30, 30, 0, mat('floor', '#4c5c4e')); floor.is_shadow_catcher = True
    x0, x1, y0, y1 = FRAME
    cx = (x0 + x1) / 2 * (-1 if mirror else 1)
    camera(max(x1 - x0, y1 - y0), cx, (y0 + y1) / 2)
    sc.render.resolution_x = round((x1 - x0) * PX); sc.render.resolution_y = round((y1 - y0) * PX)


def render(gid):
    for mirror in (False, True):
        clear()
        before = set(bpy.data.objects)
        GOALS[gid]()
        if mirror:                                             # la de la izquierda: la misma en espejo
            for ob in set(bpy.data.objects) - before:
                ob.location.x = -ob.location.x; ob.scale.x = -ob.scale.x
                ob.rotation_euler.y = -ob.rotation_euler.y; ob.rotation_euler.z = -ob.rotation_euler.z
        setup(mirror)
        sc.render.film_transparent = True
        sc.render.filepath = os.path.join(GOAL_OUT, gid + ('_l' if mirror else '_r') + '.png')
        bpy.ops.render.render(write_still=True)
        __import__('subprocess').run(['python3', os.path.join(here, 'goal_edges.py'), sc.render.filepath], check=True)
    print('porteria', gid)


os.makedirs(GOAL_OUT, exist_ok=True)
for gid in (GOALS if GOAL_ID == 'all' else [GOAL_ID]):
    render(gid)
