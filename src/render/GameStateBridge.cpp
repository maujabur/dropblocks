#include "render/GameStateBridge.hpp"
#include "app/GameState.hpp"
#include "pieces/PieceManager.hpp"
#include "DebugLogger.hpp"
#include <fstream>

// External globals
extern VisualEffectsView g_visualView;
extern bool pm_loadPiecesFromStream(std::istream&);

// ============================================================================
// GameState Bridge Functions
// ============================================================================
// These functions provide safe access to GameState internals for rendering
// and other modules without exposing the full GameState implementation.

bool db_getBoardSize(const GameState& state, int& rows, int& cols) {
    const auto& g = state.getBoard().getGrid();
    rows = (int)g.size();
    cols = rows ? (int)g[0].size() : 0;
    return rows > 0 && cols > 0;
}

bool db_getBoardCell(const GameState& state, int x, int y, Uint8& r, Uint8& g, Uint8& b, bool& occ) {
    const auto& gr = state.getBoard().getGrid();
    if (y < 0 || y >= (int)gr.size() || x < 0 || x >= (int)gr[y].size()) return false;
    const auto& c = gr[y][x];
    r = c.r; g = c.g; b = c.b; occ = c.occ;
    return true;
}

bool db_getActive(const GameState& state, int& idx, int& rot, int& x, int& y) {
    const Active& a = state.getActivePiece();
    idx = a.idx; rot = a.rot; x = a.x; y = a.y;
    return true;
}

bool db_getNextIdx(const GameState& state, int& nextIdx) {
    nextIdx = state.getNextIdx();
    return true;
}

bool db_isPaused(const GameState& state) {
    return state.isPaused();
}

bool db_isGameOver(const GameState& state) {
    return state.isGameOver();
}

int db_getScore(const GameState& state) {
    return state.getScoreValue();
}

int db_getLines(const GameState& state) {
    return state.getLinesValue();
}

int db_getLevel(const GameState& state) {
    return state.getLevelValue();
}

bool db_getPieceStats(const GameState& state, const std::vector<int>*& stats) {
    stats = &state.getPieceStats();
    return true;
}

bool db_isRunning(const GameState& state) {
    return state.isRunning();
}

void db_update(GameState& state, SDL_Renderer* renderer) {
    state.update(renderer);
}

void db_render(GameState& state, RenderManager& renderManager, const LayoutCache& layout) {
    state.render(renderManager, layout);
}

const VisualEffectsView& db_getVisualEffects() {
    return g_visualView;
}

// ============================================================================
// Piece Loading Bridge Functions
// ============================================================================

bool db_loadPiecesPath(const std::string& p) {
    std::ifstream f(p.c_str());
    if (!f.good()) {
        return false;
    }
    bool ok = pm_loadPiecesFromStream(f);
    DebugLogger::info(std::string("Pieces carregado de: ") + p + " (" + (ok ? "OK" : "vazio/erro") + ")");
    return ok;
}

