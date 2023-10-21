#include "include/actions.h"
#include "include/units.h"
#include "game.h"

#include <ace/utils/custom.h>
#include <ace/managers/log.h>

void actionMoveTo(Unit *unit, tUbCoordYX goal) {
    unit->nextAction.move.ubTargetY = goal.ubY;
    unit->nextAction.move.ubTargetX = goal.ubX;
    unit->nextAction.move.u4Wait = 0;
    unit->nextAction.move.u4Retries = 15;
    unit->nextAction.action = ActionMove;
}

enum __attribute__((__packed__)) BuildState {
    BuildStateMoveToGoal,
    BuildStateOpenConstructionSite,
    BuildStateBuilding
};

void actionBuildAt(Unit *unit, tUbCoordYX goal, BuildingType tileType) {
    actionMoveTo(unit, goal);
    unit->nextAction.build.u5BuildingType = tileType;
    unit->nextAction.build.u2State = BuildStateMoveToGoal;
    unit->nextAction.action = ActionBuild;
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
        if (unit->action.still.ubWait-- == 0) {
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
        vectorX = unit->action.move.ubTargetX - unit->loc.ubX;
        vectorY = unit->action.move.ubTargetY - unit->loc.ubY;
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
            --unit->action.move.u4Retries;
            unitSetFrame(unit, 0);
            if (unit->action.move.u4Retries == 0) {
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
    if (unit->action.move.u4Wait) {
        --unit->action.move.u4Wait;
        return;
    }
    unit->action.move.u4Wait = (type.anim.wait + 1) << 1;

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

void actionBuild(Unit *unit, UBYTE map[PATHMAP_SIZE][PATHMAP_SIZE]) {
    switch (unit->action.build.u2State) {
        case BuildStateMoveToGoal:
            actionMove(unit, map);
            if (unit->action.action == ActionStill) {
                if (unit->loc.ubX == unit->action.move.ubTargetX && unit->loc.ubY == unit->action.move.ubTargetY) {
                    // reached goal
                    unit->action.action = ActionBuild;
                    unit->action.build.u2State = BuildStateOpenConstructionSite;
                } else {
                    // could not reach goal, abort
                    logWrite("Could not reach goal");
                }
            }
            return;
        case BuildStateOpenConstructionSite: {
            UBYTE ubX = unit->action.move.ubTargetX;
            UBYTE ubY = unit->action.move.ubTargetY;
            UBYTE buildingSize = 2; // TODO...
            // TODO: pull out, and unify with the same in icons.c
            for (UBYTE x = 0; x < buildingSize; ++x) {
                for (UBYTE y = 0; y < buildingSize; ++y) {
                    // x == 0 and y == 0 is where the builder is now
                    if ((x || y) && map[ubX + x][ubY + y] != MAP_GROUND_FLAG) {
                        // can no longer build here
                        unit->action.action = ActionStill;
                        logWrite("Cannot build here");
                        return;
                    }
                }
            }
            // TODO: this should be a function to place the building
            // TODO: this should also create the building, and give it the action "being built"
            for (UBYTE x = 0; x < buildingSize; ++x) {
                for (UBYTE y = 0; y < buildingSize; ++y) {
                    map[ubX + x][ubY + y] = MAP_UNWALKABLE_FLAG;
                }
            }
            UBYTE ubTileX = ubX / TILE_SIZE_FACTOR;
            UBYTE ubTileY = ubY / TILE_SIZE_FACTOR;
            UBYTE buildingTileIdx = buildingSize == 2 ? BUILDING_CONSTRUCTION_SMALL : BUILDING_CONSTRUCTION_LARGE;
            for (UBYTE y = 0; y < buildingSize / TILE_SIZE_FACTOR; ++y) {
                for (UBYTE x = 0; x < buildingSize / TILE_SIZE_FACTOR; ++x) {
                    g_Map.m_ulTilemapXY[ubTileX + x][ubTileY + y] = tileIndexToTileBitmapOffset(buildingTileIdx++);
                }
            }

            unitSetOffMap(unit);
            unit->action.build.u2State = BuildStateBuilding;
            unit->action.build.ubBuildingID = unit->action.build.u5BuildingType; // TODO: should be the return value from above
            unit->action.build.u5buildingHPIncrease = 31; // TODO: should be based in the building type
            return;
        }
        case BuildStateBuilding: {
            if (0) {
                // TODO: check if building is alive
                unit->action.action = ActionStill;
                // TODO: what if no room? unit lost?
                unitPlace(map, unit, unit->action.move.ubTargetX, unit->action.move.ubTargetY);
            } else if (0) {
                // TODO: check if building now has full hp
                unit->action.action = ActionStill;
                // TODO: what if no room? unit lost?
                unitPlace(map, unit, unit->action.move.ubTargetX, unit->action.move.ubTargetY);
            } else {
                // TODO: transfer more HP to building
                // XXX: for now just something completely different to see an effect
                UBYTE buildingSize = 2;
                UBYTE ubX = unit->action.move.ubTargetX;
                UBYTE ubY = unit->action.move.ubTargetY;
                UBYTE ubTileX = ubX / TILE_SIZE_FACTOR;
                UBYTE ubTileY = ubY / TILE_SIZE_FACTOR;
                UBYTE buildingTileIdx = unit->action.build.ubBuildingID;
                if (!unit->action.build.u5buildingHPIncrease--) {
                    for (UBYTE y = 0; y < buildingSize / TILE_SIZE_FACTOR; ++y) {
                        for (UBYTE x = 0; x < buildingSize / TILE_SIZE_FACTOR; ++x) {
                            g_Map.m_ulTilemapXY[ubTileX + x][ubTileY + y] = tileIndexToTileBitmapOffset(buildingTileIdx++);
                        }
                    }
                    unit->action.action = ActionStill;
                    // TODO: what if no room? unit lost?
                    unitPlace(map, unit, ubX, ubY);
                }
            }
        }
    }
}

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
        case ActionBuild:
            actionBuild(unit, map);
            return;
        default:
            return;
    }
}
