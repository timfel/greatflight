#include "include/buttons.h"
#include "include/buildings.h"

Button g_GroupButtons[GROUP_BUTTON_COUNT] = {
    {
        .action = {
            .action = ActionMove,
            .ulActionData = 0,
        },
        .iconIdx = ICON_MOVE,
        .position = 0,
    },
    {
        .action = {
            .action = ActionStop,
            .ulActionData = 0,
        },
        .iconIdx = ICON_STOP,
        .position = 1,
    },
    {
        .action = {
            .action = ActionAttackMove,
            .ulActionData = 0,
        },
        .iconIdx = ICON_ATTACK,
        .position = 2,
    },
    {
        .action = {
            .action = ActionHarvest,
            .ulActionData = 0,
        },
        .iconIdx = ICON_HARVEST,
        .position = 2,
    },
};

Button g_SingleButtons[ICON_MAX - GROUP_BUTTON_COUNT] = {
    {
        .action = {
            .action = ActionChangeButtonPage,
            .ubActionDataA = 1,
        },
        .iconIdx = ICON_BUILD_BASIC,
        .position = 3,
    },
    {
        .action = {
            .action = ActionBuild,
            .ubActionDataA = BUILDING_HUMAN_FARM,
        },
        .iconIdx = ICON_HFARM,
        .position = 0 | BUTTON_PAGE(1),
    },
    {
        .action = {
            .action = ActionBuild,
            .ubActionDataA = BUILDING_HUMAN_BARRACKS,
        },
        .iconIdx = ICON_HBARRACKS,
        .position = 1 | BUTTON_PAGE(1),
    },
    {
        .action = {
            .action = ActionBuild,
            .ubActionDataA = BUILDING_HUMAN_LUMBERMILL,
        },
        .iconIdx = ICON_HLUMBERMILL,
        .position = 2 | BUTTON_PAGE(1),
    },
    {
        .action = {
            .action = ActionBuild,
            .ubActionDataA = BUILDING_HUMAN_SMITHY,
        },
        .iconIdx = ICON_HSMITHY,
        .position = 3 | BUTTON_PAGE(1),
    },
    {
        .action = {
            .action = ActionBuild,
            .ubActionDataA = BUILDING_HUMAN_TOWNHALL,
        },
        .iconIdx = ICON_HHALL,
        .position = 4 | BUTTON_PAGE(1),
    },
    {
        .action = {
            .action = ActionBuild,
            .ubActionDataA = BUILDING_HUMAN_TOWNHALL,
        },
        .iconIdx = ICON_HHALL,
        .position = 4 | BUTTON_PAGE(1),
    },
    {
        .action = {
            .action = ActionChangeButtonPage,
            .ubActionDataA = -1,
        },
        .iconIdx = ICON_HHALL,
        .position = 5 | BUTTON_PAGE(1),
    },
};
