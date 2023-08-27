#include "units.h"
#include "actions.h"
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
            .hasMana = 0,
            .speed = 4,
        },
        .iconIdx = 54,
    },
    [footman] = {},
    [grunt] = {},
    [archer] = {},
    [spearman] = {
        .spritesheetPath = "resources/units/spearthrower.bm",
        .maskPath = "resources/units/spearthrower.msk",
        .stats = {
            .maxHP = 60,
            .hasMana = 0,
            .speed = 4,
        },
        .iconIdx = 54,
    },
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

void unitManagerProcessUnits(tUnitManager *pUnitListHead, UBYTE **pTileData, tUbCoordYX viewportTopLeft, tUbCoordYX viewportBottomRight) {
    struct _unitLink *link = pUnitListHead->firstActiveUnit;
    while (link) {
        actionDo((Unit *)link, pTileData);
        tUbCoordYX loc = unitGetTilePosition((Unit *)link);
        if (loc.ubX >= viewportTopLeft.ubX
                && loc.ubY >= viewportTopLeft.ubY
                && loc.ubX <= viewportBottomRight.ubX
                && loc.ubY <= viewportBottomRight.ubY) {
            unitDraw((Unit *)link);
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
    unitSetFrame((Unit *)link, 0);
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
