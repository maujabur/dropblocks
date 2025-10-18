#pragma once

#include <SDL2/SDL.h>
#include <vector>

class GameState;
class RenderManager;
struct LayoutCache;
struct VisualEffectsView {
    bool bannerSweep;
    bool globalSweep;
    float sweepSpeedPxps;
    int sweepBandHS;
    int sweepAlphaMax;
    float sweepSoftness;
    float sweepGSpeedPxps;
    int sweepGBandHPx;
    int sweepGAlphaMax;
    float sweepGSoftness;
    int scanlineAlpha;
};

// Read-only bridge to query GameState without exposing its internals
bool db_getBoardSize(const GameState& state, int& rows, int& cols);
bool db_getBoardCell(const GameState& state, int x, int y, Uint8& r, Uint8& g, Uint8& b, bool& occ);
bool db_getActive(const GameState& state, int& idx, int& rot, int& x, int& y);
bool db_getNextIdx(const GameState& state, int& nextIdx);
bool db_isPaused(const GameState& state);
bool db_isGameOver(const GameState& state);
int db_getScore(const GameState& state);
int db_getLines(const GameState& state);
int db_getLevel(const GameState& state);
bool db_getPieceStats(const GameState& state, const std::vector<int>*& stats);

// Loop/control bridge
bool db_isRunning(const GameState& state);
void db_update(GameState& state, SDL_Renderer* renderer);
void db_render(GameState& state, RenderManager& renderManager, const LayoutCache& layout);
void db_layoutCalculate(LayoutCache& layout);
// Visual effects bridge (read-only snapshot)
const VisualEffectsView& db_getVisualEffects();


