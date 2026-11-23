"""Baby husky: boisterous. Cel-shaded base model (placeholder until the moodboards land).

Run from the project root:
    blender --background --factory-startup --python art/models/husky.py

Outputs in art/models/renders/husky/. Shape keys in the .blend:
    Plump  - overfeeding (body and belly swell together)
    Blink  - eyelids close (for idle blinks)
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from lib import toon as tk  # noqa: E402

OUT = os.path.join(HERE, "renders", "husky")
CAM = (6.0, -5.4, 2.7)
TARGET = (0.25, 0.0, 0.85)

COL = {
    "grey": (0.56, 0.60, 0.68),
    "white": (0.97, 0.96, 0.94),
    "iris": (0.52, 0.78, 0.96),    # pale blue
    "pupil": (0.05, 0.05, 0.08),
    "nose": (0.12, 0.11, 0.13),
    "mouth": (0.42, 0.11, 0.16),
    "tongue": (0.96, 0.52, 0.58),
    "inner_ear": (0.93, 0.72, 0.74),
}

tk.clear_scene()
ink = tk.ink(color=(0.16, 0.16, 0.22))
grey, white, inner_ear, tongue = (tk.toon(f"HU_{k}", COL[k]) for k in ("grey", "white", "inner_ear", "tongue"))
iris, pupil, nose_m, mouth_m = (tk.flat(f"HU_{k}", COL[k]) for k in ("iris", "pupil", "nose", "mouth"))
glint_m = tk.flat("Glint", (1, 1, 1))
O = dict(outline=ink, outline_width=0.035)

# Body: chunky ovoid, grey back with a white belly and a fluffy chest bib.
BODY_R = (0.95, 0.70, 0.66)
plump = tk.k_inflate(0.28, BODY_R[0], BODY_R[2])
tk.blob("Body", (0, 0, 0.74), BODY_R, grey, keys={"Plump": plump}, **O)
tk.blob("Belly", (0.12, 0, 0.58), (0.86, 0.60, 0.48), white,
        keys={"Plump": tk.k_inflate(0.28, 0.86, 0.48)}, **O)
tk.blob("Chest", (0.66, 0, 0.80), (0.40, 0.50, 0.50), white, **O)
for sx in (1, -1):
    for sy in (1, -1):
        tk.blob(f"Leg{sx:+d}{sy:+d}", (0.52 * sx, 0.40 * sy, 0.20), (0.21, 0.21, 0.24), white, **O)

# Tail: a curl over the back (overlapping balls; the inner outlines read as fluff).
for i, (x, z, r, mat) in enumerate([(-0.92, 1.02, 0.22, grey), (-1.05, 1.30, 0.21, grey),
                                    (-0.95, 1.55, 0.19, grey), (-0.72, 1.66, 0.16, white)]):
    tk.blob(f"Tail{i}", (x, 0, z), (r, r * 0.9, r), mat, **O)

# Head: oversized and turned toward the viewer, with the classic mask:
# grey cap, white face, a grey "widow's peak" down the forehead and white brow dots.
H = tk.HeadFrame((0.95, 0.0, 1.40), (0.78, 0.74, 0.70), yaw=-0.38)
tk.blob("Head", H.c, H.r, grey, rot=H.rot(), **O)
tk.blob("Mask", H.at(0.24, 0, -0.16), (0.62, 0.68, 0.52), white, rot=H.rot(), **O)
tk.blob("Peak", H.at(0.46, 0, 0.30), (0.34, 0.13, 0.34), grey, rot=H.rot(y=0.35))
for side in (1, -1):
    s = "L" if side > 0 else "R"
    tk.blob(f"Cheek.{s}", H.at(0.12, 0.62 * side, -0.26), (0.26, 0.18, 0.23), white, rot=H.rot(), **O)
    tk.blob(f"BrowDot.{s}", H.surface(0.66, 0.34 * side, 0.55, depth=0.99), (0.07, 0.07, 0.05), white,
            rot=H.rot(), segs=(16, 8))
    # Ears: rounded cones (taper), leaning out a little; pink inner ear in front.
    ear_c = H.at(-0.08, 0.44 * side, 0.62)
    tk.blob(f"Ear.{s}", ear_c, (0.27, 0.14, 0.36), grey, taper=0.6,
            rot=H.rot(x=-0.32 * side, y=-0.12), **O)
    tk.blob(f"InnerEar.{s}", H.at(0.0, 0.44 * side, 0.58), (0.15, 0.07, 0.24), inner_ear, taper=0.6,
            rot=H.rot(x=-0.32 * side, y=-0.12))

EYE_R = 0.145  # big baby eyes
for side in (1, -1):
    s = "L" if side > 0 else "R"
    eye = H.surface(0.80, 0.40 * side, 0.18, depth=0.97)
    tk.blob(f"Eye.{s}", eye, (EYE_R,) * 3, iris, segs=(24, 12), subsurf=1)
    look = (tk.Vector(CAM) - eye).normalized()  # pupils look at the viewer
    tk.blob(f"Pupil.{s}", eye + look * EYE_R * 0.55, (EYE_R * 0.55,) * 3, pupil, segs=(20, 10), subsurf=1)
    tk.glint(f"Glint.{s}", eye, EYE_R, CAM, glint_m, size=0.34)
    # Basis: clipped almost to nothing so wide-open eyes have no visible lid.
    tk.blob(f"Lid.{s}", eye, (EYE_R * 1.08,) * 3, white, rot=H.rot(), clip_below=0.985, segs=(32, 16),
            keys={"Blink": tk.k_override(clip_below=-0.95)}, **O)

# Muzzle with an excited open mouth and tongue.
S = tk.HeadFrame(H.at(0.66, 0, -0.22), (0.36, 0.32, 0.25), yaw=H.yaw)
tk.blob("Snout", S.c, S.r, white, rot=H.rot(), **O)
tk.blob("Nose", H.at(1.00, 0, -0.10), (0.11, 0.14, 0.09), nose_m, rot=H.rot(), segs=(24, 12))
# Wide-open mouth under the snout: a dark cavity with an outline, tongue lolling forward.
tk.blob("Mouth", S.surface(0.5, 0, -0.85, depth=0.85), (0.20, 0.22, 0.15), mouth_m, rot=H.rot(y=0.25), **O)
tk.blob("Tongue", S.surface(0.95, -0.05, -0.7, depth=1.12), (0.14, 0.13, 0.07), tongue, rot=H.rot(y=0.5), **O)

tk.setup_stage(CAM, TARGET, key_loc=(3.0, -8.5, 7.5), shadow=((0.05, 0.0), (1.5, 0.95)))
tk.standard_outputs(OUT, "husky")
print("DONE ->", OUT)
