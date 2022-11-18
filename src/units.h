#ifndef UNITS_H
#define UNITS_H

#include "bob_new.h"
#include "map.h"

#include <ace/utils/bitmap.h>
#include <ace/utils/file.h>

// TODO: this can be configurable for different systems
#define UNIT_SIZE 32
#define UNIT_SIZE_SHIFT 5
/* Units are centered on a 2x2 tilegrid. So to position a unit on a tile, we subtract 8px */
#define UNIT_POSITION_OFFSET 8

#define FRAME_SIZE 32
#define WALK_FRAMES 2
#define ATTACK_FRAMES 3
#define FRAME_COUNT (WALK_FRAMES + ATTACK_FRAMES)
#define DIRECTION_NORTH 0
#define DIRECTION_EAST FRAME_COUNT * FRAME_SIZE
#define DIRECTION_SOUTH FRAME_COUNT * FRAME_SIZE * 2
#define DIRECTION_WEST FRAME_COUNT * FRAME_SIZE * 3

typedef struct {
    union {
        const char *spritesheetPath;
        tBitMap *spritesheet;
    };
    union {
        const char *maskPath;
        tBitMap *mask;
    };
    UBYTE maxHP;
    UBYTE speed;
    UBYTE hasMana;
} UnitType;

enum UnitTypes {
    dead = 0,
    //
    peasant,
    peon,
    footman,
    grunt,
    archer,
    spearman,
    catapult,
    knight,
    raider,
    cleric,
    necrolyte,
    conjurer,
    warlock,
    spider,
    daemon,
    elemental,
    ogre,
    slime,
    thief
};

typedef struct {
    UBYTE hp;
    UBYTE mana;
} UnitStats;

_Static_assert(sizeof(UnitStats) == sizeof(UWORD), "unit stats is not 1 word");

typedef struct {
    UBYTE type;
    UBYTE action;
    union {
        ULONG ulActionData;
        struct {
            UWORD uwActionDataA;
            UWORD uwActionDataB;
        };
        struct {
            UBYTE ubActionDataA;
            UBYTE ubActionDataB;
            UBYTE ubActionDataC;
            UBYTE ubActionDataD;
        };
    };
    UnitStats stats;
    tBobNew bob;
} Unit;

// _Static_assert(sizeof(Unit) == (1 << UNIT_SIZE_SHIFT) >> 3, "unit struct is not 4 words");
// _Static_assert(sizeof(Unit) == 4 * sizeof(UWORD), "unit struct is not 4 words");

/* The global list of unit types */
extern UnitType UnitTypes[];

/* The maximum number of units the game will allocate memory for. */
#define MAX_UNITS 200

/* The list of all units. Inactive unit slots are NULL. */
extern Unit **s_pUnitList;

void unitManagerCreate(void);

void unitManagerDestroy(void);

Unit * unitNew(UBYTE type);

void unitDelete(Unit *);

_Static_assert(MAP_SIZE * TILE_SIZE < 0xfff, "map is small enough to fit locations in bytes");
static inline tUbCoordYX unitGetTilePosition(Unit *self) {
    tUbCoordYX loc;
    tUwCoordYX bobLoc = self->bob.sPos;
    loc.ubX = (bobLoc.uwX >> TILE_SHIFT) + 1;
    loc.ubY = (bobLoc.uwY >> TILE_SHIFT) + 1;
    return loc;
}

static inline void unitSetTilePosition(Unit *self, tUbCoordYX pos) {
    self->bob.sPos.uwX = (pos.ubX << TILE_SHIFT) - 8;
    self->bob.sPos.uwY = (pos.ubY << TILE_SHIFT) - 8;
}

static inline void unitDraw(Unit *self) {
    bobNewPush(&self->bob);
}

static inline void unitSetDirectionAndFrame(Unit *self, UWORD ubDir, UBYTE ubFrame) {
    bobNewSetBitMapOffset(&self->bob, (ubFrame << UNIT_SIZE_SHIFT) + ubDir);
}

static inline void unitSetFrame(Unit *self, UBYTE ubFrame) {
    bobNewSetBitMapOffset(&self->bob, ubFrame << UNIT_SIZE_SHIFT);
}

static inline UBYTE unitGetFrame(Unit *self) {
    return ((self->bob.uwOffsetY / self->bob.pBitmap->BytesPerRow) >> UNIT_SIZE_SHIFT) % FRAME_COUNT;
}

#endif
