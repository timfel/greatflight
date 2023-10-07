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
    unit->nextAction.ubActionDataB = goal.ubY;
    unit->nextAction.ubActionDataA = goal.ubX;
    unit->nextAction.ubActionDataC = 0;
    unit->nextAction.ubActionDataD = 0;
    unit->nextAction.action = ActionMove;
}

void actionStill(Unit *unit) {
    if (unit->nextAction.action) {
        unit->action = unit->nextAction.action;
        unit->ulActionData = unit->nextAction.ulActionData;
        unit->nextAction.action = 0;
    } else if (unit->ubActionDataA-- == 0) {
        unitSetFrame(unit, (unitGetFrame(unit) + (UBYTE)g_pCustom->joy0dat) % DIRECTIONS);
    }
}

#define moveTargetXMask 0b1111110000000000
#define moveTargetYMask 0b0000001111110000
#define moveCounterMask 0b0000000000001111
#define moveTargetXShift 10
#define moveTargetYShift 4
#define moveCounterShift 0
void actionMove(Unit *unit, UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE]) {
    if (unit->ubActionDataC) {
        --unit->ubActionDataC;
        return;
    }
    UnitType type = UnitTypes[unit->type];
    unit->ubActionDataC = (type.anim.wait + 2) * 2;

    UBYTE speed = type.stats.speed;
    BYTE vectorX = 0;
    BYTE vectorY = 0;

    if (unit->IX || unit->IY) {
        if (unit->IX > 0) {
            vectorX = -1;
            unit->IX -= speed;
        } else if (unit->IX < 0) {
            vectorX = 1;
            unit->IX += speed;
        }
        if (unit->IY > 0) {
            vectorY = -1;
            unit->IY -= speed;
        } else if (unit->IY < 0) {
            vectorY = 1;
            unit->IY += speed;
        }
    } else {
        // can break here
        actionStill(unit);
        if (unit->action != ActionMove) {
            // done moving
            return;
        }
        tUbCoordYX tilePos = unitGetTilePosition(unit);
        vectorX = unit->ubActionDataA - unit->loc.ubX;
        vectorY = unit->ubActionDataB - unit->loc.ubY;
        if (vectorX) {
            vectorX = vectorX > 0 ? 1 : -1;
            if (!mapIsWalkable(map, tilePos.ubX + vectorX, tilePos.ubY)) {
                // unwalkable tile horizontal
                vectorX = 0;
            }
        }
        if (vectorY) {
            vectorY = vectorY > 0 ? 1 : -1;
            if (!mapIsWalkable(map, tilePos.ubX, tilePos.ubY + vectorY)) {
                // unwalkable tile vertical
                vectorY = 0;
            }
        }
        if (!vectorX && !vectorY) {
            // reached goal or unreachable goal
            unitSetFrame(unit, 0);
            unit->action = ActionStill;
            return;
        }
        unmarkMapTile(map, tilePos.ubX, tilePos.ubY);
        if (vectorX) {
            unit->loc.ubX += vectorX;
            unit->IX = -vectorX * (PATHMAP_TILE_SIZE - speed);
        }
        if (vectorY) {
            unit->loc.ubY += vectorY;
            unit->IY = -vectorY * (PATHMAP_TILE_SIZE - speed);
        }
        tilePos = unitGetTilePosition(unit);
        markMapTile(map, tilePos.ubX, tilePos.ubY);
    }
    // next walk frame, walk frames start at row 1 (all units have 1 still frame)
    UBYTE nextFrame = ((unitGetFrame(unit) / DIRECTIONS) % type.anim.walk + 1) * DIRECTIONS;
    if (vectorX > 0) {
        nextFrame += DIRECTION_EAST;
    } else if (vectorX < 0) {
        nextFrame += DIRECTION_WEST;
    } else if (vectorY > 0) {
        nextFrame += DIRECTION_SOUTH;
    } else {
        nextFrame += DIRECTION_NORTH;
    }
    unitSetFrame(unit, nextFrame);
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

void actionDo(Unit *unit, UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE]) {
    switch (unit->action) {
        case ActionStill:
            actionStill(unit);
            return;
        case ActionMove:
            actionMove(unit, map);
            return;
        // TODO:
    }
}
