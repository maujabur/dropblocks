// DropBlocks - SDL2 single-file falling blocks (not Tetris®)
// Controles: ← → | ↓ | Z/↑ anti-horário | X horário | SPACE queda
//            P pausa | ENTER (START) reinicia | ESC sai | F12 screenshot
// Build: g++ dropblocks.cpp -o dropblocks `sdl2-config --cflags --libs` -O2

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <random>
#include <array>


// ===========================
//   PARÂMETROS & THEME
// ===========================

static int   ROUNDED_PANELS = 1;           // 1 = arredondado; 0 = retângulo
static int   HUD_FIXED_SCALE   = 6;        // escala fixa do HUD
static std::string TITLE_TEXT  = "HACKTRIS";// texto vertical (A–Z e espaço)
static int   GAP1_SCALE        = 10;       // banner ↔ tabuleiro (x scale)
static int   GAP2_SCALE        = 10;       // tabuleiro ↔ painel  (x scale)

static bool  ENABLE_BANNER_SWEEP = true;   // sweep local no banner
static bool  ENABLE_GLOBAL_SWEEP = true;   // sweep global (clareia a tela)

static float SWEEP_SPEED_PXPS  = 35.0f;    // px/s (banner)
static int   SWEEP_BAND_H_S    = 16;       // altura do banner sweep (em scale)
static int   SWEEP_ALPHA_MAX   = 40;       // 0..255 (banner)
static float SWEEP_SOFTNESS    = 0.6f;     // <1 = mais suave

static float SWEEP_G_SPEED_PXPS = 30.0f;   // px/s (global)
static int   SWEEP_G_BAND_H_PX  = 48;      // px
static int   SWEEP_G_ALPHA_MAX  = 28;      // 0..255
static float SWEEP_G_SOFTNESS   = 0.6f;    // suavidade

static int   SCANLINE_ALPHA     = 28;      // 0..255 (0 desativa)

// Caminho opcional indicado no cfg para o arquivo de peças
static std::string PIECES_FILE_PATH = "";

// Config do set de peças
static int PREVIEW_GRID = 6;               // NxN no NEXT (padrão 6)

// Randomizer
enum class RandType { SIMPLE, BAG };
static RandType RAND_TYPE = RandType::SIMPLE;
static int RAND_BAG_SIZE  = 0;             // 0 => tamanho do set

// THEME
struct RGB { Uint8 r,g,b; };
struct Theme {
    // fundo
    Uint8 bg_r=8, bg_g=8, bg_b=12;

    // tabuleiro
    Uint8 board_empty_r=28, board_empty_g=28, board_empty_b=36;

    // painel (HUD)
    Uint8 panel_fill_r=24, panel_fill_g=24, panel_fill_b=32;
    Uint8 panel_outline_r=90, panel_outline_g=90, panel_outline_b=120;
    Uint8 panel_outline_a=200;

    // banner
    Uint8 banner_bg_r=0, banner_bg_g=40, banner_bg_b=0;
    Uint8 banner_outline_r=0, banner_outline_g=60, banner_outline_b=0;
    Uint8 banner_outline_a=180;
    Uint8 banner_text_r=120, banner_text_g=255, banner_text_b=120;

    // HUD textos
    Uint8 hud_label_r=200, hud_label_g=200, hud_label_b=220;
    Uint8 hud_score_r=255, hud_score_g=240, hud_score_b=120;
    Uint8 hud_lines_r=180, hud_lines_g=255, hud_lines_b=180;
    Uint8 hud_level_r=180, hud_level_g=200, hud_level_b=255;

    // NEXT
    Uint8 next_fill_r=18, next_fill_g=18, next_fill_b=26;
    Uint8 next_outline_r=80, next_outline_g=80, next_outline_b=110;
    Uint8 next_outline_a=160;
    Uint8 next_label_r=220, next_label_g=220, next_label_b=220;
    Uint8 next_grid_dark=24, next_grid_light=30; // cinza
    // Grid colorido (prioritário se definido)
    Uint8 next_grid_dark_r=24, next_grid_dark_g=24, next_grid_dark_b=24;
    Uint8 next_grid_light_r=30, next_grid_light_g=30, next_grid_light_b=30;
    bool  next_grid_use_rgb=false;

    // overlay
    Uint8 overlay_fill_r=0, overlay_fill_g=0, overlay_fill_b=0, overlay_fill_a=200;
    Uint8 overlay_outline_r=200, overlay_outline_g=200, overlay_outline_b=220, overlay_outline_a=120;
    Uint8 overlay_top_r=255, overlay_top_g=160, overlay_top_b=160;
    Uint8 overlay_sub_r=220, overlay_sub_g=220, overlay_sub_b=220;

    // cores padrão das peças (máx 8 aplicadas pelo tema)
    RGB piece[8] = {
        {220, 80, 80},  { 80,180,120}, { 80,120,220}, {220,160, 80},
        {160, 80,220},  {200,200,200}, {200,200,200}, {200,200,200}
    };
} THEME;

// ===========================
//   MECÂNICA / ESTRUTURAS
// ===========================

static const int COLS = 10;
static const int ROWS = 20;
static const int BORDER = 10;
static const int TICK_MS_START = 600;
static const int TICK_MS_MIN   = 120;
static const int LEVEL_STEP    = 10;

struct Cell { Uint8 r,g,b; bool occ=false; };
struct Piece {
    std::string name;
    std::vector<std::vector<std::pair<int,int>>> rot; // 0..3
    Uint8 r=200,g=200,b=200;

    // --- NOVO: SRS por transição ---
    // kicks[dirIndex][fromState] -> vetor de offsets
    // dirIndex: 0 = CW (+1), 1 = CCW (-1)
    std::array<std::array<std::vector<std::pair<int,int>>,4>,2> kicksPerTrans;
    bool hasPerTransKicks=false;

    // --- Legado (fallback compatível) ---
    std::vector<std::pair<int,int>> kicksCW;
    std::vector<std::pair<int,int>> kicksCCW;
    bool hasKicks=false;
};


static std::vector<Piece> PIECES;

struct Active { int x=COLS/2, y=0, rot=0, idx=0; };

// ===========================
//   UTILS: STR / CORES
// ===========================

static void trim(std::string& s){
    size_t a=s.find_first_not_of(" \t\r\n");
    size_t b=s.find_last_not_of(" \t\r\n");
    if(a==std::string::npos){ s.clear(); return; }
    s=s.substr(a,b-a+1);
}
static bool parseHexColor(const std::string& s, Uint8& r, Uint8& g, Uint8& b){
    if (s.size()!=7 || s[0]!='#') return false;
    auto cv=[&](char c)->int{
        if(c>='0'&&c<='9') return c-'0';
        c=(char)std::toupper((unsigned char)c);
        if(c>='A'&&c<='F') return 10+(c-'A');
        return -1;
    };
    auto hx=[&](char a,char b){int A=cv(a),B=cv(b); return (A<0||B<0)?-1:(A*16+B);};
    int R=hx(s[1],s[2]), G=hx(s[3],s[4]), B=hx(s[5],s[6]);
    if(R<0||G<0||B<0) return false; r=(Uint8)R; g=(Uint8)G; b=(Uint8)B; return true;
}
static bool parseInt(const std::string& s, int& out){
    char* e=nullptr; long v=strtol(s.c_str(), &e, 10);
    if(e==s.c_str()||*e!='\0') return false; out=(int)v; return true;
}

// ===========================
//   LOADER: dropblocks.cfg
// ===========================

static void loadConfigFromStream(std::istream& in){
    std::string line;
    while(std::getline(in,line)){
        // Comentários: ; sempre; # só se vier antes do '='
        size_t eq_probe = line.find('=');
        size_t semicol = line.find(';');
        size_t hash    = line.find('#');
        size_t cut=std::string::npos;
        if(semicol!=std::string::npos) cut=semicol;
        if(hash!=std::string::npos && (eq_probe==std::string::npos || hash<eq_probe))
            cut=(cut==std::string::npos?hash:std::min(cut,hash));
        if(cut!=std::string::npos) line.resize(cut);

        trim(line); if(line.empty()) continue;
        size_t eq=line.find('='); if(eq==std::string::npos) continue;
        std::string key=line.substr(0,eq), val=line.substr(eq+1);
        trim(key); trim(val); if(key.empty()) continue;

        std::string KEY=key; for(char& c:KEY) c=(char)std::toupper((unsigned char)c);

        auto setb=[&](const char* K, bool& ref){
            if(KEY==K){ std::string v=val; for(char& c:v) c=(char)std::tolower((unsigned char)c);
                ref=(v=="1"||v=="true"||v=="on"||v=="yes"); return true; } return false; };
        auto seti=[&](const char* K, int& ref){ if(KEY==K){ ref=std::atoi(val.c_str()); return true;} return false; };
        auto setf=[&](const char* K, float& ref){ if(KEY==K){ ref=(float)std::atof(val.c_str()); return true;} return false; };
        auto setrgb=[&](const char* K, Uint8& R, Uint8& G, Uint8& B){
            if(KEY==K){ Uint8 r,g,b; if(parseHexColor(val,r,g,b)){ R=r;G=g;B=b; } return true;} return false; };
        auto seta=[&](const char* K, Uint8& ref){
            if(KEY==K){ int v=std::atoi(val.c_str()); if(v<0)v=0; if(v>255)v=255; ref=(Uint8)v; return true;} return false; };

        if(setb("ENABLE_BANNER_SWEEP", ENABLE_BANNER_SWEEP)) continue;
        if(setb("ENABLE_GLOBAL_SWEEP", ENABLE_GLOBAL_SWEEP)) continue;
        if(KEY=="TITLE_TEXT"){ TITLE_TEXT=val; continue; }
        if(seti("ROUNDED_PANELS", ROUNDED_PANELS)) continue;
        if(seti("HUD_FIXED_SCALE", HUD_FIXED_SCALE)) continue;
        if(seti("GAP1_SCALE", GAP1_SCALE) || seti("GAP2_SCALE", GAP2_SCALE)) continue;

        if(seti("SWEEP_BAND_H_S", SWEEP_BAND_H_S)) continue;
        if(seti("SWEEP_ALPHA_MAX", SWEEP_ALPHA_MAX)) continue;
        if(seti("SWEEP_G_BAND_H_PX", SWEEP_G_BAND_H_PX)) continue;
        if(seti("SWEEP_G_ALPHA_MAX", SWEEP_G_ALPHA_MAX)) continue;
        if(seti("SCANLINE_ALPHA", SCANLINE_ALPHA)) continue;

        if(setf("SWEEP_SPEED_PXPS", SWEEP_SPEED_PXPS)) continue;
        if(setf("SWEEP_SOFTNESS", SWEEP_SOFTNESS)) continue;
        if(setf("SWEEP_G_SPEED_PXPS", SWEEP_G_SPEED_PXPS)) continue;
        if(setf("SWEEP_G_SOFTNESS", SWEEP_G_SOFTNESS)) continue;

        if(KEY=="PIECES_FILE"){ PIECES_FILE_PATH=val; continue; }

        if(setrgb("BG", THEME.bg_r, THEME.bg_g, THEME.bg_b)) continue;
        if(setrgb("BOARD_EMPTY", THEME.board_empty_r, THEME.board_empty_g, THEME.board_empty_b)) continue;
        if(setrgb("PANEL_FILL", THEME.panel_fill_r, THEME.panel_fill_g, THEME.panel_fill_b)) continue;
        if(setrgb("PANEL_OUTLINE", THEME.panel_outline_r, THEME.panel_outline_g, THEME.panel_outline_b)) continue;
        if(setrgb("BANNER_BG", THEME.banner_bg_r, THEME.banner_bg_g, THEME.banner_bg_b)) continue;
        if(setrgb("BANNER_OUTLINE", THEME.banner_outline_r, THEME.banner_outline_g, THEME.banner_outline_b)) continue;
        if(setrgb("BANNER_TEXT", THEME.banner_text_r, THEME.banner_text_g, THEME.banner_text_b)) continue;

        if(setrgb("HUD_LABEL", THEME.hud_label_r, THEME.hud_label_g, THEME.hud_label_b)) continue;
        if(setrgb("HUD_SCORE", THEME.hud_score_r, THEME.hud_score_g, THEME.hud_score_b)) continue;
        if(setrgb("HUD_LINES", THEME.hud_lines_r, THEME.hud_lines_g, THEME.hud_lines_b)) continue;
        if(setrgb("HUD_LEVEL", THEME.hud_level_r, THEME.hud_level_g, THEME.hud_level_b)) continue;

        if(setrgb("NEXT_FILL", THEME.next_fill_r, THEME.next_fill_g, THEME.next_fill_b)) continue;
        if(setrgb("NEXT_OUTLINE", THEME.next_outline_r, THEME.next_outline_g, THEME.next_outline_b)) continue;
        if(setrgb("NEXT_LABEL", THEME.next_label_r, THEME.next_label_g, THEME.next_label_b)) continue;

        if(setrgb("OVERLAY_FILL", THEME.overlay_fill_r, THEME.overlay_fill_g, THEME.overlay_fill_b)) continue;
        if(setrgb("OVERLAY_OUTLINE", THEME.overlay_outline_r, THEME.overlay_outline_g, THEME.overlay_outline_b)) continue;
        if(setrgb("OVERLAY_TOP", THEME.overlay_top_r, THEME.overlay_top_g, THEME.overlay_top_b)) continue;
        if(setrgb("OVERLAY_SUB", THEME.overlay_sub_r, THEME.overlay_sub_g, THEME.overlay_sub_b)) continue;

        if(seta("PANEL_OUTLINE_A", THEME.panel_outline_a)) continue;
        if(seta("NEXT_OUTLINE_A", THEME.next_outline_a)) continue;
        if(seta("OVERLAY_FILL_A", THEME.overlay_fill_a)) continue;
        if(seta("OVERLAY_OUTLINE_A", THEME.overlay_outline_a)) continue;

        if(seti("NEXT_GRID_DARK", *(int*)&THEME.next_grid_dark)) continue;
        if(seti("NEXT_GRID_LIGHT", *(int*)&THEME.next_grid_light)) continue;

        if(KEY=="NEXT_GRID_DARK_COLOR"){
            Uint8 r,g,b; if(parseHexColor(val,r,g,b)){
                THEME.next_grid_dark_r=r; THEME.next_grid_dark_g=g; THEME.next_grid_dark_b=b;
                THEME.next_grid_use_rgb=true;
            }
            continue;
        }
        if(KEY=="NEXT_GRID_LIGHT_COLOR"){
            Uint8 r,g,b; if(parseHexColor(val,r,g,b)){
                THEME.next_grid_light_r=r; THEME.next_grid_light_g=g; THEME.next_grid_light_b=b;
                THEME.next_grid_use_rgb=true;
            }
            continue;
        }

        // cores das peças (tema) – sobrescrevem as carregadas do catálogo
        for(int i=0;i<8;i++){
            std::string pk = "PIECE"+std::to_string(i);
            if(KEY==pk){ Uint8 r,g,b; if(parseHexColor(val,r,g,b)){ THEME.piece[i]={r,g,b}; } break; }
        }
    }
}
static bool loadConfigPath(const std::string& p){
    std::ifstream f(p.c_str()); if(f.good()){ loadConfigFromStream(f); SDL_Log("Config carregado de: %s", p.c_str()); return true; } return false;
}
static void loadConfigFile(){
    if(const char* env = std::getenv("DROPBLOCKS_CFG")){ if(loadConfigPath(env)) return; }
    if(loadConfigPath("dropblocks.cfg")) return;
    if(const char* home = std::getenv("HOME")){
        std::string p = std::string(home) + "/.config/dropblocks.cfg";
        if(loadConfigPath(p)) return;
    }
    SDL_Log("Nenhum config encontrado; usando padrões.");
}

// ===========================
//   LOADER: peças (.pieces)
// ===========================

static bool parseCoordList(const std::string& val, std::vector<std::pair<int,int>>& out){
    out.clear();
    int sign=1, num=0; bool inNum=false; int x=0,y=0; bool gotX=false;
    auto push=[&](){ if(gotX){ out.push_back({x,y}); gotX=false; } };
    for(size_t i=0;i<val.size();++i){
        char c=val[i];
        if(c=='-'||c=='+'){ sign=(c=='-')?-1:1; inNum=true; num=0; continue; }
        if(std::isdigit((unsigned char)c)){ inNum=true; num = num*10 + (c-'0'); continue; }
        if(c==','||c==')'){
            if(inNum){ if(!gotX){ x=sign*num; gotX=true; } else { y=sign*num; push(); } }
            inNum=false; sign=1; num=0; continue;
        }
        if(c=='('){ inNum=false; sign=1; num=0; gotX=false; continue; }
    }
    return !out.empty();
}
static bool parseKicks(const std::string& v, std::vector<std::pair<int,int>>& out){ return parseCoordList(v,out); }
static void rotate90(std::vector<std::pair<int,int>>& pts){
    for(auto& p:pts){ int x=p.first,y=p.second; p.first=-y; p.second=x; }
}

static bool loadPiecesFromStream(std::istream& in){
    PIECES.clear(); PREVIEW_GRID=6; RAND_TYPE=RandType::SIMPLE; RAND_BAG_SIZE=0;

    std::string line, section;
    Piece cur; bool inPiece=false; bool rotExplicit=false;
    std::vector<std::pair<int,int>> rot0,rot1,rot2,rot3, base;

    auto flushPiece=[&](){
        if(!inPiece) return;
        Piece p=cur; p.rot.clear();
        if(rotExplicit){
            if(!rot0.empty()){
                p.rot.push_back(rot0);
                p.rot.push_back(rot1.empty()?rot0:rot1);
                p.rot.push_back(rot2.empty()?rot0:rot2);
                p.rot.push_back(rot3.empty()?(rot1.empty()?rot0:rot1):rot3);
            }
        } else {
            if(!base.empty()){
                std::vector<std::pair<int,int>> r0=base, r1=base, r2=base, r3=base;
                rotate90(r1);
                r2 = r1; rotate90(r2);
                r3 = r2; rotate90(r3);
                p.rot.push_back(r0);
                p.rot.push_back(r1);
                p.rot.push_back(r2);
                p.rot.push_back(r3);
            }
        }
        if(!p.rot.empty()) PIECES.push_back(p);
        cur=Piece{}; rotExplicit=false; rot0.clear(); rot1.clear(); rot2.clear(); rot3.clear(); base.clear(); inPiece=false;
    };

    while(std::getline(in,line)){
        size_t eq_probe=line.find('='); size_t semi=line.find(';'); size_t hash=line.find('#');
        size_t cut=std::string::npos; if(semi!=std::string::npos) cut=semi;
        if(hash!=std::string::npos && (eq_probe==std::string::npos || hash<eq_probe))
            cut=(cut==std::string::npos?hash:std::min(cut,hash));
        if(cut!=std::string::npos) line.resize(cut);

        trim(line); if(line.empty()) continue;

        if(line.front()=='[' && line.back()==']'){
            std::string sec=line.substr(1,line.size()-2);
            std::string SEC=sec; for(char& c:SEC) c=(char)std::toupper((unsigned char)c);
            if(SEC.rfind("PIECE.",0)==0){
                flushPiece(); inPiece=true; cur=Piece{}; rotExplicit=false;
                rot0.clear(); rot1.clear(); rot2.clear(); rot3.clear(); base.clear();
                cur.name=sec.substr(6);
            } else {
                flushPiece(); inPiece=false; section=SEC;
            }
            continue;
        }

        size_t eq=line.find('='); if(eq==std::string::npos) continue;
        std::string k=line.substr(0,eq), v=line.substr(eq+1); trim(k); trim(v);
        std::string K=k; for(char& c:K) c=(char)std::toupper((unsigned char)c);

        if(inPiece){
            if(K=="COLOR"){ Uint8 r,g,b; if(parseHexColor(v,r,g,b)){ cur.r=r; cur.g=g; cur.b=b; } continue; }
            if(K=="ROTATIONS"){ std::string vv=v; for(char& c:vv) c=(char)std::tolower((unsigned char)c); rotExplicit=(vv=="explicit"); continue; }
            if(K=="BASE"){ parseCoordList(v, base); continue; }
            if(K=="ROT0"){ if(v.rfind("sameas:",0)==0){ /* usa rot0 */ } else parseCoordList(v, rot0); rotExplicit=true; continue; }
            if(K=="ROT1"){ if(v.rfind("sameas:",0)==0){ rot1=rot0; } else parseCoordList(v, rot1); rotExplicit=true; continue; }
            if(K=="ROT2"){ if(v.rfind("sameas:",0)==0){ rot2=rot0; } else parseCoordList(v, rot2); rotExplicit=true; continue; }
            if(K=="ROT3"){ if(v.rfind("sameas:",0)==0){ rot3=rot1.empty()?rot0:rot1; } else parseCoordList(v, rot3); rotExplicit=true; continue; }
            if(K=="KICKS.CW"){ parseKicks(v, cur.kicksCW); cur.hasKicks=true; continue; }
            if(K=="KICKS.CCW"){ parseKicks(v, cur.kicksCCW); cur.hasKicks=true; continue; }
            auto setKPT = [&](int dirIdx, int fromState, const std::string& val){
                std::vector<std::pair<int,int>> tmp;
                if(parseCoordList(val, tmp)){
                    cur.kicksPerTrans[dirIdx][fromState] = tmp;
                    cur.hasPerTransKicks = true;
                    return true;
                }
                return false;
            };

            // KICKS.CW.0TO1 / 1TO2 / 2TO3 / 3TO0
            if(K.rfind("KICKS.CW.",0)==0){
                std::string t = K.substr(10); // depois de "KICKS.CW."
                if(t=="0TO1") { setKPT(0,0,v); continue; }
                if(t=="1TO2") { setKPT(0,1,v); continue; }
                if(t=="2TO3") { setKPT(0,2,v); continue; }
                if(t=="3TO0") { setKPT(0,3,v); continue; }
            }
            // KICKS.CCW.0TO3 / 3TO2 / 2TO1 / 1TO0
            if(K.rfind("KICKS.CCW.",0)==0){
                std::string t = K.substr(11); // depois de "KICKS.CCW."
                if(t=="0TO3") { setKPT(1,0,v); continue; }
                if(t=="3TO2") { setKPT(1,3,v); continue; }
                if(t=="2TO1") { setKPT(1,2,v); continue; }
                if(t=="1TO0") { setKPT(1,1,v); continue; }
            }
            continue;
        } else {
            if(section=="SET"){
                if(K=="NAME"){ /* opcional */ continue; }
                if(K=="PREVIEWGRID"){ int n; if(parseInt(v,n)&&n>0&&n<=10) PREVIEW_GRID=n; continue; }
            }
            if(section=="RANDOMIZER"){
                if(K=="TYPE"){ std::string vv=v; for(char& c:vv) c=(char)std::tolower((unsigned char)c);
                    RAND_TYPE = (vv=="bag"? RandType::BAG : RandType::SIMPLE); continue; }
                if(K=="BAGSIZE"){ int n; if(parseInt(v,n)&&n>=0) RAND_BAG_SIZE=n; continue; }
            }
        }
    }
    flushPiece();
    return !PIECES.empty();
}

static bool loadPiecesPath(const std::string& p){
    std::ifstream f(p.c_str()); if(!f.good()) return false;
    bool ok=loadPiecesFromStream(f);
    SDL_Log("Pieces carregado de: %s (%s)", p.c_str(), ok?"OK":"vazio/erro");
    return ok;
}
static bool loadPiecesFile(){
    if(const char* env = std::getenv("DROPBLOCKS_PIECES")) if(loadPiecesPath(env)) return true;
    if(!PIECES_FILE_PATH.empty()) if(loadPiecesPath(PIECES_FILE_PATH)) return true;
    if(loadPiecesPath("tetrominoes.pieces")) return true;
    if(const char* home = std::getenv("HOME")){
        std::string p = std::string(home) + "/.config/tetrominoes.pieces";
        if(loadPiecesPath(p)) return true;
    }
    return false;
}

// ===========================
//   FALLBACK peças internas
// ===========================
static void seedPiecesFallback(){
    PIECES.clear();
    auto mk = [](std::initializer_list<std::pair<int,int>> c, Uint8 r, Uint8 g, Uint8 b){
        Piece p; p.r=r; p.g=g; p.b=b;
        std::vector<std::pair<int,int>> base(c);
        std::vector<std::pair<int,int>> r0=base, r1=base, r2=base, r3=base;
        rotate90(r1); r2=r1; rotate90(r2); r3=r2; rotate90(r3);
        p.rot={r0,r1,r2,r3}; return p;
    };
    PIECES.push_back(mk({{0,0},{1,0},{0,1},{-1,0}}, 220,80,80));
    PIECES.push_back(mk({{0,0},{1,0},{-1,0},{-1,1}}, 80,180,120));
    PIECES.push_back(mk({{0,0},{1,0},{0,1},{0,2}},   80,120,220));
    PIECES.push_back(mk({{0,0},{1,0},{-1,0},{0,1},{1,1}}, 220,160,80));
    PIECES.push_back(mk({{0,0},{0,1},{1,1},{-1,0}}, 160,80,220));
}

static void applyThemePieceColors(){
    for (size_t i=0; i<PIECES.size() && i<8; ++i){
        PIECES[i].r = THEME.piece[i].r;
        PIECES[i].g = THEME.piece[i].g;
        PIECES[i].b = THEME.piece[i].b;
    }
}

// ===========================
//   COLISÃO / GRID / ETC
// ===========================
static bool collides(const Active& a, const std::vector<std::vector<Cell>>& g, int dx, int dy, int drot){
    int R = (a.rot + drot + 4)%4;
    for (auto [px,py] : PIECES[a.idx].rot[R]) {
        int x = a.x + dx + px, y = a.y + dy + py;
        if (x<0 || x>=COLS || y<0 || y>=ROWS) return true;
        if (g[y][x].occ) return true;
    }
    return false;
}
static void lockPiece(const Active& a, std::vector<std::vector<Cell>>& g){
    auto &pc = PIECES[a.idx];
    for (auto [px,py] : pc.rot[a.rot]) {
        int x=a.x+px, y=a.y+py;
        if (y>=0 && y<ROWS && x>=0 && x<COLS){
            g[y][x].occ=true; g[y][x].r=pc.r; g[y][x].g=pc.g; g[y][x].b=pc.b;
        }
    }
}
static int clearLines(std::vector<std::vector<Cell>>& g){
    int cleared=0;
    for (int y=ROWS-1; y>=0; --y){
        bool full=true; for (int x=0;x<COLS;x++) if(!g[y][x].occ){ full=false; break; }
        if (full){ cleared++; for (int yy=y; yy>0; --yy) g[yy]=g[yy-1]; g[0]=std::vector<Cell>(COLS); y++; }
    }
    return cleared;
}
static void newActive(Active& a, int idx){ a.idx=idx; a.rot=0; a.x=COLS/2; a.y=0; }

// ===========================
//   FONTE 5x7 PIXEL
// ===========================
static bool glyph(char c, int x, int y){
    auto at=[&](const char* rows[7]){ return rows[y][x]=='#'; };
    static const char* NUM[10][7] = {
        {" ### ","#   #","#  ##","# # #","##  #","#   #"," ### "}, //0
        {"  #  "," ##  ","  #  ","  #  ","  #  ","  #  "," ### "}, //1
        {" ### ","#   #","    #","   # ","  #  "," #   ","#####"}, //2
        {" ### ","#   #","    #"," ### ","    #","#   #"," ### "}, //3
        {"   # ","  ## "," # # ","#  # ","#####","   # ","   # "}, //4
        {"#####","#    ","#    ","#### ","    #","#   #"," ### "}, //5
        {" ### ","#   #","#    ","#### ","#   #","#   #"," ### "}, //6
        {"#####","    #","   # ","  #  ","  #  ","  #  ","  #  "}, //7
        {" ### ","#   #","#   #"," ### ","#   #","#   #"," ### "}, //8
        {" ### ","#   #","#   #"," ####","    #","#   #"," ### "}  //9
    };
    static const char* A_[7]={" ### ","#   #","#   #","#####","#   #","#   #","#   #"};
    static const char* B_[7]={"#### ","#   #","#   #","#### ","#   #","#   #","#### "};
    static const char* C_[7]={" ### ","#   #","#    ","#    ","#    ","#   #"," ### "};
    static const char* D_[7]={"#### ","#   #","#   #","#   #","#   #","#   #","#### "};
    static const char* E_[7]={"#####","#    ","#    ","#### ","#    ","#    ","#####"};
    static const char* F_[7]={"#####","#    ","#    ","#### ","#    ","#    ","#    "};
    static const char* G_[7]={" ### ","#   #","#    ","# ###","#   #","#   #"," ### "};
    static const char* H_[7]={"#   #","#   #","#   #","#####","#   #","#   #","#   #"};
    static const char* I_[7]={"#####","  #  ","  #  ","  #  ","  #  ","  #  ","#####"};
    static const char* J_[7]={"  ###","   # ","   # ","   # ","#  # ","#  # "," ##  "};
    static const char* K_[7]={"#   #","#  # ","# #  ","##   ","# #  ","#  # ","#   #"};
    static const char* L_[7]={"#    ","#    ","#    ","#    ","#    ","#    ","#####"};
    static const char* M_[7]={"#   #","## ##","# # #","#   #","#   #","#   #","#   #"};
    static const char* N_[7]={"#   #","##  #","# # #","#  ##","#   #","#   #","#   #"};
    static const char* O_[7]={" ### ","#   #","#   #","#   #","#   #","#   #"," ### "};
    static const char* P_[7]={"#### ","#   #","#   #","#### ","#    ","#    ","#    "};
    static const char* Q_[7]={" ### ","#   #","#   #","#   #","# # #","#  # "," ## #"};
    static const char* R_[7]={"#### ","#   #","#   #","#### ","# #  ","#  # ","#   #"};
    static const char* S_[7]={" ####","#    ","#    "," ### ","    #","    #","#### "};
    static const char* T_[7]={"#####","  #  ","  #  ","  #  ","  #  ","  #  ","  #  "};
    static const char* U_[7]={"#   #","#   #","#   #","#   #","#   #","#   #"," ### "};
    static const char* V_[7]={"#   #","#   #","#   #","#   #","#   #"," # # ","  #  "};
    static const char* W_[7]={"#   #","#   #","#   #","# # #","# # #","## ##","#   #"};
    static const char* X_[7]={"#   #","#   #"," # # ","  #  "," # # ","#   #","#   #"};
    static const char* Y_[7]={"#   #","#   #"," # # ","  #  ","  #  ","  #  ","  #  "};
    static const char* Z_[7]={"#####","    #","   # ","  #  "," #   ","#    ","#####"};
    static const char* dash[7]={"     ","     ","     "," ### ","     ","     ","     "};
    static const char* colon[7]={"     ","  #  ","     ","     ","     ","  #  ","     "};

    if(c>='0'&&c<='9') return NUM[c-'0'][y][x]=='#';
    c=(char)std::toupper((unsigned char)c);
    #define GL(CH,ARR) if(c==CH) return at(ARR);
    GL('A',A_) GL('B',B_) GL('C',C_) GL('D',D_) GL('E',E_) GL('F',F_) GL('G',G_)
    GL('H',H_) GL('I',I_) GL('J',J_) GL('K',K_) GL('L',L_) GL('M',M_) GL('N',N_)
    GL('O',O_) GL('P',P_) GL('Q',Q_) GL('R',R_) GL('S',S_) GL('T',T_) GL('U',U_)
    GL('V',V_) GL('W',W_) GL('X',X_) GL('Y',Y_) GL('Z',Z_) GL('-',dash) GL(':',colon)
    #undef GL
    return false;
}
static void drawPixelText(SDL_Renderer* ren, int x, int y, const std::string& s, int scale, Uint8 r, Uint8 g, Uint8 b){
    SDL_Rect px; SDL_SetRenderDrawColor(ren, r,g,b,255);
    int cx=x;
    for(char c : s){
        if(c=='\n'){ y += (7*scale + scale*2); cx=x; continue; }
        for(int yy=0; yy<7; ++yy) for(int xx=0; xx<5; ++xx)
            if(glyph(c,xx,yy)){ px = { cx + xx*scale, y + yy*scale, scale, scale }; SDL_RenderFillRect(ren, &px); }
        cx += 6*scale;
    }
}
static int textWidthPx(const std::string& s, int scale){
    if(s.empty()) return 0; return (int)s.size() * 6 * scale - scale;
}
static void drawPixelTextOutlined(SDL_Renderer* ren, int x, int y, const std::string& s, int scale,
                                  Uint8 fr, Uint8 fg, Uint8 fb, Uint8 or_, Uint8 og, Uint8 ob){
    const int d = std::max(1, scale/2);
    drawPixelText(ren, x-d, y,   s, scale, or_,og,ob);
    drawPixelText(ren, x+d, y,   s, scale, or_,og,ob);
    drawPixelText(ren, x,   y-d, s, scale, or_,og,ob);
    drawPixelText(ren, x,   y+d, s, scale, or_,og,ob);
    drawPixelText(ren, x-d, y-d, s, scale, or_,og,ob);
    drawPixelText(ren, x+d, y-d, s, scale, or_,og,ob);
    drawPixelText(ren, x-d, y+d, s, scale, or_,og,ob);
    drawPixelText(ren, x+d, y+d, s, scale, or_,og,ob);
    drawPixelText(ren, x,   y,   s, scale, fr,fg,fb);
}

// ===========================
//   PRIMITIVOS UI
// ===========================
static void drawRoundedFilled(SDL_Renderer* r, int x, int y, int w, int h, int rad, Uint8 R, Uint8 G, Uint8 B, Uint8 A){
    if(!ROUNDED_PANELS){ SDL_SetRenderDrawColor(r, R,G,B,A); SDL_Rect rr{ x,y,w,h }; SDL_RenderFillRect(r,&rr); return; }
    rad = std::max(0, std::min(rad, std::min(w,h)/2));
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, R,G,B,A);
    for (int yy=0; yy<h; ++yy){
        int left = x, right = x + w - 1;
        if (yy < rad){
            int dy = rad-1-yy; int dx = (int)std::floor(std::sqrt((double)rad*rad - (double)dy*dy));
            left = x + rad - dx; right = x + w - rad + dx - 1;
        } else if (yy >= h - rad){
            int dy = yy - (h - rad); int dx = (int)std::floor(std::sqrt((double)rad*rad - (double)dy*dy));
            left = x + rad - dx; right = x + w - rad + dx - 1;
        }
        SDL_Rect line{ left, y + yy, right - left + 1, 1 };
        SDL_RenderFillRect(r, &line);
    }
}
static void drawRoundedOutline(SDL_Renderer* r, int x, int y, int w, int h, int rad, int thick, Uint8 R, Uint8 G, Uint8 B, Uint8 A){
    for(int i=0;i<thick;i++){
        drawRoundedFilled(r, x+i, y+i, w-2*i, h-2*i, std::max(0,rad-i), R,G,B,A);
    }
}

// ===========================
//   UTILS
// ===========================
static std::string fmtScore(int v){
    std::string s=std::to_string(v),o; int c=0;
    for(int i=(int)s.size()-1;i>=0;--i){ o.push_back(s[i]); if(++c==3 && i>0){ o.push_back(' '); c=0; } }
    std::reverse(o.begin(), o.end()); return o;
}
static bool saveScreenshot(SDL_Renderer* ren, const char* path) {
    int w, h; SDL_GetRendererOutputSize(ren, &w, &h);
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 24, SDL_PIXELFORMAT_BGR24);
    if (!surf) return false;
    if (SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_BGR24, surf->pixels, surf->pitch) != 0) {
        SDL_FreeSurface(surf); return false;
    }
    int rc = SDL_SaveBMP(surf, path);
    SDL_FreeSurface(surf);
    return (rc == 0);
}

// ===========================
//   MAIN
// ===========================
int main(int, char**){
    loadConfigFile();

    srand((unsigned)time(nullptr));
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0){
        SDL_Log("SDL_Init error: %s", SDL_GetError()); return 1;
    }

    // Áudio
    SDL_AudioDeviceID gAudioDev=0; SDL_AudioSpec want{};
    want.freq=44100; want.format=AUDIO_F32SYS; want.channels=1; want.samples=1024;
    gAudioDev=SDL_OpenAudioDevice(nullptr,0,&want,&want,0);
    if(gAudioDev) SDL_PauseAudioDevice(gAudioDev,0);
    auto playBeep=[&](double freq,int ms,float vol=0.25f,bool square=true){
        if(!gAudioDev) return;
        int N=(int)(want.freq*(ms/1000.0));
        std::vector<float> buf(N);
        double ph=0, st=2.0*M_PI*freq/want.freq;
        for(int i=0;i<N;i++){
            float s=square?(std::sin(ph)>=0?1.f:-1.f):(float)std::sin(ph);
            buf[i]=s*vol; ph+=st; if(ph>2*M_PI) ph-=2*M_PI;
        }
        SDL_QueueAudio(gAudioDev, buf.data(), (Uint32)(buf.size()*sizeof(float)));
    };

    // Peças: externas -> fallback
    bool piecesOk = loadPiecesFile();
    if(!piecesOk){ SDL_Log("Usando fallback interno de peças."); seedPiecesFallback(); }
    // Tema sobrepõe cores das peças
    applyThemePieceColors();
    SDL_Log("Pieces: %zu, PreviewGrid=%d, Randomizer=%s, BagSize=%d",
            PIECES.size(), PREVIEW_GRID, (RAND_TYPE==RandType::BAG?"bag":"simple"), RAND_BAG_SIZE);

    // Janela/Renderer
    SDL_DisplayMode dm; SDL_GetCurrentDisplayMode(0,&dm);
    int SW = dm.w, SH = dm.h;
    SDL_Window* win = SDL_CreateWindow("DropBlocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SW, SH,
                                       SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if(!win){ SDL_Log("Win error: %s", SDL_GetError()); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!ren){ SDL_Log("Ren error: %s", SDL_GetError()); return 1; }

    // Logs úteis
    SDL_Log("CFG: TITLE_TEXT='%s'", TITLE_TEXT.c_str());
    SDL_Log("CFG: bg=#%02X%02X%02X", THEME.bg_r, THEME.bg_g, THEME.bg_b);
    SDL_Log("CFG: panel_fill=#%02X%02X%02X", THEME.panel_fill_r, THEME.panel_fill_g, THEME.panel_fill_b);
    SDL_Log("CFG: banner_text=#%02X%02X%02X", THEME.banner_text_r, THEME.banner_text_g, THEME.banner_text_b);
    SDL_Log("CFG: piece0=#%02X%02X%02X", THEME.piece[0].r, THEME.piece[0].g, THEME.piece[0].b);
    SDL_Log("CFG: ENABLE_GLOBAL_SWEEP=%d SCANLINE_ALPHA=%d", (int)ENABLE_GLOBAL_SWEEP, SCANLINE_ALPHA);

    // Grid e estado
    std::vector<std::vector<Cell>> grid(ROWS, std::vector<Cell>(COLS));
    Active act;

    // Randomizer
    std::vector<int> bag; size_t bagPos=0; std::mt19937 rng((unsigned)time(nullptr));
    auto refillBag=[&](){
        bag.clear();
        int n=(RAND_BAG_SIZE>0? RAND_BAG_SIZE : (int)PIECES.size());
        n=std::min(n,(int)PIECES.size());
        for(int i=0;i<n;i++) bag.push_back(i);
        std::shuffle(bag.begin(), bag.end(), rng);
        bagPos=0;
    };
    auto drawPieceIdx=[&](){
        if(RAND_TYPE==RandType::BAG){
            if(bagPos>=bag.size()) refillBag();
            return bag[bagPos++];
        }
        return (int)(rng()%PIECES.size());
    };

    int nextIdx = drawPieceIdx(); newActive(act, nextIdx); nextIdx = drawPieceIdx();

    bool running=true, paused=false, gameover=false;
    int score=0, lines=0, level=0, tick_ms=TICK_MS_START;
    Uint32 lastTick=SDL_GetTicks();

    auto coll=[&](int dx,int dy,int drot){ return collides(act,grid,dx,dy,drot); };
    auto trySpawn=[&](){ if (coll(0,0,0)) { gameover=true; paused=false; } };

    auto softDrop=[&](){
        if(!coll(0,1,0)) act.y++;
        else {
            lockPiece(act, grid); playBeep(220.0,40,0.18f,true);
            int c=clearLines(grid);
            if(c>0){
                lines+=c; playBeep(660.0,60+c*20,0.22f,true);
                score+=(c==1?100: c==2?300: c==3?500:800)*(level+1);
                int lv=lines/LEVEL_STEP; if(lv>level){ level=lv; tick_ms=std::max(TICK_MS_MIN,TICK_MS_START-level*40); }
            }
            newActive(act,nextIdx); nextIdx=drawPieceIdx(); trySpawn();
        }
    };
    auto hardDrop=[&](){
        while(!coll(0,1,0)) act.y++;
        lockPiece(act, grid); playBeep(300.0,35,0.18f,true); score+=2;
        int c=clearLines(grid);
        if(c>0){
            lines+=c; playBeep(720.0,60+c*20,0.22f,true);
            score+=(c==1?100: c==2?300: c==3?500:800)*(level+1);
        }
        int lv=lines/LEVEL_STEP; if(lv>level){ level=lv; tick_ms=std::max(TICK_MS_MIN,TICK_MS_START-level*40); }
        newActive(act,nextIdx); nextIdx=drawPieceIdx(); trySpawn();
    };

    auto rotateWithKicks=[&](int dir){ // +1 = CW, -1 = CCW
        int from = act.rot;
        int to   = (from + dir + 4) % 4;
        auto& p  = PIECES[act.idx];

        // 1) SRS por transição (preferência)
        if(p.hasPerTransKicks){
            int dirIdx = (dir>0? 0 : 1); // 0=CW, 1=CCW
            const auto& lst = p.kicksPerTrans[dirIdx][from];
            for(auto [kx,ky] : lst){
                if(!collides(act, grid, kx, ky, dir)){
                    act.x += kx; act.y += ky; act.rot = to; return;
                }
            }
        }

        // 2) Fallback: lista única por direção (legado)
        if(p.hasKicks){
            const auto& lst = (dir>0? p.kicksCW : p.kicksCCW);
            for(auto [kx,ky] : lst){
                if(!collides(act, grid, kx, ky, dir)){
                    act.x += kx; act.y += ky; act.rot = to; return;
                }
            }
        }

        // 3) Fallback simples
        if(!collides(act, grid, 0, 0, dir)) { act.rot = to; return; }
        int sx = (dir>0?1:-1);
        if(!collides(act, grid, sx, 0, dir)) { act.x += sx; act.rot = to; return; }
        if(!collides(act, grid, 0,-1, dir)) { act.y -= 1; act.rot = to; return; }
    };

    while(running){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT) running=false;
            if(e.type==SDL_KEYDOWN){
                SDL_Keycode k=e.key.keysym.sym;
                if(k==SDLK_ESCAPE){ running=false; }
                if(k==SDLK_F12){ const char* p="/home/user/dropblocks-screenshot.bmp"; if(saveScreenshot(ren,p)) playBeep(880.0,80,0.18f,false); }
                if(k==SDLK_p){ paused=!paused; playBeep(paused?440.0:520.0,60,0.15f,false); }
                if(gameover){
                    if(k==SDLK_RETURN){
                        for(auto& r : grid) for(auto& c : r){ c.occ=false; }
                        score=0; lines=0; level=0; tick_ms=TICK_MS_START;
                        gameover=false; paused=false;
                        nextIdx = drawPieceIdx(); newActive(act, nextIdx); nextIdx = drawPieceIdx(); trySpawn();
                        playBeep(520.0,80,0.2f,false);
                    }
                    continue;
                }
                if(paused) continue;
                if(k==SDLK_LEFT && !coll(-1,0,0)) act.x--;
                if(k==SDLK_RIGHT&& !coll( 1,0,0)) act.x++;
                if(k==SDLK_DOWN) softDrop();
                if(k==SDLK_SPACE) hardDrop();
                if(k==SDLK_z || k==SDLK_UP) rotateWithKicks(-1);
                if(k==SDLK_x) rotateWithKicks(+1);
            }
        }
        if(!paused && !gameover){
            Uint32 now=SDL_GetTicks();
            if(now-lastTick >= (Uint32)tick_ms){ softDrop(); lastTick = now; }
        }

        // Proporção 3:2 no fullscreen
        SDL_DisplayMode dmNow; SDL_GetCurrentDisplayMode(0,&dmNow);
        int SWr = dmNow.w, SHr = dmNow.h;
        int CW, CH, CX, CY;
        if (SWr * 2 >= SHr * 3) { CH = SHr; CW = (CH * 3) / 2; CX = (SWr - CW) / 2; CY = 0; }
        else { CW = SWr; CH = (CW * 2) / 3; CX = 0; CY = (SHr - CH) / 2; }

        SDL_SetRenderDrawColor(ren, THEME.bg_r, THEME.bg_g, THEME.bg_b, 255);
        SDL_RenderClear(ren);

        // Layout
        const int scale = HUD_FIXED_SCALE;
        const int GAP1 = BORDER + GAP1_SCALE*scale; // banner ↔ tabuleiro
        const int GAP2 = BORDER + GAP2_SCALE*scale; // tabuleiro ↔ painel

        int bannerW = 8*scale + 24;
        int panelTarget = (int)(CW * 0.28);

        int usableLeftW = CW - (BORDER + bannerW + GAP1) - panelTarget - GAP2;
        int cellBoard = std::min( std::max(8, usableLeftW / COLS), (CH - 2*BORDER) / ROWS );
        int GW = cellBoard * COLS, GH = cellBoard * ROWS;

        // banner
        int BX = CX + BORDER;
        int BY = CY + (CH - GH)/2;
        int BW = bannerW;
        int BH = GH;

        // tabuleiro
        int GX = BX + BW + GAP1;
        int GY = BY;

        // painel
        int panelX = GX + GW + GAP2;
        int panelW = CX + CW - panelX - BORDER;
        int panelY = BY, panelH = GH;

        // Banner
        drawRoundedFilled(ren, BX, BY, BW, BH, 10, THEME.banner_bg_r, THEME.banner_bg_g, THEME.banner_bg_b, 255);
        drawRoundedOutline(ren, BX, BY, BW, BH, 10, 2, THEME.banner_outline_r, THEME.banner_outline_g, THEME.banner_outline_b, THEME.banner_outline_a);

        // Título vertical
        {
            int bty = BY + 10;
            int cxText = BX + (BW - 5*scale)/2;
            for(size_t i=0;i<TITLE_TEXT.size();++i){
                char ch = TITLE_TEXT[i];
                if(ch==' ') { bty += 6*scale; continue; }
                ch = (char)std::toupper((unsigned char)ch);
                if (ch<'A' || ch>'Z') ch = ' ';
                drawPixelText(ren, cxText, bty, std::string(1, ch), scale, THEME.banner_text_r, THEME.banner_text_g, THEME.banner_text_b);
                bty += 9*scale;
            }
        }

        // Sweep local do banner
        if(ENABLE_BANNER_SWEEP){
            SDL_Rect clip{BX, BY, BW, BH};
            SDL_RenderSetClipRect(ren, &clip);
            int bandH = SWEEP_BAND_H_S*scale;
            int total = BH + bandH;
            float tsec = SDL_GetTicks()/1000.0f;
            int sweepY = (int)std::fmod(tsec * SWEEP_SPEED_PXPS, (float)total) - bandH;
            for (int i = 0; i < bandH; ++i) {
                float base = 0.5f * (1.0f + std::cos(((i - bandH/2.0f) * (float)M_PI) / bandH));
                float k = std::pow(base, SWEEP_SOFTNESS);
                Uint8 a = (Uint8)std::round(SWEEP_ALPHA_MAX * k);
                SDL_SetRenderDrawColor(ren, THEME.banner_text_r, THEME.banner_text_g, THEME.banner_text_b, a);
                SDL_Rect line{ BX, BY + sweepY + i, BW, 1 };
                SDL_RenderFillRect(ren, &line);
            }
            SDL_RenderSetClipRect(ren, nullptr);
        }

        // Painel (HUD)
        drawRoundedFilled(ren, panelX, panelY, panelW, panelH, 12, THEME.panel_fill_r, THEME.panel_fill_g, THEME.panel_fill_b, 255);
        drawRoundedOutline(ren, panelX, panelY, panelW, panelH, 12, 2, THEME.panel_outline_r, THEME.panel_outline_g, THEME.panel_outline_b, THEME.panel_outline_a);

        // Tabuleiro
        for(int y=0;y<ROWS;y++) for(int x=0;x<COLS;x++){
            SDL_Rect r{GX+x*cellBoard, GY+y*cellBoard, cellBoard-1, cellBoard-1};
            if(grid[y][x].occ) SDL_SetRenderDrawColor(ren, grid[y][x].r, grid[y][x].g, grid[y][x].b,255);
            else SDL_SetRenderDrawColor(ren, THEME.board_empty_r, THEME.board_empty_g, THEME.board_empty_b,255);
            SDL_RenderFillRect(ren, &r);
        }
        // peça ativa
        {
            auto &pc = PIECES[act.idx];
            for(auto [px,py] : pc.rot[act.rot]){
                SDL_Rect r{GX+(act.x+px)*cellBoard, GY+(act.y+py)*cellBoard, cellBoard-1, cellBoard-1};
                SDL_SetRenderDrawColor(ren, pc.r, pc.g, pc.b,255); SDL_RenderFillRect(ren, &r);
            }
        }

        // HUD textos
        int tx = panelX + 14, ty = panelY + 14;
        drawPixelText(ren, tx, ty, "SCORE", scale, THEME.hud_label_r, THEME.hud_label_g, THEME.hud_label_b); ty += 10*scale;
        drawPixelText(ren, tx, ty, fmtScore(score), scale+1, THEME.hud_score_r, THEME.hud_score_g, THEME.hud_score_b); ty += 12*(scale+1);

        drawPixelText(ren, tx, ty, "LINES", scale, THEME.hud_label_r, THEME.hud_label_g, THEME.hud_label_b); ty += 8*scale;
        drawPixelText(ren, tx, ty, std::to_string(lines), scale, THEME.hud_lines_r, THEME.hud_lines_g, THEME.hud_lines_b); ty += 10*scale;

        drawPixelText(ren, tx, ty, "LEVEL", scale, THEME.hud_label_r, THEME.hud_label_g, THEME.hud_label_b); ty += 8*scale;
        drawPixelText(ren, tx, ty, std::to_string(level), scale, THEME.hud_level_r, THEME.hud_level_g, THEME.hud_level_b); ty += 10*scale;

        // NEXT (quadro PREVIEW_GRID × PREVIEW_GRID)
        int boxW = panelW - 28;
        int boxH = std::min(panelH - (ty - panelY) - 14, boxW);
        int boxX = panelX + 14;
        int boxY = ty;

        drawRoundedFilled(ren, boxX, boxY, boxW, boxH, 10, THEME.next_fill_r, THEME.next_fill_g, THEME.next_fill_b, 255);
        drawRoundedOutline(ren, boxX, boxY, boxW, boxH, 10, 2, THEME.next_outline_r, THEME.next_outline_g, THEME.next_outline_b, THEME.next_outline_a);

        drawPixelText(ren, boxX + 10, boxY + 10, "NEXT", scale, THEME.next_label_r, THEME.next_label_g, THEME.next_label_b);

        int padIn = 14 + 4*scale;
        int innerX = boxX + 10, innerY = boxY + padIn;
        int innerW = boxW - 20, innerH = boxH - padIn - 10;

        int gridCols = PREVIEW_GRID, gridRows = PREVIEW_GRID;
        int cellMini = std::min(innerW / gridCols, innerH / gridRows);
        if(cellMini < 1) cellMini = 1;
        if(cellMini > cellBoard) cellMini = cellBoard;

        int gridW = cellMini * gridCols, gridH = cellMini * gridRows;
        int gridX = innerX + (innerW - gridW)/2;
        int gridY = innerY + (innerH - gridH)/2;

        // quadriculado
        for(int gy=0; gy<gridRows; ++gy){
            for(int gx=0; gx<gridCols; ++gx){
                SDL_Rect q{gridX + gx*cellMini, gridY + gy*cellMini, cellMini-1, cellMini-1};
                bool isLight = ((gx+gy)&1) != 0;
                if (THEME.next_grid_use_rgb) {
                    if (isLight)
                        SDL_SetRenderDrawColor(ren, THEME.next_grid_light_r, THEME.next_grid_light_g, THEME.next_grid_light_b, 255);
                    else
                        SDL_SetRenderDrawColor(ren, THEME.next_grid_dark_r, THEME.next_grid_dark_g, THEME.next_grid_dark_b, 255);
                } else {
                    Uint8 v = isLight ? THEME.next_grid_light : THEME.next_grid_dark;
                    SDL_SetRenderDrawColor(ren, v, v, v, 255);
                }
                SDL_RenderFillRect(ren, &q);
            }
        }

        // peça próxima centrada
        {
            auto &np = PIECES[nextIdx];
            int minx=999, maxx=-999, miny=999, maxy=-999;
            for(auto [px,py] : np.rot[0]){ minx=std::min(minx,px); maxx=std::max(maxx,px); miny=std::min(miny,py); maxy=std::max(maxy,py); }
            int pw = (maxx - minx + 1), ph = (maxy - miny + 1);
            int offX = (gridCols - pw)/2 - minx;
            int offY = (gridRows - ph)/2 - miny;
            for(auto [px,py] : np.rot[0]){
                SDL_Rect r{gridX + (px+offX)*cellMini, gridY + (py+offY)*cellMini, cellMini-1, cellMini-1};
                SDL_SetRenderDrawColor(ren, np.r, np.g, np.b,255); SDL_RenderFillRect(ren, &r);
            }
        }

        // Overlay (PAUSE / GAME OVER)
        if(gameover || paused){
            const std::string topText = paused ? "PAUSE" : "GAME OVER";
            const std::string subText = paused ? "" : "PRESS START";

            int topW = textWidthPx(topText, scale+2);
            int subW = subText.empty()? 0 : textWidthPx(subText, scale);
            int textW = std::max(topW, subW);

            int padX = 24, padY = 20;
            int textH = 7*(scale+2) + (subText.empty()?0:(8*scale + 7*scale));
            int ow = textW + padX*2;
            int oh = textH + padY*2;

            int ox = GX + (GW - ow)/2;
            int oy = GY + (GH - oh)/2;

            drawRoundedFilled(ren, ox, oy, ow, oh, 14, THEME.overlay_fill_r, THEME.overlay_fill_g, THEME.overlay_fill_b, THEME.overlay_fill_a);
            drawRoundedOutline(ren, ox, oy, ow, oh, 14, 2, THEME.overlay_outline_r, THEME.overlay_outline_g, THEME.overlay_outline_b, THEME.overlay_outline_a);

            int txc = ox + (ow - topW)/2;
            int tyc = oy + padY;
            drawPixelTextOutlined(ren, txc, tyc, topText, scale+2, THEME.overlay_top_r, THEME.overlay_top_g, THEME.overlay_top_b, 0,0,0);

            if(!subText.empty()){
                int sx = ox + (ow - subW)/2;
                int sy = tyc + 7*(scale+2) + 8*scale;
                drawPixelTextOutlined(ren, sx, sy, subText, scale, THEME.overlay_sub_r, THEME.overlay_sub_g, THEME.overlay_sub_b, 0,0,0);
            }
        }

        // Scanlines
        if(SCANLINE_ALPHA > 0){
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 0,0,0, (Uint8)SCANLINE_ALPHA);
            for(int y=0; y<SHr; y+=2){ SDL_Rect sl{0,y,SWr,1}; SDL_RenderFillRect(ren, &sl); }
        }

        // SWEEP GLOBAL (clareia)
        if(ENABLE_GLOBAL_SWEEP){
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
            float tsec = SDL_GetTicks()/1000.0f;
            int bandH = SWEEP_G_BAND_H_PX;
            int total = SHr + bandH;
            int sweepY = (int)std::fmod(tsec * SWEEP_G_SPEED_PXPS, (float)total) - bandH;
            for (int i = 0; i < bandH; ++i) {
                float base = 0.5f * (1.0f + std::cos(((i - bandH/2.0f) * (float)M_PI) / bandH));
                float k = std::pow(base, SWEEP_G_SOFTNESS);
                Uint8 a = (Uint8)std::round(SWEEP_G_ALPHA_MAX * k);
                SDL_SetRenderDrawColor(ren, 255,255,255, a);
                SDL_Rect line{ 0, sweepY + i, SWr, 1 };
                SDL_RenderFillRect(ren, &line);
            }
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(1);
    }

    // Cleanup
    // (SDL_Destroy* / CloseAudio)
    // Obs.: encerramos com o OS ao finalizar o processo, mas:
    // boa prática:
    // - SDL_CloseAudioDevice(gAudioDev);
    // - SDL_DestroyRenderer(ren);
    // - SDL_DestroyWindow(win);
    if(gAudioDev) SDL_CloseAudioDevice(gAudioDev);
    SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}
