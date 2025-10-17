#pragma once
#include <vector>
#include "app/GameTypes.hpp"

bool collides(const Active& piece, const std::vector<std::vector<Cell>>& grid, int dx, int dy, int drot);
void lockPiece(const Active& piece, std::vector<std::vector<Cell>>& grid);
class AudioSystem;
void rotateWithKicks(Active& act, const std::vector<std::vector<Cell>>& grid, int dir, AudioSystem& audio);


