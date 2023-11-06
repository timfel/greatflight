#include "include/actions.h"
#include "include/units.h"
#include "include/buildings.h"
#include "include/player.h"
#include "game.h"

#include <ace/utils/custom.h>
#include <ace/managers/log.h>

void actionMoveTo(Unit *unit, tUbCoordYX goal) {
    unit->nextAction.move.target = goal;
    unit->nextAction.move.u4Wait = 0;
    unit->nextAction.move.u4Retries = 15;
    unit->nextAction.action = ActionMove;
}

void actionMoveToNow(Unit *unit, tUbCoordYX goal) {
    unit->action.move.target = goal;
    unit->action.move.u4Wait = 0;
    unit->action.move.u4Retries = 15;
    unit->action.action = ActionMove;
}

enum __attribute__((__packed__)) HarvestState {
    HarvestMoveToHarvest,
    HarvestWaitAtForest,
    HarvestWaitInMine,
    HarvestMoveToDepot,
    HarvestWaitAtDepot,
};

#define WAIT_AT_FOREST 255
#define WAIT_IN_MINE 25 * 4
#define WAIT_AT_DEPOT 25 * 3
#define RETRY_FINDING_HARVEST 2

void actionHarvestAt(Unit *unit, tUbCoordYX goal) {
    UBYTE tile = g_Map.m_ubPathmapXY[goal.ubX][goal.ubY];
    unit->nextAction.harvest.lastHarvestLocation = goal;
    unit->nextAction.harvest.u4State = HarvestMoveToHarvest;
    unit->nextAction.harvest.ubWait = 0;
    if (tileIsHarvestable(tile)) {
        unit->nextAction.action = ActionHarvestTerrain;
    } else if (!tileIsWalkable(tile)) {
        unit->nextAction.action = ActionHarvestMine;
    } else {
        actionMoveTo(unit, goal);
    }
}

void actionAttackAt(Unit *, tUbCoordYX goal) {
    UBYTE tile = g_Map.m_ubPathmapXY[goal.ubX][goal.ubY];
    if (tileIsWalkable(tile)) {
        logWrite("attack move\n");
    } else {
        logWrite("attack unit\n");
    }
}

enum __attribute__((__packed__)) BuildState {
    BuildStateMoveToGoal,
    BuildStateOpenConstructionSite,
    BuildStateBuilding
};

void actionBuildAt(Unit *unit, tUbCoordYX goal, UBYTE tileType) {
    unit->nextAction.action = ActionBuild;
    unit->nextAction.build.target = goal;
    unit->nextAction.build.ubBuildingType = tileType;
    unit->nextAction.build.ubState = BuildStateMoveToGoal;
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
        if (unit->nextAction.action <= ActionMove && actionStill(unit)) {
            return;
        }
        tUbCoordYX tilePos = unitGetTilePosition(unit);
        tUbCoordYX target = unit->action.move.target;
        vectorX = target.ubX - unit->loc.ubX;
        vectorY = target.ubY - unit->loc.ubY;
        if (!vectorX && !vectorY) {
            // reached our goal
            unitSetFrame(unit, 0);
            if (actionStill(unit)) {
                // there's another action queued now, store the last move target for it to consume
                unit->nextAction.move.target = target;
            } else {
                unit->action.action = ActionStill;
            }
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
                if (actionStill(unit)) {
                    // there's another action queued now, store the last move target for it to consume
                    unit->nextAction.move.target = target;
                } else {
                    unit->action.action = ActionStill;
                }
            }
            return;
        }
        mapUnmarkTileOccupied(tilePos.ubX, tilePos.ubY);
        if (vectorX) {
            unit->loc.ubX += vectorX;
            unit->IX = -vectorX * (PATHMAP_TILE_SIZE - speed);
        }
        if (vectorY) {
            unit->loc.ubY += vectorY;
            unit->IY = -vectorY * (PATHMAP_TILE_SIZE - speed);
        }
        tilePos = unitGetTilePosition(unit);
        mapMarkTileOccupied(tilePos.ubX, tilePos.ubY);
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

static inline UWORD findNearestDepot(Unit *unit) {
    Building *depot = buildingManagerFindBuildingByTypeAndPlayerAndLocation(BUILDING_HUMAN_TOWNHALL, unit->owner, unit->loc);
    if (depot) {
        return depot->loc.uwYX;
    }
    return 0;
}

static inline UBYTE isNextToDepot(tUbCoordYX loc) {
    if (buildingManagerBuildingAt((tUbCoordYX){.ubX = loc.ubX - 1, .ubY = loc.ubY})) {
        return 1;
    }
    if (buildingManagerBuildingAt((tUbCoordYX){.ubX = loc.ubX - 1, .ubY = loc.ubY - 1})) {
        return 1;
    }
    if (buildingManagerBuildingAt((tUbCoordYX){.ubX = loc.ubX, .ubY = loc.ubY + 1})) {
        return 1;
    }
    if (buildingManagerBuildingAt((tUbCoordYX){.ubX = loc.ubX + 1, .ubY = loc.ubY})) {
        return 1;
    }
    return 0;
}

static inline UWORD findHarvestableTileAround(tUbCoordYX loc) {
    for (BYTE xoff = -1; xoff < 2; xoff++) {
        for (BYTE yoff = -1; yoff < 2; yoff++) {
            UBYTE actualX = loc.ubX + xoff;
            UBYTE actualY = loc.ubY + yoff;
            if (mapIsHarvestable(actualX, actualY)) {
                return (tUbCoordYX){.ubX = actualX, .ubY = actualY}.uwYX;
            }
        }
    }
    return 0;
}

static inline UWORD findHarvestSpotAround(UBYTE actualX, UBYTE actualY) {
    if (mapIsHarvestable(actualX, actualY)) {
        if (mapIsWalkable(actualX - 1, actualY)) {
            return (tUbCoordYX){.ubX = actualX - 1, .ubY = actualY}.uwYX;
        }
        if (mapIsWalkable(actualX + 1, actualY)) {
            return (tUbCoordYX){.ubX = actualX + 1, .ubY = actualY}.uwYX;
        }
        if (mapIsWalkable(actualX, actualY - 1)) {
            return (tUbCoordYX){.ubX = actualX, .ubY = actualY - 1}.uwYX;
        }
        if (mapIsWalkable(actualX, actualY + 1)) {
            return (tUbCoordYX){.ubX = actualX, .ubY = actualY + 1}.uwYX;
        }
    }
    return 0;
}

#define HARVEST_SEARCH_RANGE 8
UWORD findHarvestSpotNear(tUbCoordYX loc) {
    UBYTE actualX, actualY;
    UBYTE x = loc.ubX;
    UBYTE y = loc.ubY;
    UWORD result = 0;
    // drop out look no further than 8 tiles away
    for (UBYTE range = 0; range < HARVEST_SEARCH_RANGE; ++range) {
        for (UBYTE xoff = 0; xoff < range * 2; ++xoff) {
            actualX = x + xoff;
            for (UBYTE yoff = 0; yoff < range * 2; ++yoff) {
                actualY = y + yoff;
                if ((result = findHarvestSpotAround(actualX, actualY))) {
                    return result;
                }
                actualY = y - yoff;
                if ((result = findHarvestSpotAround(actualX, actualY))) {
                    return result;
                }
            }
            actualX = x - xoff;
            for (UBYTE yoff = 0; yoff < range * 2; ++yoff) {
                actualY = y + yoff;
                if ((result = findHarvestSpotAround(actualX, actualY))) {
                    return result;
                }
                actualY = y - yoff;
                if ((result = findHarvestSpotAround(actualX, actualY))) {
                    return result;
                }
            }
        }
    }
    return 0;
}

void actionHarvest(Unit *unit) {
    if (actionStill(unit)) {
        return;
    }
    ActionType action = unit->action.action;
    switch (unit->action.harvest.u4State) {
        case HarvestMoveToHarvest: {
            if (ACTION_IS_AFTER_MOVE(action)) {
                UWORD result = findHarvestableTileAround(unit->loc);
                if (result) {
                    // reached a harvest spot, start harvesting
                    if (action == ACTION_AFTER_MOVE(ActionHarvestMine)) {
                        unit->action.harvest.u4State = HarvestWaitInMine;
                        unit->action.harvest.ubWait = WAIT_IN_MINE;
                        unitSetOffMap(unit);
                    } else {
                        unit->action.harvest.u4State = HarvestWaitAtForest;
                        UWORD direction = result - unit->loc.uwYX;
                        // result is 1 tile away in x and/or y, so if we substract the packed locations,
                        // we get the direction with a single sub
                        switch (direction) {
                            case 0x0100:
                                // <x =y
                            case 0x00ff:
                                // <x >y
                                unit->action.harvest.u4Direction = DIRECTION_WEST;
                                break;
                            case 0xff00:
                                // >x =y
                            case 0xff01:
                                // >x <y
                                unit->action.harvest.u4Direction = DIRECTION_EAST;
                                break;
                            case 0x0001:
                                // =x <y
                            case 0x0101:
                                // <x <y
                                unit->action.harvest.u4Direction = DIRECTION_NORTH;
                                break;
                            case 0xfeff:
                                // >x >y
                            case 0xffff:
                                // ?x >y
                                unit->action.harvest.u4Direction = DIRECTION_SOUTH;
                                break;
                        }
                        unit->action.harvest.ubWait = 0;
                    }
                    return;
                }
                if (++unit->action.harvest.ubWait < RETRY_FINDING_HARVEST) {
                    // else we have found nothing to harvest here anymore, find a new spot nearby
                    unit->action.harvest.lastHarvestLocation = unit->loc;
                } else {
                    unit->action.action = ActionStill;
                    return;
                }
            }
            // we are not in the right location to harvest, find a spot and start moving
            UWORD loc = 0;
            if ((loc = findHarvestSpotNear(unit->action.harvest.lastHarvestLocation))) {
                unit->nextAction.action = ACTION_AFTER_MOVE(unit->action.action);
                unit->nextAction.harvest.u4State = HarvestMoveToHarvest;
                actionMoveToNow(unit, (tUbCoordYX){.uwYX = loc});
            } else {
                logWrite("Could not find tile to harvest from\n");
                unit->action.action = ActionStill;
            }
            return;
        }
        case HarvestWaitAtForest: {
            UWORD result = findHarvestableTileAround(unit->loc);
            if (!result) {
                // someone else removed our trees, find a new spot
                unit->action.action = ACTION_WITHOUT_MOVE(unit->action.action);
                unit->action.harvest.u4State = HarvestMoveToHarvest;
                return;
            }
            UBYTE wait = unit->action.harvest.ubWait;
            UnitType *type = &UnitTypes[unit->type];
            if (wait % ((type->anim.wait + 1) << 1) == 0) {
                // next attack frame, attack frames start after walk frames + 1 still frame
                UBYTE nextFrame = ((unitGetFrame(unit) / DIRECTIONS + 1) % type->anim.attack + type->anim.walk + 1) * DIRECTIONS;
                unitSetFrame(unit, nextFrame + unit->action.harvest.u4Direction);
            }
            if (++unit->action.harvest.ubWait < WAIT_AT_FOREST) {
                return;
            }
            mapDecGraphicTileAt((tUbCoordYX){.uwYX = result}.ubX, (tUbCoordYX){.uwYX = result}.ubY);

            unit->action.action = ACTION_WITHOUT_MOVE(unit->action.action);
            unit->action.harvest.u4State = HarvestMoveToDepot;
            return;
        }
        case HarvestWaitInMine: {
            if (--unit->action.harvest.ubWait) {
                return;
            }
            unitPlace(unit, unit->action.harvest.lastHarvestLocation);
            unit->action.action = ACTION_WITHOUT_MOVE(unit->action.action);
            unit->action.harvest.u4State = HarvestMoveToDepot;
            return;
        }
        case HarvestMoveToDepot: {
            if (ACTION_IS_AFTER_MOVE(action)) {
                if (isNextToDepot(unit->loc)) {
                    unitSetOffMap(unit);
                    unit->action.harvest.u4State = HarvestWaitAtDepot;
                    unit->action.harvest.ubWait = WAIT_AT_DEPOT;
                } else {
                    // just give up
                    unit->action.action = ActionStill;
                }
            } else {
                UWORD loc = 0;
                if ((loc = findNearestDepot(unit))) {
                    unit->nextAction.action = ACTION_AFTER_MOVE(unit->action.action);
                    unit->nextAction.harvest.u4State = HarvestMoveToDepot;
                    unit->nextAction.harvest.lastHarvestLocation = unit->loc;
                    actionMoveToNow(unit, (tUbCoordYX){.uwYX = loc});
                } else {
                    logWrite("Could not find depot\n");
                    unit->action.action = ActionStill;
                }
            }
            return;
        }
        case HarvestWaitAtDepot: {
            if (--unit->action.harvest.ubWait) {
                return;
            }
            unitPlace(unit, unit->nextAction.move.target);
            unit->action.action = ACTION_WITHOUT_MOVE(unit->action.action);
            unit->action.harvest.u4State = HarvestMoveToHarvest;
        }
    }
};

void actionCast(Unit  __attribute__((__unused__)) *unit) {
    return;
};

void actionDie(Unit  __attribute__((__unused__)) *unit) {
    return;
};

#define WAIT_AT_BUILDING 1
void actionBuild(Unit *unit) {
    switch (unit->action.build.ubState) {
        case BuildStateMoveToGoal:
            // no check for interrupted, since this happens on the next frame
            unit->nextAction.action = ACTION_AFTER_MOVE(unit->action.action);
            unit->nextAction.build.ubBuildingType = unit->action.build.ubBuildingType;
            unit->nextAction.build.ubState = BuildStateOpenConstructionSite;
            actionMoveToNow(unit, unit->action.build.target);
            return;
        case BuildStateOpenConstructionSite: {
            tUbCoordYX target = unit->nextAction.move.target;
            if (unit->loc.uwYX != target.uwYX) {
                // could not reach goal, abort
                logWrite("Could not reach goal");
                unit->action.action = ActionStill;
                return;
            }
            UBYTE typeIdx = unit->action.build.ubBuildingType;
            if (!buildingCanBeAt(typeIdx, target, 1)) {
                unit->action.action = ActionStill;
                logWrite("Cannot build here");
                return;
            }
            BuildingType *type = &BuildingTypes[typeIdx];
            UWORD gold = type->costs.gold;
            UWORD lumber = type->costs.wood;
            Player *owner = &g_pPlayers[unit->owner];
            if (owner->gold >= gold && owner->lumber >= lumber) {
                owner->gold -= gold;
                owner->lumber -= lumber;
            } else {
                logWrite("Not enough resources\n");
                return;
            }
            unitSetOffMap(unit);
            UBYTE newBuildingId = buildingNew(typeIdx, target, unit->owner);
            unit->action.build.ubState = BuildStateBuilding;
            unit->action.build.ubBuildingID = newBuildingId;
            unit->action.build.ubBuildingHPWait = WAIT_AT_BUILDING;
            unit->action.build.ubBuildingHPIncrease = type->costs.hpIncrease;
            return;
        }
        case BuildStateBuilding: {
            Building *building = &g_BuildingManager.building[unit->action.build.ubBuildingID];
            if (!building->hp) {
                // building was destroyed while building
                unit->action.action = ActionStill;
                // TODO: what if no room? unit lost?
                unitPlace(unit, building->loc);
            } else if (building->hp >= buildingTypeMaxHealth(&BuildingTypes[building->type])) {
                unit->action.action = ActionStill;
                // TODO: what if no room? unit lost?
                unitPlace(unit, building->loc);
            } else {
                if (--unit->action.build.ubBuildingHPWait) {
                    return;
                }
                unit->action.build.ubBuildingHPWait = WAIT_AT_BUILDING;
                building->hp += unit->action.build.ubBuildingHPIncrease;
            }
        }
    }
}

void actionDo(Unit *unit) {
    switch (ACTION_WITHOUT_MOVE(unit->action.action)) {
        case ActionStill:
            actionStill(unit);
            return;
        case ActionMove:
            actionMove(unit);
            return;
        case ActionStop:
            unit->action.action = ActionStill;
            return;
        case ActionHarvestTerrain:
        case ActionHarvestMine:
            actionHarvest(unit);
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
    UWORD maxHp = buildingTypeMaxHealth(type);
    if (building->hp >= maxHp) {
        building->hp = maxHp;
        building->action.action = ActionStill;
        if (g_Screen.m_pSelectedBuilding == building) {
            g_Screen.m_ubBottomPanelDirty = 1;
        }
        tUbCoordYX loc = building->loc;
        mapSetGraphicTileRangeSquare(loc.ubX, loc.ubY, type->size, type->tileIdx);
    }
}

void actionTrain(Building *building) {
    if (!building->action.train.uwTimeLeft) {
        // first time
        UnitType *type = &UnitTypes[building->action.train.u5UnitType1];
        UWORD gold = type->costs.gold;
        UWORD lumber = type->costs.lumber;
        UWORD time = type->costs.timeBase << type->costs.timeShift;
        Player *owner = &g_pPlayers[building->owner];
        if (owner->gold >= gold && owner->lumber >= lumber) {
            owner->gold -= gold;
            owner->lumber -= lumber;
        } else {
            logWrite("Not enough resources\n");
            return;
        }
        g_Screen.m_ubTopPanelDirty = 1;
        building->action.train.uwTimeLeft = time;
    }
    if (--building->action.train.uwTimeLeft > 1) {
        return;
    }
    Unit *unit = unitNew(building->action.train.u5UnitType1, building->owner);
    unitPlace(unit, building->loc);
    if (building->action.train.u5UnitType2) {
        building->action.train.u5UnitType1 = building->action.train.u5UnitType2;
        building->action.train.u5UnitType2 = building->action.train.u5UnitType3;
        building->action.train.u5UnitType3 = 0;
        building->action.train.uwTimeLeft = 0;
    } else {
        building->action.action = ActionStill;
    }
    if (g_Screen.m_pSelectedBuilding == building) {
        g_Screen.m_ubBottomPanelDirty = 1;
    }
}

void actionRemoveBuilding(Building *building) {
    if (!--building->action.die.ubTimeout) {
        buildingDestroy(building);
    }
}

void buildingDo(Building *building) {
    switch (building->action.action) {
        case ActionStill:
            return;
        case ActionBeingBuilt:
            actionBeingBuilt(building);
            return;
        case ActionTrain:
            actionTrain(building);
            return;
        case ActionDie:
            actionRemoveBuilding(building);
            return;
        default:
            return;
    }
}