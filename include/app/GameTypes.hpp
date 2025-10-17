#pragma once
#include <SDL2/SDL.h>

// Forward-declared board size constants provided by the main build
extern const int COLS;
extern const int ROWS;

struct Cell { Uint8 r, g, b; bool occ = false; };
struct Active { int x, y, rot, idx; };


