#include "include/actions.h"
#include "include/units.h"
#include "include/buildings.h"
#include "include/player.h"
#include "include/utils.h"
#include "include/game.h"

#include <ace/utils/custom.h>
#include <ace/managers/log.h>

#define WAIT_AT_FOREST 255
#define WAIT_IN_MINE 25 * 4
#define WAIT_AT_DEPOT 25 * 3
#define RETRY_FINDING_HARVEST 200
#define CARRIED_RESOURCE_AMOUNT 100
#define HARVEST_SEARCH_RANGE 8
#define WAIT_AT_BUILDING 1
#define WAIT_UNTIL_FOLLOW 25
#define RETRY_PATHING 255
#define RETRY_FINDING_HARVEST 200

void actionRepair(Unit *unit, Building *target) {
    unit->nextAction.repair.ubBuildingId = target->id;
    unit->nextAction.repair.ubWait = 0;
    unit->nextAction.action = ActionRepair;
}

void actionAttackUnit(Unit *unit, Unit *target) {
    unit->nextAction.attack.ubUnitId = target->id;
    unit->nextAction.attack.ubWait = 0;
    unit->nextAction.action = ActionAttackUnit;
}

void actionAttackBuilding(Unit *unit, Building *target) {
    unit->nextAction.attack.ubUnitId = target->id;
    unit->nextAction.attack.ubWait = 0;
    unit->nextAction.action = ActionAttackBuilding;
}

void actionFollow(Unit *unit, Unit *target) {
    unit->nextAction.follow.ubUnitId = target->id;
    unit->nextAction.follow.ubWait = WAIT_UNTIL_FOLLOW;
    unit->nextAction.action = ActionFollow;
}

void actionMoveTo(Unit *unit, tUbCoordYX goal) {
    unit->nextAction.move.target = goal;
    unit->nextAction.move.u4Wait = 0;
    unit->nextAction.move.ubRetries = RETRY_PATHING;
    unit->nextAction.action = ActionMove;
}

void actionMoveToNow(Unit *unit, tUbCoordYX goal) {
    unit->action.move.target = goal;
    unit->action.move.u4Wait = 0;
    unit->action.move.ubRetries = 1;
    unit->action.action = ActionMove;
}

enum __attribute__((__packed__)) HarvestState {
    HarvestMoveToForest,
    HarvestMoveToMine,
    HarvestWaitAtForest,
    HarvestWaitInMine,
    HarvestMoveToDepot,
    HarvestWaitAtDepot,
};

void actionHarvestAt(Unit *unit, tUbCoordYX goal) {
    unit->nextAction.harvest.ubWait = 0;
    if (mapIsHarvestable(goal.ubX, goal.ubY)) {
        actionHarvestTile(unit, goal);
        return;
    }
    Building *building = buildingManagerBuildingAt(goal);
    if (building && building->type == BUILDING_GOLD_MINE) {
        actionHarvestMine(unit, building);
        return;
    }
    logMessage(MSG_NO_HARVEST);
}

void actionHarvestTile(Unit *unit, tUbCoordYX goal) {
    unit->nextAction.harvest.u4State = HarvestMoveToForest;
    unit->nextAction.action = ActionHarvestTerrain;
    unit->nextAction.harvest.ubWait = RETRY_FINDING_HARVEST;
    unit->nextAction.harvest.lastHarvestLocation = goal;
}

void actionHarvestMine(Unit *unit, Building *mine) {
    unit->nextAction.harvest.u4State = HarvestMoveToMine;
    unit->nextAction.action = ActionHarvestMine;
    unit->nextAction.harvest.ubWait = RETRY_FINDING_HARVEST;
    unit->nextAction.harvest.lastMineId = mine->id;
}

void actionAttackAt(Unit *self, tUbCoordYX goal) {
    Unit *unit = unitManagerUnitAt(goal);
    if (unit && unit != self) {
        actionAttackUnit(self, unit);
        return;
    }
    Building *building = buildingManagerBuildingAt(goal);
    if (building) {
        actionAttackBuilding(self, building);
        return;
    }
    unit->nextAction.attack.goal = goal;
    unit->nextAction.attack.ubWait = 0;
    unit->nextAction.action = ActionAttackMove;
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
        if (vectorX && vectorY) {
            vectorX = vectorX > 0 ? 1 : -1;
            vectorY = vectorY > 0 ? 1 : -1;
            if (!mapIsWalkable(tilePos.ubX + vectorX, tilePos.ubY + vectorY)) {
                if (mapIsWalkable(tilePos.ubX + vectorX, tilePos.ubY)) {
                    vectorY = 0;
                } else if (mapIsWalkable(tilePos.ubX, tilePos.ubY + vectorY)) {
                    vectorX = 0;
                } else {
                    vectorX = vectorY = 0;
                }
            }
        } else if (vectorX) {
            vectorX = vectorX > 0 ? 1 : -1;
            if (!mapIsWalkable(tilePos.ubX + vectorX, tilePos.ubY)) {
                // unwalkable tile horizontal
                vectorX = 0;
            }
        } else if (vectorY) {
            vectorY = vectorY > 0 ? 1 : -1;
            if (!mapIsWalkable(tilePos.ubX + vectorX, tilePos.ubY + vectorY)) {
                // unwalkable tile vertical
                vectorY = 0;
            }
        }
        if (!vectorX && !vectorY) {
            // unreachable step, wait a little?
            unitSetFrame(unit, 0);
            if (--unit->action.move.ubRetries == 0) {
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
        mapUnmarkUnitSight(tilePos.ubX, tilePos.ubY, SIGHT_MEDIUM);
        if (vectorX) {
            unit->loc.ubX += vectorX;
            unit->IX = -vectorX * (PATHMAP_TILE_SIZE - speed);
        }
        if (vectorY) {
            unit->loc.ubY += vectorY;
            unit->IY = -vectorY * (PATHMAP_TILE_SIZE - speed);
        }
        tilePos = unitGetTilePosition(unit);
        mapMarkTileOccupied(unit->id, unit->owner, tilePos.ubX, tilePos.ubY);
        if (unit->owner == g_ubThisPlayer) {
            mapMarkUnitSight(tilePos.ubX, tilePos.ubY, SIGHT_MEDIUM);
        }
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

static inline UBYTE isNextToBuilding(tUbCoordYX a, BuildingTypeIndex type) {
    for (UBYTE x = a.ubX - 1; x < a.ubX + 2; ++x) {
        for (UBYTE y = a.ubY - 1; y < a.ubY + 2; ++y) {
            UBYTE id = mapGetBuildingAt(x, y);
            if ((BYTE)id != -1 && g_BuildingManager.building[id].type == type) {
                return 1;
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
        case HarvestMoveToMine: {
            Building *mine = &g_BuildingManager.building[unit->action.harvest.lastMineId];
            if (ACTION_IS_AFTER_MOVE(action)) {
                if (!mine->hp || mine->type != BUILDING_GOLD_MINE) {
                    // mine is dead
                    unit->action.action = ActionStill;
                    return;
                }
                if (isNextToBuilding(unit->loc, BUILDING_GOLD_MINE)) {
                    unit->action.harvest.u4State = HarvestWaitInMine;
                    unit->action.harvest.ubWait = WAIT_IN_MINE;
                    unitSetOffMap(unit);
                    return;
                } else if (--unit->action.harvest.ubWait == 0) {
                    unit->action.action = ActionStill;
                    return;
                }
            }
            unit->nextAction.action = ACTION_AFTER_MOVE(action);
            unit->nextAction.harvest.u4State = HarvestMoveToMine;
            actionMoveToNow(unit, mine->loc);
            return;
        }
        case HarvestMoveToForest: {
            if (ACTION_IS_AFTER_MOVE(action)) {
                UWORD result = findHarvestableTileAround(unit->loc);
                if (result) {
                    // reached a harvest spot, start harvesting
                    unit->action.harvest.u4State = HarvestWaitAtForest;
                    unit->action.harvest.ubWait = WAIT_AT_FOREST;
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
                    return;
                }
                if (--unit->action.harvest.ubWait == 0) {
                    unit->action.action = ActionStill;
                    return;
                } else {
                    // else we have found nothing to harvest here (anymore?), find a new spot nearby
                    // unit->action.harvest.lastHarvestLocation = unit->loc;
                }
            }
            // we are not in the right location to harvest, find a spot and start moving
            UWORD loc = 0;
            if ((loc = findHarvestSpotNear(unit->action.harvest.lastHarvestLocation))) {
                unit->nextAction.action = ACTION_AFTER_MOVE(action);
                unit->nextAction.harvest.u4State = HarvestMoveToForest;
                actionMoveToNow(unit, (tUbCoordYX){.uwYX = loc});
            } else {
                logMessage(MSG_NO_MORE_TREES);
                unit->action.action = ActionStill;
            }
            return;
        }
        case HarvestWaitAtForest: {
            UWORD result = findHarvestableTileAround(unit->loc);
            if (!result) {
                // someone else removed our trees, find a new spot
                unit->action.action = ACTION_WITHOUT_MOVE(action);
                unit->action.harvest.u4State = HarvestMoveToForest;
                return;
            }
            UBYTE wait = unit->action.harvest.ubWait;
            UnitType *type = &UnitTypes[unit->type];
            if (wait % ((type->anim.wait + 1) << 1) == 0) {
                // next attack frame, attack frames start after walk frames + 1 still frame
                UBYTE nextFrame = ((unitGetFrame(unit) / DIRECTIONS + 1) % type->anim.attack + type->anim.walk + 1) * DIRECTIONS;
                unitSetFrame(unit, nextFrame + unit->action.harvest.u4Direction);
            }
            if (--unit->action.harvest.ubWait) {
                return;
            }
            mapDecGraphicTileAt((tUbCoordYX){.uwYX = result}.ubX, (tUbCoordYX){.uwYX = result}.ubY);

            unit->action.action = ACTION_WITHOUT_MOVE(action);
            unit->action.harvest.u4State = HarvestMoveToDepot;
            unit->action.harvest.ubWait = RETRY_FINDING_HARVEST;
            return;
        }
        case HarvestWaitInMine: {
            if (--unit->action.harvest.ubWait) {
                return;
            }
            unitPlace(unit, g_BuildingManager.building[unit->action.harvest.lastMineId].loc);
            unit->action.action = ACTION_WITHOUT_MOVE(action);
            unit->action.harvest.u4State = HarvestMoveToDepot;
            unit->action.harvest.ubWait = RETRY_FINDING_HARVEST;
            return;
        }
        case HarvestMoveToDepot: {
            if (ACTION_IS_AFTER_MOVE(action)) {
                // XXX: depot types
                if (isNextToBuilding(unit->loc, BUILDING_HUMAN_TOWNHALL)) {
                    unitSetOffMap(unit);
                    unit->action.harvest.u4State = HarvestWaitAtDepot;
                    unit->action.harvest.ubWait = WAIT_AT_DEPOT;
                    return;
                } else if (--unit->action.harvest.ubWait == 0) {
                    // just give up
                    unit->action.action = ActionStill;
                    return;
                }
            }
            UWORD loc = 0;
            if ((loc = findNearestDepot(unit))) {
                unit->nextAction.action = ACTION_AFTER_MOVE(action);
                unit->nextAction.harvest.u4State = HarvestMoveToDepot;
                if (ACTION_WITHOUT_MOVE(action) == ActionHarvestTerrain) {
                    unit->nextAction.harvest.lastHarvestLocation = unit->loc;
                }
                actionMoveToNow(unit, (tUbCoordYX){.uwYX = loc});
            } else {
                logMessage(MSG_NO_DEPOT);
                unit->action.action = ActionStill;
            }
            return;
        }
        case HarvestWaitAtDepot: {
            if (--unit->action.harvest.ubWait) {
                return;
            }
            if (ACTION_WITHOUT_MOVE(action) == ActionHarvestTerrain) {
                g_pPlayers[unit->owner].lumber += CARRIED_RESOURCE_AMOUNT;
            } else {
                g_pPlayers[unit->owner].gold += CARRIED_RESOURCE_AMOUNT;
            }
            unitPlace(unit, unit->nextAction.move.target);
            unit->action.action = ACTION_WITHOUT_MOVE(action);
            if (ACTION_WITHOUT_MOVE(action) == ActionHarvestTerrain) {
                unit->action.harvest.u4State = HarvestMoveToForest;
            } else {
                unit->action.harvest.u4State = HarvestMoveToMine;
            }
            unit->action.harvest.ubWait = RETRY_FINDING_HARVEST;
        }
    }
};

void actionCast(Unit  __attribute__((__unused__)) *unit) {
    return;
};

void actionDie(Unit  __attribute__((__unused__)) *unit) {
    return;
};

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
                logMessage(MSG_CANNOT_REACH_GOAL);
                unit->action.action = ActionStill;
                return;
            }
            UBYTE typeIdx = unit->action.build.ubBuildingType;
            if (!buildingCanBeAt(typeIdx, target, 1)) {
                unit->action.action = ActionStill;
                logMessage(MSG_CANNOT_BUILD_HERE);
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
                logMessage(MSG_NOT_ENOUGH_RESOURCES);
                unit->action.action = ActionStill;
                return;
            }
            unitSetOffMap(unit);
            Building *b = buildingNew(typeIdx, target, unit->owner);
            if (!b) {
                logMessage(MSG_TOO_MANY_BUILDINGS);
                unit->action.action = ActionStill;
                return;
            }
            unit->action.build.ubState = BuildStateBuilding;
            unit->action.build.ubBuildingID = b->id;
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

static void _actionFollow(Unit *self) {
    tUbCoordYX loc = self->loc;
    tUbCoordYX targetLoc = unitById(self->action.follow.ubUnitId)->loc;
    if (fast2dDistance(loc, targetLoc) < 3) {
        self->action.follow.ubWait = WAIT_UNTIL_FOLLOW;
    } else if (--self->action.follow.ubWait) {
        // wait
    } else {
        self->nextAction.action = ACTION_AFTER_MOVE(self->action.action);
        self->nextAction.follow.ubUnitId = self->action.follow.ubUnitId;
        self->nextAction.follow.ubWait = 1;
        actionMoveToNow(self, targetLoc);
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
        case ActionFollow:
            _actionFollow(unit);
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
        buildingFinishBuilding(building);
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
            logMessage(MSG_NOT_ENOUGH_RESOURCES);
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