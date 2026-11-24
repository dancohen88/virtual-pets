"""Both eggs beside both baby pets, for checking scale and style consistency.
Run after the two egg scripts and the two pet scripts:

    blender --background --factory-startup --python art/models/make_egg_sheet.py

Writes art/models/renders/eggs_and_pets.png (device size) and eggs_and_pets_x2.png.
"""
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
from lib import toon as tk  # noqa: E402

R = os.path.join(HERE, "renders")
order = [("egg_sausage_dog", "egg_sausage_dog_320x240.png"), ("sausage_dog", "sausage_dog_320x240.png"),
         ("egg_husky", "egg_husky_320x240.png"), ("husky", "husky_320x240.png")]
missing = [os.path.join(R, d, f) for d, f in order if not os.path.exists(os.path.join(R, d, f))]
if missing:
    sys.exit("Render these first: " + ", ".join(missing))
row = np.concatenate([tk.load_rgba(os.path.join(R, d, f)) for d, f in order], axis=1)
tk.save_rgba(row, os.path.join(R, "eggs_and_pets.png"))
tk.save_rgba(tk.upscale(row, 2), os.path.join(R, "eggs_and_pets_x2.png"))
print("DONE ->", os.path.join(R, "eggs_and_pets_x2.png"))
