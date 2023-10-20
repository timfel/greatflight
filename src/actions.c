#include "include/actions.h"
#include "include/units.h"

#include <ace/utils/custom.h>

void actionMoveTo(Unit *unit, tUbCoordYX goal) {
    unit->nextAction.ubActionDataB = goal.ubY;
    unit->nextAction.ubActionDataA = goal.ubX;
    unit->nextAction.ubActionDataC = 0;
    unit->nextAction.ubActionDataD = 25; // retries
    unit->nextAction.action = ActionMove;
}

void actionStop(Unit *unit) {
    unit->nextAction.action = ActionStop;
}

UBYTE actionStill(Unit *unit) {
    if (unit->nextAction.action) {
        unit->action.action = unit->nextAction.action;
        unit->action.ulActionData = unit->nextAction.ulActionData;
        unit->nextAction.action = 0;
        return 1;
    } else if (unit->action.action == ActionStill) {
        if (unit->action.ubActionDataA-- == 0) {
            unitSetFrame(unit, (unitGetFrame(unit) + (UBYTE)g_pCia[CIA_A]->talo) % DIRECTIONS);
        }
    }
    return 0;
}

#define moveTargetXMask 0b1111110000000000
#define moveTargetYMask 0b0000001111110000
#define moveCounterMask 0b0000000000001111
#define moveTargetXShift 10
#define moveTargetYShift 4
#define moveCounterShift 0
void actionMove(Unit *unit, UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE]) {
    UnitType type = UnitTypes[unit->type];
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
        if (actionStill(unit)) {
            return;
        }
        tUbCoordYX tilePos = unitGetTilePosition(unit);
        vectorX = unit->action.ubActionDataA - unit->loc.ubX;
        vectorY = unit->action.ubActionDataB - unit->loc.ubY;
        if (!vectorX && !vectorY) {
            // reached our goal
            unitSetFrame(unit, 0);
            unit->action.action = ActionStill;
            return;
        }
        if (vectorX) {
            vectorX = vectorX > 0 ? 1 : -1;
            if (!mapIsWalkable(map, tilePos.ubX + vectorX, tilePos.ubY)) {
                // unwalkable tile horizontal
                vectorX = 0;
            }
        }
        if (vectorY) {
            vectorY = vectorY > 0 ? 1 : -1;
            if (!mapIsWalkable(map, tilePos.ubX + vectorX, tilePos.ubY + vectorY)) {
                // unwalkable tile vertical
                vectorY = 0;
            }
        }
        if (!vectorX && !vectorY) {
            // unreachable step, wait a little?
            --unit->action.ubActionDataD;
            unitSetFrame(unit, 0);
            if (unit->action.ubActionDataD == 0) {
                // done trying
                unit->action.action = ActionStill;
            }
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
    if (unit->action.ubActionDataC) {
        --unit->action.ubActionDataC;
        return;
    }
    unit->action.ubActionDataC = (type.anim.wait + 1) * 2;

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
    switch (unit->action.action) {
        case ActionStill:
            actionStill(unit);
            return;
        case ActionMove:
            actionMove(unit, map);
            return;
        case ActionStop:
            unit->action.action = ActionStill;
            return;
        default:
            return;
    }
}
