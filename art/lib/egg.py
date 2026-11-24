"""Eggs for the cel-shaded pets: a shell that can split along a jagged crack line, ink crack
strokes that follow the same line, patches that hug the shell, and the frame/render/sheet
pipeline for incubation stages (0-3) and hatch frames. Imported by art/models/egg_*.py.

Techniques used (names worth looking up):
  * cel shading + inverted-hull outlines   - from lib/toon.py
  * folding a sphere into a cap            - the split halves are spheres whose vertices past the
                                             crack line are folded onto it, closing each half
                                             with an inner "lid" drawn in a dark interior colour
  * limited animation                      - 4 hatch key poses, not smooth in-betweens
"""
import math
import os

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector

from . import toon as tk


# ---------------------------------------------------------------- shape
class EggShape:
    """A tapered ellipsoid: a unit-sphere point (x, y, z) maps to (x*f*rx, y*f*ry, z*rz) with
    f = 1 - taper*(z+1)/2, so the shape narrows toward the top like an egg."""

    def __init__(self, center, radii, taper):
        self.c, self.r, self.taper = Vector(center), radii, taper

    def map(self, u, scale=1.0):
        f = 1.0 - self.taper * (u.z + 1.0) / 2.0
        return Vector((u.x * f * self.r[0], u.y * f * self.r[1], u.z * self.r[2])) * scale

    @staticmethod
    def unit(theta, z):
        z = max(-1.0, min(1.0, z))
        rho = math.sqrt(max(0.0, 1.0 - z * z))
        return Vector((rho * math.cos(theta), rho * math.sin(theta), z))

    def point(self, theta, z, lift=0.0):
        return self.c + self.map(self.unit(theta, z)) + self.normal(theta, z) * lift

    def frame(self, theta, z):
        """Local (east, north, normal) basis on the surface."""
        e = 1e-3
        p0 = self.map(self.unit(theta, z))
        pe = self.map(self.unit(theta + e, z))
        pn = self.map(self.unit(theta, min(0.999, z + e)))
        east = (pe - p0).normalized()
        north = (pn - p0).normalized()
        n = east.cross(north).normalized()
        if n.dot(p0) < 0:
            n = -n
        north = n.cross(east).normalized()
        return east, north, n

    def normal(self, theta, z):
        return self.frame(theta, z)[2]

    def world_z(self, z):
        return self.c.z + z * self.r[2]


def tri(t):
    """Triangle wave: period 1, range -1..1 (gives the zigzag of the crack line)."""
    t %= 1.0
    return 4.0 * abs(t - 0.5) - 1.0


class Crack:
    """The split line, as unit-sphere height z = c0 + amp * tri(theta * teeth / 2pi + phase)."""

    def __init__(self, c0, amp, teeth, phase=0.0):
        self.c0, self.amp, self.teeth, self.phase = c0, amp, teeth, phase

    def __call__(self, theta):
        return self.c0 + self.amp * tri(theta * self.teeth / (2 * math.pi) + self.phase)

    @property
    def top(self):
        return self.c0 + self.amp

    def corners(self, th0, th1):
        """Zigzag corner angles between th0 and th1 (inclusive ends)."""
        step = math.pi / self.teeth  # half a tooth
        k0 = math.ceil((th0 / step) + self.phase * 2)
        out = [th0]
        k = k0
        while True:
            th = (k / 2.0 - self.phase) * 2 * math.pi / self.teeth
            if th >= th1:
                break
            if th > th0:
                out.append(th)
            k += 1
        out.append(th1)
        return out


# ------------------------------------------------------------- geometry
def created(fn, *args, **kw):
    """Run fn and return the objects it added to the scene (a blob adds its outline shell too)."""
    before = {o.name for o in bpy.data.objects}
    fn(*args, **kw)
    return [o for o in bpy.data.objects if o.name not in before]


def shell(name, E, mat, part="whole", crack=None, interior=None, outline=None, outline_width=0.035,
          inflate=0.0, clip_below=None, segs=(96, 48), subsurf=2):
    """Egg shell (or a cap of it). part = whole | top | bottom.

    top/bottom fold every vertex past the crack line onto it, closing the half with an inner
    surface that gets the `interior` material (the dark inside of the shell).
    clip_below + inflate make a pattern cap (e.g. the husky's grey crown) on a whole egg;
    clip_below may be a function of theta, so the cap's edge can dip (a widow's peak).
    """
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segs[0], v_segments=segs[1], radius=1.0)
    coords, folded = [], []
    for v in bm.verts:
        x, y, z = v.co
        th = math.atan2(y, x)
        mode, c = None, None
        if part == "top":
            mode, c = "below", crack(th)
        elif part == "bottom":
            mode, c = "above", crack(th)
        elif clip_below is not None:
            mode, c = "below", clip_below(th) if callable(clip_below) else clip_below
        was_folded = False
        if mode == "below" and z < c:
            rho = math.sqrt(max(1e-9, 1.0 - z * z))
            k = math.sqrt(max(0.0, 1.0 - c * c)) * (z + 1.0) / (c + 1.0) / rho
            x, y, z, was_folded = x * k, y * k, c, True
        elif mode == "above" and z > c:
            rho = math.sqrt(max(1e-9, 1.0 - z * z))
            k = math.sqrt(max(0.0, 1.0 - c * c)) * (1.0 - z) / (1.0 - c) / rho
            x, y, z, was_folded = x * k, y * k, c, True
        coords.append(E.map(Vector((x, y, z)), 1.0 + inflate))
        folded.append(was_folded)

    for v, co in zip(bm.verts, coords):
        v.co = co
    bm.normal_update()
    shell_coords = [v.co + v.normal * outline_width for v in bm.verts] if outline is not None else None
    fold_idx = {i for i, f in enumerate(folded) if f}

    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    me.materials.append(mat)
    if interior is not None and part in ("top", "bottom"):
        me.materials.append(interior)
        for p in me.polygons:
            if all(vi in fold_idx for vi in p.vertices):
                p.material_index = 1
    _finish(me, name, E.c, subsurf)

    if shell_coords is not None:
        for v, co in zip(bm.verts, shell_coords):
            v.co = co
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])
        sme = bpy.data.meshes.new(name + ".Outline")
        bm.to_mesh(sme)
        sme.materials.append(outline)
        sob = _finish(sme, name + ".Outline", E.c, subsurf)
        sob.visible_shadow = False  # an enclosing opaque shell would otherwise shadow the egg
    bm.free()


def _finish(me, name, loc, subsurf):
    for p in me.polygons:
        p.use_smooth = True
    ob = bpy.data.objects.new(name, me)
    ob.location = loc
    tk.scene().collection.objects.link(ob)
    if subsurf:
        mod = ob.modifiers.new("Subsurf", "SUBSURF")
        mod.levels = 1
        mod.render_levels = subsurf
    return ob


def on_surface_rot(E, theta, z, spin=0.0):
    """Euler rotation putting a blob's local X/Y along the surface (east/north, turned by `spin`)
    and its local Z along the outward normal."""
    east, north, n = E.frame(theta, z)
    m = Matrix((east, north, n)).transposed()
    q = Quaternion(n, spin) @ m.to_quaternion()
    return tuple(q.to_euler("XYZ"))


def patch(name, E, theta, z, radii, mat, spin=0.0, embed=0.55, outline=None, width=0.03):
    """A coloured patch hugging the shell: a flattened blob sunk into the surface so only its
    gently domed top shows, with an ink edge where it meets the shell."""
    n = E.normal(theta, z)
    loc = E.point(theta, z) - n * radii[2] * embed
    tk.blob(name, loc, radii, mat, rot=on_surface_rot(E, theta, z, spin), outline=outline,
            outline_width=width, segs=(32, 16))


def stroke(name, E, pts, mat, radius=0.017, lift=0.01):
    """An ink line drawn on the shell through (theta, z) points (a beveled curve, i.e. a tube)."""
    cu = bpy.data.curves.new(name, "CURVE")
    cu.dimensions = "3D"
    cu.bevel_depth = radius
    cu.bevel_resolution = 2
    cu.use_fill_caps = True
    sp = cu.splines.new("POLY")
    sp.points.add(len(pts) - 1)
    for p, (th, z) in zip(sp.points, pts):
        w = E.point(th, z, lift)
        p.co = (w.x, w.y, w.z, 1.0)
    cu.materials.append(mat)
    ob = bpy.data.objects.new(name, cu)
    ob.visible_shadow = False
    tk.scene().collection.objects.link(ob)
    return ob


def densify(pts, n=4):
    """Insert n-1 points between each pair (keeps long strokes hugging the curved shell)."""
    out = [pts[0]]
    for (a0, b0), (a1, b1) in zip(pts, pts[1:]):
        for i in range(1, n + 1):
            t = i / n
            out.append((a0 + (a1 - a0) * t, b0 + (b1 - b0) * t))
    return out


def seam(crack, th0, th1):
    return densify([(th, crack(th)) for th in crack.corners(th0, th1)])


def branch(crack, theta, length=0.2, drift=0.06, toward=-1):
    """A short side crack leaving the seam at `theta`, heading down (toward=-1) or up (+1)."""
    z = crack(theta)
    return densify([(theta, z), (theta + drift, z + toward * length * 0.55),
                    (theta - drift * 0.4, z + toward * length)], 3)


# ---------------------------------------------------------------- scene
def empty(name, loc, parent=None):
    e = bpy.data.objects.new(name, None)
    e.location = loc
    tk.scene().collection.objects.link(e)
    if parent is not None:
        adopt(e, parent)
    return e


def adopt(child, parent):
    """Parent without moving: the child keeps its world position at the parent's rest pose."""
    bpy.context.view_layer.update()
    child.parent = parent
    child.matrix_parent_inverse = parent.matrix_world.inverted()


class Ctx:
    """What a pet's decorate() and baby() callbacks get: shape, crack line, ink, camera."""

    def __init__(self, E, crack, ink, cam, front):
        self.E, self.crack, self.ink, self.cam, self.front = E, crack, ink, cam, front


def build_egg(cfg):
    """Build the whole egg scene and render every frame + sheets. See egg_sausage_dog.py."""
    tk.clear_scene()
    E = EggShape(cfg["center"], cfg["radii"], cfg["taper"])
    crack = Crack(*cfg["crack"])
    cam = Vector(cfg["cam"])
    front = math.atan2(cam.y, cam.x)  # the shell's side facing the camera
    ink = tk.ink(color=cfg["ink"])
    shell_m = tk.toon(cfg["name"] + "_shell", cfg["shell"])
    inside_m = tk.flat(cfg["name"] + "_inside", cfg["interior"])
    ctx = Ctx(E, crack, ink, cam, front)

    cut_z = E.world_z(crack.c0)
    root = empty("EggRoot", (0, 0, 0))
    lid_e = empty("Lid", (0, 0, cut_z), root)
    baby_e = empty("Baby", (0, 0, cut_z), root)

    g = {}
    g["whole"] = created(shell, "Shell", E, shell_m, outline=ink)
    g["top"] = created(shell, "ShellTop", E, shell_m, part="top", crack=crack, interior=inside_m, outline=ink)
    g["bottom"] = created(shell, "ShellBottom", E, shell_m, part="bottom", crack=crack, interior=inside_m,
                          outline=ink)
    lid_dec, base_dec = cfg["decorate"](ctx)
    g["lid_dec"], g["base_dec"] = lid_dec, base_dec

    # Crack stages build on each other: stage n shows strokes 1..n.
    span = cfg.get("crack_spans", (0.35, 0.95, 1.65))
    g["crack1"] = [stroke("Crack1", E, seam(crack, front - span[0], front + span[0] * 0.6), ink)]
    g["crack2"] = [stroke("Crack2a", E, seam(crack, front - span[1], front - span[0]), ink),
                   stroke("Crack2b", E, seam(crack, front + span[0] * 0.6, front + span[1] * 0.8), ink),
                   stroke("Crack2c", E, branch(crack, front + 0.08, 0.20, toward=-1), ink)]
    g["crack3"] = [stroke("Crack3a", E, seam(crack, front - span[2], front - span[1]), ink),
                   stroke("Crack3b", E, seam(crack, front + span[1] * 0.8, front + span[2]), ink),
                   stroke("Crack3c", E, branch(crack, front - 0.62, 0.24, drift=-0.07, toward=-1), ink),
                   stroke("Crack3d", E, branch(crack, front + 0.55, 0.16, toward=+1), ink)]

    g["baby"] = created(cfg["baby"], ctx, Vector((0, 0, cut_z)))

    for name in ("whole", "bottom", "base_dec", "crack1", "crack2", "crack3"):
        for o in g[name]:
            adopt(o, root)
    for o in g["top"] + g["lid_dec"]:
        adopt(o, lid_e)
    for o in g["baby"]:
        adopt(o, baby_e)

    sh = cfg["shadow"]
    tk.setup_stage(tuple(cam), cfg["target"], key_loc=cfg["key"], shadow=sh)
    shadow_ob = bpy.data.objects["BlobShadow"]

    # Screen-space helpers for the hatch poses: `toward` points at the camera (rotating about it
    # tilts the lid in the picture plane), `right` is screen-right.
    toward = Vector((cam.x, cam.y, 0)).normalized()
    right = (-toward).cross(Vector((0, 0, 1))).normalized()
    up = Vector((0, 0, 1))
    lid_rest = Vector((0, 0, cut_z))
    head_top = cfg.get("head_top", 0.36)

    def pose(stage=0, halves=False, baby=False, squash=1.0, lid_off=(0, 0), lid_tilt=0.0, baby_up=0.0):
        vis = set()
        vis.update(g["base_dec"] + g["lid_dec"])
        if halves:
            vis.update(g["top"] + g["bottom"])
        else:
            vis.update(g["whole"])
            for s in range(1, stage + 1):
                vis.update(g[f"crack{s}"])
        if baby:
            vis.update(g["baby"])
        names = {o.name for o in vis}
        for o in bpy.data.objects:
            if o.type in ("MESH", "CURVE") and o.name != shadow_ob.name:
                o.hide_render = o.name not in names
        sx = 1.0 + (1.0 - squash) * 0.6
        root.scale = (sx, sx, squash)
        lid_e.location = lid_rest + up * lid_off[0] + right * lid_off[1]
        lid_e.rotation_mode = "QUATERNION"
        lid_e.rotation_quaternion = Quaternion(toward, lid_tilt)
        baby_e.location = lid_rest + up * baby_up

    frames = [
        ("stage0", dict(stage=0)),
        ("stage1", dict(stage=1)),
        ("stage2", dict(stage=2)),
        ("stage3", dict(stage=3)),
        # Hatch, in limited-animation key poses: squash (anticipation), pop, peek, shell hat.
        ("hatch1", dict(stage=3, squash=0.88)),
        ("hatch2", dict(halves=True, lid_off=(0.24, 0.05), lid_tilt=-0.28)),
        ("hatch3", dict(halves=True, baby=True, baby_up=0.06, lid_off=(0.62, 0.34), lid_tilt=-0.62)),
        ("hatch4", dict(halves=True, baby=True, baby_up=head_top,
                        lid_off=(head_top + cfg.get("hat_lift", 0.30), -0.04), lid_tilt=0.30)),
    ]

    out = cfg["out"]
    os.makedirs(out, exist_ok=True)
    name = cfg["name"]
    sprites = {}
    shadow_ob.hide_render = True
    for fname, kw in frames:
        pose(**kw)
        path = tk.render(os.path.join(out, f"{name}_{fname}_sprite.png"), 320, 240)
        sprites[fname] = tk.load_rgba(path)

    # Shadow-only layer (optional on device: draw it under the egg, unrotated, when the egg leans).
    pose(stage=0)
    for o in bpy.data.objects:
        if o.type in ("MESH", "CURVE"):
            o.hide_render = o is not shadow_ob
    shadow_ob.hide_render = False
    shadow = tk.load_rgba(tk.render(os.path.join(out, f"{name}_shadow.png"), 320, 240))

    bg = np.array(cfg.get("backdrop", (0.98, 0.90, 0.84)), dtype=np.float32)

    def composite(px):
        base = shadow[..., :3] * shadow[..., 3:4] + bg * (1 - shadow[..., 3:4])
        rgb = px[..., :3] * px[..., 3:4] + base * (1 - px[..., 3:4])
        return np.concatenate([rgb, np.ones_like(px[..., 3:4])], axis=-1)

    frames_rgb = {k: composite(v) for k, v in sprites.items()}
    tk.save_rgba(frames_rgb["stage0"], os.path.join(out, f"{name}_320x240.png"))
    # Pixel rows are stored bottom-up, so the second row of the sheet goes first.
    row_stages = np.concatenate([frames_rgb[f"stage{i}"] for i in range(4)], axis=1)
    row_hatch = np.concatenate([frames_rgb[f"hatch{i}"] for i in range(1, 5)], axis=1)
    sheet = np.concatenate([row_hatch, row_stages], axis=0)
    tk.save_rgba(sheet, os.path.join(out, f"{name}_sheet.png"))
    tk.save_rgba(tk.upscale(sheet, 2), os.path.join(out, f"{name}_sheet_x2.png"))

    a = sprites["stage0"][..., 3] > 0.5
    rows = np.where(a.any(axis=1))[0]
    cols = np.where(a.any(axis=0))[0]
    print(f"[{name}] stage0 sprite bbox: {cols.max() - cols.min() + 1} x {rows.max() - rows.min() + 1} px")

    pose(stage=0)
    shadow_ob.hide_render = False
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, f"{name}.blend"))
    print("DONE ->", out)
