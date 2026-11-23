"""Side-by-side sheet of both pets at 2x device size (run after both model scripts).

    blender --background --factory-startup --python art/models/make_sheet.py
"""
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from lib import toon as tk  # noqa: E402

R = os.path.join(HERE, "renders")
frames = [tk.load_rgba(os.path.join(R, p, f"{p}_320x240.png")) for p in ("sausage_dog", "husky")]
tk.save_rgba(tk.upscale(np.concatenate(frames, axis=1), 2), os.path.join(R, "both_pets_x2.png"))
print("DONE ->", os.path.join(R, "both_pets_x2.png"))
