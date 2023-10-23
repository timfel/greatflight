#include "include/actions.h"
#include "include/units.h"
#include "include/buildings.h"
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

void actionBuildAt(Unit *unit, tUbCoordYX goal, UBYTE tileType) {
    actionMoveTo(unit, goal);
    unit->nextAction.build.u6BuildingType = tileType;
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

void actionMove(Unit *unit) {
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
            if (!mapIsWalkable(tilePos.ubX + vectorX, tilePos.ubY)) {
                // unwalkable tile horizontal
                vectorX = 0;
            }
        }
        if (vectorY) {
            vectorY = vectorY > 0 ? 1 : -1;
            if (!mapIsWalkable(tilePos.ubX + vectorX, tilePos.ubY + vectorY)) {
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
        unmarkMapTile(tilePos.ubX, tilePos.ubY);
        if (vectorX) {
            unit->loc.ubX += vectorX;
            unit->IX = -vectorX * (PATHMAP_TILE_SIZE - speed);
        }
        if (vectorY) {
            unit->loc.ubY += vectorY;
            unit->IY = -vectorY * (PATHMAP_TILE_SIZE - speed);
        }
        tilePos = unitGetTilePosition(unit);
        markMapTile(tilePos.ubX, tilePos.ubY);
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

void actionBuild(Unit *unit) {
    switch (unit->action.build.u2State) {
        case BuildStateMoveToGoal:
            actionMove(unit);
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
            UBYTE typeIdx = unit->action.build.u6BuildingType;
            if (!buildingCanBeAt(typeIdx, unit->action.move.target, 1)) {
                unit->action.action = ActionStill;
                logWrite("Cannot build here");
                return;
            }
            UBYTE newBuildingId = buildingNew(typeIdx, unit->action.move.target);
            unitSetOffMap(unit);
            unit->action.build.u2State = BuildStateBuilding;
            unit->action.build.ubBuildingID = newBuildingId;
            unit->action.build.u6buildingHPWait = -1;
            unit->action.build.u6buildingHPIncrease = BuildingTypes[typeIdx].costs.hpIncrease;
            return;
        }
        case BuildStateBuilding: {
            Building *building = &g_BuildingManager.building[unit->action.build.ubBuildingID];
            if (!building->hp) {
                // building was destroyed while building
                unit->action.action = ActionStill;
                // TODO: what if no room? unit lost?
                unitPlace(unit, unit->action.move.ubTargetX, unit->action.move.ubTargetY);
            } else if (building->hp >= BuildingTypes[building->type].stats.maxHP) {
                unit->action.action = ActionStill;
                // TODO: what if no room? unit lost?
                unitPlace(unit, unit->action.move.ubTargetX, unit->action.move.ubTargetY);
            } else {
                if (--unit->action.build.u6buildingHPWait) {
                    return;
                }
                building->hp += unit->action.build.u6buildingHPIncrease + 4;
            }
        }
    }
}

void actionDo(Unit *unit) {
    switch (unit->action.action) {
        case ActionStill:
            actionStill(unit);
            return;
        case ActionMove:
            actionMove(unit);
            return;
        case ActionStop:
            unit->action.action = ActionStill;
            return;
        case ActionBuild:
            actionBuild(unit);
            return;
        default:
            return;
    }
}

void actionBeingBuilt(Building *building) {
    BuildingType *type = &BuildingTypes[building->type];
    if (building->hp >= type->stats.maxHP) {
        building->hp = type->stats.maxHP;
        building->action.action = ActionStill;
        tUbCoordYX loc = building->loc;
        loc.uwYX = loc.uwYX / TILE_SIZE_FACTOR;
        UBYTE buildingTileIdx = type->tileIdx;
        UBYTE buildingSize = type->size / TILE_SIZE_FACTOR;
        for (UBYTE y = 0; y < buildingSize; ++y) {
            for (UBYTE x = 0; x < buildingSize; ++x) {
                g_Map.m_ulTilemapXY[loc.ubX + x][loc.ubY + y] = tileIndexToTileBitmapOffset(buildingTileIdx++);
            }
        }
    }
}

void buildingDo(Building *building) {
    switch (building->action.action) {
        case ActionStill:
            return;
        case ActionBeingBuilt:
            actionBeingBuilt(building);
            return;
        default:
            return;
    }
}