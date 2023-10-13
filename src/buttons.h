#ifndef BUTTONS_H
#define BUTTONS_H

#include "icons.h"
#include "include/actions.h"

typedef struct __attribute__ ((__packed__)) {
    Action action;
    IconIdx iconIdx;
} Button;
_Static_assert(sizeof(Button) == sizeof(UWORD), "button is not 2 bytes");

extern Button g_GroupButtons[8]; // 8 bits, but we have 6 button slots only anyway
extern Button g_SingleButtons[ICON_MAX - 8];

#endif
