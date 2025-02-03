import os
import sys
from PIL import Image

i = Image.open(sys.argv[1])
os.makedirs(os.path.dirname(sys.argv[2]), exist_ok=True)
with open(sys.argv[2], "w") as f:
    print("GIMP Palette", file=f)
    print(f"Name: '{sys.argv[1]}'", file=f)
    print("Columns: 16", file=f)
    print("#", file=f)
    if i.mode == "P":
        for idx, (_, color) in enumerate(i.getcolors()):
            if idx >= 32:
                break
            print(*i.getpalette()[color * 3:color * 3 + 3], "    Index", idx, file=f)
    else:
        for idx, (_, (r, g, b, *_)) in enumerate(i.getcolors()):
            if idx >= 32:
                break
            print(r, g, b, "    Index", idx, file=f)
