#include "pieces/PieceManager.hpp"
#include "pieces/Piece.hpp"
#include "ConfigTypes.hpp"
#include <SDL2/SDL.h>
#include <algorithm>
#include <ctime>
#include <string>
#include <istream>
#include <cctype>

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

// ---------- Internal parsing helpers (migrated from dropblocks.cpp) ----------
static bool pm_parseHexColor(const std::string& s, Uint8& r, Uint8& g, Uint8& b){
    std::string color = s;
    if (color.size() == 6 && color[0] != '#') color = "#" + color;
    if (color.size()!=7 || color[0]!='#') return false;
    auto cv=[&](char c)->int{ if(c>='0'&&c<='9') return c-'0'; c=(char)std::toupper((unsigned char)c); if(c>='A'&&c<='F') return 10+(c-'A'); return -1; };
    auto hx=[&](char a,char b){int A=cv(a),B=cv(b); return (A<0||B<0)?-1:(A*16+B);} ;
    int R=hx(color[1],color[2]), G=hx(color[3],color[4]), B=hx(color[5],color[6]);
    if(R<0||G<0||B<0) return false; r=(Uint8)R; g=(Uint8)G; b=(Uint8)B; return true;
}

static bool pm_parseInt(const std::string& s, int& out){
    char* e=nullptr; long v=strtol(s.c_str(), &e, 10);
    if(e==s.c_str()||*e!='\0') return false; out=(int)v; return true;
}

static bool pm_parseCoordList(const std::string& val, std::vector<std::pair<int,int>>& out){
    out.clear(); size_t pos = 0;
    while(pos < val.size()){
        while(pos < val.size() && (val[pos] == ' ' || val[pos] == '\t')) pos++;
        if(pos >= val.size()) break; if(val[pos] != '('){ pos++; continue; } pos++;
        int x = 0, y = 0, sign = 1; if(pos < val.size() && val[pos] == '-') { sign=-1; pos++; } else if(pos<val.size() && val[pos]=='+'){ pos++; }
        while(pos < val.size() && std::isdigit((unsigned char)val[pos])){ x = x*10 + (val[pos]-'0'); pos++; }
        x *= sign; if(pos < val.size() && val[pos] == ',') pos++;
        sign = 1; if(pos < val.size() && val[pos] == '-') { sign=-1; pos++; } else if(pos<val.size() && val[pos]=='+'){ pos++; }
        while(pos < val.size() && std::isdigit((unsigned char)val[pos])){ y = y*10 + (val[pos]-'0'); pos++; }
        y *= sign; if(pos < val.size() && val[pos] == ')') pos++;
        out.push_back({x, y}); if(pos < val.size() && val[pos] == ';') pos++;
    }
    return !out.empty();
}

static bool pm_parseKicks(const std::string& v, std::vector<std::pair<int,int>>& out){ return pm_parseCoordList(v,out); }
static void pm_rotate90(std::vector<std::pair<int,int>>& pts){ for(auto& p:pts){ int x=p.first,y=p.second; p.first=-y; p.second=x; } }

static std::string pm_parsePiecesLine(const std::string& line) {
    size_t semi = line.find(';'); size_t cut = std::string::npos;
    if (semi != std::string::npos) {
        if (semi == 0 || (semi > 0 && line[semi-1] == ' ')) { cut = semi; }
        else {
            size_t eq_probe = line.find('='); if (eq_probe != std::string::npos && semi > eq_probe) {
                size_t paren_after_semi = line.find('(', semi);
                if (paren_after_semi == std::string::npos) { cut = semi; }
            }
        }
    }
    if (cut != std::string::npos) { std::string result = line; result.resize(cut); return result; }
    return line;
}

static void pm_buildPieceRotations(Piece& piece, const std::vector<std::pair<int,int>>& base,
                                   const std::vector<std::pair<int,int>>& rot0,
                                   const std::vector<std::pair<int,int>>& rot1,
                                   const std::vector<std::pair<int,int>>& rot2,
                                   const std::vector<std::pair<int,int>>& rot3,
                                   bool rotExplicit) {
    piece.rot.clear();
    if (rotExplicit) {
        if (!rot0.empty()) {
            piece.rot.push_back(rot0);
            piece.rot.push_back(rot1.empty() ? rot0 : rot1);
            piece.rot.push_back(rot2.empty() ? rot0 : rot2);
            piece.rot.push_back(rot3.empty() ? (rot1.empty() ? rot0 : rot1) : rot3);
        }
    } else {
        if (!base.empty()) {
            std::vector<std::pair<int,int>> r0 = base, r1 = base, r2 = base, r3 = base;
            pm_rotate90(r1); r2 = r1; pm_rotate90(r2); r3 = r2; pm_rotate90(r3);
            piece.rot.push_back(r0); piece.rot.push_back(r1); piece.rot.push_back(r2); piece.rot.push_back(r3);
        }
    }
}

static bool pm_processPieceProperty(Piece& cur, const std::string& key, const std::string& val,
                                    std::vector<std::pair<int,int>>& base,
                                    std::vector<std::pair<int,int>>& rot0,
                                    std::vector<std::pair<int,int>>& rot1,
                                    std::vector<std::pair<int,int>>& rot2,
                                    std::vector<std::pair<int,int>>& rot3,
                                    bool& rotExplicit) {
    if (key == "COLOR") { Uint8 r, g, b; if (pm_parseHexColor(val, r, g, b)) { cur.r=r; cur.g=g; cur.b=b; } return true; }
    if (key == "ROTATIONS") { std::string vv = val; for (char& c : vv) c = (char)std::tolower((unsigned char)c); rotExplicit = (vv == "explicit"); return true; }
    if (key == "BASE") { pm_parseCoordList(val, base); return true; }
    if (key == "ROT0") { if (val.rfind("sameas:", 0) == 0) { /* keep rot0 */ } else pm_parseCoordList(val, rot0); rotExplicit = true; return true; }
    if (key == "ROT1") { if (val.rfind("sameas:", 0) == 0) { rot1 = rot0; } else pm_parseCoordList(val, rot1); rotExplicit = true; return true; }
    if (key == "ROT2") { if (val.rfind("sameas:", 0) == 0) { rot2 = rot0; } else pm_parseCoordList(val, rot2); rotExplicit = true; return true; }
    if (key == "ROT3") { if (val.rfind("sameas:", 0) == 0) { rot3 = rot1.empty() ? rot0 : rot1; } else pm_parseCoordList(val, rot3); rotExplicit = true; return true; }
    if (key == "KICKS.CW") { pm_parseKicks(val, cur.kicksCW); cur.hasKicks = true; return true; }
    if (key == "KICKS.CCW") { pm_parseKicks(val, cur.kicksCCW); cur.hasKicks = true; return true; }
    auto setKPT = [&](int dirIdx, int fromState, const std::string& v) {
        std::vector<std::pair<int,int>> tmp; if (pm_parseCoordList(v, tmp)) { cur.kicksPerTrans[dirIdx][fromState] = tmp; cur.hasPerTransKicks = true; return true; } return false; };
    if (key.rfind("KICKS.CW.", 0) == 0) { std::string t = key.substr(10); if (t=="0TO1") { setKPT(0,0,val); return true; } if (t=="1TO2") { setKPT(0,1,val); return true; } if (t=="2TO3") { setKPT(0,2,val); return true; } if (t=="3TO0") { setKPT(0,3,val); return true; } }
    if (key.rfind("KICKS.CCW.", 0) == 0) { std::string t = key.substr(11); if (t=="0TO3") { setKPT(1,0,val); return true; } if (t=="3TO2") { setKPT(1,3,val); return true; } if (t=="2TO1") { setKPT(1,2,val); return true; } if (t=="1TO0") { setKPT(1,1,val); return true; } }
    return false;
}

bool pm_loadPiecesFromStream(std::istream& in) {
    PIECES.clear(); g_randomizerType = (RandType)0; g_randBagSize = 0;
    std::string line, section; Piece cur; bool inPiece = false; bool rotExplicit = false;
    std::vector<std::pair<int,int>> rot0, rot1, rot2, rot3, base;
    auto flushPiece = [&]() {
        if (!inPiece) return; pm_buildPieceRotations(cur, base, rot0, rot1, rot2, rot3, rotExplicit);
        if (!cur.rot.empty()) { PIECES.push_back(cur); }
        cur = Piece{}; rotExplicit = false; rot0.clear(); rot1.clear(); rot2.clear(); rot3.clear(); base.clear(); inPiece = false; };
    while (std::getline(in, line)) {
        line = pm_parsePiecesLine(line); auto trim = [&](std::string& s){ size_t a=s.find_first_not_of(" \t\r\n"); size_t b=s.find_last_not_of(" \t\r\n"); if (a==std::string::npos) { s.clear(); return; } s=s.substr(a,b-a+1); };
        trim(line); if (line.empty()) continue;
        if (line.front() == '[' && line.back() == ']') {
            std::string sec = line.substr(1, line.size() - 2); std::string SEC = sec; for (char& c : SEC) c = (char)std::toupper((unsigned char)c);
            if (SEC.rfind("PIECE.", 0) == 0) { flushPiece(); inPiece = true; cur = Piece{}; rotExplicit = false; rot0.clear(); rot1.clear(); rot2.clear(); rot3.clear(); base.clear(); cur.name = sec.substr(6); }
            else { flushPiece(); inPiece = false; section = SEC; }
            continue;
        }
        size_t eq = line.find('='); if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1); auto trim2 = [&](std::string& s){ size_t a=s.find_first_not_of(" \t\r\n"); size_t b=s.find_last_not_of(" \t\r\n"); if (a==std::string::npos) { s.clear(); return; } s=s.substr(a,b-a+1); };
        trim2(k); trim2(v); std::string K = k; for (char& c : K) c = (char)std::toupper((unsigned char)c);
        if (inPiece) { if (pm_processPieceProperty(cur, K, v, base, rot0, rot1, rot2, rot3, rotExplicit)) continue; }
        else {
            if (section == "SET") { if (K == "NAME") { /* optional */ continue; } if (K == "PREVIEWGRID" || K == "PREVIEW_GRID") { int n; if (pm_parseInt(v, n) && n > 0 && n <= 10) g_previewGrid = n; continue; } }
            if (section == "RANDOMIZER") { if (K == "TYPE") { std::string vv = v; for (char& c : vv) c = (char)std::tolower((unsigned char)c); g_randomizerType = (vv == "bag" ? (RandType)1 : (RandType)0); continue; }
                if (K == "BAGSIZE") { int n; if (pm_parseInt(v, n) && n >= 0) g_randBagSize = n; continue; } }
        }
    }
    flushPiece(); return !PIECES.empty();
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


