#include "include/actions.h"
#include "include/units.h"
#include "include/player.h"
#include "ace/types.h"
#include "ace/macros.h"
#include "ace/managers/memory.h"

UnitType UnitTypes[] = {
    [dead] = {},
    [peasant] = {},
    [peon] = {
        .spritesheetPath = "resources/units/peon.bm",
        .maskPath = "resources/units/peon.msk",
        .stats = {
            .maxHP = 30,
            .maxMana = 0,
            .speed = 2,
        },
        .iconIdx = 54,
        .anim = {
            .large = 0,
            .walk = 3,
            .attack = 2,
            .fall = 0,
            .wait = 2,
        },
    },
    [footman] = {},
    [grunt] = {},
    [archer] = {},
    [spearman] = {},
    [catapult] = {},
    [knight] = {},
    [raider] = {},
    [cleric] = {},
    [necrolyte] = {},
    [conjurer] = {},
    [warlock] = {},
    [spider] = {},
    [daemon] = {},
    [elemental] = {},
    [ogre] = {},
    [slime] = {},
    [thief] = {}
};
_Static_assert(sizeof(UnitTypes) == unitTypeCount * sizeof(UnitType));

typedef struct _unitmanager {
    Unit units[MAX_UNITS];
    UBYTE freeUnitsStack[MAX_UNITS];
    UBYTE unitCount;
} tUnitManager;

#ifdef ACE_DEBUG
static inline UBYTE unitManagerUnitIsActive(Unit *unit) {
    return unitGetTilePosition(unit).uwYX != UNIT_FREE_TILE_POSITION.uwYX;
}
#endif

tUnitManager * unitManagerCreate(void) {
    tUnitManager *mgr = (tUnitManager *)memAllocFastClear(sizeof(tUnitManager));
    mgr->unitCount = 0;
    for (UBYTE i = 0; i < MAX_UNITS; ++i) {
#ifdef ACE_DEBUG
        unitSetTilePosition(&mgr->units[i], NULL, UNIT_FREE_TILE_POSITION);
#endif
        mgr->freeUnitsStack[i] = i;
    }

    // TODO: lazy loading of spritesheets
    for (UWORD i = 0; i < sizeof(UnitTypes) / sizeof(UnitType); i++) {
        if (UnitTypes[i].spritesheetPath) {
            tBitMap *bmp = bitmapCreateFromFile(UnitTypes[i].spritesheetPath, 0);
            tBitMap *mask = bitmapCreateFromFile(UnitTypes[i].maskPath, 0);
            UnitTypes[i].spritesheet = bmp;
            UnitTypes[i].mask = mask;
        }
    }

    return mgr;
}

void unitManagerDestroy(tUnitManager *mgr) {
    for (UWORD i = 0; i < sizeof(UnitTypes) / sizeof(UnitType); i++) {
        if (UnitTypes[i].spritesheet) {
            bitmapDestroy(UnitTypes[i].spritesheet);
            bitmapDestroy(UnitTypes[i].mask);
        }
    }
    for (UBYTE i = 0; i < mgr->unitCount; ++i) {
#ifdef ACE_DEBUG
        if (unitManagerUnitIsActive(&mgr->units[i]))
#endif
        unitDelete(mgr, &mgr->units[i]);
    }
    memFree(mgr, sizeof(tUnitManager));
}

static inline void unitDraw(Unit *self, tUbCoordYX viewportTopLeft) {
    self->bob.sPos.uwX = (self->x - (viewportTopLeft.ubX / (TILE_SIZE / PATHMAP_TILE_SIZE) * (TILE_SIZE / PATHMAP_TILE_SIZE))) * PATHMAP_TILE_SIZE + self->IX;
    self->bob.sPos.uwY = (self->y - (viewportTopLeft.ubY / (TILE_SIZE / PATHMAP_TILE_SIZE) * (TILE_SIZE / PATHMAP_TILE_SIZE))) * PATHMAP_TILE_SIZE + self->IY;
    bobPush(&self->bob);
}

static inline void unitOffscreen(Unit *self) {
    self->bob.sPos.uwX = UWORD_MAX;
}

void unitManagerProcessUnits(tUnitManager *mgr, UBYTE pPathMap[PATHMAP_SIZE][PATHMAP_SIZE], tUbCoordYX viewportTopLeft, tUbCoordYX viewportBottomRight) {
    for (UBYTE i = 0; i < mgr->unitCount; ++i) {
        Unit *unit = &mgr->units[i];
        actionDo(unit, pPathMap);
        tUbCoordYX loc = unitGetTilePosition(unit);
        if (loc.ubX >= viewportTopLeft.ubX
                && loc.ubY >= viewportTopLeft.ubY
                && loc.ubX <= viewportBottomRight.ubX
                && loc.ubY <= viewportBottomRight.ubY) {
            unitDraw(unit, viewportTopLeft);
        } else {
            unitOffscreen(unit);
        }
        if(blitIsIdle()) {
            bobProcessNext();
        }
    }
}

Unit *unitManagerUnitAt(tUnitManager *mgr, tUbCoordYX tile) {
    for (UBYTE i = 0; i < mgr->unitCount; ++i) {
        Unit *unit = &mgr->units[i];
        tUbCoordYX loc = unitGetTilePosition(unit);
        if (loc.uwYX == tile.uwYX) {
            return unit;
        }
    }
    return NULL;
}

Unit * unitNew(tUnitManager *mgr, UnitTypeIndex typeIdx) {
    if (mgr->unitCount >= MAX_UNITS) {
        return NULL;
    }
    UnitType *type = &UnitTypes[typeIdx];
    Unit *unit = &mgr->units[mgr->freeUnitsStack[mgr->unitCount]];
    mgr->unitCount++;

    bobInit(&unit->bob, 16, 16, 0, type->spritesheet->Planes[0], type->mask->Planes[0], 0, 0);
    unit->type = typeIdx;
    unitSetTilePosition(unit, NULL, UNIT_INIT_TILE_POSITION);
    return unit;
}

void unitDelete(tUnitManager *mgr, Unit *unit) {
#ifdef ACE_DEBUG
    if (!unitManagerUnitIsActive(unit)) {
        logWrite("Deleting inactive unit!!!");
    }
    unitSetTilePosition(unit, NULL, UNIT_FREE_TILE_POSITION);
    ULONG longIdx = (ULONG)unit - (ULONG)mgr->units;
    if (longIdx > 255) {
        logWrite("ALARM! unit idx is whaaat? %ld\n", longIdx);
    }
#endif
    UBYTE idx = (ULONG)unit - (ULONG)mgr->units;
    mgr->unitCount--;
    mgr->freeUnitsStack[mgr->unitCount] = idx;
}

UBYTE unitCanBeAt(UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE], Unit __attribute__((__unused__)) *unit, UBYTE x, UBYTE y) {
    // TODO: when we have units larger than 1 tile, check that here
    return mapIsWalkable(map, x, y);
}

UBYTE unitPlace(UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE], Unit *unit, UBYTE x, UBYTE y) {
    UBYTE actualX, actualY;
    // drop out no further than 5 tiles away
    for (UBYTE range = 0; range < 5; ++range) {
        for (UBYTE xoff = 0; xoff < range * 2; ++xoff) {
            actualX = x + xoff;
            for (UBYTE yoff = 0; yoff < range * 2; ++yoff) {
                actualY = y + yoff;
                if (unitCanBeAt(map, unit, actualX, actualY)) {
                    unit->x = actualX;
                    unit->y = actualY;
                    markMapTile(map, actualX, actualY);
                    return 1;
                }
                actualY = y - yoff;
                if (unitCanBeAt(map, unit, actualX, actualY)) {
                    unit->x = actualX;
                    unit->y = actualY;
                    markMapTile(map, actualX, actualY);
                    return 1;
                }
            }
            actualX = x - xoff;
            for (UBYTE yoff = 0; yoff < range * 2; ++yoff) {
                actualY = y + yoff;
                if (unitCanBeAt(map, unit, actualX, actualY)) {
                    unit->x = actualX;
                    unit->y = actualY;
                    markMapTile(map, actualX, actualY);
                    return 1;
                }
                actualY = y - yoff;
                if (unitCanBeAt(map, unit, actualX, actualY)) {
                    unit->x = actualX;
                    unit->y = actualY;
                    markMapTile(map, actualX, actualY);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void loadUnit(tUnitManager *mgr, tFile *map) {
    UBYTE type, x, y;
    fileRead(map, &type, 1);
    Unit *unit = unitNew(mgr, type);
    fileRead(map, &x, 1);
    fileRead(map, &y, 1);
    if (!unitPlace(g_Map.m_ubPathmapXY, unit, x, y)) {
        logWrite("DANGER WILL ROBINSON! Could not place unit at %d:%d", x, y);
        unitDelete(mgr, unit);
        return;
    }
    fileRead(map, &unit->action, 1);
    fileRead(map, &unit->action.ubActionDataA, 1);
    fileRead(map, &unit->action.ubActionDataB, 1);
    fileRead(map, &unit->action.ubActionDataC, 1);
    fileRead(map, &unit->action.ubActionDataD, 1);
    fileRead(map, &unit->stats.hp, 1);
    if (unit->stats.hp == 0) {
        unit->stats.hp = UnitTypes[unit->type].stats.maxHP;
    }
    fileRead(map, &unit->stats.mana, 1);
    if (unit->stats.mana == 0) {
        unit->stats.mana = UnitTypes[unit->type].stats.maxMana;
    }
}

void unitsLoad(tUnitManager *mgr, tFile *map) {
    for (UBYTE i = 0; i < sizeof(g_pPlayers) / sizeof(struct Player); ++i) {
        UBYTE count;
        fileRead(map, &count, 1);
        for (UBYTE unit = 0; unit < count; ++unit) {
            loadUnit(mgr, map);
        }
    }
}
