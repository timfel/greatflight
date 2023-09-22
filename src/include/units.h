#ifndef UNITS_H
#define UNITS_H

#include "include/map.h"
#include "icons.h"

#include <ace/managers/bob.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/file.h>

// TODO: this can be configurable for different systems
#define BPP 4
#define UNIT_SIZE 16
#define UNIT_SIZE_SHIFT 4
#define UNIT_FRAME_BYTES (UNIT_SIZE / 8 + UNIT_SIZE * BPP)
/* Units are centered on a 2x2 tilegrid. So to position a unit on a tile, we subtract 8px */
#define UNIT_POSITION_OFFSET 8

#define FRAME_SIZE 16
#define WALK_FRAMES 3
#define ATTACK_FRAMES 2
#define FRAME_COUNT (WALK_FRAMES + ATTACK_FRAMES)
#define DIRECTION_NORTH 0
#define DIRECTION_EAST (FRAME_COUNT * FRAME_SIZE)
#define DIRECTION_SOUTH (FRAME_COUNT * FRAME_SIZE * 2)
#define DIRECTION_WEST (FRAME_COUNT * FRAME_SIZE * 3)

#define UNIT_FREE_TILE_POSITION ((tUbCoordYX){.ubY = -1, .ubX = -1})
#define UNIT_INIT_TILE_POSITION ((tUbCoordYX){.ubY = 1, .ubX = 1})

typedef struct {
    union {
        const char *spritesheetPath;
        tBitMap *spritesheet;
    };
    union {
        const char *maskPath;
        tBitMap *mask;
    };
    UBYTE iconIdx;
    struct {
        UBYTE maxHP;
        UBYTE speed;
        UBYTE maxMana;
    } stats;
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
    thief,
    unitTypeCount
};

typedef struct {
    UBYTE hp;
    UBYTE mana;
} UnitStats;

_Static_assert(sizeof(UnitStats) == sizeof(UWORD), "unit stats is not 1 word");

typedef struct _unitmanager tUnitManager;

typedef struct {
    UBYTE type;
    union {
        struct {
            UBYTE y;
            UBYTE x;
        };
        tUbCoordYX loc;
    };
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
    tBob bob;
    UBYTE frame;
    unsigned IX:4;
    unsigned IY:4;
} Unit;

extern void unitsLoad(tUnitManager *mgr, tFile *map);

/* The global list of unit types */
extern UnitType UnitTypes[];

/* The maximum number of units the game will allocate memory for. */
#define MAX_UNITS 200

/**
 * @brief Create the unit manager.
 * 
 * @return a pointer to the unit list.
 */
tUnitManager *unitManagerCreate(void);

/**
 * @brief Deallocate resources used by the unit manager
 * @param pUnitListHead unit list pointer created from unitManagerCreate()
 */
void unitManagerDestroy(tUnitManager *pUnitListHead);

/**
 * @brief Initialized a new unit of a specific type.
 * 
 * @param pUnitListHead unit list pointer created from unitManagerCreate()
 * @param type of unit to initialize
 * @return new unit pointer
 */
Unit * unitNew(tUnitManager *pUnitListHead, enum UnitTypes type);

/**
 * @brief Free the selected unit.
 * 
 * @param pUnitListHead unit list pointer created from unitManagerCreate()
 * @param unit to free
 */
void unitDelete(tUnitManager *pUnitListHead, Unit *unit);

/**
 * @brief Process all units' drawing and actions. Must be called after bobBegin() and before bobEnd()!
 * 
 * @param pUnitListHead unit list pointer created from unitManagerCreate()
 * @param pTileData 2d tile grid of map
 * @param viewportTopLeft top left visible tile
 * @param viewportBottomRight bottom right visible tile
 */
void unitManagerProcessUnits(tUnitManager *pUnitListHead, UBYTE **pTileData, tUbCoordYX viewportTopLeft, tUbCoordYX viewportBottomRight);

/**
 * @brief Return unit on tile, if any
 * 
 * @param pUnitListHead unit list pointer created from unitManagerCreate()
 * @param tile position where to look for unit
 * @return Unit* or NULL, if no unit is at the position
 */
Unit *unitManagerUnitAt(tUnitManager *pUnitListHead, tUbCoordYX tile);

_Static_assert(MAP_SIZE * TILE_SIZE < 0xfff, "map is small enough to fit locations in bytes");
static inline tUbCoordYX unitGetTilePosition(Unit *self) {
    return self->loc;
}

static inline void unitSetTilePosition(Unit *self, UBYTE **map, tUbCoordYX pos) {
    self->loc = pos;
    if (pos.ubX > 1) {
        markMapTile(map, pos.ubX, pos.ubY);
    }
}

static inline void unitDraw(Unit *self, tUbCoordYX viewportTopLeft) {
    self->bob.sPos.uwX = (self->x + viewportTopLeft.ubX) * (TILE_SIZE / 2) + self->IX;
    self->bob.sPos.uwY = (self->y + viewportTopLeft.ubY) * (TILE_SIZE / 2) + self->IY;
    bobPush(&self->bob);
}

static inline void unitSetFrame(Unit *self, UBYTE ubFrame) {
    UWORD offset = ubFrame * UNIT_FRAME_BYTES;
    bobSetFrame(&self->bob, UnitTypes[self->type].spritesheet->Planes[0] + offset, UnitTypes[self->type].mask->Planes[0] + offset);
}

static inline UBYTE unitGetFrame(Unit *self) {
    return self->frame;
}

#endif
