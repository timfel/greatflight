#include "include/utils.h"
#include "ace/macros.h"

UBYTE fast2dDistance(tUbCoordYX a, tUbCoordYX b) {
    // assumes that actual values are maximum 64
    BYTE x = ABS((BYTE)a.ubX - (BYTE)b.ubX);
    BYTE y = ABS((BYTE)a.ubY - (BYTE)b.ubY);
    // this function computes the distance from 0,0 to x,y with 3.5% error
    // found here: https://www.reddit.com/r/askmath/comments/ekzurn/understanding_a_fast_distance_calculation/
    int mn = MIN(x, y);
    return (x + y - (mn / (2 << 1)) - (mn / (2 << 2)) + (mn / (2 << 4)));
}
