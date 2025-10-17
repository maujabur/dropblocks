#pragma once
#include <vector>
#include "app/GameTypes.hpp"

class AudioSystem;

class GameBoard {
private:
    std::vector<std::vector<Cell>> grid_;
public:
    GameBoard() : grid_(ROWS, std::vector<Cell>(COLS)) {}
    const std::vector<std::vector<Cell>>& getGrid() const { return grid_; }
    std::vector<std::vector<Cell>>& getGrid() { return grid_; }
    bool canPlacePiece(const Active& piece, int dx, int dy, int drot) const;
    void placePiece(const Active& piece);
    int clearLines();
    bool isGameOver(const Active& piece) const;
    void reset();
    int getTensionLevel() const;
    void checkTension(AudioSystem& audio) const;
};


