"""Baby sausage dog: cheeky. Cel-shaded base model (placeholder until the moodboards land).

Run from the project root:
    blender --background --factory-startup --python art/models/sausage_dog.py

Outputs in art/models/renders/sausage_dog/. Shape keys in the .blend:
    Plump      - overfeeding (body swells and sags)
    Smug       - eyelids drop to half-mast
    BrowRaise  - one eyebrow up (the cheeky look)
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from lib import toon as tk  # noqa: E402

OUT = os.path.join(HERE, "renders", "sausage_dog")
CAM = (6.0, -5.4, 2.6)
TARGET = (0.4, 0.0, 0.72)

COL = {
    "fur": (0.80, 0.55, 0.33),     # caramel
    "dark": (0.50, 0.30, 0.18),    # cocoa ears/tail
    "muzzle": (0.96, 0.82, 0.64),
    "nose": (0.16, 0.10, 0.08),
    "eye": (0.06, 0.04, 0.04),
}

tk.clear_scene()
ink = tk.ink()
fur, dark, muzzle = (tk.toon(f"SD_{k}", COL[k]) for k in ("fur", "dark", "muzzle"))
nose_m, eye_m, white = tk.flat("SD_nose", COL["nose"]), tk.flat("SD_eye", COL["eye"]), tk.flat("Glint", (1, 1, 1))
stroke = tk.flat("SD_stroke", (0.24, 0.13, 0.08))
O = dict(outline=ink, outline_width=0.035)

# Body: long and low even as a baby. blunt<1 turns the ellipsoid into a sausage.
BODY_R = (1.25, 0.55, 0.50)
tk.blob("Body", (0, 0, 0.62), BODY_R, fur, blunt=0.6, segs=(64, 32),
        keys={"Plump": tk.k_inflate(0.30, BODY_R[0], BODY_R[2])}, **O)
for sx in (1, -1):
    for sy in (1, -1):
        tk.blob(f"Leg{sx:+d}{sy:+d}", (0.78 * sx, 0.32 * sy, 0.17), (0.17, 0.17, 0.2), fur, **O)
tk.blob("Tail", (-1.36, 0, 0.86), (0.25, 0.075, 0.075), dark, rot=(0, 0.75, 0), **O)

# Head: oversized (baby proportions) and turned toward the viewer.
# pitch < 0 lifts the chin a touch: smug, not sleepy.
H = tk.HeadFrame((1.22, 0.0, 1.10), (0.70, 0.66, 0.62), yaw=-0.38, pitch=-0.14)
tk.blob("Head", H.c, H.r, fur, rot=H.rot(), **O)
S = tk.HeadFrame(H.at(0.60, 0, -0.20), (0.34, 0.30, 0.24), yaw=H.yaw, pitch=H.pitch)
tk.blob("Snout", S.c, S.r, muzzle, rot=H.rot(), **O)
tk.blob("Nose", H.at(0.93, 0, -0.10), (0.10, 0.13, 0.08), nose_m, rot=H.rot(), segs=(24, 12))
# Smirk: an ink stroke on the snout's surface, hitched up on the side facing the viewer.
tk.blob("Mouth", S.surface(0.85, -0.38, -0.38), (0.022, 0.13, 0.02), stroke, rot=H.rot(x=-0.5, z=0.6),
        segs=(16, 8))

EYE_R = 0.10
for side in (1, -1):
    s = "L" if side > 0 else "R"
    eye = H.surface(0.78, 0.46 * side, 0.22, depth=0.97)
    tk.blob(f"Eye.{s}", eye, (EYE_R,) * 3, eye_m, segs=(24, 12), subsurf=1)
    tk.glint(f"Glint.{s}", eye, EYE_R, CAM, white, lift=-0.25)
    # Eyelid: a fur-coloured dome over the eye; clip_below sets where its lower edge sits.
    # Basis: clipped almost to nothing (open eye); Smug: lower edge at half-mast.
    tk.blob(f"Lid.{s}", eye, (EYE_R * 1.12,) * 3, fur, rot=H.rot(), clip_below=0.97, segs=(32, 16),
            keys={"Smug": tk.k_override(clip_below=0.2)}, **O)
    # One brow (the side facing the viewer) hitches up for the cheeky look.
    brow_keys = {"BrowRaise": tk.k_override(rot=H.rot(x=0.35))} if side < 0 else {}
    brow_up = 0.62 if side < 0 else 0.48
    tk.blob(f"Brow.{s}", H.surface(0.66, 0.44 * side, brow_up, depth=1.0), (0.03, 0.12, 0.026), stroke,
            rot=H.rot(x=-0.2 * side), segs=(16, 8), keys=brow_keys)
    # Floppy ears hang beside the head.
    tk.blob(f"Ear.{s}", H.at(0.02, 0.64 * side, -0.14), (0.26, 0.07, 0.46), dark,
            rot=H.rot(x=0.15 * side, y=0.22), **O)

tk.setup_stage(CAM, TARGET, key_loc=(3.0, -8.5, 7.5), shadow=((0.15, 0.0), (1.9, 0.75)))
tk.set_key("Smug", 1.0)
tk.set_key("BrowRaise", 1.0)
tk.standard_outputs(OUT, "sausage_dog")
print("DONE ->", OUT)
