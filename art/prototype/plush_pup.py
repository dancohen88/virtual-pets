"""Prototype: a plush, sausage-dog-inspired pet built entirely from code.

Run from the project root:
    blender --background --factory-startup --python art/prototype/plush_pup.py

Writes renders and an editable .blend to art/prototype/renders/.
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Euler, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "renders")
os.makedirs(OUT, exist_ok=True)

BG_SRGB = (0.98, 0.90, 0.84)  # pastel peach backdrop for previews

scene = bpy.context.scene
for ob in list(bpy.data.objects):
    bpy.data.objects.remove(ob, do_unlink=True)


# ---------------------------------------------------------------- helpers
def srgb_to_linear(c):
    return tuple(((x + 0.055) / 1.055) ** 2.4 if x > 0.04045 else x / 12.92 for x in c)


def set_input(node, name, value):
    if name in node.inputs:
        node.inputs[name].default_value = value


def node_material(name):
    m = bpy.data.materials.new(name)
    try:
        m.use_nodes = True  # deprecated in newer Blender, harmless if ignored
    except Exception:
        pass
    nt = m.node_tree
    bsdf = next((n for n in nt.nodes if n.type == "BSDF_PRINCIPLED"), None)
    if bsdf is None:
        bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
        out = next(n for n in nt.nodes if n.type == "OUTPUT_MATERIAL")
        nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    return m, nt, bsdf


def plush_material(name, color_srgb, fuzz=1.0, bump=0.15):
    """Soft fabric: very rough, strong sheen (the velvety rim glow), tiny bump noise."""
    m, nt, bsdf = node_material(name)
    set_input(bsdf, "Base Color", (*srgb_to_linear(color_srgb), 1.0))
    set_input(bsdf, "Roughness", 0.9)
    set_input(bsdf, "Sheen Weight", fuzz)
    set_input(bsdf, "Sheen Roughness", 0.35)
    set_input(bsdf, "Specular IOR Level", 0.25)

    coords = nt.nodes.new("ShaderNodeTexCoord")
    noise = nt.nodes.new("ShaderNodeTexNoise")
    set_input(noise, "Scale", 60.0)
    set_input(noise, "Detail", 6.0)
    bump_node = nt.nodes.new("ShaderNodeBump")
    set_input(bump_node, "Strength", bump)
    set_input(bump_node, "Distance", 0.02)
    nt.links.new(coords.outputs["Object"], noise.inputs["Vector"])
    nt.links.new(noise.outputs["Fac"], bump_node.inputs["Height"])
    nt.links.new(bump_node.outputs["Normal"], bsdf.inputs["Normal"])
    return m


def bead_material(name, color_srgb, roughness=0.08):
    """Glossy plastic for safety-bead eyes and nose: the highlight is what makes them read as alive."""
    m, _, bsdf = node_material(name)
    set_input(bsdf, "Base Color", (*srgb_to_linear(color_srgb), 1.0))
    set_input(bsdf, "Roughness", roughness)
    return m


def blob(name, center, radii, mat, rot=(0, 0, 0), blunt=None, segs=(48, 24), subsurf=2):
    """A squashed sphere. `blunt` < 1 flattens the ends along X, turning an ellipsoid into a sausage."""
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs[0], v_segments=segs[1], radius=1.0)
    rmat = Euler(rot).to_matrix()
    for v in bm.verts:
        x, y, z = v.co
        if blunt:
            x = math.copysign(abs(x) ** blunt, x)
        v.co = rmat @ Vector((x * radii[0], y * radii[1], z * radii[2]))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = True
    me.materials.append(mat)
    ob = bpy.data.objects.new(name, me)
    ob.location = center
    scene.collection.objects.link(ob)
    if subsurf:
        mod = ob.modifiers.new("Subsurf", "SUBSURF")
        mod.levels = 1
        mod.render_levels = subsurf
    return ob


def add_plump_key(ob, radii, amount):
    """Shape key that inflates the mesh along its normals, most around the lower belly."""
    me = ob.data
    ob.shape_key_add(name="Basis")
    key = ob.shape_key_add(name="Plump")
    for i, v in enumerate(me.vertices):
        co, n = v.co, v.normal
        middle = 1.0 - min(1.0, abs(co.x) / radii[0]) ** 2   # fattest mid-body
        belly = 0.55 + 0.45 * max(0.0, -co.z / radii[2])       # sags downward
        key.data[i].co = co + n * amount * middle * belly
    return key


# ------------------------------------------------------------- materials
caramel = plush_material("Caramel", (0.80, 0.55, 0.33))
cocoa = plush_material("Cocoa", (0.52, 0.32, 0.19))
muzzle = plush_material("Muzzle", (0.95, 0.80, 0.63))
nose_mat = bead_material("Nose", (0.13, 0.08, 0.07), roughness=0.25)
eye_mat = bead_material("Eye", (0.02, 0.02, 0.02))

# ------------------------------------------------------------------ pup
BODY_C, BODY_R = (0.0, 0.0, 1.0), (1.8, 0.72, 0.68)
HEAD_C, HEAD_R = Vector((1.85, 0.0, 1.6)), (0.62, 0.6, 0.56)

body = blob("Body", BODY_C, BODY_R, caramel, blunt=0.55, segs=(64, 32))
head = blob("Head", HEAD_C, HEAD_R, caramel)
blob("Snout", (2.36, 0.0, 1.45), (0.40, 0.32, 0.27), muzzle)
blob("Nose", (2.79, 0.0, 1.53), (0.10, 0.12, 0.08), nose_mat, segs=(24, 12))

for side in (1, -1):
    eye_dir = Vector((0.62, 0.52 * side, 0.42)).normalized()
    blob(f"Eye.{'L' if side > 0 else 'R'}", HEAD_C + eye_dir * 0.57,
         (0.075, 0.075, 0.075), eye_mat, segs=(24, 12), subsurf=1)
    blob(f"Ear.{'L' if side > 0 else 'R'}", (1.70, 0.58 * side, 1.32),
         (0.27, 0.06, 0.5), cocoa, rot=(0.06 * side, 0.3, 0))
    for front in (1, -1):
        blob(f"Leg.{'F' if front > 0 else 'B'}{'L' if side > 0 else 'R'}",
             (1.15 * front, 0.40 * side, 0.28), (0.2, 0.2, 0.3), caramel)

blob("Tail", (-2.0, 0.0, 1.35), (0.32, 0.09, 0.09), cocoa, rot=(0, 0.6, 0))

add_plump_key(body, BODY_R, 0.32)
add_plump_key(head, HEAD_R, 0.06)  # chubby cheeks too


def set_plump(value):
    for ob in (body, head):
        ob.data.shape_keys.key_blocks["Plump"].value = value


# --------------------------------------------------------- stage & lights
bpy.ops.mesh.primitive_plane_add(size=40, location=(0, 0, 0))
ground = bpy.context.active_object
ground.name = "ShadowCatcher"
ground.is_shadow_catcher = True  # renders only the shadow, so sprites get a soft contact shadow

target = bpy.data.objects.new("Target", None)
target.location = (0.35, 0.0, 1.0)
scene.collection.objects.link(target)


def aim(ob):
    c = ob.constraints.new("TRACK_TO")
    c.target = target
    c.track_axis = "TRACK_NEGATIVE_Z"
    c.up_axis = "UP_Y"


cam_data = bpy.data.cameras.new("Camera")
cam_data.lens = 60
cam = bpy.data.objects.new("Camera", cam_data)
cam.location = (5.0, -8.0, 3.4)
scene.collection.objects.link(cam)
aim(cam)
scene.camera = cam


def area_light(name, loc, energy, size, color, shadow=True):
    d = bpy.data.lights.new(name, "AREA")
    d.energy, d.size, d.color = energy, size, color
    d.use_shadow = shadow  # only the big overhead key casts, giving one soft contact shadow
    ob = bpy.data.objects.new(name, d)
    ob.location = loc
    scene.collection.objects.link(ob)
    aim(ob)


area_light("Key", (2.0, -3.0, 8.0), 1300, 7.0, (1.0, 0.95, 0.88))
area_light("Fill", (-5.0, -4.0, 2.5), 250, 6.0, (0.88, 0.92, 1.0), shadow=False)
area_light("Rim", (-3.0, 5.0, 4.5), 700, 3.0, (1.0, 1.0, 1.0), shadow=False)

world = bpy.data.worlds.new("World")
scene.world = world
try:
    world.use_nodes = True
except Exception:
    pass
bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
set_input(bg, "Color", (*srgb_to_linear(BG_SRGB), 1.0))
set_input(bg, "Strength", 0.4)

# ---------------------------------------------------------------- render
scene.render.engine = "CYCLES"
scene.cycles.use_denoising = True
scene.render.film_transparent = True
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
try:
    prefs = bpy.context.preferences.addons["cycles"].preferences
    prefs.compute_device_type = "METAL"
    prefs.get_devices()
    for d in prefs.devices:
        d.use = True
    scene.cycles.device = "GPU"
except Exception as e:
    print("GPU setup failed, rendering on CPU:", e)


def render(name, w, h, samples):
    path = os.path.join(OUT, name)
    scene.render.resolution_x, scene.render.resolution_y = w, h
    scene.cycles.samples = samples
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    return path


def load_rgba(path):
    img = bpy.data.images.load(path)
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
    bpy.data.images.remove(img)
    return px


def save_rgba(arr, name):
    h, w = arr.shape[:2]
    img = bpy.data.images.new("tmp", w, h, alpha=True)
    img.pixels.foreach_set(np.ascontiguousarray(arr, dtype=np.float32).ravel())
    img.filepath_raw = os.path.join(OUT, name)
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def over_backdrop(px):
    a = px[..., 3:4]
    rgb = px[..., :3] * a + np.array(BG_SRGB, dtype=np.float32) * (1.0 - a)
    return np.concatenate([rgb, np.ones_like(a)], axis=-1)


# 1. Hero shot
set_plump(0.0)
hero = render("hero_sprite.png", 1280, 960, 128)
save_rgba(over_backdrop(load_rgba(hero)), "hero.png")

# 2. Normal vs plump, side by side (same model, one shape-key slider)
normal = load_rgba(render("_normal.png", 800, 600, 96))
set_plump(1.0)
plump = load_rgba(render("_plump.png", 800, 600, 96))
save_rgba(np.concatenate([over_backdrop(normal), over_backdrop(plump)], axis=1), "normal_vs_plump.png")
for tmp in ("_normal.png", "_plump.png"):
    os.remove(os.path.join(OUT, tmp))

# 3. Actual Core2 screen size, plus a 3x nearest-neighbour blow-up to inspect pixels
set_plump(0.0)
device = over_backdrop(load_rgba(render("_device.png", 320, 240, 64)))
os.remove(os.path.join(OUT, "_device.png"))
save_rgba(device, "core2_320x240.png")
save_rgba(device.repeat(3, axis=0).repeat(3, axis=1), "core2_320x240_x3.png")

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "plush_pup.blend"))
print("DONE ->", OUT)
