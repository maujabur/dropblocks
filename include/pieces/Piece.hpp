#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <array>
#include <utility>

struct Piece {
    std::string name;
    std::vector<std::vector<std::pair<int,int>>> rot; // 0..3
    Uint8 r = 200, g = 200, b = 200;

    Piece() : name(), rot(4), r(200), g(200), b(200) {}

    // SRS per-transition kicks
    std::array<std::array<std::vector<std::pair<int,int>>,4>,2> kicksPerTrans;
    bool hasPerTransKicks = false;

    // Legacy (fallback compatible)
    std::vector<std::pair<int,int>> kicksCW;
    std::vector<std::pair<int,int>> kicksCCW;
    bool hasKicks = false;
};


