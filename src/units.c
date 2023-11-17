#include "include/actions.h"
#include "include/units.h"
#include "include/player.h"
#include "game.h"
#include "ace/types.h"
#include "ace/macros.h"
#include "ace/managers/memory.h"

tUnitManager g_UnitManager;

UnitType UnitTypes[] = {
    [dead] = {},
    [peasant] = {
        .name = "Peasant",
        .spritesheetPath = "resources/units/peasant.bm",
        .maskPath = "resources/units/peasant.msk",
        .stats = {
            .hpShift = 0,
            .hpBase = HP_30,
            .maxMana = 0,
            .speed = 2,
        },
        .iconIdx = ICON_PEASANT,
        .costs = {
            .gold = 400,
            .lumber = 0,
            .timeBase = HP_30,
            .timeShift = 3,
        },
        .anim = {
            .large = 0,
            .walk = 3,
            .attack = 2,
            .fall = 0,
            .wait = 2,
        },
    },
    [peon] = {},
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

void unitManagerInitialize(void) {
    g_UnitManager.unitCount = 0;
    for (UBYTE i = 0; i < MAX_UNITS; ++i) {
#ifdef ACE_DEBUG
        g_UnitManager.units[i].loc = UNIT_FREE_TILE_POSITION;
#endif
        g_UnitManager.units[i].id = i;
        g_UnitManager.freeUnitsStack[i] = i;
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
    for (UBYTE i = 0; i < g_UnitManager.unitCount; ++i) {
#ifdef ACE_DEBUG
        if (unitManagerUnitIsActive(&g_UnitManager.units[i]))
#endif
        unitDelete(&g_UnitManager.units[i]);
    }
}

static inline void unitDraw(Unit *self, tUbCoordYX viewportTopLeft) {
    UnitType *type = &UnitTypes[self->type];
    UWORD uwX = (self->x - (viewportTopLeft.ubX / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR)) * PATHMAP_TILE_SIZE + self->IX;
    UWORD uwY = (self->y - (viewportTopLeft.ubY / TILE_SIZE_FACTOR * TILE_SIZE_FACTOR)) * PATHMAP_TILE_SIZE + self->IY;
    self->bob.sPos.uwX = uwX;
    self->bob.sPos.uwY = uwY;
    UBYTE ubDstOffs = uwX & 0xF;
    UBYTE ubSize = type->anim.large ? 24 : 16;
    UBYTE ubBlitWidth = (ubSize + ubDstOffs + 15) & 0xF0;
    UBYTE ubBlitWords = ubBlitWidth >> 4;
    UWORD uwBlitSize = ((ubSize * BPP) << HSIZEBITS) | ubBlitWords;
    WORD wSrcModulo = (ubSize >> 3) - (ubBlitWords << 1);
    UWORD uwBltCon1 = ubDstOffs << BSHIFTSHIFT;
    UWORD uwBltCon0 = uwBltCon1 | USEA | USEB | USEC | USED | MINTERM_COOKIE;
    WORD wDstModulo = MAP_BUFFER_WIDTH_BYTES - (ubBlitWords << 1);
    UBYTE *pB = self->bob.sprite;
    ULONG ulSrcOffs = MAP_BUFFER_BYTES_PER_ROW * uwY + uwX / 8;
    UBYTE *pCD = &g_Screen.m_map.m_pBuffer->pBack->Planes[0][ulSrcOffs];
    UWORD uwLastMask = 0xFFFF << (ubBlitWidth - ubSize);
    UBYTE *pA = self->bob.mask;
    blitWait();
    g_pCustom->bltcon0 = uwBltCon0;
    g_pCustom->bltcon1 = uwBltCon1;
    g_pCustom->bltalwm = uwLastMask;
    g_pCustom->bltamod = wSrcModulo;
    g_pCustom->bltapt = (APTR)pA;
    g_pCustom->bltbmod = wSrcModulo;
    g_pCustom->bltcmod = wDstModulo;
    g_pCustom->bltdmod = wDstModulo;
    g_pCustom->bltbpt = (APTR)pB;
    g_pCustom->bltcpt = (APTR)pCD;
    g_pCustom->bltdpt = (APTR)pCD;
    g_pCustom->bltsize = uwBlitSize;
}

static inline void unitOffscreen(Unit *self) {
    self->bob.sPos.uwX = UWORD_MAX;
}

void unitManagerProcessUnits(tUbCoordYX viewportTopLeft, tUbCoordYX viewportBottomRight) {
    for (UBYTE i = 0; i < g_UnitManager.unitCount; ++i) {
        Unit *unit = &g_UnitManager.units[i];
        actionDo(unit);
        tUbCoordYX loc = unitGetTilePosition(unit);
        if (loc.ubX >= viewportTopLeft.ubX
                && loc.ubY >= viewportTopLeft.ubY
                && loc.ubX <= viewportBottomRight.ubX
                && loc.ubY <= viewportBottomRight.ubY) {
            unitDraw(unit, viewportTopLeft);
        } else {
            unitOffscreen(unit);
        }
    }
}

Unit *unitManagerUnitAt(tUbCoordYX tile) {
    UBYTE id = mapGetUnitAt(tile.ubX, tile.ubY);
    if (id < MAX_UNITS) {
        Unit *unit = &g_UnitManager.units[id];
#ifdef ACE_DEBUG
        if (!unitManagerUnitIsActive(unit)) {
            logWrite("FATAL: Found inactive unit in cache!!!");
        }
        if (unit->loc.uwYX != tile.uwYX) {
            logWrite("FATAL: Found unit in cache at different location!!!");
        }
#endif
        return unit;
    }
    return NULL;
}

Unit * unitNew(UnitTypeIndex typeIdx, UBYTE owner) {
    if (g_UnitManager.unitCount >= MAX_UNITS) {
        return NULL;
    }
    UnitType *type = &UnitTypes[typeIdx];
    Unit *unit = &g_UnitManager.units[g_UnitManager.freeUnitsStack[g_UnitManager.unitCount]];
    g_UnitManager.unitCount++;

    unit->bob.sprite = type->spritesheet->Planes[0];
    unit->bob.mask = type->spritesheet->Planes[0];
    unit->type = typeIdx;
    unit->loc = UNIT_INIT_TILE_POSITION;
    unit->owner = owner;
    unit->stats.hp = unitTypeMaxHealth(&UnitTypes[unit->type]);
    unit->stats.mana = 0;
    return unit;
}

void unitDelete(Unit *unit) {
#ifdef ACE_DEBUG
    if (!unitManagerUnitIsActive(unit)) {
        logWrite("Deleting inactive unit!!!");
    }
    unit->loc = UNIT_FREE_TILE_POSITION;
#endif
    g_UnitManager.unitCount--;
    g_UnitManager.freeUnitsStack[g_UnitManager.unitCount] = unit->id;
}

UBYTE unitCanBeAt(Unit *, UBYTE x, UBYTE y) {
    // TODO: when we have units larger than 1 tile, check that here
    return mapIsWalkable(x, y);
}

UBYTE unitPlace(Unit *unit, tUbCoordYX loc) {
    UBYTE x = loc.ubX;
    UBYTE y = loc.ubY;
    UBYTE actualX, actualY;
    // drop out no further than 5 tiles away
    for (UBYTE range = 0; range < 5; ++range) {
        for (UBYTE xoff = 0; xoff < range * 2; ++xoff) {
            actualX = x + xoff;
            for (UBYTE yoff = 0; yoff < range * 2; ++yoff) {
                actualY = y + yoff;
                if (unitCanBeAt(unit, actualX, actualY)) {
                    unit->x = actualX;
                    unit->y = actualY;
                    mapMarkTileOccupied(unit->id, unit->owner, actualX, actualY);
                    mapMarkUnitSight(actualX, actualY, SIGHT_MEDIUM);
                    return 1;
                }
                actualY = y - yoff;
                if (unitCanBeAt(unit, actualX, actualY)) {
                    unit->x = actualX;
                    unit->y = actualY;
                    mapMarkTileOccupied(unit->id, unit->owner, actualX, actualY);
                    mapMarkUnitSight(actualX, actualY, SIGHT_MEDIUM);
                    return 1;
                }
            }
            actualX = x - xoff;
            for (UBYTE yoff = 0; yoff < range * 2; ++yoff) {
                actualY = y + yoff;
                if (unitCanBeAt(unit, actualX, actualY)) {
                    unit->x = actualX;
                    unit->y = actualY;
                    mapMarkTileOccupied(unit->id, unit->owner, actualX, actualY);
                    mapMarkUnitSight(actualX, actualY, SIGHT_MEDIUM);
                    return 1;
                }
                actualY = y - yoff;
                if (unitCanBeAt(unit, actualX, actualY)) {
                    unit->x = actualX;
                    unit->y = actualY;
                    mapMarkTileOccupied(unit->id, unit->owner, actualX, actualY);
                    mapMarkUnitSight(actualX, actualY, SIGHT_MEDIUM);
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void loadUnit(tFile *map, UBYTE owner) {
    UBYTE type, x, y;
    fileRead(map, &type, 1);
    Unit *unit = unitNew(type, owner);
    fileRead(map, &x, 1);
    fileRead(map, &y, 1);
    if (!unitPlace(unit, (tUbCoordYX){.ubX = x, .ubY = y})) {
        logWrite("DANGER WILL ROBINSON! Could not place unit at %d:%d", x, y);
        unitDelete(unit);
        return;
    }
    fileRead(map, &unit->action, 1);
    fileRead(map, &unit->action.ubActionDataA, 1);
    fileRead(map, &unit->action.ubActionDataB, 1);
    fileRead(map, &unit->action.ubActionDataC, 1);
    fileRead(map, &unit->action.ubActionDataD, 1);
    fileRead(map, &unit->stats.hp, 1);
    if (unit->stats.hp == 0) {
        unit->stats.hp = unitTypeMaxHealth(&UnitTypes[unit->type]);
    }
    fileRead(map, &unit->stats.mana, 1);
    if (unit->stats.mana == 0) {
        unit->stats.mana = UnitTypes[unit->type].stats.maxMana;
    }
}

void unitsLoad(tFile *map) {
    for (UBYTE i = 0; i < sizeof(g_pPlayers) / sizeof(Player); ++i) {
        UBYTE count;
        fileRead(map, &count, 1);
        for (UBYTE unit = 0; unit < count; ++unit) {
            loadUnit(map, i);
        }
    }
}

tUbCoordYX unitGetTilePosition(Unit *self) {
    return self->loc;
}

void unitSetFrame(Unit *self, UBYTE ubFrame) {
    self->frame = ubFrame;
    UnitType *type = &UnitTypes[self->type];
    UWORD offset = ubFrame * (type->anim.large ? (24 / 8 * 24 * BPP) : (16 / 8 * 16 * BPP));
    self->bob.sprite = type->spritesheet->Planes[0] + offset;
    self->bob.mask = type->mask->Planes[0] + offset;
}

UBYTE unitGetFrame(Unit *self) {
    return self->frame;
}

void unitSetOffMap(Unit *self) {
    mapUnmarkTileOccupied(self->x, self->y);
    mapUnmarkUnitSight(self->x, self->y, SIGHT_MEDIUM);
    for (UBYTE unitIdx = 0; unitIdx < g_Screen.m_ubSelectedUnitCount; ++unitIdx) {
        if (g_Screen.m_pSelectedUnit[unitIdx] == self) {
            g_Screen.m_ubSelectedUnitCount = 0;
            g_Screen.m_ubBottomPanelDirty = 1;
            break;
        }
    }
    self->loc = UNIT_FREE_TILE_POSITION;
}
