#include "app/GameHelpers.hpp"

extern const int COLS;

void newActive(Active& a, int idx) {
    a.idx = idx;
    a.rot = 0;
    a.x = COLS / 2;
    a.y = 0;
}

