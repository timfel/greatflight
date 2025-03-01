#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/state.h>

#include "include/main.h"
#include "include/game.h"
#include "include/menu.h"

tStateManager *g_pGameStateManager = 0;

tState g_pGameStates[] = {
    [STATE_MAIN_MENU] = {.cbCreate = menuGSCreate, .cbLoop = menuGSLoop, .cbDestroy = menuGSDestroy, .cbResume = menuGSCreate, .cbSuspend = menuGSDestroy},
    [STATE_INGAME] = {.cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy}
};

void genericCreate(void) {
  keyCreate();
  mouseCreate(MOUSE_PORT_1 | MOUSE_PORT_2);
  g_pGameStateManager = stateManagerCreate();
  statePush(g_pGameStateManager, &g_pGameStates[STATE_MAIN_MENU]);
}

void genericProcess(void) {
  keyProcess();
  mouseProcess();
  stateProcess(g_pGameStateManager);
}

void genericDestroy(void) {
  stateManagerDestroy(g_pGameStateManager);
  mouseDestroy();
  keyDestroy();
}
