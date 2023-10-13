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
            .wait = 3,
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

struct _unitLink {
    Unit unit;
    struct _unitLink *prev;
    struct _unitLink *next;
};

struct _unitmanager {
    struct _unitLink *nextFreeUnit;
    struct _unitLink *firstActiveUnit;
    struct _unitLink unitList[0];
};

#define UNIT_MANAGER_SIZE (sizeof(struct _unitmanager) + sizeof(struct _unitLink) * MAX_UNITS)

#ifdef ACE_DEBUG
static inline UBYTE unitManagerUnitIsActive(Unit *unit) {
    return unitGetTilePosition(unit).uwYX != UNIT_FREE_TILE_POSITION.uwYX;
}
#endif

tUnitManager * unitManagerCreate(void) {
    struct _unitmanager *mgr = (struct _unitmanager *)memAllocFastClear(UNIT_MANAGER_SIZE);
    
    // link all units together into the singly linked free list
    mgr->nextFreeUnit = &mgr->unitList[0];
    for (UWORD i = 0; i < MAX_UNITS - 1; i++) {
#ifdef ACE_DEBUG
        unitSetTilePosition((Unit *)(&mgr->unitList[i]), NULL, UNIT_FREE_TILE_POSITION);
#endif
        mgr->unitList[i].next = &mgr->unitList[i + 1];
        mgr->unitList[i].prev = NULL;
    }
    mgr->unitList[MAX_UNITS - 1].prev = NULL;
    mgr->unitList[MAX_UNITS - 1].next = NULL;

    // active list doubly-linked, starts empty
    mgr->firstActiveUnit = NULL;

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

void unitManagerDestroy(tUnitManager *pUnitListHead) {
    for (UWORD i = 0; i < sizeof(UnitTypes) / sizeof(UnitType); i++) {
        if (UnitTypes[i].spritesheet) {
            bitmapDestroy(UnitTypes[i].spritesheet);
            bitmapDestroy(UnitTypes[i].mask);
        }
    }
    struct _unitLink *l = pUnitListHead->firstActiveUnit;
    while (l) {
#ifdef ACE_DEBUG
        if (unitManagerUnitIsActive((Unit *)l))
#endif
        unitDelete(pUnitListHead, (Unit *)l);
        l = l->next;
    }
    memFree(pUnitListHead, UNIT_MANAGER_SIZE);
}

static inline void unitDraw(Unit *self, tUbCoordYX viewportTopLeft) {
    self->bob.sPos.uwX = (self->x - viewportTopLeft.ubX) * PATHMAP_TILE_SIZE + self->IX;
    self->bob.sPos.uwY = (self->y - viewportTopLeft.ubY) * PATHMAP_TILE_SIZE + self->IY;
    bobPush(&self->bob);
}

static inline void unitOffscreen(Unit *self) {
    self->bob.sPos.uwX = UWORD_MAX;
}

void unitManagerProcessUnits(tUnitManager *pUnitListHead, UBYTE pPathMap[PATHMAP_SIZE][PATHMAP_SIZE], tUbCoordYX viewportTopLeft, tUbCoordYX viewportBottomRight) {
    struct _unitLink *link = pUnitListHead->firstActiveUnit;
    while (link) {
        actionDo((Unit *)link, pPathMap);
        tUbCoordYX loc = unitGetTilePosition((Unit *)link);
        if (loc.ubX >= viewportTopLeft.ubX
                && loc.ubY >= viewportTopLeft.ubY
                && loc.ubX <= viewportBottomRight.ubX
                && loc.ubY <= viewportBottomRight.ubY) {
            unitDraw((Unit *)link, viewportTopLeft);
        } else {
            unitOffscreen((Unit *)link);
        }
        if(blitIsIdle()) {
            bobProcessNext();
        }
        link = link->next;
    }
}

Unit *unitManagerUnitAt(tUnitManager *pUnitListHead, tUbCoordYX tile) {
    struct _unitLink *link = pUnitListHead->firstActiveUnit;
    while (link) {
        tUbCoordYX loc = unitGetTilePosition((Unit *)link);
        if (loc.uwYX == tile.uwYX) {
            return (Unit *)link;
        }
        link = link->next;
    }
    return NULL;
}

Unit * unitNew(tUnitManager *pUnitListHead, enum UnitTypes typeIdx) {
    UnitType *type = &UnitTypes[typeIdx];
    struct _unitLink *link = pUnitListHead->nextFreeUnit;
    if (!link) {
        return NULL;
    }

    // remove from free list, this list is singly linked,
    // because we only ever remove or insert at the front
    pUnitListHead->nextFreeUnit = link->next;

    // add to active list, doubly linked, because random
    // elements need to be removable
    link->next = pUnitListHead->firstActiveUnit;
    pUnitListHead->firstActiveUnit = link;
    if (link->next) {
        link->next->prev = link;
    }
    bobInit(&((Unit *)link)->bob, 16, 16, 0, type->spritesheet->Planes[0], type->mask->Planes[0], 0, 0);
    ((Unit *)link)->type = typeIdx;
    unitSetTilePosition((Unit *)link, NULL, UNIT_INIT_TILE_POSITION);
    return (Unit *)link;
}

void unitDelete(tUnitManager *pUnitListHead, Unit *unit) {
#ifdef ACE_DEBUG
    if (!unitManagerUnitIsActive(unit)) {
        logWrite("Deleting inactive unit!!!");
    }
    unitSetTilePosition(unit, NULL, UNIT_FREE_TILE_POSITION);
#endif
    struct _unitLink *prev = ((struct _unitLink *)unit)->prev;
    if (prev) {
        prev->next = ((struct _unitLink *)unit)->next;
        // first active unit unchanged
        ((struct _unitLink *)unit)->prev = NULL;
    } else {
        // this was the first active unit
        pUnitListHead->firstActiveUnit = ((struct _unitLink *)unit)->next;
    }
    ((struct _unitLink *)unit)->next = pUnitListHead->nextFreeUnit;
    pUnitListHead->nextFreeUnit = (struct _unitLink *)unit;
}

UBYTE unitCanBeAt(UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE], Unit __attribute__((__unused__)) *unit, UBYTE x, UBYTE y) {
    // TODO: when we have units larger than 1 tile, check that here
    return mapIsWalkable(map, x, y);
}

UBYTE unitPlace(UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE], Unit *unit, UBYTE x, UBYTE y) {
    UBYTE range = 5; // drop out no further than 5 tiles away
    UBYTE actualX, actualY;
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
