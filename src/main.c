#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/state.h>

#include "include/main.h"
#include "include/game.h"
#include "include/menu.h"

tStateManager *g_pGameStateManager = 0;
tState *g_pGameState = 0;
tState *g_pMenuState = 0;

void genericCreate(void) {
  keyCreate();
  mouseCreate(MOUSE_PORT_1 | MOUSE_PORT_2);

  g_pGameStateManager = stateManagerCreate();

  g_pMenuState = stateCreate(menuGSCreate, menuGSLoop, menuGSDestroy, menuGSSuspend, menuGSResume);
  g_pGameState = stateCreate(gameGsCreate, gameGsLoop, gameGsDestroy, 0, 0);

  statePush(g_pGameStateManager, g_pMenuState);
}

void genericProcess(void) {
  keyProcess();
  mouseProcess();
  stateProcess(g_pGameStateManager); // Process current gamestate's loop
}

void genericDestroy(void) {
  stateManagerDestroy(g_pGameStateManager);
  stateDestroy(g_pGameState);
  keyDestroy();
}
