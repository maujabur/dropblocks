#include "pieces/PieceManager.hpp"
#include "pieces/Piece.hpp"
#include "ConfigTypes.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <ctime>

// External data owned by main app
extern std::vector<Piece> PIECES;
extern std::string PIECES_FILE_PATH;

// Local state
static std::vector<int> g_bag;
static size_t g_bagPos = 0;
static std::mt19937 g_rng((unsigned)time(nullptr));
static int g_nextIdx = 0;
static int g_previewGrid = 6;
static RandType g_randomizerType = (RandType)0; // default to SIMPLE without including its definition here
static int g_randBagSize = 0;

PieceManager::PieceManager() {}

int PieceManager::getNextPiece() {
    if (g_bagPos >= g_bag.size()) {
        refillBag();
    }
    int piece = g_bag[g_bagPos++];
    return piece;
}

int PieceManager::getCurrentNextPiece() const { return g_nextIdx; }
void PieceManager::setNextPiece(int id) { g_nextIdx = id; }

void PieceManager::refillBag() {
    g_bag.clear();
    int n = (g_randBagSize > 0 ? g_randBagSize : (int)PIECES.size());
    n = std::min(n, (int)PIECES.size());
    for (int i = 0; i < n; i++) g_bag.push_back(i);
    std::shuffle(g_bag.begin(), g_bag.end(), g_rng);
    g_bagPos = 0;
}

void PieceManager::initialize() { refillBag(); g_nextIdx = getNextPiece(); }
void PieceManager::reset() { g_bagPos = 0; initialize(); }

std::mt19937& PieceManager::getRng() { return g_rng; }

int PieceManager::getPreviewGrid() const { return g_previewGrid; }
void PieceManager::setPreviewGrid(int grid) { g_previewGrid = grid; }
void PieceManager::setRandomizerType(RandType type) { g_randomizerType = type; }
void PieceManager::setRandBagSize(int size) { g_randBagSize = size; }
int PieceManager::getRandBagSize() const { return g_randBagSize; }
RandType PieceManager::getRandomizerType() const { return g_randomizerType; }

// Bridge helper implemented in main TU (dropblocks.cpp)
extern bool db_loadPiecesPath(const std::string& p);

bool PieceManager::loadPiecesFile() {
    // 1) Environment override
    if (const char* env = std::getenv("DROPBLOCKS_PIECES")) {
        if (db_loadPiecesPath(env)) return true;
    }
    // 2) Configured path
    if (!PIECES_FILE_PATH.empty()) {
        if (db_loadPiecesPath(PIECES_FILE_PATH)) return true;
    }
    // 3) Default file in CWD
    if (db_loadPiecesPath("default.pieces")) return true;
    // 4) User config dir
    if (const char* home = std::getenv("HOME")) {
        std::string p = std::string(home) + "/.config/default.pieces";
        if (db_loadPiecesPath(p)) return true;
    }
    return false;
}

void PieceManager::seedFallback() {
    SDL_Log("Usando fallback interno de peças.");
    PIECES.clear();
    auto rotate90 = [](std::vector<std::pair<int,int>>& pts){
        for (auto& p : pts) { int x = p.first, y = p.second; p.first = -y; p.second = x; }
    };
    auto mk = [&](const char* name, std::initializer_list<std::pair<int,int>> c, Uint8 r, Uint8 g, Uint8 b){
        Piece p; p.name = name; p.r=r; p.g=g; p.b=b; 
        for (auto& coord : c) p.rot[0].push_back(coord);
        // Build 4 rotations from base
        p.rot[1] = p.rot[0]; rotate90(p.rot[1]);
        p.rot[2] = p.rot[1]; rotate90(p.rot[2]);
        p.rot[3] = p.rot[2]; rotate90(p.rot[3]);
        return p; };

    // 7 tetrominós padrão (Guideline)
    PIECES.push_back(mk("I", {{0,0},{1,0},{2,0},{3,0}}, 80,120,220));
    PIECES.push_back(mk("O", {{0,0},{1,0},{0,1},{1,1}}, 220,180,80));
    PIECES.push_back(mk("T", {{0,0},{1,0},{2,0},{1,1}}, 160,80,220));
    PIECES.push_back(mk("S", {{1,0},{2,0},{0,1},{1,1}}, 80,220,80));
    PIECES.push_back(mk("Z", {{0,0},{1,0},{1,1},{2,1}}, 220,80,80));
    PIECES.push_back(mk("L", {{0,0},{0,1},{0,2},{1,2}}, 220,160,80));
    PIECES.push_back(mk("J", {{1,0},{1,1},{1,2},{0,2}}, 80,180,220));

    // SRS kicks oficiais (fallback) para JLSTZ
    auto setJLSTZ = [](Piece& p){
        p.hasPerTransKicks = true;
        // CW (dirIdx=0)
        p.kicksPerTrans[0][0] = {{0,0},{-1,0},{-1,1},{0,-2},{-1,-2}}; // 0->1
        p.kicksPerTrans[0][1] = {{0,0},{1,0},{1,-1},{0,2},{1,2}};     // 1->2
        p.kicksPerTrans[0][2] = {{0,0},{1,0},{1,1},{0,-2},{1,-2}};    // 2->3
        p.kicksPerTrans[0][3] = {{0,0},{-1,0},{-1,-1},{0,2},{-1,2}};  // 3->0
        // CCW (dirIdx=1)
        p.kicksPerTrans[1][0] = {{0,0},{1,0},{1,1},{0,-2},{1,-2}};    // 0->3
        p.kicksPerTrans[1][3] = {{0,0},{1,0},{1,-1},{0,2},{1,2}};     // 3->2
        p.kicksPerTrans[1][2] = {{0,0},{-1,0},{-1,1},{0,-2},{-1,-2}}; // 2->1
        p.kicksPerTrans[1][1] = {{0,0},{-1,0},{-1,-1},{0,2},{-1,2}};  // 1->0
    };

    // SRS kicks oficiais (fallback) para I
    auto setI = [](Piece& p){
        p.hasPerTransKicks = true;
        // CW (dirIdx=0)
        p.kicksPerTrans[0][0] = {{0,0},{-2,0},{1,0},{-2,-1},{1,2}};   // 0->1
        p.kicksPerTrans[0][1] = {{0,0},{-1,0},{2,0},{-1,2},{2,-1}};   // 1->2
        p.kicksPerTrans[0][2] = {{0,0},{2,0},{-1,0},{2,1},{-1,-2}};   // 2->3
        p.kicksPerTrans[0][3] = {{0,0},{1,0},{-2,0},{1,-2},{-2,1}};   // 3->0
        // CCW (dirIdx=1)
        p.kicksPerTrans[1][0] = {{0,0},{-1,0},{2,0},{-1,2},{2,-1}};   // 0->3
        p.kicksPerTrans[1][3] = {{0,0},{-2,0},{1,0},{-2,-1},{1,2}};   // 3->2
        p.kicksPerTrans[1][2] = {{0,0},{1,0},{-2,0},{1,-2},{-2,1}};   // 2->1
        p.kicksPerTrans[1][1] = {{0,0},{2,0},{-1,0},{2,1},{-1,-2}};   // 1->0
    };

    // Aplicar kicks aos respectivos tetrominós
    for (auto& p : PIECES) {
        if (p.name == "I") setI(p);
        else if (p.name == "O") { /* O não precisa; rotação não altera shape */ }
        else setJLSTZ(p);
    }
}

void PieceManager::initializeRandomizer() {
    // Default randomizer settings (SIMPLE, full bag)
    g_randomizerType = (RandType)0; // SIMPLE
    g_randBagSize = 0;
}


