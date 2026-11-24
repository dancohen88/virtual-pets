"""Shared prototype pup: geometry, materials, stage and render helpers.

Imported by render scripts that run inside Blender. (plush_pup.py predates this module and
still carries its own copy; fold it in once the art direction settles.)
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Euler, Vector

BG_SRGB = (0.98, 0.90, 0.84)
CAM_LOC = Vector((5.0, -8.0, 3.4))
COLORS = {  # sRGB
    "caramel": (0.80, 0.55, 0.33),
    "cocoa": (0.52, 0.32, 0.19),
    "muzzle": (0.95, 0.80, 0.63),
    "nose": (0.13, 0.08, 0.07),
    "eye": (0.02, 0.02, 0.02),
    "outline": (0.24, 0.13, 0.08),
}


def scene():
    return bpy.context.scene


def clear_scene():
    for ob in list(bpy.data.objects):
        bpy.data.objects.remove(ob, do_unlink=True)


def srgb_to_linear(c):
    return tuple(((x + 0.055) / 1.055) ** 2.4 if x > 0.04045 else x / 12.92 for x in c)


def set_input(node, name, value):
    if name in node.inputs:
        node.inputs[name].default_value = value


def _material(name):
    m = bpy.data.materials.new(name)
    if m.node_tree is None:  # older Blender needed use_nodes; 5.x always has a tree
        m.use_nodes = True
    return m, m.node_tree


def _principled(nt):
    bsdf = next((n for n in nt.nodes if n.type == "BSDF_PRINCIPLED"), None)
    if bsdf is None:
        bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
        nt.links.new(bsdf.outputs["BSDF"], _output(nt).inputs["Surface"])
    return bsdf


def _output(nt):
    return next(n for n in nt.nodes if n.type == "OUTPUT_MATERIAL")


# ------------------------------------------------------------ material styles
def plush_material(name, color, fuzz=1.0, bump=0.15, noise_scale=60.0, subsurface=0.0):
    m, nt = _material(name)
    bsdf = _principled(nt)
    set_input(bsdf, "Base Color", (*srgb_to_linear(color), 1.0))
    set_input(bsdf, "Roughness", 0.9)
    set_input(bsdf, "Sheen Weight", fuzz)
    set_input(bsdf, "Sheen Roughness", 0.35)
    set_input(bsdf, "Specular IOR Level", 0.25)
    if subsurface:
        set_input(bsdf, "Subsurface Weight", subsurface)
        set_input(bsdf, "Subsurface Scale", 0.05)
    coords = nt.nodes.new("ShaderNodeTexCoord")
    noise = nt.nodes.new("ShaderNodeTexNoise")
    set_input(noise, "Scale", noise_scale)
    set_input(noise, "Detail", 6.0)
    bump_node = nt.nodes.new("ShaderNodeBump")
    set_input(bump_node, "Strength", bump)
    set_input(bump_node, "Distance", 0.02)
    nt.links.new(coords.outputs["Object"], noise.inputs["Vector"])
    nt.links.new(noise.outputs["Fac"], bump_node.inputs["Height"])
    nt.links.new(bump_node.outputs["Normal"], bsdf.inputs["Normal"])
    return m


def bead_material(name, color, roughness=0.08):
    m, nt = _material(name)
    bsdf = _principled(nt)
    set_input(bsdf, "Base Color", (*srgb_to_linear(color), 1.0))
    set_input(bsdf, "Roughness", roughness)
    return m


def toon_material(name, color, shade=(0.66, 0.58, 0.70), hi=1.10, bands=(0.0, 0.30, 0.78)):
    """Cel shading (EEVEE only): diffuse light -> Shader to RGB -> constant colour ramp -> emission.

    The ramp snaps smooth lighting into flat bands; the shadow band is hue-shifted toward
    purple rather than just darkened, which is what makes cartoon shadows look painted.
    """
    m, nt = _material(name)
    for n in list(nt.nodes):
        if n.type != "OUTPUT_MATERIAL":
            nt.nodes.remove(n)
    diffuse = nt.nodes.new("ShaderNodeBsdfDiffuse")
    to_rgb = nt.nodes.new("ShaderNodeShaderToRGB")
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    emit = nt.nodes.new("ShaderNodeEmission")
    ramp.color_ramp.interpolation = "CONSTANT"
    dark = tuple(c * s for c, s in zip(color, shade))
    light = tuple(min(1.0, c * hi) for c in color)
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = bands[0], (*srgb_to_linear(dark), 1)
    els[1].position, els[1].color = bands[1], (*srgb_to_linear(color), 1)
    e = els.new(bands[2])
    e.color = (*srgb_to_linear(light), 1)
    nt.links.new(diffuse.outputs["BSDF"], to_rgb.inputs["Shader"])
    nt.links.new(to_rgb.outputs["Color"], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], emit.inputs["Color"])
    nt.links.new(emit.outputs["Emission"], _output(nt).inputs["Surface"])
    return m


def flat_material(name, color, alpha=1.0):
    """Unlit colour, optionally see-through (for outlines, eye glints, blob shadows)."""
    m, nt = _material(name)
    for n in list(nt.nodes):
        if n.type != "OUTPUT_MATERIAL":
            nt.nodes.remove(n)
    emit = nt.nodes.new("ShaderNodeEmission")
    set_input(emit, "Color", (*srgb_to_linear(color), 1))
    out = _output(nt)
    if alpha < 1.0:
        transp = nt.nodes.new("ShaderNodeBsdfTransparent")
        mix = nt.nodes.new("ShaderNodeMixShader")
        set_input(mix, "Fac", alpha)
        nt.links.new(transp.outputs["BSDF"], mix.inputs[1])
        nt.links.new(emit.outputs["Emission"], mix.inputs[2])
        nt.links.new(mix.outputs["Shader"], out.inputs["Surface"])
    else:
        nt.links.new(emit.outputs["Emission"], out.inputs["Surface"])
    return m


def outline_material(name, color):
    """Inverted-hull outline (EEVEE): the shell's faces are flipped, so backface culling hides
    the side nearest the camera and only the far side peeks out around the silhouette as ink.
    Opaque on purpose: EEVEE dithers semi-transparent materials, which speckles at sprite size."""
    m = flat_material(name, color)
    m.use_backface_culling = True
    return m


# ---------------------------------------------------------------- geometry
def blob(name, center, radii, mat, rot=(0, 0, 0), blunt=None, segs=(48, 24), subsurf=2,
         outline=None, outline_width=0.055):
    """A squashed sphere. `blunt` < 1 flattens the X ends (ellipsoid -> sausage).
    With `outline` (a material), also builds an inverted-hull shell around it."""
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs[0], v_segments=segs[1], radius=1.0)
    rmat = Euler(rot).to_matrix()
    for v in bm.verts:
        x, y, z = v.co
        if blunt:
            x = math.copysign(abs(x) ** blunt, x)
        v.co = rmat @ Vector((x * radii[0], y * radii[1], z * radii[2]))
    obs = [_mesh_object(name, bm, mat, center, subsurf)]
    if outline is not None:
        bm.normal_update()
        for v in bm.verts:
            v.co += v.normal * outline_width
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        shell = _mesh_object(name + ".Outline", bm, outline, center, subsurf)
        shell.visible_shadow = False  # an enclosing opaque shell would otherwise shadow the whole body
        obs.append(shell)
    bm.free()
    return obs[0]


def _mesh_object(name, bm, mat, center, subsurf):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    for p in me.polygons:
        p.use_smooth = True
    me.materials.append(mat)
    ob = bpy.data.objects.new(name, me)
    ob.location = center
    scene().collection.objects.link(ob)
    if subsurf:
        mod = ob.modifiers.new("Subsurf", "SUBSURF")
        mod.levels = 1
        mod.render_levels = subsurf
    return ob


def add_plump_key(ob, radii, amount):
    me = ob.data
    ob.shape_key_add(name="Basis")
    key = ob.shape_key_add(name="Plump")
    for i, v in enumerate(me.vertices):
        co, n = v.co, v.normal
        middle = 1.0 - min(1.0, abs(co.x) / radii[0]) ** 2
        belly = 0.55 + 0.45 * max(0.0, -co.z / radii[2])
        key.data[i].co = co + n * amount * middle * belly
    return key


BODY_C, BODY_R = (0.0, 0.0, 1.0), (1.8, 0.72, 0.68)
HEAD_C, HEAD_R = Vector((1.85, 0.0, 1.6)), (0.62, 0.6, 0.56)


def build_pup(m, outline=None, eye_glints=None):
    """m: dict of materials keyed caramel/cocoa/muzzle/nose/eye. Returns {'body','head'}."""
    kw = dict(outline=outline)
    body = blob("Body", BODY_C, BODY_R, m["caramel"], blunt=0.55, segs=(64, 32), **kw)
    head = blob("Head", HEAD_C, HEAD_R, m["caramel"], **kw)
    blob("Snout", (2.36, 0.0, 1.45), (0.40, 0.32, 0.27), m["muzzle"], **kw)
    blob("Nose", (2.79, 0.0, 1.53), (0.10, 0.12, 0.08), m["nose"], segs=(24, 12))
    for side in (1, -1):
        s = "L" if side > 0 else "R"
        eye_pos = HEAD_C + Vector((0.62, 0.52 * side, 0.42)).normalized() * 0.57
        blob(f"Eye.{s}", eye_pos, (0.075,) * 3, m["eye"], segs=(24, 12), subsurf=1)
        if eye_glints is not None:  # painted catchlight for styles without real reflections
            to_cam = (CAM_LOC - eye_pos).normalized()
            blob(f"Glint.{s}", eye_pos + to_cam * 0.07 + Vector((0, 0, 0.03)), (0.024,) * 3,
                 eye_glints, segs=(12, 6), subsurf=0)
        blob(f"Ear.{s}", (1.70, 0.58 * side, 1.32), (0.27, 0.06, 0.5), m["cocoa"],
             rot=(0.06 * side, 0.3, 0), **kw)
        for front in (1, -1):
            blob(f"Leg.{'F' if front > 0 else 'B'}{s}", (1.15 * front, 0.40 * side, 0.28),
                 (0.2, 0.2, 0.3), m["caramel"], **kw)
    blob("Tail", (-2.0, 0.0, 1.35), (0.32, 0.09, 0.09), m["cocoa"], rot=(0, 0.6, 0), **kw)
    add_plump_key(body, BODY_R, 0.32)
    add_plump_key(head, HEAD_R, 0.06)
    parts = {"body": body, "head": head}
    set_plump(parts, 0.0)  # new shape keys aren't guaranteed to start at zero
    return parts


def set_plump(parts, value):
    for ob in (parts["body"], parts["head"]):
        ob.data.shape_keys.key_blocks["Plump"].value = value


# ------------------------------------------------------------------- stage
def setup_stage(lights="soft", shadow_catcher=True, world_strength=0.4):
    sc = scene()
    target = bpy.data.objects.new("Target", None)
    target.location = (0.35, 0.0, 1.0)
    sc.collection.objects.link(target)

    def aim(ob):
        c = ob.constraints.new("TRACK_TO")
        c.target, c.track_axis, c.up_axis = target, "TRACK_NEGATIVE_Z", "UP_Y"

    cam_data = bpy.data.cameras.new("Camera")
    cam_data.lens = 60
    cam = bpy.data.objects.new("Camera", cam_data)
    cam.location = CAM_LOC
    sc.collection.objects.link(cam)
    aim(cam)
    sc.camera = cam

    def area(name, loc, energy, size, color, shadow=True):
        d = bpy.data.lights.new(name, "AREA")
        d.energy, d.size, d.color, d.use_shadow = energy, size, color, shadow
        ob = bpy.data.objects.new(name, d)
        ob.location = loc
        sc.collection.objects.link(ob)
        aim(ob)

    if lights == "soft":
        area("Key", (2.0, -3.0, 8.0), 1300, 7.0, (1.0, 0.95, 0.88))
        area("Fill", (-5.0, -4.0, 2.5), 250, 6.0, (0.88, 0.92, 1.0), shadow=False)
        area("Rim", (-3.0, 5.0, 4.5), 700, 3.0, (1.0, 1.0, 1.0), shadow=False)
    else:  # toon: one dominant light so the bands read clearly
        area("Key", (3.0, -5.0, 7.0), 1500, 2.0, (1.0, 1.0, 1.0))

    if shadow_catcher:
        bpy.ops.mesh.primitive_plane_add(size=40, location=(0, 0, 0))
        bpy.context.active_object.is_shadow_catcher = True

    world = bpy.data.worlds.new("World")
    sc.world = world
    if world.node_tree is None:
        world.use_nodes = True
    bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    set_input(bg, "Color", (*srgb_to_linear(BG_SRGB), 1.0))
    set_input(bg, "Strength", world_strength)


def use_cycles():
    sc = scene()
    sc.render.engine = "CYCLES"
    sc.cycles.use_denoising = True
    sc.view_settings.view_transform = "AgX"
    try:
        prefs = bpy.context.preferences.addons["cycles"].preferences
        prefs.compute_device_type = "METAL"
        prefs.get_devices()
        for d in prefs.devices:
            d.use = True
        sc.cycles.device = "GPU"
    except Exception as e:
        print("GPU setup failed, rendering on CPU:", e)


def use_eevee():
    sc = scene()
    for engine in ("BLENDER_EEVEE", "BLENDER_EEVEE_NEXT"):
        try:
            sc.render.engine = engine
            break
        except TypeError:
            continue
    sc.view_settings.view_transform = "Standard"  # flat colours must come out exactly as authored


def render(out_dir, name, w, h, samples=64):
    sc = scene()
    sc.render.film_transparent = True
    sc.render.resolution_percentage = 100
    sc.render.image_settings.file_format = "PNG"
    sc.render.image_settings.color_mode = "RGBA"
    sc.render.resolution_x, sc.render.resolution_y = w, h
    if sc.render.engine == "CYCLES":
        sc.cycles.samples = samples
    else:
        sc.eevee.taa_render_samples = samples
    path = os.path.join(out_dir, name)
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)
    return path


def load_rgba(path):
    img = bpy.data.images.load(path)
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
    bpy.data.images.remove(img)
    return px


def save_rgba(arr, path):
    h, w = arr.shape[:2]
    img = bpy.data.images.new("tmp", w, h, alpha=True)
    img.pixels.foreach_set(np.ascontiguousarray(arr, dtype=np.float32).ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def over_backdrop(px, bg=BG_SRGB):
    a = px[..., 3:4]
    rgb = px[..., :3] * a + np.array(bg, dtype=np.float32) * (1.0 - a)
    return np.concatenate([rgb, np.ones_like(a)], axis=-1)
