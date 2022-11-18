#include "units.h"
#include "actions.h"
#include "ace/types.h"
#include "ace/macros.h"
#include "ace/managers/memory.h"

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
};

union freeBlock {
    Unit unit;
    union freeBlock *next;
};
_Static_assert(sizeof(union freeBlock) == sizeof(Unit), "freeBlock is not Unit size");

// with 10 units per frame, we need 20 frames to get through all units
// so on NTSC, we'd handle each unit 3 times per second. This should give
// generally nice animation and speed
#define UNITS_PER_FRAME 10

static union freeBlock *s_arena;
Unit **s_pUnitList;

void unitManagerCreate(void) {
    s_arena = memAllocFastClear(sizeof(union freeBlock) * MAX_UNITS);
    s_pUnitList = memAllocFastClear(sizeof(Unit *) * MAX_UNITS);

    for (UWORD i = 0; i < MAX_UNITS; i++) {
        s_arena[i].next = &s_arena[(i + 1) % MAX_UNITS];
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
}

void unitManagerDestroy(void) {
    for (UWORD i = 0; i < sizeof(UnitTypes) / sizeof(UnitType); i++) {
        if (UnitTypes[i].spritesheet) {
            bitmapDestroy(UnitTypes[i].spritesheet);
            bitmapDestroy(UnitTypes[i].mask);
        }
    }
    memFree(s_arena, sizeof(union freeBlock) * MAX_UNITS);
    memFree(s_pUnitList, sizeof(Unit) * MAX_UNITS);
}

// void processUnits(void *unitManager) {
//     struct unitManager *m = (struct unitManager *)unitManager;
//     UBYTE start = m->index;
//     UBYTE end = start + UNITS_PER_FRAME;
//     for (UBYTE i = start; i < end; ++i) {
//         struct unit *unit = &m->units[i];
//         Actions[unit->header.currentAction](unit);
//     }
// }

Unit * unitNew(UBYTE typeIdx) {
    UnitType *type = &UnitTypes[typeIdx];
    Unit *unit = &s_arena->next->unit;
    ULONG idx = ((ULONG)unit - (ULONG)s_arena) / sizeof(union freeBlock);
    if (idx == 0) {
        // TODO: no more free space!!
        return NULL;
    }
    s_arena->next = s_arena->next->next;
    bobNewInit(&unit->bob, 32, 32, 1, type->spritesheet, type->mask, 0, 0);
    unitSetFrame(unit, 0);
    unit->type = typeIdx;
    s_pUnitList[idx] = unit;
    return unit;
}

void unitDelete(Unit *unit) {
    union freeBlock *curFreeBlk = s_arena->next;
    union freeBlock *newFreeBlk = (union freeBlock *)unit;
    ULONG idx = ((ULONG)unit - (ULONG)s_arena) / sizeof(union freeBlock);
    s_pUnitList[idx] = NULL;
    // TODO: track use after delete
    s_arena->next = newFreeBlk;
    newFreeBlk->next = curFreeBlk;
}
