"""Render the same pup in three contrasting styles for art direction.

Run from the project root:
    blender --background --factory-startup --python art/prototype/style_tests.py

Writes art/prototype/renders/styles/: one exact-size Core2 frame (320x240) per style,
plus styles_sheet_x2.png with all three side by side at 2x (plush | toon | pixel).
"""
import os
import sys

import bpy
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pup_model as pm  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "renders", "styles")
os.makedirs(OUT, exist_ok=True)
C = pm.COLORS
frames = {}

# 1. Soft plush (Cycles): the prototype pushed toward fabric — coarser, deeper fibre bump
#    and a touch of subsurface so light bleeds softly like stuffed fleece.
pm.clear_scene()
mats = {k: pm.plush_material(f"P_{k}", C[k], bump=0.35, noise_scale=90)
        for k in ("caramel", "cocoa", "muzzle")}
mats["nose"] = pm.bead_material("P_nose", C["nose"], 0.25)
mats["eye"] = pm.bead_material("P_eye", C["eye"])
pm.build_pup(mats)
pm.setup_stage("soft")
pm.use_cycles()
frames["plush"] = pm.over_backdrop(pm.load_rgba(pm.render(OUT, "_plush.png", 320, 240, 96)))

# 2. Cel-shaded with ink outlines (EEVEE).
pm.clear_scene()
ink = pm.outline_material("Ink", C["outline"])
mats = {k: pm.toon_material(f"T_{k}", C[k]) for k in ("caramel", "cocoa", "muzzle")}
mats["nose"] = pm.flat_material("T_nose", C["nose"])
mats["eye"] = pm.flat_material("T_eye", C["eye"])
pm.build_pup(mats, outline=ink, eye_glints=pm.flat_material("Glint", (1, 1, 1)))
bpy.ops.mesh.primitive_circle_add(vertices=48, radius=1.0, fill_type="NGON", location=(0.2, 0, 0.01))
shadow = bpy.context.active_object
shadow.scale = (2.5, 0.85, 1)
# Solid tint of the backdrop rather than a see-through shadow (EEVEE would dither it).
shadow.data.materials.append(pm.flat_material("BlobShadow", tuple(c * 0.84 for c in pm.BG_SRGB)))
pm.setup_stage("toon", shadow_catcher=False, world_strength=0.15)
pm.use_eevee()
frames["toon"] = pm.over_backdrop(pm.load_rgba(pm.render(OUT, "_toon.png", 320, 240, 32)))

# 3. Chunky pixel: the toon scene rendered tiny, then cleaned up like hand-made sprite art —
#    hard alpha, a reduced palette (k-means), and a 1-pixel dark outline.
shadow.hide_render = True
small = pm.load_rgba(pm.render(OUT, "_pixel_src.png", 80, 60, 16))
mask = small[..., 3] > 0.5
rgb = small[..., :3]
eyes = mask & (rgb.mean(-1) < 0.2)  # keep the darkest pixels as eyes/nose, or k-means eats them
pts = rgb[mask]
rng = np.random.default_rng(1)
centers = pts[rng.choice(len(pts), 10, replace=False)]
for _ in range(15):
    labels = np.argmin(((pts[:, None] - centers[None]) ** 2).sum(-1), axis=1)
    for k in range(len(centers)):
        if np.any(labels == k):
            centers[k] = pts[labels == k].mean(0)
rgb[mask] = centers[labels]
rgb[eyes] = C["eye"]
grown = mask.copy()
grown[1:] |= mask[:-1]
grown[:-1] |= mask[1:]
grown[:, 1:] |= mask[:, :-1]
grown[:, :-1] |= mask[:, 1:]
edge = grown & ~mask
rgb[edge] = C["outline"]
alpha = (grown).astype(np.float32)[..., None]
pixel = pm.over_backdrop(np.concatenate([rgb, alpha], axis=-1))
frames["pixel"] = pixel.repeat(4, axis=0).repeat(4, axis=1)

for tmp in ("_plush.png", "_toon.png", "_pixel_src.png"):
    os.remove(os.path.join(OUT, tmp))
for name, img in frames.items():
    pm.save_rgba(img, os.path.join(OUT, f"style_{name}_320x240.png"))
sheet = np.concatenate([frames[k].repeat(2, 0).repeat(2, 1) for k in ("plush", "toon", "pixel")], axis=1)
pm.save_rgba(sheet, os.path.join(OUT, "styles_sheet_x2.png"))
print("DONE ->", OUT)
