import math
import os
import sys
from PIL import Image, ImagePalette

if len(sys.argv) == 4:
    PALETTEIMG = sys.argv[1]
    OUTPUT = sys.argv[2]
    imgoff = 3
elif len(sys.argv) >= 6:
    TILESIZEX = int(sys.argv[1])
    TILESIZEY = int(sys.argv[2])
    PALETTEIMG = sys.argv[3]
    OUTPUT = sys.argv[4]
    imgoff = 5
else:
    assert False, """
    Either pass a [palette] [output] [image] to quantize a single image or [tilex] [tiley] [palette] [output] [images+]
    to quantize and create a tilemap of multiple images
    """

# ensure palettes are amiga compatible
palette = Image.open(PALETTEIMG).convert('RGB')
palettecolors = len(palette.getcolors())
palette = palette.quantize(palettecolors)
palette.putpalette(ImagePalette.ImagePalette('RGB', [(b >> 4) | (b >> 4 << 4) for b in palette.palette.getdata()[1]]))

# ensure output path exists
os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)

# open images and quantize to palette
IMAGES = [Image.open(i).convert('RGB').quantize(palette=palette) for i in sys.argv[imgoff:]]

if len(IMAGES) > 1:
    outimg = Image.new('P', (TILESIZEX * 16, sum(i.height for i in IMAGES)))
    outimg.putpalette(palette.palette)
    xoff = 0
    yoff = 0
    for i in IMAGES:
        # TODO: support filling with the first tile
        if i.width % TILESIZEX != 0:
            print(f"skipping {i!r} for now")
            continue
        if i.height % TILESIZEY != 0:
            print(f"skipping {i!r} for now")
            continue
        # split into tiles and store in tilemap
        for tileY in range(0, i.height, TILESIZEX):
            for tileX in range(0, i.width, TILESIZEY):
                outimg.paste(i.copy().crop((tileX, tileY, tileX + TILESIZEX, tileY + TILESIZEY)), (xoff, yoff, xoff + TILESIZEX, yoff + TILESIZEY))
                xoff += TILESIZEX
                if xoff > TILESIZEX * 16:
                    xoff = 0
                    yoff += TILESIZEY
    outimg.crop((0, 0, TILESIZEX * 16, yoff + TILESIZEY))
else:
    outimg = IMAGES[0]

# bug in Pillow? outimg is indexed to a palette, but it seems to "sometimes" store indices past the last palette entry
for x in range(outimg.width):
    for y in range(outimg.height):
        if outimg.getpixel((x, y)) >= palettecolors:
            outimg.putpixel((x, y), palettecolors - 1)

outimg.save(OUTPUT)
