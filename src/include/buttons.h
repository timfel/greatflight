#ifndef BUTTONS_H
#define BUTTONS_H

#include "include/icons.h"
#include "include/actions.h"

#define GET_BUTTON_POSITION(value) = (((UBYTE)(value)) & 0b111)
#define GET_BUTTON_PAGE(value) = ((((UBYTE)(value)) & 0b11100000) >> 5)
#define BUTTON_PAGE(X) (((UBYTE)(X)) << 5)

typedef struct __attribute__ ((__packed__)) {
    Action action;
    IconIdx iconIdx;
    UBYTE position;
} Button;
_Static_assert(sizeof(Button) == sizeof(UBYTE) * 7, "button is too large");

#define GROUP_BUTTON_COUNT 4
extern Button g_GroupButtons[GROUP_BUTTON_COUNT]; // we have 6 button slots only anyway
extern Button g_SingleButtons[ICON_MAX - GROUP_BUTTON_COUNT];

#endif
