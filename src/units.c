#include "units.h"
#include "actions.h"
#include "ace/types.h"
#include "ace/macros.h"
#include "ace/managers/memory.h"

#include <stdint.h>

UnitType UnitTypes[] = {
    [dead] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [peasant] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [peon] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [footman] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [grunt] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [archer] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [spearman] = {
        .spritesheetPath = "resources/units/spearthrower.bm",
        .maskPath = "resources/units/spearthrower.msk",
        .maxHP = 60,
        .hasMana = 0,
        .speed = 4
    },
    [catapult] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [knight] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [raider] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [cleric] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [necrolyte] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [conjurer] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [warlock] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [spider] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [daemon] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [elemental] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [ogre] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [slime] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1},
    [thief] = {.spritesheet = NULL, .maxHP = 0, .hasMana = 0, .speed = 1}
};
_Static_assert(sizeof(UnitTypes) == unitTypeCount * sizeof(UnitType));

#define UNIT_FREE_TILE_POSITION ((tUbCoordYX){.ubY = -1, .ubX = -1})
#define UNIT_INIT_TILE_POSITION ((tUbCoordYX){.ubY = -1, .ubX = 0})

Unit * unitManagerCreate(void) {
    Unit *pUnitListHead = memAllocFastClear(sizeof(Unit) * MAX_UNITS);
    // link all units together in a circular linked list
    for (uint16_t i = 0; i < MAX_UNITS - 1; i++) {
        pUnitListHead[i].nextFreeUnit = &pUnitListHead[i + 1];
    }
    pUnitListHead[MAX_UNITS - 1].nextFreeUnit = pUnitListHead;
    // TODO: lazy loading of spritesheets
    for (uint16_t i = 0; i < sizeof(UnitTypes) / sizeof(UnitType); i++) {
        if (UnitTypes[i].spritesheetPath) {
            tBitMap *bmp = bitmapCreateFromFile(UnitTypes[i].spritesheetPath, 0);
            tBitMap *mask = bitmapCreateFromFile(UnitTypes[i].maskPath, 0);
            UnitTypes[i].spritesheet = bmp;
            UnitTypes[i].mask = mask;
        }
    }

    return pUnitListHead;
}

void unitManagerDestroy(Unit *pUnitListHead) {
    for (uint16_t i = 0; i < sizeof(UnitTypes) / sizeof(UnitType); i++) {
        if (UnitTypes[i].spritesheet) {
            bitmapDestroy(UnitTypes[i].spritesheet);
            bitmapDestroy(UnitTypes[i].mask);
        }
    }
    for (uint16_t i = 0; i < MAX_UNITS; i++) {
        unitDelete(pUnitListHead, &pUnitListHead[i]);
    }
    memFree(pUnitListHead, sizeof(Unit) * MAX_UNITS);
}

static inline uint8_t unitManagerUnitIsActive(Unit *unit) {
    return unitGetTilePosition(unit).uwYX != UNIT_FREE_TILE_POSITION.uwYX;
}

void unitManagerProcessUnits(Unit *pUnitListHead, uint8_t **pTileData, tUbCoordYX viewportTopLeft, tUbCoordYX viewportBottomRight) {
    for (uint8_t i = 0; i < MAX_UNITS; i++) {
        Unit *unit = &pUnitListHead[i];
        if (unitManagerUnitIsActive(unit)) {
            actionDo(unit, pTileData);
            tUbCoordYX loc = unitGetTilePosition(unit);
            if (loc.ubX >= viewportTopLeft.ubX
                    && loc.ubY >= viewportTopLeft.ubY
                    && loc.ubX <= viewportBottomRight.ubX
                    && loc.ubY <= viewportBottomRight.ubY) {
                unitDraw(unit);
            }
        }
    }
}

Unit *unitManagerUnitAt(Unit *pUnitListHead, tUbCoordYX tile) {
    for (UBYTE i = 0; i < MAX_UNITS; i++) {
        Unit *unit = &pUnitListHead[i];
        if (unitManagerUnitIsActive(unit)) {
            tUbCoordYX loc = unitGetTilePosition(unit);
            if (loc.uwYX == tile.uwYX) {
                return unit;
            }
        }
    }
    return NULL;
}

Unit * unitNew(Unit *pUnitListHead, enum UnitTypes typeIdx) {
    UnitType *type = &UnitTypes[typeIdx];
    Unit *unit = pUnitListHead->nextFreeUnit;
    if (unit == pUnitListHead) {
        // no more free units
        return NULL;
    }
    pUnitListHead->nextFreeUnit = unit->nextFreeUnit;
    bobNewInit(&unit->bob, 32, 32, 1, type->spritesheet, type->mask, 0, 0);
    unitSetFrame(unit, 0);
    unit->type = typeIdx;
    unitSetTilePosition(unit, UNIT_INIT_TILE_POSITION);
    return unit;
}

void unitDelete(Unit *pUnitListHead, Unit *unit) {
    if (unitManagerUnitIsActive(unit)) {
        unitSetTilePosition(unit, UNIT_FREE_TILE_POSITION);
        unit->nextFreeUnit = pUnitListHead->nextFreeUnit;
        pUnitListHead->nextFreeUnit = unit;
    }
}
