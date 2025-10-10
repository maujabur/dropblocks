/**
 * @file dropblocks.cpp
 * @brief DropBlocks - SDL2 single-file falling blocks game (not Tetris®)
 * @author DropBlocks Team
 * @version 1.0
 * @date 2025
 * 
 * A modern, customizable falling blocks game inspired by Tetris®.
 * Features advanced visual effects, customizable themes, and multiple piece sets.
 * 
 * @section controls Controls
 * - ← → : Move piece left/right
 * - ↓ : Soft drop
 * - Z/↑ : Rotate counter-clockwise
 * - X : Rotate clockwise
 * - SPACE : Hard drop
 * - P : Pause
 * - ENTER : Restart (after Game Over)
 * - ESC : Quit
 * - F12 : Screenshot
 * 
 * @section build Build
 * g++ dropblocks.cpp -o dropblocks `sdl2-config --cflags --libs` -O2
 * 
 * @section dependencies Dependencies
 * - SDL2 (graphics and audio)
 * - C++17 compatible compiler
 */

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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// ===========================
//   PARÂMETROS & THEME
// ===========================

static int   ROUNDED_PANELS = 1;           // 1 = arredondado; 0 = retângulo
static int   HUD_FIXED_SCALE   = 6;        // escala fixa do HUD
static std::string TITLE_TEXT  = "---H A C K T R I S";// texto vertical (A–Z e espaço)
static int   GAP1_SCALE        = 10;       // banner ↔ tabuleiro (x scale)
static int   GAP2_SCALE        = 10;       // tabuleiro ↔ painel  (x scale)

static bool  ENABLE_BANNER_SWEEP = true;   // sweep local no banner
static bool  ENABLE_GLOBAL_SWEEP = true;   // sweep global (clareia a tela)

static float SWEEP_SPEED_PXPS  = 15.0f;    // px/s (banner)
static int   SWEEP_BAND_H_S    = 30;       // altura do banner sweep (em scale)
static int   SWEEP_ALPHA_MAX   = 100;      // 0..255 (banner)
static float SWEEP_SOFTNESS    = 0.7f;     // <1 = mais suave

static float SWEEP_G_SPEED_PXPS = 20.0f;   // px/s (global)
static int   SWEEP_G_BAND_H_PX  = 100;     // px
static int   SWEEP_G_ALPHA_MAX  = 50;      // 0..255
static float SWEEP_G_SOFTNESS   = 0.9f;    // suavidade

static int   SCANLINE_ALPHA     = 20;      // 0..255 (0 desativa)

// Caminho opcional indicado no cfg para o arquivo de peças
static std::string PIECES_FILE_PATH = "";

// Config do set de peças
static int PREVIEW_GRID = 6;               // NxN no NEXT (padrão 6)

/**
 * @brief Randomizer type enumeration
 * 
 * Defines the different piece randomization algorithms available.
 */
enum class RandType { 
    SIMPLE,  /**< Simple random selection */
    BAG      /**< Bag-based randomizer (7-bag system) */
};
static RandType RAND_TYPE = RandType::SIMPLE;
static int RAND_BAG_SIZE  = 0;             // 0 => tamanho do set

/**
 * @brief RGB color structure
 */
struct RGB { Uint8 r,g,b; };

/**
 * @brief Theme configuration structure
 * 
 * Contains all visual theme settings including colors, transparency values,
 * and visual effect parameters for customizing the game's appearance.
 */
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

    // cores das peças (array dinâmico, sem limitação)
    std::vector<RGB> piece_colors;
} THEME;

// ===========================
//   MECÂNICA / ESTRUTURAS
// ===========================

/** @brief Number of columns in the game board */
static const int COLS = 10;
/** @brief Number of rows in the game board */
static const int ROWS = 20;
/** @brief Border size around the game board */
static const int BORDER = 10;
/** @brief Initial game tick interval in milliseconds */
static const int TICK_MS_START = 600;
/** @brief Minimum game tick interval in milliseconds */
static const int TICK_MS_MIN   = 120;
/** @brief Lines required to advance to next level */
static const int LEVEL_STEP    = 10;

/**
 * @brief Game board cell structure
 * 
 * Represents a single cell on the game board with color and occupancy information.
 */
struct Cell { Uint8 r,g,b; bool occ=false; };

/**
 * @brief Tetris piece structure
 * 
 * Contains all information about a tetris piece including its rotations,
 * kick data for SRS (Super Rotation System), and visual properties.
 */
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

/**
 * @brief Active piece structure
 * 
 * Represents the currently falling piece with its position, rotation, and piece index.
 */
struct Active { int x=COLS/2, y=0, rot=0, idx=0; };

// ===========================
//   UTILS: STR / CORES / PARSING
// ===========================

/**
 * @brief Trim whitespace from string
 * 
 * Removes leading and trailing whitespace characters from a string.
 * 
 * @param s Reference to string to trim (modified in place)
 */
static void trim(std::string& s){
    size_t a=s.find_first_not_of(" \t\r\n");
    size_t b=s.find_last_not_of(" \t\r\n");
    if(a==std::string::npos){ s.clear(); return; }
    s=s.substr(a,b-a+1);
}
/**
 * @brief Parse hexadecimal color string
 * 
 * Parses a hexadecimal color string in format #RRGGBB or RRGGBB
 * and extracts the RGB components.
 * 
 * @param s Input color string (e.g., "#FF0000" or "FF0000")
 * @param r Reference to store red component (0-255)
 * @param g Reference to store green component (0-255)
 * @param b Reference to store blue component (0-255)
 * @return true if parsing successful, false otherwise
 */
static bool parseHexColor(const std::string& s, Uint8& r, Uint8& g, Uint8& b){
    // Aceita tanto #RRGGBB quanto RRGGBB
    std::string color = s;
    if (color.size() == 6 && color[0] != '#') {
        // Adiciona # se não tiver
        color = "#" + color;
    }
    if (color.size()!=7 || color[0]!='#') return false;
    auto cv=[&](char c)->int{
        if(c>='0'&&c<='9') return c-'0';
        c=(char)std::toupper((unsigned char)c);
        if(c>='A'&&c<='F') return 10+(c-'A');
        return -1;
    };
    auto hx=[&](char a,char b){int A=cv(a),B=cv(b); return (A<0||B<0)?-1:(A*16+B);};
    int R=hx(color[1],color[2]), G=hx(color[3],color[4]), B=hx(color[5],color[6]);
    if(R<0||G<0||B<0) return false; r=(Uint8)R; g=(Uint8)G; b=(Uint8)B; return true;
}
static bool parseInt(const std::string& s, int& out){
    char* e=nullptr; long v=strtol(s.c_str(), &e, 10);
    if(e==s.c_str()||*e!='\0') return false; out=(int)v; return true;
}
static bool parseCoordList(const std::string& val, std::vector<std::pair<int,int>>& out){
    out.clear();
    
    // Parse coordinates like "(0,0);(1,0);(0,1);(1,1)"
    size_t pos = 0;
    while(pos < val.size()){
        // Skip whitespace
        while(pos < val.size() && (val[pos] == ' ' || val[pos] == '\t')) pos++;
        if(pos >= val.size()) break;
        
        // Look for opening parenthesis
        if(val[pos] != '('){
            pos++;
            continue;
        }
        pos++; // skip '('
        
        // Parse x coordinate
        int x = 0, y = 0;
        int sign = 1;
        if(pos < val.size() && val[pos] == '-'){ sign = -1; pos++; }
        else if(pos < val.size() && val[pos] == '+'){ sign = 1; pos++; }
        
        while(pos < val.size() && std::isdigit(val[pos])){
            x = x * 10 + (val[pos] - '0');
            pos++;
        }
        x *= sign;
        
        // Skip comma
        if(pos < val.size() && val[pos] == ',') pos++;
        
        // Parse y coordinate
        sign = 1;
        if(pos < val.size() && val[pos] == '-'){ sign = -1; pos++; }
        else if(pos < val.size() && val[pos] == '+'){ sign = 1; pos++; }
        
        while(pos < val.size() && std::isdigit(val[pos])){
            y = y * 10 + (val[pos] - '0');
            pos++;
        }
        y *= sign;
        
        // Skip closing parenthesis
        if(pos < val.size() && val[pos] == ')') pos++;
        
        // Add coordinate
        out.push_back({x, y});
        
        // Skip semicolon
        if(pos < val.size() && val[pos] == ';') pos++;
    }
    
    return !out.empty();
}
static bool parseKicks(const std::string& v, std::vector<std::pair<int,int>>& out){ return parseCoordList(v,out); }
static void rotate90(std::vector<std::pair<int,int>>& pts){
    for(auto& p:pts){ int x=p.first,y=p.second; p.first=-y; p.second=x; }
}

// ===========================
//   CONFIGURAÇÃO E CARREGAMENTO
// ===========================

// Funções auxiliares para parsing de configuração
static std::string parseConfigLine(const std::string& line) {
    // Comentários: ; sempre; # só se vier antes do '='
    size_t eq_probe = line.find('=');
    size_t semicol = line.find(';');
    size_t hash = line.find('#');
    size_t cut = std::string::npos;
    
    if (semicol != std::string::npos) cut = semicol;
    if (hash != std::string::npos && (eq_probe == std::string::npos || hash < eq_probe))
        cut = (cut == std::string::npos ? hash : std::min(cut, hash));
    if (cut != std::string::npos) {
        std::string result = line;
        result.resize(cut);
        return result;
    }
    return line;
}

static bool processBasicConfigs(const std::string& key, const std::string& val, int& processedLines) {
    auto setb = [&](const char* K, bool& ref) {
        if (key == K) { 
            std::string v = val; 
            for (char& c : v) c = (char)std::tolower((unsigned char)c);
            ref = (v == "1" || v == "true" || v == "on" || v == "yes"); 
            return true; 
        } 
        return false; 
    };
    auto seti = [&](const char* K, int& ref) { 
        if (key == K) { ref = std::atoi(val.c_str()); return true; } 
        return false; 
    };
    auto setf = [&](const char* K, float& ref) { 
        if (key == K) { ref = (float)std::atof(val.c_str()); return true; } 
        return false; 
    };

    if (setb("ENABLE_BANNER_SWEEP", ENABLE_BANNER_SWEEP)) { processedLines++; return true; }
    if (setb("ENABLE_GLOBAL_SWEEP", ENABLE_GLOBAL_SWEEP)) { processedLines++; return true; }
    if (seti("ROUNDED_PANELS", ROUNDED_PANELS)) { processedLines++; return true; }
    if (seti("HUD_FIXED_SCALE", HUD_FIXED_SCALE)) { processedLines++; return true; }
    if (seti("GAP1_SCALE", GAP1_SCALE)) { processedLines++; return true; }
    if (seti("GAP2_SCALE", GAP2_SCALE)) { processedLines++; return true; }
    if (seti("SWEEP_BAND_H_S", SWEEP_BAND_H_S)) { processedLines++; return true; }
    if (seti("SWEEP_ALPHA_MAX", SWEEP_ALPHA_MAX)) { processedLines++; return true; }
    if (seti("SWEEP_G_BAND_H_PX", SWEEP_G_BAND_H_PX)) { processedLines++; return true; }
    if (seti("SWEEP_G_ALPHA_MAX", SWEEP_G_ALPHA_MAX)) { processedLines++; return true; }
    if (seti("SCANLINE_ALPHA", SCANLINE_ALPHA)) { processedLines++; return true; }
    if (setf("SWEEP_SPEED_PXPS", SWEEP_SPEED_PXPS)) { processedLines++; return true; }
    if (setf("SWEEP_SOFTNESS", SWEEP_SOFTNESS)) { processedLines++; return true; }
    if (setf("SWEEP_G_SPEED_PXPS", SWEEP_G_SPEED_PXPS)) { processedLines++; return true; }
    if (setf("SWEEP_G_SOFTNESS", SWEEP_G_SOFTNESS)) { processedLines++; return true; }
    
    return false;
}

static bool processThemeColors(const std::string& key, const std::string& val, int& processedLines) {
    auto setrgb = [&](const char* K, Uint8& R, Uint8& G, Uint8& B) {
        if (key == K) { 
            Uint8 r, g, b; 
            if (parseHexColor(val, r, g, b)) { 
                R = r; G = g; B = b; 
                return true; 
            } 
        } 
        return false; 
    };
    auto seta = [&](const char* K, Uint8& ref) {
        if (key == K) { 
            int v = std::atoi(val.c_str()); 
            if (v < 0) v = 0; 
            if (v > 255) v = 255; 
            ref = (Uint8)v; 
            return true; 
        } 
        return false; 
    };

    // Cores básicas
    if (setrgb("BG", THEME.bg_r, THEME.bg_g, THEME.bg_b)) { processedLines++; return true; }
    if (setrgb("BOARD_EMPTY", THEME.board_empty_r, THEME.board_empty_g, THEME.board_empty_b)) { processedLines++; return true; }
    if (setrgb("PANEL_FILL", THEME.panel_fill_r, THEME.panel_fill_g, THEME.panel_fill_b)) { processedLines++; return true; }
    if (setrgb("PANEL_OUTLINE", THEME.panel_outline_r, THEME.panel_outline_g, THEME.panel_outline_b)) { processedLines++; return true; }
    if (setrgb("BANNER_BG", THEME.banner_bg_r, THEME.banner_bg_g, THEME.banner_bg_b)) { processedLines++; return true; }
    if (setrgb("BANNER_OUTLINE", THEME.banner_outline_r, THEME.banner_outline_g, THEME.banner_outline_b)) { processedLines++; return true; }
    if (setrgb("BANNER_TEXT", THEME.banner_text_r, THEME.banner_text_g, THEME.banner_text_b)) { processedLines++; return true; }

    // Cores HUD
    if (setrgb("HUD_LABEL", THEME.hud_label_r, THEME.hud_label_g, THEME.hud_label_b)) { processedLines++; return true; }
    if (setrgb("HUD_SCORE", THEME.hud_score_r, THEME.hud_score_g, THEME.hud_score_b)) { processedLines++; return true; }
    if (setrgb("HUD_LINES", THEME.hud_lines_r, THEME.hud_lines_g, THEME.hud_lines_b)) { processedLines++; return true; }
    if (setrgb("HUD_LEVEL", THEME.hud_level_r, THEME.hud_level_g, THEME.hud_level_b)) { processedLines++; return true; }

    // Cores NEXT
    if (setrgb("NEXT_FILL", THEME.next_fill_r, THEME.next_fill_g, THEME.next_fill_b)) { processedLines++; return true; }
    if (setrgb("NEXT_OUTLINE", THEME.next_outline_r, THEME.next_outline_g, THEME.next_outline_b)) { processedLines++; return true; }
    if (setrgb("NEXT_LABEL", THEME.next_label_r, THEME.next_label_g, THEME.next_label_b)) { processedLines++; return true; }

    // Cores OVERLAY
    if (setrgb("OVERLAY_FILL", THEME.overlay_fill_r, THEME.overlay_fill_g, THEME.overlay_fill_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_OUTLINE", THEME.overlay_outline_r, THEME.overlay_outline_g, THEME.overlay_outline_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_TOP", THEME.overlay_top_r, THEME.overlay_top_g, THEME.overlay_top_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_SUB", THEME.overlay_sub_r, THEME.overlay_sub_g, THEME.overlay_sub_b)) { processedLines++; return true; }

    // Alpha values
    if (seta("PANEL_OUTLINE_A", THEME.panel_outline_a)) { processedLines++; return true; }
    if (seta("NEXT_OUTLINE_A", THEME.next_outline_a)) { processedLines++; return true; }
    if (seta("OVERLAY_FILL_A", THEME.overlay_fill_a)) { processedLines++; return true; }
    if (seta("OVERLAY_OUTLINE_A", THEME.overlay_outline_a)) { processedLines++; return true; }

    return false;
}

// Declaração forward para AudioSystem
struct AudioSystem;

static bool processAudioConfigs(const std::string& key, const std::string& val, int& processedLines, AudioSystem& audio);

static bool processSpecialConfigs(const std::string& key, const std::string& val, int& processedLines) {
    // Configurações especiais
    if (key == "TITLE_TEXT") { TITLE_TEXT = val; processedLines++; return true; }
    if (key == "PIECES_FILE") { PIECES_FILE_PATH = val; processedLines++; return true; }
    
    // Grid colors
    if (key == "NEXT_GRID_DARK") { 
        *(int*)&THEME.next_grid_dark = std::atoi(val.c_str()); 
        processedLines++; 
        return true; 
    }
    if (key == "NEXT_GRID_LIGHT") { 
        *(int*)&THEME.next_grid_light = std::atoi(val.c_str()); 
        processedLines++; 
        return true; 
    }
    
    // Grid colors RGB
    if (key == "NEXT_GRID_DARK_COLOR") {
        Uint8 r, g, b;
        if (parseHexColor(val, r, g, b)) {
            THEME.next_grid_dark_r = r; THEME.next_grid_dark_g = g; THEME.next_grid_dark_b = b;
            THEME.next_grid_use_rgb = true;
            processedLines++;
        }
        return true;
    }
    if (key == "NEXT_GRID_LIGHT_COLOR") {
        Uint8 r, g, b;
        if (parseHexColor(val, r, g, b)) {
            THEME.next_grid_light_r = r; THEME.next_grid_light_g = g; THEME.next_grid_light_b = b;
            THEME.next_grid_use_rgb = true;
            processedLines++;
        }
        return true;
    }

    // Cores das peças (PIECE0, PIECE1, etc.)
    if (key.rfind("PIECE", 0) == 0) {
        std::string numStr = key.substr(5);
        int pieceIndex = -1;
        try {
            pieceIndex = std::stoi(numStr);
        } catch (...) {
            return false;
        }
        
        Uint8 r, g, b;
        if (parseHexColor(val, r, g, b)) {
            if (pieceIndex >= (int)THEME.piece_colors.size()) {
                THEME.piece_colors.resize(pieceIndex + 1, {200, 200, 200});
            }
            THEME.piece_colors[pieceIndex] = {r, g, b};
            processedLines++;
        }
        return true;
    }
    
    return false;
}

static void loadConfigFromStream(std::istream& in, AudioSystem& audio) {
    std::string line;
    int lineNum = 0;
    int processedLines = 0;
    int skippedLines = 0;
    
    while (std::getline(in, line)) {
        lineNum++;
        
        // Parse da linha (remove comentários)
        line = parseConfigLine(line);
        trim(line);
        
        if (line.empty()) {
            skippedLines++;
            continue;
        }
        
        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            skippedLines++;
            continue;
        }
        
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        trim(key);
        trim(val);
        
        if (key.empty()) {
            skippedLines++;
            continue;
        }

        std::string KEY = key;
        for (char& c : KEY) c = (char)std::toupper((unsigned char)c);

        // Processar usando funções especializadas
        if (processBasicConfigs(KEY, val, processedLines)) continue;
        if (processThemeColors(KEY, val, processedLines)) continue;
        if (processAudioConfigs(KEY, val, processedLines, audio)) continue;
        if (processSpecialConfigs(KEY, val, processedLines)) continue;
        
        // Linha não reconhecida
        skippedLines++;
    }
}
static bool loadConfigPath(const std::string& p, AudioSystem& audio){
    std::ifstream f(p.c_str()); if(f.good()){ loadConfigFromStream(f, audio); return true; } return false;
}
/**
 * @brief Load configuration file
 * 
 * Attempts to load configuration from multiple sources in order:
 * 1. Environment variable DROPBLOCKS_CFG
 * 2. default.cfg
 * 3. dropblocks.cfg
 * 4. Command line arguments
 * 
 * @param audio Audio system reference for audio configuration
 */
static void loadConfigFile(AudioSystem& audio){
    if(const char* env = std::getenv("DROPBLOCKS_CFG")){ 
        if(loadConfigPath(env, audio)) { 
            printf("INFO: Config carregado de: %s\n", env); 
            return; 
        } 
    }
    if(loadConfigPath("default.cfg", audio)) { 
        printf("INFO: Config carregado de: default.cfg\n"); 
        return; 
    }
    if(loadConfigPath("dropblocks.cfg", audio)) { 
        printf("INFO: Config carregado de: dropblocks.cfg\n"); 
        return; 
    } // fallback para compatibilidade
    if(const char* home = std::getenv("HOME")){
        std::string p = std::string(home) + "/.config/default.cfg";
        if(loadConfigPath(p, audio)) { 
            printf("INFO: Config carregado de: %s\n", p.c_str()); 
            return; 
        }
        std::string p2 = std::string(home) + "/.config/dropblocks.cfg";
        if(loadConfigPath(p2, audio)) { 
            printf("INFO: Config carregado de: %s\n", p2.c_str()); 
            return; 
        }
    }
    printf("INFO: Nenhum config encontrado; usando padrões.\n");
}



// Funções auxiliares para parsing de peças
static std::string parsePiecesLine(const std::string& line) {
    size_t semi = line.find(';');
    size_t cut = std::string::npos;
    
    // Cut at ; only if it's at the start of line (comment) or if it's after = but before coordinates
    if (semi != std::string::npos) {
        // If ; is at start of line, it's a comment
        if (semi == 0 || (semi > 0 && line[semi-1] == ' ')) {
            cut = semi;
        }
        // If ; is after =, check if it's before coordinates (has parentheses)
        else {
            size_t eq_probe = line.find('=');
            if (eq_probe != std::string::npos && semi > eq_probe) {
                // Look for parentheses after the ; to see if it's coordinate separation
                size_t paren_after_semi = line.find('(', semi);
                if (paren_after_semi == std::string::npos) {
                    // No parentheses after ;, so it's a comment
                    cut = semi;
                }
                // If there are parentheses after ;, it's coordinate separation, don't cut
            }
        }
    }
    
    if (cut != std::string::npos) {
        std::string result = line;
        result.resize(cut);
        return result;
    }
    return line;
}

static void buildPieceRotations(Piece& piece, const std::vector<std::pair<int,int>>& base, 
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
            rotate90(r1);
            r2 = r1; rotate90(r2);
            r3 = r2; rotate90(r3);
            piece.rot.push_back(r0);
            piece.rot.push_back(r1);
            piece.rot.push_back(r2);
            piece.rot.push_back(r3);
        }
    }
}

static bool processPieceProperty(Piece& cur, const std::string& key, const std::string& val,
                                std::vector<std::pair<int,int>>& base,
                                std::vector<std::pair<int,int>>& rot0,
                                std::vector<std::pair<int,int>>& rot1,
                                std::vector<std::pair<int,int>>& rot2,
                                std::vector<std::pair<int,int>>& rot3,
                                bool& rotExplicit) {
    if (key == "COLOR") { 
        Uint8 r, g, b; 
        if (parseHexColor(val, r, g, b)) { 
            cur.r = r; cur.g = g; cur.b = b; 
        } 
        return true; 
    }
    if (key == "ROTATIONS") { 
        std::string vv = val; 
        for (char& c : vv) c = (char)std::tolower((unsigned char)c); 
        rotExplicit = (vv == "explicit"); 
        return true; 
    }
    if (key == "BASE") { 
        parseCoordList(val, base); 
        return true; 
    }
    if (key == "ROT0") { 
        if (val.rfind("sameas:", 0) == 0) { /* usa rot0 */ } 
        else parseCoordList(val, rot0); 
        rotExplicit = true; 
        return true; 
    }
    if (key == "ROT1") { 
        if (val.rfind("sameas:", 0) == 0) { rot1 = rot0; } 
        else parseCoordList(val, rot1); 
        rotExplicit = true; 
        return true; 
    }
    if (key == "ROT2") { 
        if (val.rfind("sameas:", 0) == 0) { rot2 = rot0; } 
        else parseCoordList(val, rot2); 
        rotExplicit = true; 
        return true; 
    }
    if (key == "ROT3") { 
        if (val.rfind("sameas:", 0) == 0) { rot3 = rot1.empty() ? rot0 : rot1; } 
        else parseCoordList(val, rot3); 
        rotExplicit = true; 
        return true; 
    }
    if (key == "KICKS.CW") { 
        parseKicks(val, cur.kicksCW); 
        cur.hasKicks = true; 
        return true; 
    }
    if (key == "KICKS.CCW") { 
        parseKicks(val, cur.kicksCCW); 
        cur.hasKicks = true; 
        return true; 
    }
    
    auto setKPT = [&](int dirIdx, int fromState, const std::string& val) {
        std::vector<std::pair<int,int>> tmp;
        if (parseCoordList(val, tmp)) {
            cur.kicksPerTrans[dirIdx][fromState] = tmp;
            cur.hasPerTransKicks = true;
            return true;
        }
        return false;
    };

    // KICKS.CW.0TO1 / 1TO2 / 2TO3 / 3TO0
    if (key.rfind("KICKS.CW.", 0) == 0) {
        std::string t = key.substr(10); // depois de "KICKS.CW."
        if (t == "0TO1") { setKPT(0, 0, val); return true; }
        if (t == "1TO2") { setKPT(0, 1, val); return true; }
        if (t == "2TO3") { setKPT(0, 2, val); return true; }
        if (t == "3TO0") { setKPT(0, 3, val); return true; }
    }
    // KICKS.CCW.0TO3 / 3TO2 / 2TO1 / 1TO0
    if (key.rfind("KICKS.CCW.", 0) == 0) {
        std::string t = key.substr(11); // depois de "KICKS.CCW."
        if (t == "0TO3") { setKPT(1, 0, val); return true; }
        if (t == "3TO2") { setKPT(1, 3, val); return true; }
        if (t == "2TO1") { setKPT(1, 2, val); return true; }
        if (t == "1TO0") { setKPT(1, 1, val); return true; }
    }
    
    return false;
}

static bool loadPiecesFromStream(std::istream& in) {
    PIECES.clear(); 
    PREVIEW_GRID = 6; 
    RAND_TYPE = RandType::SIMPLE; 
    RAND_BAG_SIZE = 0;

    std::string line, section;
    Piece cur; 
    bool inPiece = false; 
    bool rotExplicit = false;
    std::vector<std::pair<int,int>> rot0, rot1, rot2, rot3, base;

    auto flushPiece = [&]() {
        if (!inPiece) return;
        
        buildPieceRotations(cur, base, rot0, rot1, rot2, rot3, rotExplicit);
        
        if (!cur.rot.empty()) {
            PIECES.push_back(cur);
        }
        
        cur = Piece{}; 
        rotExplicit = false; 
        rot0.clear(); rot1.clear(); rot2.clear(); rot3.clear(); base.clear(); 
        inPiece = false;
    };

    while (std::getline(in, line)) {
        line = parsePiecesLine(line);
        trim(line); 
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            std::string sec = line.substr(1, line.size() - 2);
            std::string SEC = sec; 
            for (char& c : SEC) c = (char)std::toupper((unsigned char)c);
            
            if (SEC.rfind("PIECE.", 0) == 0) {
                flushPiece(); 
                inPiece = true; 
                cur = Piece{}; 
                rotExplicit = false;
                rot0.clear(); rot1.clear(); rot2.clear(); rot3.clear(); base.clear();
                cur.name = sec.substr(6);
            } else {
                flushPiece(); 
                inPiece = false; 
                section = SEC;
            }
            continue;
        }

        size_t eq = line.find('='); 
        if (eq == std::string::npos) continue;
        
        std::string k = line.substr(0, eq), v = line.substr(eq + 1); 
        trim(k); trim(v);
        std::string K = k; 
        for (char& c : K) c = (char)std::toupper((unsigned char)c);

        if (inPiece) {
            if (processPieceProperty(cur, K, v, base, rot0, rot1, rot2, rot3, rotExplicit)) {
                continue;
            }
        } else {
            if (section == "SET") {
                if (K == "NAME") { /* opcional */ continue; }
                if (K == "PREVIEWGRID") { 
                    int n; 
                    if (parseInt(v, n) && n > 0 && n <= 10) PREVIEW_GRID = n; 
                    continue; 
                }
            }
            if (section == "RANDOMIZER") {
                if (K == "TYPE") { 
                    std::string vv = v; 
                    for (char& c : vv) c = (char)std::tolower((unsigned char)c);
                    RAND_TYPE = (vv == "bag" ? RandType::BAG : RandType::SIMPLE); 
                    continue; 
                }
                if (K == "BAGSIZE") { 
                    int n; 
                    if (parseInt(v, n) && n >= 0) RAND_BAG_SIZE = n; 
                    continue; 
                }
            }
        }
    }
    flushPiece();
    return !PIECES.empty();
}

static bool loadPiecesPath(const std::string& p){
    std::ifstream f(p.c_str()); 
    if(!f.good()) {
        return false;
    }
    bool ok=loadPiecesFromStream(f);
    SDL_Log("Pieces carregado de: %s (%s)", p.c_str(), ok?"OK":"vazio/erro");
    return ok;
}
/**
 * @brief Load pieces file
 * 
 * Attempts to load piece definitions from multiple sources in order:
 * 1. PIECES_FILE_PATH from configuration
 * 2. default.pieces
 * 3. tetris_original.pieces
 * Falls back to default piece set if no file is found.
 * 
 * @return true if pieces loaded successfully, false otherwise
 */
static bool loadPiecesFile(){
    if(const char* env = std::getenv("DROPBLOCKS_PIECES")) {
        if(loadPiecesPath(env)) return true;
    }
    if(!PIECES_FILE_PATH.empty()) {
        if(loadPiecesPath(PIECES_FILE_PATH)) return true;
    }
    if(loadPiecesPath("default.pieces")) return true;
    if(const char* home = std::getenv("HOME")){
        std::string p = std::string(home) + "/.config/default.pieces";
        if(loadPiecesPath(p)) return true;
    }
    return false;
}

static void seedPiecesFallback(){
    SDL_Log("Usando fallback interno de peças.");
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

static void initDefaultPieceColors(){
    // Se não há cores definidas, inicializa com cores padrão
    if(THEME.piece_colors.empty()){
        THEME.piece_colors = {
            {220, 80, 80},  // Vermelho
            { 80,180,120},  // Verde
            { 80,120,220},  // Azul
            {220,160, 80},  // Laranja
            {160, 80,220},  // Roxo
            {200,200,200},  // Cinza claro
            {200,200,200},  // Cinza claro
            {200,200,200}   // Cinza claro
        };
    }
    
    // NÃO preenche automaticamente - deixa o CFG controlar quantas cores tem
}

static void applyThemePieceColors(){
    initDefaultPieceColors();
    
    // Aplica cores do tema APENAS para as peças que têm cor definida no CFG
    for (size_t i=0; i<PIECES.size(); ++i){
        if(i < THEME.piece_colors.size()) {
            // Usa cor do tema se disponível (CFG tem prioridade)
            PIECES[i].r = THEME.piece_colors[i].r;
            PIECES[i].g = THEME.piece_colors[i].g;
            PIECES[i].b = THEME.piece_colors[i].b;
        } else {
            // Para peças "extra", verifica se tem cor do arquivo
            if(PIECES[i].r == 0 && PIECES[i].g == 0 && PIECES[i].b == 0) {
                // Se a peça não tem cor definida no arquivo, usa cor padrão
                size_t defaultIndex = i % 8; // Cicla pelas 8 cores padrão
                PIECES[i].r = THEME.piece_colors[defaultIndex].r;
                PIECES[i].g = THEME.piece_colors[defaultIndex].g;
                PIECES[i].b = THEME.piece_colors[defaultIndex].b;
            }
            // Se a peça já tem cor do arquivo, mantém ela (não sobrescreve)
        }
    }
}

// ===========================
//   MECÂNICA DO JOGO
// ===========================
/**
 * @brief Check if piece collides with board or other pieces
 * 
 * Tests if the active piece would collide with the board boundaries or
 * other locked pieces at the specified position and rotation.
 * 
 * @param a Active piece to test
 * @param g Game board grid
 * @param dx X offset to test
 * @param dy Y offset to test
 * @param drot Rotation offset to test
 * @return true if collision detected, false otherwise
 */
static bool collides(const Active& a, const std::vector<std::vector<Cell>>& g, int dx, int dy, int drot){
    int R = (a.rot + drot + 4)%4;
    for (auto [px,py] : PIECES[a.idx].rot[R]) {
        int x = a.x + dx + px, y = a.y + dy + py;
        if (x<0 || x>=COLS || y<0 || y>=ROWS) return true;
        if (g[y][x].occ) return true;
    }
    return false;
}
/**
 * @brief Lock piece to the board
 * 
 * Permanently places the active piece on the board at its current position.
 * 
 * @param a Active piece to lock
 * @param g Game board grid (modified in place)
 */
static void lockPiece(const Active& a, std::vector<std::vector<Cell>>& g){
    auto &pc = PIECES[a.idx];
    for (auto [px,py] : pc.rot[a.rot]) {
        int x=a.x+px, y=a.y+py;
        if (y>=0 && y<ROWS && x>=0 && x<COLS){
            g[y][x].occ=true; g[y][x].r=pc.r; g[y][x].g=pc.g; g[y][x].b=pc.b;
        }
    }
}
/**
 * @brief Clear completed lines
 * 
 * Removes all completed horizontal lines from the board and returns the count.
 * 
 * @param g Game board grid (modified in place)
 * @return Number of lines cleared
 */
static int clearLines(std::vector<std::vector<Cell>>& g){
    int cleared=0;
    for (int y=ROWS-1; y>=0; --y){
        bool full=true; for (int x=0;x<COLS;x++) if(!g[y][x].occ){ full=false; break; }
        if (full){ cleared++; for (int yy=y; yy>0; --yy) g[yy]=g[yy-1]; g[0]=std::vector<Cell>(COLS); y++; }
    }
    return cleared;
}
static void newActive(Active& a, int idx){ a.idx=idx; a.rot=0; a.x=COLS/2; a.y=0; }
// Declaração forward para rotateWithKicks
static void rotateWithKicks(Active& act, const std::vector<std::vector<Cell>>& grid, int dir, AudioSystem& audio);

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
//   RENDERIZAÇÃO E INTERFACE
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
//   UTILITÁRIOS
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
//   SISTEMA DE ÁUDIO
// ===========================

// Estrutura para sistema de áudio expandido
struct AudioSystem {
    SDL_AudioDeviceID device = 0;
    SDL_AudioSpec spec;
    
    // Configurações de áudio
    float masterVolume = 1.0f;
    float sfxVolume = 0.6f;
    float ambientVolume = 0.3f;
    bool enableMovementSounds = true;
    bool enableAmbientSounds = true;
    bool enableComboSounds = true;
    bool enableLevelUpSounds = true;
    
    // Estado para sons ambientais
    Uint32 lastSweepSound = 0;
    Uint32 lastScanlineSound = 0;
    Uint32 lastMelody = 0;
    Uint32 lastTension = 0;
    
    bool initialize() {
        spec.freq = 44100; 
        spec.format = AUDIO_F32SYS; 
        spec.channels = 1; 
        spec.samples = 1024;
        device = SDL_OpenAudioDevice(nullptr, 0, &spec, &spec, 0);
        if (device) SDL_PauseAudioDevice(device, 0);
        return device != 0;
    }
    
    void playBeep(double freq, int ms, float vol = 0.25f, bool square = true) {
        if (!device) return;
        int N = (int)(spec.freq * (ms / 1000.0));
        std::vector<float> buf(N);
        double ph = 0, st = 2.0 * M_PI * freq / spec.freq;
        for (int i = 0; i < N; i++) {
            float s = square ? (std::sin(ph) >= 0 ? 1.f : -1.f) : (float)std::sin(ph);
            buf[i] = s * vol * masterVolume; 
            ph += st; 
            if (ph > 2 * M_PI) ph -= 2 * M_PI;
        }
        SDL_QueueAudio(device, buf.data(), (Uint32)(buf.size() * sizeof(float)));
    }
    
    // Novos métodos de síntese
    void playChord(double baseFreq, int notes[], int count, int ms, float vol = 0.15f) {
        if (!device) return;
        vol *= masterVolume * sfxVolume;
        for (int i = 0; i < count; i++) {
            playBeep(baseFreq * notes[i], ms, vol, false);
        }
    }
    
    void playArpeggio(double baseFreq, int notes[], int count, int noteMs, float vol = 0.12f) {
        if (!device) return;
        vol *= masterVolume * sfxVolume;
        for (int i = 0; i < count; i++) {
            playBeep(baseFreq * notes[i], noteMs, vol, false);
            // Não usar SDL_Delay aqui para não bloquear o thread principal
        }
    }
    
    void playSweep(double startFreq, double endFreq, int ms, float vol = 0.10f) {
        if (!device) return;
        vol *= masterVolume * sfxVolume;
        int steps = 20;
        for (int i = 0; i <= steps; i++) {
            double freq = startFreq + (endFreq - startFreq) * (i / (double)steps);
            playBeep(freq, ms / steps, vol, false);
        }
    }
    
    // Sons específicos do jogo
    void playMovementSound() {
        if (enableMovementSounds) playBeep(150.0, 8, 0.06f, true);
    }
    
    void playRotationSound(bool cw = true) {
        if (enableMovementSounds) playBeep(cw ? 350.0 : 300.0, 15, 0.10f, false);
    }
    
    void playSoftDropSound() {
        if (enableMovementSounds) playBeep(200.0, 12, 0.08f, true);
    }
    
    void playHardDropSound() {
        if (enableMovementSounds) playBeep(400.0, 20, 0.12f, true);
    }
    
    void playKickSound() {
        if (enableMovementSounds) playBeep(250.0, 15, 0.08f, true);
    }
    
    void playLevelUpSound() {
        if (enableLevelUpSounds) {
            playBeep(880.0, 100, 0.25f, false);
            playBeep(1320.0, 80, 0.20f, false);
        }
    }
    
    void playGameOverSound() {
        if (enableLevelUpSounds) {
            // Sequência icônica de game over - descendente e dramática
            playBeep(440.0, 200, 0.3f, false);  // A4
            playBeep(392.0, 200, 0.3f, false);  // G4
            playBeep(349.0, 200, 0.3f, false);  // F4
            playBeep(294.0, 300, 0.4f, false);  // D4 (mais longo)
        }
    }
    
    void playComboSound(int combo) {
        if (enableComboSounds && combo > 1) {
            double freq = 440.0 + (combo * 50.0);
            playBeep(freq, 100 + combo * 20, 0.15f + combo * 0.02f, true);
        }
    }
    
    void playTetrisSound() {
        if (enableComboSounds) {
            int notes[] = {1, 5, 8, 12}; // C, E, G, C (oitava)
            playArpeggio(220.0, notes, 4, 50, 0.20f);  // Mais rápido
        }
    }
    
    void playBackgroundMelody(int level) {
        if (!enableAmbientSounds) return;
        Uint32 now = SDL_GetTicks();
        if (now - lastMelody > 3000) {  // A cada 3 segundos
            double baseFreq = 220.0 + (level * 20.0);
            double melody[] = {1.0, 1.25, 1.5, 1.875, 2.0};  // Pentatônica
            
            for (int i = 0; i < 3; i++) {
                double freq = baseFreq * melody[i % 5];
                playBeep(freq, 200, 0.05f * ambientVolume, false);
            }
            lastMelody = now;
        }
    }
    
    void playTensionSound(int filledRows) {
        if (!enableAmbientSounds || filledRows < 5) return;
        Uint32 now = SDL_GetTicks();
        if (now - lastTension > 1000) {
            playBeep(80.0, 300, 0.08f * ambientVolume, true);
            lastTension = now;
        }
    }
    
    void playSweepEffect() {
        if (!enableAmbientSounds) return;
        Uint32 now = SDL_GetTicks();
        if (now - lastSweepSound > 2000) {  // A cada 2 segundos
            playBeep(50.0, 100, 0.03f * ambientVolume, false);
            lastSweepSound = now;
        }
    }
    
    void playScanlineEffect() {
        if (!enableAmbientSounds) return;
        Uint32 now = SDL_GetTicks();
        if (now - lastScanlineSound > 5000) {  // A cada 5 segundos
            playBeep(15.0, 200, 0.02f * ambientVolume, true);
            lastScanlineSound = now;
        }
    }
    
    void cleanup() {
        if (device) SDL_CloseAudioDevice(device);
    }
};

// Implementação da função processAudioConfigs
static bool processAudioConfigs(const std::string& key, const std::string& val, int& processedLines, AudioSystem& audio) {
    auto setb = [&](const char* K, bool& ref) {
        if (key == K) { 
            std::string v = val; 
            for (char& c : v) c = (char)std::tolower((unsigned char)c);
            ref = (v == "1" || v == "true" || v == "on" || v == "yes"); 
            return true; 
        } 
        return false; 
    };
    auto setf = [&](const char* K, float& ref) { 
        if (key == K) { 
            float v = (float)std::atof(val.c_str());
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            ref = v; 
            return true; 
        } 
        return false; 
    };

    // Configurações de áudio
    if (setf("AUDIO_MASTER_VOLUME", audio.masterVolume)) { processedLines++; return true; }
    if (setf("AUDIO_SFX_VOLUME", audio.sfxVolume)) { processedLines++; return true; }
    if (setf("AUDIO_AMBIENT_VOLUME", audio.ambientVolume)) { processedLines++; return true; }
    if (setb("ENABLE_MOVEMENT_SOUNDS", audio.enableMovementSounds)) { processedLines++; return true; }
    if (setb("ENABLE_AMBIENT_SOUNDS", audio.enableAmbientSounds)) { processedLines++; return true; }
    if (setb("ENABLE_COMBO_SOUNDS", audio.enableComboSounds)) { processedLines++; return true; }
    if (setb("ENABLE_LEVEL_UP_SOUNDS", audio.enableLevelUpSounds)) { processedLines++; return true; }
    
    return false;
}

// Implementação da função rotateWithKicks
static void rotateWithKicks(Active& act, const std::vector<std::vector<Cell>>& grid, int dir, AudioSystem& audio){ // +1 = CW, -1 = CCW
    int from = act.rot;
    int to   = (from + dir + 4) % 4;
    auto& p  = PIECES[act.idx];

    // 1) SRS por transição (preferência)
    if(p.hasPerTransKicks){
        int dirIdx = (dir>0? 0 : 1); // 0=CW, 1=CCW
        const auto& lst = p.kicksPerTrans[dirIdx][from];
        for(auto [kx,ky] : lst){
            if(!collides(act, grid, kx, ky, dir)){
                act.x += kx; act.y += ky; act.rot = to; 
                if(kx != 0 || ky != 0) audio.playKickSound();  // Som de kick
                return;
            }
        }
    }

    // 2) Fallback: lista única por direção (legado)
    if(p.hasKicks){
        const auto& lst = (dir>0? p.kicksCW : p.kicksCCW);
        for(auto [kx,ky] : lst){
            if(!collides(act, grid, kx, ky, dir)){
                act.x += kx; act.y += ky; act.rot = to; 
                if(kx != 0 || ky != 0) audio.playKickSound();  // Som de kick
                return;
            }
        }
    }

    // 3) Fallback simples
    if(!collides(act, grid, 0, 0, dir)) { act.rot = to; return; }
    int sx = (dir>0?1:-1);
    if(!collides(act, grid, sx, 0, dir)) { act.x += sx; act.rot = to; return; }
    if(!collides(act, grid, 0,-1, dir)) { act.y -= 1; act.rot = to; return; }
}

// ===========================
//   FUNÇÕES AUXILIARES DO JOGO
// ===========================

// Estrutura para sistema de combos
struct ComboSystem {
    int combo = 0;
    Uint32 lastClear = 0;
    
    void onLineClear(AudioSystem& audio) {
        Uint32 now = SDL_GetTicks();
        if (now - lastClear < 2000) {  // Combo ativo (2 segundos)
            combo++;
        } else {
            combo = 1;
        }
        lastClear = now;
        
        // Som de combo
        audio.playComboSound(combo);
    }
    
    void reset() {
        combo = 0;
        lastClear = 0;
    }
};

// Estrutura para estado do jogo
struct GameState {
    std::vector<std::vector<Cell>> grid;
    Active act;
    std::vector<int> bag;
    size_t bagPos = 0;
    std::mt19937 rng;
    int nextIdx;
    bool running = true, paused = false, gameover = false;
    int score = 0, lines = 0, level = 0, tick_ms = TICK_MS_START;
    Uint32 lastTick;
    ComboSystem combo;
    
    GameState() : grid(ROWS, std::vector<Cell>(COLS)), rng((unsigned)time(nullptr)) {}
};

// Implementação da função initializeRandomizer
static void initializeRandomizer(GameState& state) {
    auto refillBag = [&]() {
        state.bag.clear();
        int n = (RAND_BAG_SIZE > 0 ? RAND_BAG_SIZE : (int)PIECES.size());
        n = std::min(n, (int)PIECES.size());
        for (int i = 0; i < n; i++) state.bag.push_back(i);
        std::shuffle(state.bag.begin(), state.bag.end(), state.rng);
        state.bagPos = 0;
    };
    
    auto drawPieceIdx = [&]() {
        if (RAND_TYPE == RandType::BAG) {
            if (state.bagPos >= state.bag.size()) refillBag();
            return state.bag[state.bagPos++];
        }
        return (int)(state.rng() % PIECES.size());
    };
    
    state.nextIdx = drawPieceIdx(); 
    newActive(state.act, state.nextIdx); 
    state.nextIdx = drawPieceIdx();
    state.lastTick = SDL_GetTicks();
    state.combo.reset();  // Reset combo no início
}

// Estrutura para cache de layout
struct LayoutCache {
    int SWr, SHr, CW, CH, CX, CY;
    int scale, GAP1, GAP2;
    int bannerW, panelTarget, usableLeftW;
    int cellBoard, GW, GH;
    int BX, BY, BW, BH, GX, GY;
    int panelX, panelW, panelY, panelH;
    bool dirty = true;
    
    void calculate() {
        SDL_DisplayMode dmNow; 
        SDL_GetCurrentDisplayMode(0, &dmNow);
        SWr = dmNow.w; 
        SHr = dmNow.h;
        
        if (SWr * 2 >= SHr * 3) { 
            CH = SHr; 
            CW = (CH * 3) / 2; 
            CX = (SWr - CW) / 2; 
            CY = 0; 
        } else { 
            CW = SWr; 
            CH = (CW * 2) / 3; 
            CX = 0; 
            CY = (SHr - CH) / 2; 
        }
        
        scale = HUD_FIXED_SCALE;
        GAP1 = BORDER + GAP1_SCALE * scale;
        GAP2 = BORDER + GAP2_SCALE * scale;
        
        bannerW = 8 * scale + 24;
        panelTarget = (int)(CW * 0.28);
        usableLeftW = CW - (BORDER + bannerW + GAP1) - panelTarget - GAP2;
        cellBoard = std::min(std::max(8, usableLeftW / COLS), (CH - 2 * BORDER) / ROWS);
        GW = cellBoard * COLS; 
        GH = cellBoard * ROWS;
        
        BX = CX + BORDER; 
        BY = CY + (CH - GH) / 2; 
        BW = bannerW; 
        BH = GH;
        GX = BX + BW + GAP1; 
        GY = BY;
        panelX = GX + GW + GAP2; 
        panelW = CX + CW - panelX - BORDER; 
        panelY = BY; 
        panelH = GH;
        
        dirty = false;
    }
};

// Função comum para eliminar duplicação
static void processPieceFall(GameState& state, AudioSystem& audio) {
    auto coll = [&](int dx, int dy, int drot) { return collides(state.act, state.grid, dx, dy, drot); };
    if (!coll(0, 1, 0)) {
        state.act.y++;
    } else {
        lockPiece(state.act, state.grid); 
        audio.playBeep(220.0, 25, 0.12f, true);  // Som de peça travando - mais sutil
        
        int c = clearLines(state.grid);
        if (c > 0) {
            state.lines += c; 
            
            // Sistema de combos
            state.combo.onLineClear(audio);
            
            // Som especial para Tetris (4 linhas)
            if (c == 4) {
                audio.playTetrisSound();
            } else {
                // Som normal de linha limpa - mais responsivo
                double freq = 440.0 + (c * 110.0);  // 440, 550, 660 Hz para 1, 2, 3 linhas
                audio.playBeep(freq, 30 + c * 10, 0.18f, false);  // Som mais curto e suave
            }
            
            state.score += (c == 1 ? 100 : c == 2 ? 300 : c == 3 ? 500 : 800) * (state.level + 1);
            int lv = state.lines / LEVEL_STEP; 
            if (lv > state.level) { 
                state.level = lv; 
                state.tick_ms = std::max(TICK_MS_MIN, TICK_MS_START - state.level * 40);
                audio.playLevelUpSound();  // Som de level up
            }
        } else {
            // Se não limpou linhas, reseta combo
            state.combo.reset();
        }
        
        newActive(state.act, state.nextIdx); 
        state.nextIdx = (int)(state.rng() % PIECES.size()); 
        if (coll(0, 0, 0)) { 
            state.gameover = true; 
            state.paused = false; 
            state.combo.reset();  // Reseta combo no game over
            audio.playGameOverSound();  // Som icônico de game over
        }
    }
}

static void handleInput(GameState& state, AudioSystem& audio, SDL_Renderer* ren) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) state.running = false;
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode k = e.key.keysym.sym;
            if (k == SDLK_ESCAPE) { state.running = false; }
            if (k == SDLK_F12) { 
                // Gerar timestamp para nome do arquivo
                time_t now = time(0);
                struct tm* timeinfo = localtime(&now);
                char filename[64];
                strftime(filename, sizeof(filename), "dropblocks-screenshot_%Y-%m-%d_%H-%M-%S.bmp", timeinfo);
                
                if (saveScreenshot(ren, filename)) audio.playBeep(880.0, 80, 0.18f, false); 
            }
            if (k == SDLK_p) { 
                state.paused = !state.paused; 
                audio.playBeep(state.paused ? 440.0 : 520.0, 30, 0.12f, false); 
            }
            if (state.gameover) {
                if (k == SDLK_RETURN) {
                    for (auto& r : state.grid) for (auto& c : r) { c.occ = false; }
                    state.score = 0; state.lines = 0; state.level = 0; state.tick_ms = TICK_MS_START;
                    state.gameover = false; state.paused = false;
                    initializeRandomizer(state);
                    audio.playBeep(520.0, 40, 0.15f, false);
                }
                continue;
            }
            if (state.paused) continue;
            
            auto coll = [&](int dx, int dy, int drot) { return collides(state.act, state.grid, dx, dy, drot); };
            if (k == SDLK_LEFT && !coll(-1, 0, 0)) {
                state.act.x--;
                audio.playMovementSound();
            }
            if (k == SDLK_RIGHT && !coll(1, 0, 0)) {
                state.act.x++;
                audio.playMovementSound();
            }
            if (k == SDLK_DOWN) {
                // Soft drop
                audio.playSoftDropSound();
                processPieceFall(state, audio);
            }
            if (k == SDLK_SPACE) {
                // Hard drop
                while (!coll(0, 1, 0)) state.act.y++;
                state.score += 2; // Bonus por hard drop
                audio.playHardDropSound();
                processPieceFall(state, audio);
            }
            if (k == SDLK_z || k == SDLK_UP) {
                rotateWithKicks(state.act, state.grid, -1, audio);
                audio.playRotationSound(false);  // CCW
            }
            if (k == SDLK_x) {
                rotateWithKicks(state.act, state.grid, +1, audio);
                audio.playRotationSound(true);   // CW
            }
        }
    }
}

static void checkTensionSound(const GameState& state, AudioSystem& audio) {
    int filledRows = 0;
    for (int y = ROWS - 5; y < ROWS; y++) {  // Últimas 5 linhas
        bool hasBlocks = false;
        for (int x = 0; x < COLS; x++) {
            if (state.grid[y][x].occ) { hasBlocks = true; break; }
        }
        if (hasBlocks) filledRows++;
    }
    
    audio.playTensionSound(filledRows);
}

static void updateGame(GameState& state, AudioSystem& audio) {
    if (!state.paused && !state.gameover) {
        Uint32 now = SDL_GetTicks();
        if (now - state.lastTick >= (Uint32)state.tick_ms) {
            processPieceFall(state, audio);
            state.lastTick = now;
        }
        
        // Verificar tensão do tabuleiro
        checkTensionSound(state, audio);
        
        // Música de fundo
        audio.playBackgroundMelody(state.level);
    }
}

// Funções especializadas extraídas da main
static bool initializeSDL() {
    // Inicializar cada subsistema separadamente para maior compatibilidade
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_INIT_VIDEO error: %s", SDL_GetError());
        return false;
    }
    
    if (SDL_Init(SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_INIT_TIMER error: %s", SDL_GetError());
        return false;
    }
    
    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_INIT_EVENTS error: %s", SDL_GetError());
        return false;
    }
    
    // Áudio e gamepad são opcionais
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        SDL_Log("Warning: SDL_INIT_AUDIO failed: %s", SDL_GetError());
    }
    
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("Warning: SDL_INIT_GAMECONTROLLER failed: %s", SDL_GetError());
    }
    
    return true;
}

static bool initializeGame(GameState& state, AudioSystem& audio) {
    // Carregar configuração
    loadConfigFile(audio);
    
    // Carregar peças
    bool piecesOk = loadPiecesFile();
    if (!piecesOk) {
        seedPiecesFallback();
    }
    
    // Aplicar tema
    applyThemePieceColors();
    printf("Pieces: %zu, PreviewGrid=%d, Randomizer=%s, BagSize=%d\n",
           PIECES.size(), PREVIEW_GRID, (RAND_TYPE == RandType::BAG ? "bag" : "simple"), RAND_BAG_SIZE);
    printf("Audio: Master=%.1f, SFX=%.1f, Ambient=%.1f\n", 
           audio.masterVolume, audio.sfxVolume, audio.ambientVolume);
    fflush(stdout);
    
    return true;
}

static bool initializeWindow(SDL_Window*& win, SDL_Renderer*& ren) {
    SDL_DisplayMode dm; 
    SDL_GetCurrentDisplayMode(0, &dm);
    int SW = dm.w, SH = dm.h;
    
    win = SDL_CreateWindow("DropBlocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SW, SH,
                          SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) { 
        SDL_Log("Win error: %s", SDL_GetError()); 
        return false; 
    }
    
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { 
        SDL_Log("Ren error: %s", SDL_GetError()); 
        return false; 
    }
    
    // Esconder cursor do mouse
    SDL_ShowCursor(SDL_DISABLE);
    
    return true;
}

// Declaração forward para initializeRandomizer
static void initializeRandomizer(GameState& state);

// ===========================
//   FUNÇÕES DE RENDERIZAÇÃO
// ===========================

static void renderBackground(SDL_Renderer* ren, const LayoutCache& layout) {
    SDL_SetRenderDrawColor(ren, THEME.bg_r, THEME.bg_g, THEME.bg_b, 255);
    SDL_RenderClear(ren);
}

static void renderBanner(SDL_Renderer* ren, const LayoutCache& layout, AudioSystem& audio) {
    // Banner
    drawRoundedFilled(ren, layout.BX, layout.BY, layout.BW, layout.BH, 10, 
                     THEME.banner_bg_r, THEME.banner_bg_g, THEME.banner_bg_b, 255);
    drawRoundedOutline(ren, layout.BX, layout.BY, layout.BW, layout.BH, 10, 2, 
                      THEME.banner_outline_r, THEME.banner_outline_g, THEME.banner_outline_b, THEME.banner_outline_a);

    // Título vertical
    int bty = layout.BY + 10, cxText = layout.BX + (layout.BW - 5 * layout.scale) / 2;
    for (size_t i = 0; i < TITLE_TEXT.size(); ++i) {
        char ch = TITLE_TEXT[i];
        if (ch == ' ') { bty += 6 * layout.scale; continue; }
        ch = (char)std::toupper((unsigned char)ch);
        if (ch < 'A' || ch > 'Z') ch = ' ';
        drawPixelText(ren, cxText, bty, std::string(1, ch), layout.scale, 
                     THEME.banner_text_r, THEME.banner_text_g, THEME.banner_text_b);
        bty += 9 * layout.scale;
    }

    // Sweep local do banner
    if (ENABLE_BANNER_SWEEP) {
        SDL_Rect clip{layout.BX, layout.BY, layout.BW, layout.BH};
        SDL_RenderSetClipRect(ren, &clip);
        
        // Usar modo ADD para clarear o banner
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
        
        int bandH = SWEEP_BAND_H_S * layout.scale;
        int total = layout.BH + bandH;
        float tsec = SDL_GetTicks() / 1000.0f;
        int sweepY = (int)std::fmod(tsec * SWEEP_SPEED_PXPS, (float)total) - bandH;
        for (int i = 0; i < bandH; ++i) {
            // Usar função gaussiana para bordas muito suaves
            float normalizedPos = (float)i / (float)bandH; // 0.0 a 1.0
            float center = 0.5f;
            float distance = (normalizedPos - center) * 2.0f; // -1.0 a 1.0
            float sigma = 0.3f + (1.0f - SWEEP_SOFTNESS) * 0.4f; // 0.3 a 0.7
            float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
            Uint8 a = (Uint8)std::round(SWEEP_ALPHA_MAX * softness);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, a);
            SDL_Rect line{layout.BX, layout.BY + sweepY + i, layout.BW, 1};
            SDL_RenderFillRect(ren, &line);
        }
        
        // Resetar modo de blend e clipping
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        SDL_RenderSetClipRect(ren, nullptr);
        
        // Som de sweep
        audio.playSweepEffect();
    }
}

static void renderBoard(SDL_Renderer* ren, const GameState& state, const LayoutCache& layout) {
    // Tabuleiro vazio
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            SDL_Rect r{layout.GX + x * layout.cellBoard, layout.GY + y * layout.cellBoard, 
                      layout.cellBoard - 1, layout.cellBoard - 1};
            SDL_SetRenderDrawColor(ren, THEME.board_empty_r, THEME.board_empty_g, THEME.board_empty_b, 255);
            SDL_RenderFillRect(ren, &r);
        }
    }

    // Peças fixas
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            if (state.grid[y][x].occ) {
                SDL_Rect r{layout.GX + x * layout.cellBoard, layout.GY + y * layout.cellBoard, 
                          layout.cellBoard - 1, layout.cellBoard - 1};
                SDL_SetRenderDrawColor(ren, state.grid[y][x].r, state.grid[y][x].g, state.grid[y][x].b, 255);
                SDL_RenderFillRect(ren, &r);
            }
        }
    }

    // Peça ativa
    auto& pc = PIECES[state.act.idx];
    for (auto [px, py] : pc.rot[state.act.rot]) {
        SDL_Rect r{layout.GX + (state.act.x + px) * layout.cellBoard, 
                  layout.GY + (state.act.y + py) * layout.cellBoard, 
                  layout.cellBoard - 1, layout.cellBoard - 1};
        SDL_SetRenderDrawColor(ren, pc.r, pc.g, pc.b, 255); 
        SDL_RenderFillRect(ren, &r);
    }
}

static void renderHUD(SDL_Renderer* ren, const GameState& state, const LayoutCache& layout) {
    // Painel (HUD)
    drawRoundedFilled(ren, layout.panelX, layout.panelY, layout.panelW, layout.panelH, 12, 
                     THEME.panel_fill_r, THEME.panel_fill_g, THEME.panel_fill_b, 255);
    drawRoundedOutline(ren, layout.panelX, layout.panelY, layout.panelW, layout.panelH, 12, 2, 
                      THEME.panel_outline_r, THEME.panel_outline_g, THEME.panel_outline_b, THEME.panel_outline_a);

    // HUD textos
    int tx = layout.panelX + 14, ty = layout.panelY + 14;
    drawPixelText(ren, tx, ty, "SCORE", layout.scale, THEME.hud_label_r, THEME.hud_label_g, THEME.hud_label_b); ty += 10 * layout.scale;
    drawPixelText(ren, tx, ty, fmtScore(state.score), layout.scale + 1, THEME.hud_score_r, THEME.hud_score_g, THEME.hud_score_b); ty += 12 * (layout.scale + 1);
    drawPixelText(ren, tx, ty, "LINES", layout.scale, THEME.hud_label_r, THEME.hud_label_g, THEME.hud_label_b); ty += 8 * layout.scale;
    drawPixelText(ren, tx, ty, std::to_string(state.lines), layout.scale, THEME.hud_lines_r, THEME.hud_lines_g, THEME.hud_lines_b); ty += 10 * layout.scale;
    drawPixelText(ren, tx, ty, "LEVEL", layout.scale, THEME.hud_label_r, THEME.hud_label_g, THEME.hud_label_b); ty += 8 * layout.scale;
    drawPixelText(ren, tx, ty, std::to_string(state.level), layout.scale, THEME.hud_level_r, THEME.hud_level_g, THEME.hud_level_b); ty += 10 * layout.scale;

    // NEXT (quadro PREVIEW_GRID × PREVIEW_GRID)
    int boxW = layout.panelW - 28;
    int boxH = std::min(layout.panelH - (ty - layout.panelY) - 14, boxW);
    int boxX = layout.panelX + 14;
    int boxY = ty;

    drawRoundedFilled(ren, boxX, boxY, boxW, boxH, 10, THEME.next_fill_r, THEME.next_fill_g, THEME.next_fill_b, 255);
    drawRoundedOutline(ren, boxX, boxY, boxW, boxH, 10, 2, THEME.next_outline_r, THEME.next_outline_g, THEME.next_outline_b, THEME.next_outline_a);

    drawPixelText(ren, boxX + 10, boxY + 10, "NEXT", layout.scale, THEME.next_label_r, THEME.next_label_g, THEME.next_label_b);

    int padIn = 14 + 4 * layout.scale;
    int innerX = boxX + 10, innerY = boxY + padIn;
    int innerW = boxW - 20, innerH = boxH - padIn - 10;

    int gridCols = PREVIEW_GRID, gridRows = PREVIEW_GRID;
    int cellMini = std::min(innerW / gridCols, innerH / gridRows);
    if (cellMini < 1) cellMini = 1;
    if (cellMini > layout.cellBoard) cellMini = layout.cellBoard;

    int gridW = cellMini * gridCols, gridH = cellMini * gridRows;
    int gridX = innerX + (innerW - gridW) / 2;
    int gridY = innerY + (innerH - gridH) / 2;

    // quadriculado
    for (int gy = 0; gy < gridRows; ++gy) {
        for (int gx = 0; gx < gridCols; ++gx) {
            SDL_Rect q{gridX + gx * cellMini, gridY + gy * cellMini, cellMini - 1, cellMini - 1};
            bool isLight = ((gx + gy) & 1) != 0;
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
        auto& np = PIECES[state.nextIdx];
        int minx = 999, maxx = -999, miny = 999, maxy = -999;
        for (auto [px, py] : np.rot[0]) { 
            minx = std::min(minx, px); maxx = std::max(maxx, px); 
            miny = std::min(miny, py); maxy = std::max(maxy, py); 
        }
        int pw = (maxx - minx + 1), ph = (maxy - miny + 1);
        int offX = (gridCols - pw) / 2 - minx;
        int offY = (gridRows - ph) / 2 - miny;
        for (auto [px, py] : np.rot[0]) {
            SDL_Rect r{gridX + (px + offX) * cellMini, gridY + (py + offY) * cellMini, cellMini - 1, cellMini - 1};
            SDL_SetRenderDrawColor(ren, np.r, np.g, np.b, 255); 
            SDL_RenderFillRect(ren, &r);
        }
    }
}

static void renderOverlay(SDL_Renderer* ren, const GameState& state, const LayoutCache& layout) {
    if (state.gameover || state.paused) {
        const std::string topText = state.paused ? "PAUSE" : "GAME OVER";
        const std::string subText = state.paused ? "" : "PRESS START";
        int topW = textWidthPx(topText, layout.scale + 2);
        int subW = subText.empty() ? 0 : textWidthPx(subText, layout.scale);
        int textW = std::max(topW, subW);
        int padX = 24, padY = 20;
        int textH = 7 * (layout.scale + 2) + (subText.empty() ? 0 : (8 * layout.scale + 7 * layout.scale));
        int ow = textW + padX * 2, oh = textH + padY * 2;
        int ox = layout.GX + (layout.GW - ow) / 2, oy = layout.GY + (layout.GH - oh) / 2;
        drawRoundedFilled(ren, ox, oy, ow, oh, 14, THEME.overlay_fill_r, THEME.overlay_fill_g, THEME.overlay_fill_b, THEME.overlay_fill_a);
        drawRoundedOutline(ren, ox, oy, ow, oh, 14, 2, THEME.overlay_outline_r, THEME.overlay_outline_g, THEME.overlay_outline_b, THEME.overlay_outline_a);
        int txc = ox + (ow - topW) / 2, tyc = oy + padY;
        drawPixelTextOutlined(ren, txc, tyc, topText, layout.scale + 2, THEME.overlay_top_r, THEME.overlay_top_g, THEME.overlay_top_b, 0, 0, 0);
        if (!subText.empty()) {
            int sx = ox + (ow - subW) / 2, sy = tyc + 7 * (layout.scale + 2) + 8 * layout.scale;
            drawPixelTextOutlined(ren, sx, sy, subText, layout.scale, THEME.overlay_sub_r, THEME.overlay_sub_g, THEME.overlay_sub_b, 0, 0, 0);
        }
    }
}

static void renderPostEffects(SDL_Renderer* ren, const LayoutCache& layout, AudioSystem& audio) {
    // Scanlines
    if (SCANLINE_ALPHA > 0) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, (Uint8)SCANLINE_ALPHA);
        for (int y = 0; y < layout.SHr; y += 2) { 
            SDL_Rect sl{0, y, layout.SWr, 1}; 
            SDL_RenderFillRect(ren, &sl); 
        }
        
        // Som de scanline
        audio.playScanlineEffect();
    }

    // SWEEP GLOBAL (clareia)
    if (ENABLE_GLOBAL_SWEEP) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
        float tsec = SDL_GetTicks() / 1000.0f;
        int bandH = SWEEP_G_BAND_H_PX;
        int total = layout.SHr + bandH;
        int sweepY = (int)std::fmod(tsec * SWEEP_G_SPEED_PXPS, (float)total) - bandH;
        for (int i = 0; i < bandH; ++i) {
            // Usar função gaussiana para bordas muito suaves
            float normalizedPos = (float)i / (float)bandH; // 0.0 a 1.0
            float center = 0.5f;
            float distance = (normalizedPos - center) * 2.0f; // -1.0 a 1.0
            float sigma = 0.3f + (1.0f - SWEEP_G_SOFTNESS) * 0.4f; // 0.3 a 0.7
            float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
            Uint8 a = (Uint8)std::round(SWEEP_G_ALPHA_MAX * softness);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, a);
            SDL_Rect line{0, sweepY + i, layout.SWr, 1};
            SDL_RenderFillRect(ren, &line);
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
}

// ===========================
//   MAIN E CONTROLES
// ===========================
/**
 * @brief Main game entry point
 * 
 * Initializes SDL2, loads configuration and piece sets, sets up audio,
 * and runs the main game loop until the user quits.
 * 
 * @param argc Command line argument count (unused)
 * @param argv Command line arguments (unused)
 * @return Exit status (0 for success)
 */
int main(int, char**) {
    printf("=== DROPBLOCKS STARTING ===\n");
    printf("VERSION: 5.1 - Granular Debugging\n");
    printf("BUILD: %s %s\n", __DATE__, __TIME__);
    printf("FIXES: Added step-by-step debugging to identify exact crash location\n");
    fflush(stdout);

    // Inicialização
    printf("DEBUG: Step 1 - Initializing SDL2...\n");
    fflush(stdout);
    if (!initializeSDL()) return 1;
    printf("DEBUG: Step 1 - SDL2 initialized successfully\n");
    fflush(stdout);
    
    printf("DEBUG: Step 2 - Initializing AudioSystem...\n");
    fflush(stdout);
    AudioSystem audio;
    if (!audio.initialize()) {
        printf("Warning: Audio initialization failed, continuing without sound\n");
    }
    printf("DEBUG: Step 2 - AudioSystem initialized\n");
    fflush(stdout);
    
    printf("DEBUG: Step 3 - Initializing GameState...\n");
    fflush(stdout);
    GameState state;
    if (!initializeGame(state, audio)) return 1;
    printf("DEBUG: Step 3 - GameState initialized successfully\n");
    fflush(stdout);
    
    printf("DEBUG: Step 4 - Initializing Window...\n");
    fflush(stdout);
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;
    if (!initializeWindow(win, ren)) return 1;
    printf("DEBUG: Step 4 - Window initialized successfully\n");
    fflush(stdout);
    
    printf("DEBUG: Step 5 - Initializing Randomizer...\n");
    fflush(stdout);
    initializeRandomizer(state);
    printf("DEBUG: Step 5 - Randomizer initialized successfully\n");
    fflush(stdout);

    // Loop principal
    while (state.running) {
        handleInput(state, audio, ren);
        updateGame(state, audio);

        // Cache de layout
        static LayoutCache layout;
        if (layout.dirty) {
            layout.calculate();
        }

        // Renderização modular
        renderBackground(ren, layout);
        renderBanner(ren, layout, audio);
        renderBoard(ren, state, layout);
        renderHUD(ren, state, layout);
        renderOverlay(ren, state, layout);
        renderPostEffects(ren, layout, audio);

        SDL_RenderPresent(ren);
        SDL_Delay(1);
    }

    // Cleanup
    audio.cleanup();
    SDL_DestroyRenderer(ren); 
    SDL_DestroyWindow(win); 
    SDL_Quit();
    return 0;
}
