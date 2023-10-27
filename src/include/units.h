#ifndef UNITS_H
#define UNITS_H

#include "include/actions.h"
#include "include/map.h"
#include "include/icons.h"

#include <ace/managers/log.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/file.h>

// TODO: this can be configurable for different systems
#define BPP 4

#define UNIT_FREE_TILE_POSITION ((tUbCoordYX){.ubY = -1, .ubX = -1})
#define UNIT_INIT_TILE_POSITION ((tUbCoordYX){.ubY = 1, .ubX = 1})
#define DIRECTION_NORTH 0
#define DIRECTION_EAST 1
#define DIRECTION_SOUTH 2
#define DIRECTION_WEST 3
#define DIRECTIONS 4

/* 
We store max health as a combination of two values:
the shift value, and a base value.
We want to end up with max health values that can be
mapped into the range 0-30 for the health bars cheaply.
For example, to get a health of 80, we say:
    shift = 2, base 20.
    Then, the max health is 20 << 2 = 80.
    To map it into the health bar range, we
    do (80 >> 2) + (80 >> 2 >> 1) = 20 + 10 = 30.
To get a health of 60, we say:
    shift = 1, base = 30.
    Then, the max health is 30 << 1 = 60.
    To map it into the health bar range, we
    do (60 >> 1) = 30.
To get a health of 200, we say:
    shift = 3, base = 25.
    Then, the max health is 25 << 3 = 200.
    To map it into the health bar range, we
    do (200 >> 3) + (200 >> 3 >> 2) = 25 + 6 = 31
    and we drop the extra pixel.
*/
typedef enum __attribute__((__packed__)) {
    HP_20 = 20,
    HP_25 = 25,
    HP_30 = 30
} HPBase;

typedef struct {
    const char* name;
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
        UBYTE hpShift;
        HPBase hpBase;
        UBYTE speed;
        UBYTE maxMana;
    } stats;
    struct {
        UWORD gold;
        UWORD lumber;
        UBYTE timeShift;
        HPBase timeBase;
    } costs;
    struct {
        unsigned large:1; // 16x16 or 24x24 pixels
        unsigned walk:2; // 0,1,2,3 walking frames
        unsigned attack:2; // 0,1,2,3 attack frames
        unsigned fall:1; // 0,1 falling frames
    unsigned wait:2; // 2,4,6,8 wait frames between animations frames
    } anim;
} UnitType;

typedef enum __attribute__((__packed__)) {
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
} UnitTypeIndex;

typedef struct {
    UWORD hp;
    UBYTE mana;
} UnitStats;

typedef struct _unitmanager tUnitManager;

typedef struct _unit {
    UBYTE type;
    UBYTE owner;
    union {
        struct {
             // tile Y
            UBYTE y;
            // tile X
            UBYTE x;
        };
        tUbCoordYX loc;
    };
    Action action;
    Action nextAction;

    UnitStats stats;
    struct {
        tUwCoordYX sPos;
        PLANEPTR sprite;
        PLANEPTR mask;
    } bob;
    UBYTE frame;
    BYTE IX;
    BYTE IY;
} Unit;

extern void unitsLoad(tFile *map);

/* The global list of unit types */
extern UnitType UnitTypes[];

/* The maximum number of units the game will allocate memory for. */
#define MAX_UNITS 200

static inline UBYTE unitTypeMaxHealth(UnitType *type) {
    return type->stats.hpBase << type->stats.hpShift;
}

/**
 * @brief Initialize the unit manager.
 */
void unitManagerInitialize(void);

/**
 * @brief Deallocate resources used by the unit manager
 */
void unitManagerDestroy(void);

/**
 * @brief Process all units' drawing and actions. Must be called after bobBegin() and before bobEnd()!
 * 
 * @param pUnitListHead unit list pointer created from unitManagerInitialize()
 * @param pTileData 2d tile grid of map
 * @param viewportTopLeft top left visible tile
 * @param viewportBottomRight bottom right visible tile
 */
void unitManagerProcessUnits(tUbCoordYX viewportTopLeft, tUbCoordYX viewportBottomRight);

/**
 * @brief Initialized a new unit of a specific type.
 * 
 * @param pUnitListHead unit list pointer created from unitManagerInitialize()
 * @param type of unit to initialize
 * @return new unit pointer
 */
Unit * unitNew(UnitTypeIndex type, UBYTE owner);

/**
 * @brief Free the selected unit.
 * 
 * @param pUnitListHead unit list pointer created from unitManagerInitialize()
 * @param unit to free
 */
void unitDelete(Unit *unit);

/**
 * @brief Return unit on tile, if any
 * 
 * @param pUnitListHead unit list pointer created from unitManagerInitialize()
 * @param tile position where to look for unit
 * @return Unit* or NULL, if no unit is at the position
 */
Unit *unitManagerUnitAt(tUbCoordYX tile);

_Static_assert(MAP_SIZE * TILE_SIZE < 0xfff, "map is small enough to fit locations in bytes");

tUbCoordYX unitGetTilePosition(Unit *self);

UBYTE unitPlace(Unit *unit, UBYTE x, UBYTE y);

void unitSetOffMap(Unit *self);

void unitSetFrame(Unit *self, UBYTE ubFrame);

UBYTE unitGetFrame(Unit *self);

#endif
