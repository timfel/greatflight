#ifndef UNITS_H
#define UNITS_H

#include "map.h"
#include "icons.h"

#include <stdint.h>
#include <ace/managers/bob.h>
#include <ace/utils/bitmap.h>
#include <ace/utils/file.h>

// TODO: this can be configurable for different systems
#define BPP 4
#define UNIT_SIZE 32
#define UNIT_SIZE_SHIFT 5
#define UNIT_FRAME_BYTES (UNIT_SIZE / 8 + UNIT_SIZE * BPP)
/* Units are centered on a 2x2 tilegrid. So to position a unit on a tile, we subtract 8px */
#define UNIT_POSITION_OFFSET 8

#define FRAME_SIZE 32
#define WALK_FRAMES 2
#define ATTACK_FRAMES 3
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
    uint8_t iconIdx;
    struct {
        uint8_t maxHP;
        uint8_t speed;
        uint8_t hasMana;
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
    uint8_t hp;
    uint8_t mana;
} UnitStats;

_Static_assert(sizeof(UnitStats) == sizeof(uint16_t), "unit stats is not 1 word");

typedef struct _unitmanager tUnitManager;

typedef struct {
    uint8_t type;
    uint8_t action;
    union {
        uint32_t ulActionData;
        struct {
            uint16_t uwActionDataA;
            uint16_t uwActionDataB;
        };
        struct {
            uint8_t ubActionDataA;
            uint8_t ubActionDataB;
            uint8_t ubActionDataC;
            uint8_t ubActionDataD;
        };
    };
    UnitStats stats;
    tBob bob;
    uint8_t frame;
} Unit;

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
void unitManagerProcessUnits(tUnitManager *pUnitListHead, uint8_t **pTileData, tUbCoordYX viewportTopLeft, tUbCoordYX viewportBottomRight);

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
    tUbCoordYX loc;
    tUwCoordYX bobLoc = self->bob.sPos;
    loc.ubX = (bobLoc.uwX >> TILE_SHIFT) + 1;
    loc.ubY = (bobLoc.uwY >> TILE_SHIFT) + 1;
    return loc;
}

static inline void unitSetTilePosition(Unit *self, UBYTE **map, tUbCoordYX pos) {
    self->bob.sPos.uwX = (pos.ubX << TILE_SHIFT) - 8;
    self->bob.sPos.uwY = (pos.ubY << TILE_SHIFT) - 8;
    if (pos.ubX > 1) {
        markMapTile(map, pos.ubX, pos.ubY);
    }
}

static inline void unitDraw(Unit *self) {
    bobPush(&self->bob);
}

static inline void unitSetFrame(Unit *self, UBYTE ubFrame) {
    uint16_t offset = ubFrame * UNIT_FRAME_BYTES;
    bobSetFrame(&self->bob, UnitTypes[self->type].spritesheet->Planes[0] + offset, UnitTypes[self->type].mask->Planes[0] + offset);
}

static inline UBYTE unitGetFrame(Unit *self) {
    return self->frame;
}

#endif
