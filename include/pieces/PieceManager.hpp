#pragma once

#include <random>
#include "Interfaces.hpp"

/**
 * @brief Piece randomization algorithm types
 */
enum class RandType { 
    SIMPLE,  /**< Simple random selection */
    BAG      /**< Bag-based randomizer (7-bag system) */
};

class PieceManager : public IPieceManager {
public:
    PieceManager();

    // Piece generation
    int getNextPiece() override;
    int getCurrentNextPiece() const override;
    void setNextPiece(int id) override;

    // Lifecycle
    void initialize() override;
    void reset() override;
    void initializeRandomizer() override;

    // Configuration
    int getPreviewGrid() const override;
    void setPreviewGrid(int grid) override;
    void setRandomizerType(RandType type) override;
    void setRandBagSize(int size) override;

    // Additional API
    int getRandBagSize() const;
    RandType getRandomizerType() const;
    std::mt19937& getRng();
    bool loadPiecesFile();
    void seedFallback();

private:
    void refillBag();
};


