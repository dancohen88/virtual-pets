"""Shared toolkit for the cel-shaded pets: materials, soft "blob" geometry with ink outlines,
shape keys, stage, render and image helpers. Imported by scripts running inside Blender.

Techniques used (names worth looking up):
  * toon / cel shading  - Diffuse -> Shader to RGB -> constant ColorRamp -> Emission (EEVEE only)
  * inverted hull       - a slightly inflated copy with flipped faces; backface culling hides the
                          near side so only the rim shows, as an ink outline
  * shape keys          - alternative vertex positions blended by a 0..1 slider (plump, smug...)
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Euler, Vector


# ----------------------------------------------------------------- basics
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
    nt = m.node_tree
    for n in list(nt.nodes):
        if n.type != "OUTPUT_MATERIAL":
            nt.nodes.remove(n)
    out = next(n for n in nt.nodes if n.type == "OUTPUT_MATERIAL")
    return m, nt, out


# -------------------------------------------------------------- materials
def toon(name, color, shade=(0.66, 0.58, 0.70), hi=1.10, bands=(0.0, 0.30, 0.78)):
    """Three flat bands: shadow (hue-shifted toward purple, like painted cartoon shadows), base, highlight."""
    m, nt, out = _material(name)
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
    nt.links.new(emit.outputs["Emission"], out.inputs["Surface"])
    return m


def flat(name, color):
    """Unlit, opaque colour (eyes, glints, ink strokes). Opaque on purpose: EEVEE dithers
    semi-transparent materials, which speckles at sprite size."""
    m, nt, out = _material(name)
    emit = nt.nodes.new("ShaderNodeEmission")
    set_input(emit, "Color", (*srgb_to_linear(color), 1))
    nt.links.new(emit.outputs["Emission"], out.inputs["Surface"])
    return m


def ink(name="Ink", color=(0.22, 0.13, 0.10)):
    """Outline material for inverted-hull shells."""
    m = flat(name, color)
    m.use_backface_culling = True
    return m


# --------------------------------------------------------------- geometry
def blob(name, center, radii, mat, rot=(0, 0, 0), blunt=None, taper=0.0, clip_below=None,
         segs=(48, 24), subsurf=2, outline=None, outline_width=0.04, keys=None):
    """A soft squashed sphere, optionally with an ink-outline shell and shape keys.

    blunt       < 1 flattens the X ends (ellipsoid -> sausage)
    taper       narrows the shape toward +Z (rounded cone, for ears)
    clip_below  flattens everything below this unit-sphere height (a dome, for eyelids)
    keys        {name: spec}; a spec is a function (see k_* helpers) returning new vertex positions.
                The outline shell gets matching keys so it follows the surface.
    """
    params = dict(radii=radii, rot=rot, blunt=blunt, taper=taper, clip_below=clip_below)
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs[0], v_segments=segs[1], radius=1.0)
    unit = [v.co.copy() for v in bm.verts]

    def shape(p):
        rmat = Euler(p["rot"]).to_matrix()
        out = []
        for u in unit:
            x, y, z = u
            c = p["clip_below"]
            if c is not None and z < c:
                # Fold everything below the cut into the flat disc of a spherical cap
                # (radius shrinks from the cut's edge to zero at the bottom pole).
                rho = math.sqrt(max(1e-9, 1.0 - z * z))
                k = math.sqrt(max(0.0, 1.0 - c * c)) * (z + 1.0) / (c + 1.0) / rho
                x, y, z = x * k, y * k, c
            if p["blunt"]:
                x = math.copysign(abs(x) ** p["blunt"], x)
            f = 1.0 - p["taper"] * (z + 1.0) / 2.0
            out.append(rmat @ Vector((x * f * p["radii"][0], y * f * p["radii"][1], z * p["radii"][2])))
        return out

    def normals_for(coords):
        for v, c in zip(bm.verts, coords):
            v.co = c
        bm.normal_update()
        return [v.normal.copy() for v in bm.verts]

    base = shape(params)
    base_n = normals_for(base)
    variants = {"Basis": base}
    for kname, spec in (keys or {}).items():
        variants[kname] = spec(shape=shape, params=params, coords=base, normals=base_n)

    shells = None
    if outline is not None:  # compute before flipping faces, so normals point outward
        shells = {k: [c + n * outline_width for c, n in zip(cs, normals_for(cs))]
                  for k, cs in variants.items()}

    for v, c in zip(bm.verts, base):
        v.co = c
    ob = _mesh_object(name, bm, mat, center, subsurf)
    _add_keys(ob, variants)

    if shells is not None:
        for v, c in zip(bm.verts, shells["Basis"]):
            v.co = c
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        shell = _mesh_object(name + ".Outline", bm, outline, center, subsurf)
        shell.visible_shadow = False  # an enclosing opaque shell would otherwise shadow the body
        _add_keys(shell, shells)
    bm.free()
    return ob


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


def _add_keys(ob, variants):
    if len(variants) < 2:
        return
    ob.shape_key_add(name="Basis")
    for name, coords in variants.items():
        if name == "Basis":
            continue
        kb = ob.shape_key_add(name=name, from_mix=False)
        for i, c in enumerate(coords):
            kb.data[i].co = c
        kb.value = 0.0  # new keys aren't guaranteed to start at zero


def set_key(name, value):
    """Set a shape key on every object that has it (surface and outline shell stay in sync)."""
    for ob in bpy.data.objects:
        keys = getattr(ob.data, "shape_keys", None)
        if keys and name in keys.key_blocks:
            keys.key_blocks[name].value = value


# Shape-key specs ------------------------------------------------------
def k_override(**over):
    """Rebuild the blob with some parameters changed (e.g. clip_below for an eyelid closing)."""
    def spec(shape, params, coords, normals):
        p = dict(params)
        p.update(over)
        return shape(p)
    return spec


def k_inflate(amount, half_length, half_height):
    """Swell along normals: most in the middle of the length, sagging toward the belly."""
    def spec(shape, params, coords, normals):
        out = []
        for c, n in zip(coords, normals):
            middle = 1.0 - min(1.0, abs(c.x) / half_length) ** 2
            belly = 0.55 + 0.45 * max(0.0, -c.z / half_height)
            out.append(c + n * amount * middle * belly)
        return out
    return spec


def k_shift(vec):
    def spec(shape, params, coords, normals):
        v = Vector(vec)
        return [c + v for c in coords]
    return spec


class HeadFrame:
    """Places parts relative to an ellipsoid (a head, a snout) that is turned toward the viewer:
    yaw turns it, pitch < 0 lifts the chin."""

    def __init__(self, center, radii, yaw=0.0, pitch=0.0):
        self.c = Vector(center)
        self.r = radii
        self.yaw, self.pitch = yaw, pitch
        self.m = Euler((0, pitch, yaw)).to_matrix()

    def at(self, fwd, side, up):
        """Head-local offset (forward, side, up) -> world position."""
        return self.c + self.m @ Vector((fwd, side, up))

    def surface(self, fwd, side, up, depth=1.0):
        """Point on the head ellipsoid in a head-local direction (depth < 1 sinks it in)."""
        d = Vector((fwd, side, up)).normalized()
        k = 1.0 / math.sqrt((d.x / self.r[0]) ** 2 + (d.y / self.r[1]) ** 2 + (d.z / self.r[2]) ** 2)
        return self.c + self.m @ (d * k * depth)

    def rot(self, x=0.0, y=0.0, z=0.0):
        """Local roll/pitch/yaw, then the head's yaw (Blender XYZ Euler applies Z last)."""
        return (x, y + self.pitch, z + self.yaw)


def glint(name, eye_pos, eye_r, cam_loc, mat, lift=0.35, size=0.3):
    """Painted catchlight: toon shading has no real reflections, so the sparkle is a tiny white dot.
    Lower `lift` when eyelids are half closed, or the dot floats on the lid."""
    to_cam = (Vector(cam_loc) - Vector(eye_pos)).normalized()
    pos = Vector(eye_pos) + to_cam * eye_r * 0.9 + Vector((0, 0, eye_r * lift))
    return blob(name, pos, (eye_r * size,) * 3, mat, segs=(12, 6), subsurf=0)


# ------------------------------------------------------------------ stage
def setup_stage(cam_loc, target, key_loc, lens=60, backdrop=(0.98, 0.90, 0.84), shadow=None, key_size=0.4):
    """Camera + one dominant key light (so the toon bands read) + dim world + optional blob shadow.

    shadow: (center_xy, radii_xy) for a flat ellipse in a solid tint of the backdrop.
    """
    sc = scene()
    tgt = bpy.data.objects.new("Target", None)
    tgt.location = target
    sc.collection.objects.link(tgt)

    def aim(ob):
        c = ob.constraints.new("TRACK_TO")
        c.target, c.track_axis, c.up_axis = tgt, "TRACK_NEGATIVE_Z", "UP_Y"

    cam_data = bpy.data.cameras.new("Camera")
    cam_data.lens = lens
    cam = bpy.data.objects.new("Camera", cam_data)
    cam.location = cam_loc
    sc.collection.objects.link(cam)
    aim(cam)
    sc.camera = cam

    light = bpy.data.lights.new("Key", "AREA")
    light.energy, light.size = 1500, key_size  # small light = crisp shadow edges, less band-edge noise
    lob = bpy.data.objects.new("Key", light)
    lob.location = key_loc
    sc.collection.objects.link(lob)
    aim(lob)

    world = bpy.data.worlds.new("World")
    sc.world = world
    if world.node_tree is None:
        world.use_nodes = True
    bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    set_input(bg, "Color", (*srgb_to_linear(backdrop), 1.0))
    set_input(bg, "Strength", 0.15)

    if shadow:
        (sx, sy), (rx, ry) = shadow
        bpy.ops.mesh.primitive_circle_add(vertices=48, radius=1.0, fill_type="NGON", location=(sx, sy, 0.005))
        s = bpy.context.active_object
        s.name = "BlobShadow"
        s.scale = (rx, ry, 1)
        s.data.materials.append(flat("BlobShadow", tuple(c * 0.84 for c in backdrop)))

    for engine in ("BLENDER_EEVEE", "BLENDER_EEVEE_NEXT"):
        try:
            sc.render.engine = engine
            break
        except TypeError:
            continue
    sc.view_settings.view_transform = "Standard"  # flat colours come out exactly as authored


# ----------------------------------------------------------------- output
def render(path, w, h, samples=64):
    sc = scene()
    sc.render.film_transparent = True
    sc.render.resolution_percentage = 100
    sc.render.image_settings.file_format = "PNG"
    sc.render.image_settings.color_mode = "RGBA"
    sc.render.resolution_x, sc.render.resolution_y = w, h
    sc.eevee.taa_render_samples = samples
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


def over_backdrop(px, bg=(0.98, 0.90, 0.84)):
    a = px[..., 3:4]
    rgb = px[..., :3] * a + np.array(bg, dtype=np.float32) * (1.0 - a)
    return np.concatenate([rgb, np.ones_like(a)], axis=-1)


def upscale(px, n):
    """Nearest-neighbour blow-up, to inspect device-size pixels."""
    return px.repeat(n, axis=0).repeat(n, axis=1)


def standard_outputs(out_dir, name, plump_key="Plump"):
    """Device frame, transparent sprite, 2x blow-up, normal-vs-plump sheet, and the .blend."""
    os.makedirs(out_dir, exist_ok=True)
    set_key(plump_key, 0.0)
    sprite = render(os.path.join(out_dir, f"{name}_sprite_320x240.png"), 320, 240)
    normal = over_backdrop(load_rgba(sprite))
    save_rgba(normal, os.path.join(out_dir, f"{name}_320x240.png"))
    save_rgba(upscale(normal, 2), os.path.join(out_dir, f"{name}_320x240_x2.png"))
    set_key(plump_key, 1.0)
    tmp = render(os.path.join(out_dir, "_plump.png"), 320, 240)
    plump = over_backdrop(load_rgba(tmp))
    os.remove(tmp)
    save_rgba(upscale(np.concatenate([normal, plump], axis=1), 2),
              os.path.join(out_dir, f"{name}_normal_vs_plump_x2.png"))
    set_key(plump_key, 0.0)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out_dir, f"{name}.blend"))
