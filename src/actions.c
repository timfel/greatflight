#include "actions.h"

enum ActionTypes {
    ActionStill,
    ActionMove,
    ActionAttackMove,
    ActionAttackTarget,
    ActionHarvest,
    ActionCast,
    ActionDie
};

void actionMoveTo(Unit *unit, tUbCoordYX goal) {
    unit->ubActionDataA = goal.ubX;
    unit->ubActionDataB = goal.ubY;
    unit->ubActionDataC = 0;
    unit->ubActionDataD = 0;
    unit->action = ActionMove;
}

#define moveTargetXMask 0b1111110000000000
#define moveTargetYMask 0b0000001111110000
#define moveCounterMask 0b0000000000001111
#define moveTargetXShift 10
#define moveTargetYShift 4
#define moveCounterShift 0
void actionMove(Unit *unit, UBYTE **map) {
    UnitType type = UnitTypes[unit->type];
    UBYTE speed = type.stats.speed;

    tUbCoordYX tilePos = unitGetTilePosition(unit);

    unmarkMapTile(map, tilePos.ubX, tilePos.ubY);

    BYTE vectorX = unit->ubActionDataA - unit->loc.ubX;
    BYTE vectorY = unit->ubActionDataB - unit->loc.ubY;

    if (map[tilePos.ubX + ((vectorX > 0) - (vectorX < 0))][tilePos.ubY] > 15) {
        // unwalkable tile horizontal
        vectorX = 0;
    }
    if (map[tilePos.ubX][tilePos.ubY + ((vectorY > 0) - (vectorY < 0))] > 15) {
        // unwalkable tile vertical
        vectorY = 0;
    }

    BYTE absVX = vectorX < 0 ? -vectorX : vectorX;
    BYTE absVY = vectorY < 0 ? -vectorY : vectorY;
    UWORD length = absVX + absVY;
    if (length == 0) {
        // reached goal or unreachable goal
        unitSetFrame(unit, 0);
        unit->action = ActionStill;
    }
    if (absVX) {
        BYTE xdist = vectorX / absVX * speed + unit->IX;
        BYTE tileStep = xdist / (TILE_SIZE / 2);
        BYTE stepRem = xdist % (TILE_SIZE / 2);
        unit->loc.ubX += tileStep;
        unit->IX = stepRem;
    }
    if (absVY) {
        BYTE ydist = vectorY / absVY * speed + unit->IY;
        BYTE tileStep = ydist / (TILE_SIZE / 2);
        BYTE stepRem = ydist % (TILE_SIZE / 2);
        unit->loc.ubY += tileStep;
        unit->IY = stepRem;
    }

    tilePos = unitGetTilePosition(unit);
    markMapTile(map, tilePos.ubX, tilePos.ubY);

    UBYTE nextFrame = unitGetFrame(unit) ? 0 : 1;
    if (absVX > absVY) {
        if (vectorX > 0) {
            unitSetFrame(unit, DIRECTION_EAST + nextFrame);
        } else {
            unitSetFrame(unit, DIRECTION_WEST + nextFrame);
        }
    } else {
        if (vectorY > 0) {
            unitSetFrame(unit, DIRECTION_SOUTH + nextFrame);
        } else {
            unitSetFrame(unit, DIRECTION_NORTH + nextFrame);
        }
    }
};

void actionAttackMove(Unit  __attribute__((__unused__)) *unit) {
    return;
};

void actionAttackTarget(Unit  __attribute__((__unused__)) *unit) {
    return;
};

void actionHarvest(Unit  __attribute__((__unused__)) *unit) {
    return;
};

void actionCast(Unit  __attribute__((__unused__)) *unit) {
    return;
};

void actionDie(Unit  __attribute__((__unused__)) *unit) {
    return;
};

void actionDo(Unit *unit, UBYTE **map) {
    switch (unit->action) {
        case ActionStill:
            return;
        case ActionMove:
            actionMove(unit, map);
            return;
        // TODO:
    }
}
