"""The husky's egg: white shell with a grey crown and widow's peak (the husky mask), white brow
dots, two soft pointy ear bumps and a fluffy tail curl.

Run from the project root:
    blender --background --factory-startup --python art/models/egg_husky.py

Outputs in art/models/renders/egg_husky/ (same set as egg_sausage_dog.py).
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from lib import egg  # noqa: E402
from lib import toon as tk  # noqa: E402

COL = {
    "shell": (0.97, 0.96, 0.94),
    "grey": (0.56, 0.60, 0.68),
    "inner_ear": (0.93, 0.72, 0.74),
    "iris": (0.52, 0.78, 0.96),
    "pupil": (0.05, 0.05, 0.08),
    "nose": (0.12, 0.11, 0.13),
    "interior": (0.42, 0.44, 0.54),   # inside of the shell: a muted slate
}
CROWN = 0.34  # unit height where the grey crown starts (kept above the crack line)
PEAK_DIP, PEAK_WIDTH = 0.13, 0.42  # the crown edge dips into a V at the front: the widow's peak


def decorate(ctx):
    E, f, ink = ctx.E, ctx.front, ctx.ink
    grey, white = tk.toon("EGH_grey", COL["grey"]), tk.toon("EGH_white", COL["shell"])
    inner = tk.toon("EGH_inner", COL["inner_ear"])
    lid, base = [], []
    # Grey crown: an inflated copy of the egg, cut off below a height that dips at the front, so
    # the mask's widow's peak is part of the crown's own edge (one ink line, no separate blob).
    def crown_edge(th):
        d = abs((th - f + math.pi) % (2 * math.pi) - math.pi)  # angle from the front, 0..pi
        return CROWN - PEAK_DIP * max(0.0, 1.0 - d / PEAK_WIDTH)

    lid += egg.created(egg.shell, "Crown", E, grey, clip_below=crown_edge, inflate=0.012, outline=ink)
    for side in (1, -1):
        lid += egg.created(egg.patch, f"BrowDot.{side:+d}", E, f + 0.36 * side, 0.55, (0.05, 0.045, 0.05),
                           white, outline=ink, width=0.025)
        # Soft pointy ear bump: a rounded cone (taper) standing on the crown, leaning outward.
        th, z = f + 1.05 * side, 0.80
        east, north, n = E.frame(th, z)
        lean = (n * 0.55 + tk.Vector((0, 0, 1)) * 0.45).normalized()
        rot = tuple(tk.Vector((0, 0, 1)).rotation_difference(lean).to_euler("XYZ"))
        base_pt = E.point(th, z) - n * 0.03
        lid += egg.created(tk.blob, f"Ear.{side:+d}", base_pt + lean * 0.10, (0.13, 0.09, 0.17), grey, rot=rot,
                           taper=0.62, outline=ink, outline_width=0.03)
        lid += egg.created(tk.blob, f"InnerEar.{side:+d}", base_pt + lean * 0.10 + (ctx.cam - base_pt).normalized() * 0.03,
                           (0.07, 0.04, 0.11), inner, rot=rot, taper=0.62)
    # Fluffy tail curl on the side.
    p, n = E.point(f + 1.7, -0.25), E.normal(f + 1.7, -0.25)
    base += egg.created(tk.blob, "Tail.A", p + n * 0.03, (0.12, 0.11, 0.11), grey, outline=ink, outline_width=0.03)
    base += egg.created(tk.blob, "Tail.B", p + n * 0.10 + tk.Vector((0, 0, 0.14)), (0.08, 0.075, 0.075), white,
                        outline=ink, outline_width=0.03)
    return lid, base


def baby(ctx, at):
    """Newborn head: grey with a white mask, pointy ears, big pale-blue eyes."""
    ink, cam = ctx.ink, ctx.cam
    grey, white, inner = (tk.toon(f"EGH_b_{k}", COL[k]) for k in ("grey", "shell", "inner_ear"))
    iris, pupil, nose = (tk.flat(f"EGH_{k}", COL[k]) for k in ("iris", "pupil", "nose"))
    glint = tk.flat("EGH_glint", (1, 1, 1))
    O = dict(outline=ink, outline_width=0.03)
    H = tk.HeadFrame(at, (0.42, 0.40, 0.37), yaw=ctx.front)
    tk.blob("Pup.Head", H.c, H.r, grey, rot=H.rot(), **O)
    tk.blob("Pup.Mask", H.at(0.16, 0, -0.10), (0.32, 0.36, 0.26), white, rot=H.rot(), **O)
    tk.blob("Pup.Nose", H.at(0.44, 0, -0.06), (0.06, 0.08, 0.05), nose, rot=H.rot(), segs=(16, 8))
    for side in (1, -1):
        tk.blob(f"Pup.Ear.{side:+d}", H.at(-0.06, 0.24 * side, 0.34), (0.13, 0.08, 0.19), grey, taper=0.6,
                rot=H.rot(x=-0.30 * side), **O)
        tk.blob(f"Pup.InnerEar.{side:+d}", H.at(0.0, 0.24 * side, 0.32), (0.07, 0.04, 0.12), inner, taper=0.6,
                rot=H.rot(x=-0.30 * side))
        e = H.surface(0.84, 0.38 * side, 0.12, depth=0.97)
        tk.blob(f"Pup.Eye.{side:+d}", e, (0.085,) * 3, iris, segs=(20, 10), subsurf=1)
        look = (cam - e).normalized()
        tk.blob(f"Pup.Pupil.{side:+d}", e + look * 0.05, (0.047,) * 3, pupil, segs=(16, 8), subsurf=1)
        tk.glint(f"Pup.Glint.{side:+d}", e, 0.085, cam, glint, size=0.34)


CFG = dict(
    name="egg_husky",
    out=os.path.join(HERE, "renders", "egg_husky"),
    center=(0, 0, 0.84), radii=(0.62, 0.62, 0.84), taper=0.22,
    crack=(0.10, 0.07, 8, 0.25),
    shell=COL["shell"], interior=COL["interior"], ink=(0.16, 0.16, 0.22),
    decorate=decorate, baby=baby, head_top=0.38, hat_lift=0.24,
    cam=(6.0, -5.4, 2.7), target=(0.0, 0.0, 0.85), key=(3.0, -8.5, 7.5),
    shadow=((0.05, 0.0), (0.88, 0.64)),
)

egg.build_egg(CFG)
