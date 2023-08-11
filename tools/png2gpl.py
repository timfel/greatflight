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
    for idx, (_, (r, g, b)) in enumerate(i.getcolors()):
        print(r, g, b, "    Index", idx, file=f)
