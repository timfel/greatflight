#ifndef UNITS_H
#define UNITS_H

#include "include/actions.h"
#include "include/map.h"
#include "include/icons.h"

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
    struct {
        UWORD gold;
        UWORD lumber;
        UWORD time;
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
    UBYTE hp;
    UBYTE mana;
} UnitStats;

_Static_assert(sizeof(UnitStats) == sizeof(UWORD), "unit stats is not 1 word");

typedef struct _unitmanager tUnitManager;

typedef struct _unit {
    UBYTE type;
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
Unit * unitNew(UnitTypeIndex type);

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
