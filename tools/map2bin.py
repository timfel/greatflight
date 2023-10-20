import os
import sys
from typing import Any

# convenience globals, to be kept in sync with the unit definitions
mapglobals = dict(
    peon=2,
    unit_defaults=[
        0, # action
        0, 0, 0, 0, # action data
        0, # hp (default)
        0, # mana (default)
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
