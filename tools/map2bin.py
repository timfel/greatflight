import os
import sys
from typing import Any

# convenience globals, to be kept in sync with the loadUnit function
mapglobals = dict(
    # units
    peasant=1,
    peon=2,
    # buildings
    townhall=0,
    goldmine=8,
    # defaults
    unit_defaults=[
        0, # action
        0, 0, 0, 0, # action data
        0, # hp (default)
        0, # mana (default)
    ],
    building_defaults=[
        0, # action
        0, 0, 0, 0, # action data
        0, # hp (default)
    ],
)

os.makedirs(os.path.dirname(sys.argv[2]), exist_ok=True)
with open(sys.argv[2], "wb") as output:
    with open(sys.argv[1], "r") as input:
        mapdict: dict[str, str | list[list[int]] | list[dict[str, int]]] = eval(input.read(), mapglobals)
        output.write(mapdict["tileset"].encode("ascii"))
        for y in range(32):
            for x in range(32):
                output.write(bytearray([mapdict["map"][x][y]]))
        for player in mapdict["players"]:
            output.write(bytearray([int(player["gold"] / 100)]))
            output.write(bytearray([int(player["lumber"] / 100)]))
            output.write(bytearray([player["ai"]]))
        for units in mapdict["units"]:
            output.write(bytearray([len(units)]))
            for unit in units:
                output.write(bytearray(unit))
        for buildings in mapdict["buildings"]:
            output.write(bytearray([len(buildings)]))
            for b in buildings:
                hp = b[-1]
                b[-1] = hp & 0xff
                b.append((hp >> 8) & 0xff)
                output.write(bytearray(b))
