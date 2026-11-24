"""The sausage dog's egg: caramel shell with cocoa "ear" patches, spots and a tail curl.

Run from the project root:
    blender --background --factory-startup --python art/models/egg_sausage_dog.py

Outputs in art/models/renders/egg_sausage_dog/:
    egg_sausage_dog_stage{0..3}_sprite.png   incubation: whole, then cracks 1-3 (transparent)
    egg_sausage_dog_hatch{1..4}_sprite.png   hatch key poses: squash, pop, peek, shell hat
    egg_sausage_dog_shadow.png               blob shadow as its own layer
    egg_sausage_dog_sheet(_x2).png           stages (top row) and hatch (bottom row) on the backdrop
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from lib import egg  # noqa: E402
from lib import toon as tk  # noqa: E402

COL = {
    "shell": (0.86, 0.62, 0.40),   # a touch lighter than the pup's caramel, so patches read
    "patch": (0.50, 0.30, 0.18),   # cocoa, as the pup's ears and tail
    "muzzle": (0.96, 0.82, 0.64),
    "fur": (0.80, 0.55, 0.33),
    "nose": (0.16, 0.10, 0.08),
    "eye": (0.06, 0.04, 0.04),
    "interior": (0.48, 0.32, 0.22),   # inside of the shell: a muted, darker caramel
}


def decorate(ctx):
    E, f, ink = ctx.E, ctx.front, ctx.ink
    cocoa = tk.toon("EGS_patch", COL["patch"])
    lid, base = [], []
    # Floppy-ear patches high on both sides, drooping outward.
    for side in (1, -1):
        lid += egg.created(egg.patch, f"EarPatch.{side:+d}", E, f + side * 0.80, 0.55, (0.12, 0.27, 0.10),
                           cocoa, spin=-0.45 * side, outline=ink)
    lid += egg.created(egg.patch, "Spot.Top", E, f - 0.15, 0.72, (0.06, 0.055, 0.06), cocoa, outline=ink)
    base += egg.created(egg.patch, "Spot.A", E, f + 0.28, -0.22, (0.11, 0.09, 0.08), cocoa, spin=0.4,
                        outline=ink)
    base += egg.created(egg.patch, "Spot.B", E, f - 0.42, -0.52, (0.07, 0.06, 0.06), cocoa, outline=ink)
    # A little tail curl peeking round the side.
    p = E.point(f + 1.75, -0.38)
    n = E.normal(f + 1.75, -0.38)
    base += egg.created(tk.blob, "Tail.A", p + n * 0.02, (0.10, 0.09, 0.09), cocoa, outline=ink,
                        outline_width=0.03)
    base += egg.created(tk.blob, "Tail.B", p + n * 0.09 + tk.Vector((0, 0, 0.11)), (0.07, 0.065, 0.065), cocoa,
                        outline=ink, outline_width=0.03)
    return lid, base


def baby(ctx, at):
    """Newborn head for the peek and shell-hat frames: big eyes, floppy ears, cream muzzle."""
    ink, cam = ctx.ink, ctx.cam
    fur, dark, muzzle = (tk.toon(f"EGS_{k}", COL[k]) for k in ("fur", "patch", "muzzle"))
    nose, eye, white = tk.flat("EGS_nose", COL["nose"]), tk.flat("EGS_eye", COL["eye"]), tk.flat("EGS_glint", (1, 1, 1))
    O = dict(outline=ink, outline_width=0.03)
    H = tk.HeadFrame(at, (0.40, 0.38, 0.35), yaw=ctx.front)
    tk.blob("Pup.Head", H.c, H.r, fur, rot=H.rot(), **O)
    tk.blob("Pup.Muzzle", H.at(0.30, 0, -0.10), (0.19, 0.18, 0.13), muzzle, rot=H.rot(), **O)
    tk.blob("Pup.Nose", H.at(0.47, 0, -0.05), (0.06, 0.075, 0.05), nose, rot=H.rot(), segs=(16, 8))
    for side in (1, -1):
        e = H.surface(0.82, 0.40 * side, 0.20, depth=0.97)
        tk.blob(f"Pup.Eye.{side:+d}", e, (0.06,) * 3, eye, segs=(16, 8), subsurf=1)
        tk.glint(f"Pup.Glint.{side:+d}", e, 0.06, cam, white, size=0.36)
        tk.blob(f"Pup.Ear.{side:+d}", H.at(-0.02, 0.38 * side, -0.02), (0.13, 0.05, 0.27), dark,
                rot=H.rot(x=0.18 * side, y=0.15), **O)


CFG = dict(
    name="egg_sausage_dog",
    out=os.path.join(HERE, "renders", "egg_sausage_dog"),
    center=(0, 0, 0.84), radii=(0.60, 0.60, 0.84), taper=0.26,
    crack=(0.20, 0.07, 9, 0.0),          # c0, amplitude, teeth, phase
    shell=COL["shell"], interior=COL["interior"], ink=(0.22, 0.13, 0.10),
    decorate=decorate, baby=baby, head_top=0.36,
    cam=(6.0, -5.4, 2.7), target=(0.0, 0.0, 0.85), key=(3.0, -8.5, 7.5),
    shadow=((0.05, 0.0), (0.85, 0.62)),
)

egg.build_egg(CFG)
