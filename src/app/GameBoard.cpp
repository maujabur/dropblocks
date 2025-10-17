#include "app/GameBoard.hpp"
#include "audio/AudioSystem.hpp"
#include "game/Mechanics.hpp"
#include <algorithm>

// now provided by Mechanics.hpp/cpp

bool GameBoard::canPlacePiece(const Active& piece, int dx, int dy, int drot) const { return !collides(piece, grid_, dx, dy, drot); }
void GameBoard::placePiece(const Active& piece) { lockPiece(piece, grid_); }

int GameBoard::clearLines() {
    int linesCleared = 0;
    for (int y = ROWS - 1; y >= 0; y--) {
        bool fullLine = true;
        for (int x = 0; x < COLS; x++) if (!grid_[y][x].occ) { fullLine = false; break; }
        if (fullLine) { grid_.erase(grid_.begin() + y); grid_.insert(grid_.begin(), std::vector<Cell>(COLS)); linesCleared++; y++; }
    }
    return linesCleared;
}

bool GameBoard::isGameOver(const Active& piece) const { return collides(piece, grid_, 0, 0, 0); }
void GameBoard::reset() { for (auto& row : grid_) for (auto& cell : row) cell.occ = false; }

int GameBoard::getTensionLevel() const {
    int filledRows = 0; for (int y = ROWS - 5; y < ROWS; y++) { bool hasBlocks = false; for (int x = 0; x < COLS; x++) if (grid_[y][x].occ) { hasBlocks = true; break; } if (hasBlocks) filledRows++; }
    return filledRows;
}

void GameBoard::checkTension(AudioSystem& audio) const { audio.playTensionSound(getTensionLevel()); }


