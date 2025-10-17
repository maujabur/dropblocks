/**
 * DropBlocks (SDL2)
 * - Modular architecture: Config, Theme, Audio, Pieces, Render layers, App loop
 * - Visual effects via bridge (no global externs)
 * - Centralized input (quit/pause/restart) via InputManager
 * - Guideline-like rotations (SRS) with fallback pieces
 *
 * Controls
 * - Keyboard: Arrow keys, Z/X/Up, Space, P, Enter, ESC, F12
 * - Joystick: D-pad, A/B/X/Y, Start/Back, analog (deadzone)
 *
 * Build (example)
 * - g++ dropblocks.cpp -o dropblocks `sdl2-config --cflags --libs` -O2
 */

// TODO: Add a configurable countdown timer for exhibitions
// TODO: Make regions configurable by position and size
// TODO: Refine resolution handling and screen ratio support
// TODO: Improve thousand-separator formatting in scores
// TODO: Remove grayscale fallback paths
// TODO: Ensure joystick and keyboard invoke the same input methods
// TODO: Verify joystick and keyboard can be used together seamlessly
// TODO: Produce a statically-linked Windows build; verify required DLLs
// TODO: Produce a successful Raspberry Pi build
// TODO: verificar o processo de atribuição de cores padrão para as peças
// TODO: Add HUD region showing dropped piece counts by type
// DESIRED: convert the config manager to a component that can be reused by other applications  
// DESIRED: convert all itens possible to components that can be reused by other applications
// REMEMBER: to ask about the parser

#include <SDL2/SDL.h>
#include "render/RenderLayer.hpp"
#include "render/RenderManager.hpp"
#include "render/LayoutCache.hpp"
#include "app/GameInitializer.hpp"
#include "app/GameLoop.hpp"
#include "app/GameCleanup.hpp"
#include "render/Primitives.hpp"
#include "render/Layers.hpp"
#include "ConfigManager.hpp"
#include "config/ConfigProcessors.hpp"
#include "input/IInputManager.hpp"
#include "DebugLogger.hpp"
#include "input/InputHandler.hpp"
#include "input/KeyboardInput.hpp"
#include "input/JoystickInput.hpp"
#include "input/InputManager.hpp"
#include "ThemeManager.hpp"
#include "pieces/Piece.hpp"
#include "audio/AudioSystem.hpp"
#include "pieces/PieceManager.hpp"
#include "render/GameStateBridge.hpp"
#include "game/Mechanics.hpp"
#include "app/GameTypes.hpp"
#include "app/ComboSystem.hpp"
#include "app/GameBoard.hpp"
#include "app/ScoreSystem.hpp"
#include "app/GameState.hpp"
#include "app/GameHelpers.hpp"
#include "ConfigTypes.hpp"
#include "util/UiUtil.hpp"

// STL
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <functional>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <random>
#include <array>
#include <memory>

// ===========================
//   DEFINIÇÕES DE VERSÃO
// ===========================
#define DROPBLOCKS_VERSION "6.24"
#define DROPBLOCKS_BUILD_INFO "Phase 14: Config processors modular"
#define DROPBLOCKS_FEATURES "ConfigProcessors module extracted (~200 lines); config parsing centralized"

// ===========================
//   FORWARD DECLARATIONS
// ===========================
// Forward declarations for structures
struct Theme;
struct Piece;
enum class RandType;
struct VisualConfig;
struct AudioConfig;
struct InputConfig;
struct PiecesConfig;
struct GameConfig;

// Use canonical interfaces
#include "Interfaces.hpp"
#include "di/DependencyContainer.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// ===========================
//   PARÂMETROS & THEME
// ===========================

int   ROUNDED_PANELS = 1;           // 1 = arredondado; 0 = retângulo (synced from VisualConfig.layout)
int   HUD_FIXED_SCALE   = 6;        // escala fixa do HUD
std::string TITLE_TEXT  = "---H A C K T R I S";// texto vertical (A–Z e espaço)
int   GAP1_SCALE        = 10;       // banner ↔ tabuleiro (x scale)
int   GAP2_SCALE        = 10;       // tabuleiro ↔ painel  (x scale)

// Caminho opcional indicado no cfg para o arquivo de peças
std::string PIECES_FILE_PATH = "";

// Config do set de peças
static int PREVIEW_GRID = 6;               // NxN no NEXT (padrão 6)

// Visual effects bridge backing store (read by db_getVisualEffects)
static VisualEffectsView g_visualView{};

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

// ===========================
//   MECÂNICA / ESTRUTURAS
// ===========================

/** @brief Number of columns in the game board */
const int COLS = 10;
/** @brief Number of rows in the game board */
const int ROWS = 20;
/** @brief Border size around the game board */
int BORDER = 10;
/** @brief Initial game tick interval in milliseconds */
static int TICK_MS_START = 400;
/** @brief Minimum game tick interval in milliseconds */
static int TICK_MS_MIN   = 80;
/** @brief Speed acceleration per level (ms reduction) */
int SPEED_ACCELERATION = 50;
/** @brief Aspect ratio correction factor for LED screen distortion */
float ASPECT_CORRECTION_FACTOR = 0.75f;
/** @brief Lines required to advance to next level */
static int LEVEL_STEP    = 10;

// Forward declarations moved to include/config/ConfigProcessors.hpp


// Global manager instances (temporary during migration)
GameConfig gameConfig;
ThemeManager themeManager;

// Forward declarations for classes
class PieceManager;

// Forward declarations for functions that use pieceManager
// moved to PieceManager: bool pm_loadPiecesFromStream(std::istream& in)
// processJoystickConfigs moved to include/config/ConfigProcessors.hpp

// Temporary global variables eliminated - now using PieceManager private fields

std::vector<Piece> PIECES;


// ===========================
//   UTILS: STR / CORES / PARSING
// ===========================

// trim(), parseHexColor(), and parsing helpers moved to src/config/ConfigProcessors.cpp

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

    // Visual effect keys now handled via ConfigManager VisualConfig -> applyConfigToTheme/db_getVisualEffects
    // Layout shortcuts kept for backward compatibility
    if (seti("ROUNDED_PANELS", ROUNDED_PANELS)) { processedLines++; return true; }
    if (seti("HUD_FIXED_SCALE", HUD_FIXED_SCALE)) { processedLines++; return true; }
    if (seti("GAP1_SCALE", GAP1_SCALE)) { processedLines++; return true; }
    if (seti("GAP2_SCALE", GAP2_SCALE)) { processedLines++; return true; }
    
    return false;
}

static bool processThemeColors(const std::string& key, const std::string& val, int& processedLines) {
    auto setrgb = [&](const char* K, Uint8& R, Uint8& G, Uint8& B) {
        if (key == K) { 
            Uint8 r, g, b; 
            if (ConfigProcessors::parseHexColor(val, r, g, b)) { 
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
    if (setrgb("BG", themeManager.getTheme().bg_r, themeManager.getTheme().bg_g, themeManager.getTheme().bg_b)) { processedLines++; return true; }
    if (setrgb("BOARD_EMPTY", themeManager.getTheme().board_empty_r, themeManager.getTheme().board_empty_g, themeManager.getTheme().board_empty_b)) { processedLines++; return true; }
    if (setrgb("PANEL_FILL", themeManager.getTheme().panel_fill_r, themeManager.getTheme().panel_fill_g, themeManager.getTheme().panel_fill_b)) { processedLines++; return true; }
    if (setrgb("PANEL_OUTLINE", themeManager.getTheme().panel_outline_r, themeManager.getTheme().panel_outline_g, themeManager.getTheme().panel_outline_b)) { processedLines++; return true; }
    if (setrgb("BANNER_BG", themeManager.getTheme().banner_bg_r, themeManager.getTheme().banner_bg_g, themeManager.getTheme().banner_bg_b)) { processedLines++; return true; }
    if (setrgb("BANNER_OUTLINE", themeManager.getTheme().banner_outline_r, themeManager.getTheme().banner_outline_g, themeManager.getTheme().banner_outline_b)) { processedLines++; return true; }
    if (setrgb("BANNER_TEXT", themeManager.getTheme().banner_text_r, themeManager.getTheme().banner_text_g, themeManager.getTheme().banner_text_b)) { processedLines++; return true; }

    // Cores HUD
    if (setrgb("HUD_LABEL", themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b)) { processedLines++; return true; }
    if (setrgb("HUD_SCORE", themeManager.getTheme().hud_score_r, themeManager.getTheme().hud_score_g, themeManager.getTheme().hud_score_b)) { processedLines++; return true; }
    if (setrgb("HUD_LINES", themeManager.getTheme().hud_lines_r, themeManager.getTheme().hud_lines_g, themeManager.getTheme().hud_lines_b)) { processedLines++; return true; }
    if (setrgb("HUD_LEVEL", themeManager.getTheme().hud_level_r, themeManager.getTheme().hud_level_g, themeManager.getTheme().hud_level_b)) { processedLines++; return true; }

    // Cores NEXT
    if (setrgb("NEXT_FILL", themeManager.getTheme().next_fill_r, themeManager.getTheme().next_fill_g, themeManager.getTheme().next_fill_b)) { processedLines++; return true; }
    if (setrgb("NEXT_OUTLINE", themeManager.getTheme().next_outline_r, themeManager.getTheme().next_outline_g, themeManager.getTheme().next_outline_b)) { processedLines++; return true; }
    if (setrgb("NEXT_LABEL", themeManager.getTheme().next_label_r, themeManager.getTheme().next_label_g, themeManager.getTheme().next_label_b)) { processedLines++; return true; }

    // Cores OVERLAY
    if (setrgb("OVERLAY_FILL", themeManager.getTheme().overlay_fill_r, themeManager.getTheme().overlay_fill_g, themeManager.getTheme().overlay_fill_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_OUTLINE", themeManager.getTheme().overlay_outline_r, themeManager.getTheme().overlay_outline_g, themeManager.getTheme().overlay_outline_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_TOP", themeManager.getTheme().overlay_top_r, themeManager.getTheme().overlay_top_g, themeManager.getTheme().overlay_top_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_SUB", themeManager.getTheme().overlay_sub_r, themeManager.getTheme().overlay_sub_g, themeManager.getTheme().overlay_sub_b)) { processedLines++; return true; }

    // Alpha values
    if (seta("PANEL_OUTLINE_A", themeManager.getTheme().panel_outline_a)) { processedLines++; return true; }
    if (seta("NEXT_OUTLINE_A", themeManager.getTheme().next_outline_a)) { processedLines++; return true; }
    if (seta("OVERLAY_FILL_A", themeManager.getTheme().overlay_fill_a)) { processedLines++; return true; }
    if (seta("OVERLAY_OUTLINE_A", themeManager.getTheme().overlay_outline_a)) { processedLines++; return true; }

    return false;
}

// Declaração forward para AudioSystem e JoystickSystem (from modules)
struct AudioSystem;
class JoystickSystem;

static bool processAudioConfigs(const std::string& key, const std::string& val, int& processedLines, AudioSystem& audio);
static bool processJoystickConfigs(const std::string& key, const std::string& val, int& processedLines, JoystickSystem& joystick);

static bool processSpecialConfigs(const std::string& key, const std::string& val, int& processedLines) {
    // Configurações especiais
    if (key == "TITLE_TEXT") { TITLE_TEXT = val; processedLines++; return true; }
    if (key == "PIECES_FILE") { PIECES_FILE_PATH = val; processedLines++; return true; }
    
    // Grid colors
    if (key == "NEXT_GRID_DARK") { 
        *(int*)&themeManager.getTheme().next_grid_dark = std::atoi(val.c_str()); 
        processedLines++; 
        return true; 
    }
    if (key == "NEXT_GRID_LIGHT") { 
        *(int*)&themeManager.getTheme().next_grid_light = std::atoi(val.c_str()); 
        processedLines++; 
        return true; 
    }
    
    // Grid colors RGB
    if (key == "NEXT_GRID_DARK_COLOR") {
        Uint8 r, g, b;
        if (ConfigProcessors::parseHexColor(val, r, g, b)) {
            themeManager.getTheme().next_grid_dark_r = r; themeManager.getTheme().next_grid_dark_g = g; themeManager.getTheme().next_grid_dark_b = b;
            themeManager.getTheme().next_grid_use_rgb = true;
            processedLines++;
        }
        return true;
    }
    if (key == "NEXT_GRID_LIGHT_COLOR") {
        Uint8 r, g, b;
        if (ConfigProcessors::parseHexColor(val, r, g, b)) {
            themeManager.getTheme().next_grid_light_r = r; themeManager.getTheme().next_grid_light_g = g; themeManager.getTheme().next_grid_light_b = b;
            themeManager.getTheme().next_grid_use_rgb = true;
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
        if (ConfigProcessors::parseHexColor(val, r, g, b)) {
            if (pieceIndex >= (int)themeManager.getTheme().piece_colors.size()) {
                themeManager.getTheme().piece_colors.resize(pieceIndex + 1, {200, 200, 200});
            }
            themeManager.getTheme().piece_colors[pieceIndex] = {r, g, b};
            processedLines++;
        }
        return true;
    }
    
    return false;
}

static void loadConfigFromStream(std::istream& in, AudioSystem& audio, JoystickSystem& joystick, PieceManager& pieceManager) {
    
    std::string line;
    int lineNum = 0;
    int processedLines = 0;
    int skippedLines = 0;
    
    while (std::getline(in, line)) {
        lineNum++;
        
        // Parse da linha (remove comentários)
        line = ConfigProcessors::parseConfigLine(line);
        ConfigProcessors::trim(line);
        
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
        ConfigProcessors::trim(key);
        ConfigProcessors::trim(val);
        
        if (key.empty()) {
            skippedLines++;
            continue;
        }

        std::string KEY = key;
        for (char& c : KEY) c = (char)std::toupper((unsigned char)c);

        // Processar usando funções especializadas do módulo ConfigProcessors
        if (ConfigProcessors::processBasicConfigs(KEY, val, processedLines)) continue;
        if (ConfigProcessors::processThemeColors(KEY, val, processedLines, themeManager)) continue;
        if (ConfigProcessors::processAudioConfigs(KEY, val, processedLines, audio)) continue;
        if (ConfigProcessors::processJoystickConfigs(KEY, val, processedLines, joystick, pieceManager)) continue;
        if (ConfigProcessors::processSpecialConfigs(KEY, val, processedLines, themeManager)) continue;
        
        // Linha não reconhecida
        skippedLines++;
    }
    
}
static bool loadConfigPath(const std::string& p, AudioSystem& audio, JoystickSystem& joystick, PieceManager& pieceManager){
    
    std::ifstream f(p.c_str()); 
    if(f.good()){ 
        loadConfigFromStream(f, audio, joystick, pieceManager);
        return true; 
    } 
    return false;
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
static void loadConfigFile(AudioSystem& audio, JoystickSystem& joystick, PieceManager& pieceManager){
    
    if(const char* env = std::getenv("DROPBLOCKS_CFG")){ 
        if(loadConfigPath(env, audio, joystick, pieceManager)) { 
            DebugLogger::info("Config carregado de: " + std::string(env)); 
            return; 
        } 
    }
    if(loadConfigPath("default.cfg", audio, joystick, pieceManager)) { 
        DebugLogger::info("Config carregado de: default.cfg"); 
        return; 
    }
    if(loadConfigPath("dropblocks.cfg", audio, joystick, pieceManager)) { 
        DebugLogger::info("Config carregado de: dropblocks.cfg"); 
        return; 
    } // fallback para compatibilidade
    if(const char* home = std::getenv("HOME")){
        std::string p = std::string(home) + "/.config/default.cfg";
        if(loadConfigPath(p, audio, joystick, pieceManager)) { 
            DebugLogger::info("Config carregado de: " + p); 
            return; 
        }
        std::string p2 = std::string(home) + "/.config/dropblocks.cfg";
        if(loadConfigPath(p2, audio, joystick, pieceManager)) { 
            DebugLogger::info("Config carregado de: " + p2); 
            return; 
        }
    }
    DebugLogger::info("Nenhum config encontrado; usando padrões.");
}




extern bool pm_loadPiecesFromStream(std::istream&);
bool db_loadPiecesPath(const std::string& p){
    std::ifstream f(p.c_str()); 
    if(!f.good()) {
        return false;
    }
    bool ok=pm_loadPiecesFromStream(f);
    DebugLogger::info(std::string("Pieces carregado de: ") + p + " (" + (ok?"OK":"vazio/erro") + ")");
    return ok;
}

static void applyThemePieceColors(){
    themeManager.applyPieceColors(PIECES);
}


// ===========================
//   FUNÇÕES DE APLICAÇÃO DE CONFIGURAÇÃO
// ===========================

/**
 * @brief Apply audio configuration to AudioSystem
 */
static void applyConfigToAudio(AudioSystem& audio, const AudioConfig& config) {
    audio.masterVolume = config.masterVolume;
    audio.sfxVolume = config.sfxVolume;
    audio.ambientVolume = config.ambientVolume;
    audio.enableMovementSounds = config.enableMovementSounds;
    audio.enableAmbientSounds = config.enableAmbientSounds;
    audio.enableComboSounds = config.enableComboSounds;
    audio.enableLevelUpSounds = config.enableLevelUpSounds;
}

/**
 * @brief Apply visual configuration to global theme
 */
static void applyConfigToTheme(const VisualConfig& config) {
    
    // Apply colors
    themeManager.getTheme().bg_r = config.colors.background.r;
    themeManager.getTheme().bg_g = config.colors.background.g;
    themeManager.getTheme().bg_b = config.colors.background.b;
    
    
    themeManager.getTheme().board_empty_r = config.colors.boardEmpty.r;
    themeManager.getTheme().board_empty_g = config.colors.boardEmpty.g;
    themeManager.getTheme().board_empty_b = config.colors.boardEmpty.b;
    
    themeManager.getTheme().panel_fill_r = config.colors.panelFill.r;
    themeManager.getTheme().panel_fill_g = config.colors.panelFill.g;
    themeManager.getTheme().panel_fill_b = config.colors.panelFill.b;
    
    themeManager.getTheme().panel_outline_r = config.colors.panelOutline.r;
    themeManager.getTheme().panel_outline_g = config.colors.panelOutline.g;
    themeManager.getTheme().panel_outline_b = config.colors.panelOutline.b;
    themeManager.getTheme().panel_outline_a = config.colors.panelOutlineAlpha;
    
    // Banner
    themeManager.getTheme().banner_bg_r = config.colors.bannerBg.r;
    themeManager.getTheme().banner_bg_g = config.colors.bannerBg.g;
    themeManager.getTheme().banner_bg_b = config.colors.bannerBg.b;
    
    themeManager.getTheme().banner_outline_r = config.colors.bannerOutline.r;
    themeManager.getTheme().banner_outline_g = config.colors.bannerOutline.g;
    themeManager.getTheme().banner_outline_b = config.colors.bannerOutline.b;
    themeManager.getTheme().banner_outline_a = config.colors.bannerOutlineAlpha;
    
    themeManager.getTheme().banner_text_r = config.colors.bannerText.r;
    themeManager.getTheme().banner_text_g = config.colors.bannerText.g;
    themeManager.getTheme().banner_text_b = config.colors.bannerText.b;
    
    // HUD
    themeManager.getTheme().hud_label_r = config.colors.hudLabel.r;
    themeManager.getTheme().hud_label_g = config.colors.hudLabel.g;
    themeManager.getTheme().hud_label_b = config.colors.hudLabel.b;
    
    themeManager.getTheme().hud_score_r = config.colors.hudScore.r;
    themeManager.getTheme().hud_score_g = config.colors.hudScore.g;
    themeManager.getTheme().hud_score_b = config.colors.hudScore.b;
    
    themeManager.getTheme().hud_lines_r = config.colors.hudLines.r;
    themeManager.getTheme().hud_lines_g = config.colors.hudLines.g;
    themeManager.getTheme().hud_lines_b = config.colors.hudLines.b;
    
    themeManager.getTheme().hud_level_r = config.colors.hudLevel.r;
    themeManager.getTheme().hud_level_g = config.colors.hudLevel.g;
    themeManager.getTheme().hud_level_b = config.colors.hudLevel.b;
    
    // NEXT
    themeManager.getTheme().next_fill_r = config.colors.nextFill.r;
    themeManager.getTheme().next_fill_g = config.colors.nextFill.g;
    themeManager.getTheme().next_fill_b = config.colors.nextFill.b;
    
    themeManager.getTheme().next_outline_r = config.colors.nextOutline.r;
    themeManager.getTheme().next_outline_g = config.colors.nextOutline.g;
    themeManager.getTheme().next_outline_b = config.colors.nextOutline.b;
    themeManager.getTheme().next_outline_a = config.colors.nextOutlineAlpha;
    
    themeManager.getTheme().next_label_r = config.colors.nextLabel.r;
    themeManager.getTheme().next_label_g = config.colors.nextLabel.g;
    themeManager.getTheme().next_label_b = config.colors.nextLabel.b;
    
    themeManager.getTheme().next_grid_dark_r = config.colors.nextGridDark.r;
    themeManager.getTheme().next_grid_dark_g = config.colors.nextGridDark.g;
    themeManager.getTheme().next_grid_dark_b = config.colors.nextGridDark.b;
    
    themeManager.getTheme().next_grid_light_r = config.colors.nextGridLight.r;
    themeManager.getTheme().next_grid_light_g = config.colors.nextGridLight.g;
    themeManager.getTheme().next_grid_light_b = config.colors.nextGridLight.b;
    themeManager.getTheme().next_grid_use_rgb = config.colors.nextGridUseRgb;
    
    // Overlay
    themeManager.getTheme().overlay_fill_r = config.colors.overlayFill.r;
    themeManager.getTheme().overlay_fill_g = config.colors.overlayFill.g;
    themeManager.getTheme().overlay_fill_b = config.colors.overlayFill.b;
    themeManager.getTheme().overlay_fill_a = config.colors.overlayFillAlpha;
    
    themeManager.getTheme().overlay_outline_r = config.colors.overlayOutline.r;
    themeManager.getTheme().overlay_outline_g = config.colors.overlayOutline.g;
    themeManager.getTheme().overlay_outline_b = config.colors.overlayOutline.b;
    themeManager.getTheme().overlay_outline_a = config.colors.overlayOutlineAlpha;
    
    themeManager.getTheme().overlay_top_r = config.colors.overlayTop.r;
    themeManager.getTheme().overlay_top_g = config.colors.overlayTop.g;
    themeManager.getTheme().overlay_top_b = config.colors.overlayTop.b;
    
    themeManager.getTheme().overlay_sub_r = config.colors.overlaySub.r;
    themeManager.getTheme().overlay_sub_g = config.colors.overlaySub.g;
    themeManager.getTheme().overlay_sub_b = config.colors.overlaySub.b;
    
    // Apply effects
    // Visual effects now flow via g_visualView and bridge only
    g_visualView.bannerSweep = config.effects.bannerSweep;
    g_visualView.globalSweep = config.effects.globalSweep;
    g_visualView.sweepSpeedPxps = config.effects.sweepSpeedPxps;
    g_visualView.sweepBandHS = config.effects.sweepBandHS;
    g_visualView.sweepAlphaMax = config.effects.sweepAlphaMax;
    g_visualView.sweepSoftness = config.effects.sweepSoftness;
    g_visualView.sweepGSpeedPxps = config.effects.sweepGSpeedPxps;
    g_visualView.sweepGBandHPx = config.effects.sweepGBandHPx;
    g_visualView.sweepGAlphaMax = config.effects.sweepGAlphaMax;
    g_visualView.sweepGSoftness = config.effects.sweepGSoftness;
    g_visualView.scanlineAlpha = config.effects.scanlineAlpha;
    
    // Apply layout
    ROUNDED_PANELS = config.layout.roundedPanels;
    HUD_FIXED_SCALE = config.layout.hudFixedScale;
    GAP1_SCALE = config.layout.gap1Scale;
    GAP2_SCALE = config.layout.gap2Scale;
    
    // Apply text
    TITLE_TEXT = config.titleText;
}

// Implementação da função processAudioConfigs
static bool processAudioConfigs(const std::string& key, const std::string& val, int& processedLines, AudioSystem& audio) {
    // Delegate to the new AudioConfig system
    if (audio.loadFromConfig(key, val)) {
        processedLines++;
        return true;
    }
    
    // Legacy compatibility - update old fields for backward compatibility
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

    // Update legacy fields for backward compatibility
    if (setf("AUDIO_MASTER_VOLUME", audio.masterVolume)) { processedLines++; return true; }
    if (setf("AUDIO_SFX_VOLUME", audio.sfxVolume)) { processedLines++; return true; }
    if (setf("AUDIO_AMBIENT_VOLUME", audio.ambientVolume)) { processedLines++; return true; }
    if (setb("ENABLE_MOVEMENT_SOUNDS", audio.enableMovementSounds)) { processedLines++; return true; }
    if (setb("ENABLE_AMBIENT_SOUNDS", audio.enableAmbientSounds)) { processedLines++; return true; }
    if (setb("ENABLE_COMBO_SOUNDS", audio.enableComboSounds)) { processedLines++; return true; }
    if (setb("ENABLE_LEVEL_UP_SOUNDS", audio.enableLevelUpSounds)) { processedLines++; return true; }
    
    return false;
}

// ===========================
//   SISTEMA DE JOYSTICK MODULAR
// ===========================

// Joystick classes moved to include/input/JoystickSystem.hpp and src/input/JoystickSystem.cpp
// JoystickInput moved to include/input/JoystickInput.hpp and src/input/JoystickInput.cpp

// Global piece manager instance
PieceManager pieceManager;

// Funções que usam pieceManager (definidas após sua declaração)
static bool loadPiecesFile(){ return pieceManager.loadPiecesFile(); }
static void seedPiecesFallback(){ pieceManager.seedFallback(); }

/* moved to src/pieces/PieceManager.cpp: pm_loadPiecesFromStream */

static bool processJoystickConfigs(const std::string& key, const std::string& val, int& processedLines, JoystickSystem& joystick) {
    auto seti = [&](const char* K, int& ref) { 
        if (key == K) { 
            int v = std::atoi(val.c_str());
            if (v >= 0 && v < 32) {
                ref = v; 
                return true; 
            }
        } 
        return false; 
    };
    auto setf = [&](const char* K, float& ref) { 
        if (key == K) { 
            float v = (float)std::atof(val.c_str());
            if (v >= 0.0f && v <= 1.0f) {
                ref = v; 
                return true; 
            }
        } 
        return false; 
    };

    // Get reference to config for easier access
    JoystickConfig& config = joystick.getConfig();

    // Configurações de mapeamento de botões
    if (seti("JOYSTICK_BUTTON_LEFT", config.buttonLeft)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_RIGHT", config.buttonRight)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_DOWN", config.buttonDown)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_UP", config.buttonUp)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_ROTATE_CCW", config.buttonRotateCCW)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_ROTATE_CW", config.buttonRotateCW)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_SOFT_DROP", config.buttonSoftDrop)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_HARD_DROP", config.buttonHardDrop)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_PAUSE", config.buttonPause)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_START", config.buttonStart)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_QUIT", config.buttonQuit)) { processedLines++; return true; }
    
    // Configurações de analógico
    if (setf("JOYSTICK_ANALOG_DEADZONE", config.analogDeadzone)) { processedLines++; return true; }
    if (setf("JOYSTICK_ANALOG_SENSITIVITY", config.analogSensitivity)) { processedLines++; return true; }
    if (key == "JOYSTICK_INVERT_Y_AXIS") {
        int v = std::atoi(val.c_str());
        config.invertYAxis = (v != 0);
        processedLines++;
        return true;
    }
    
    // Configurações de timing
    if (key == "JOYSTICK_MOVE_REPEAT_DELAY") {
        int v = std::atoi(val.c_str());
        if (v >= 50 && v <= 1000) {
            config.moveRepeatDelay = v;
            processedLines++;
            return true;
        }
    }
    if (key == "JOYSTICK_SOFT_DROP_DELAY") {
        int v = std::atoi(val.c_str());
        if (v >= 50 && v <= 500) {
            config.softDropRepeatDelay = v;
            processedLines++;
            return true;
        }
    }
    
    // Configurações de velocidade do jogo
    if (seti("GAME_SPEED_START_MS", gameConfig.tickMsStart)) { processedLines++; return true; }
    if (seti("GAME_SPEED_MIN_MS", gameConfig.tickMsMin)) { processedLines++; return true; }
    if (seti("GAME_SPEED_ACCELERATION", SPEED_ACCELERATION)) { processedLines++; return true; }
    
    // Configurações de renderização
    if (setf("ASPECT_CORRECTION_FACTOR", ASPECT_CORRECTION_FACTOR)) { processedLines++; return true; }
    if (key == "PREVIEW_GRID") {
        int v = std::atoi(val.c_str());
        if (v > 0 && v <= 10) {
            pieceManager.setPreviewGrid(v);
            processedLines++;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Apply game configuration to GameState
 */
static void applyConfigToGame(GameState& state, const GameConfig& config) {
    gameConfig.tickMsStart = config.tickMsStart;
    gameConfig.tickMsMin = config.tickMsMin;
    SPEED_ACCELERATION = config.speedAcceleration;
    LEVEL_STEP = config.levelStep;
    ASPECT_CORRECTION_FACTOR = config.aspectCorrectionFactor;
    
    // Apply configuration to the new modular systems
    state.getScore().setTickMs(config.tickMsStart);
}

// ============
// Bridge impl
// ============
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
    r = c.r; g = c.g; b = c.b; occ = c.occ; return true;
}
bool db_getActive(const GameState& state, int& idx, int& rot, int& x, int& y) {
    const Active& a = state.getActivePiece();
    idx = a.idx; rot = a.rot; x = a.x; y = a.y; return true;
}
bool db_getNextIdx(const GameState& state, int& nextIdx) {
    nextIdx = state.getNextIdx(); return true;
}
bool db_isPaused(const GameState& state) { return state.isPaused(); }
bool db_isGameOver(const GameState& state) { return state.isGameOver(); }
int db_getScore(const GameState& state) { return state.getScoreValue(); }
int db_getLines(const GameState& state) { return state.getLinesValue(); }
int db_getLevel(const GameState& state) { return state.getLevelValue(); }
bool db_isRunning(const GameState& state) { return state.isRunning(); }
void db_update(GameState& state, SDL_Renderer* renderer) { state.update(renderer); }
void db_render(GameState& state, RenderManager& renderManager, const LayoutCache& layout) { state.render(renderManager, layout); }
void db_layoutCalculate(LayoutCache& layout);
extern VisualEffectsView g_visualView; // already defined above
const VisualEffectsView& db_getVisualEffects() { return g_visualView; }

/**
 * @brief Apply pieces configuration to global theme
 */
static void applyConfigToPieces(const PiecesConfig& config) {
    
    // Apply piece colors
    if (!config.pieceColors.empty()) {
        themeManager.getTheme().piece_colors.clear();
        for (const auto& color : config.pieceColors) {
            themeManager.getTheme().piece_colors.push_back({color.r, color.g, color.b});
        }
    }
}

// Implementação da função initializeRandomizer
static void initializeRandomizer(GameState& state) {
    pieceManager.initializeRandomizer();
    
    // Use the new PieceManager system
    state.getPieces().reset();
    
    // Get the first piece and set it as active
    int firstPiece = state.getPieces().getNextPiece();
    newActive(state.getActivePiece(), firstPiece);
    
    // Set the next piece
    state.getPieces().setNextPiece(state.getPieces().getNextPiece());
    
    state.setLastTick(SDL_GetTicks());
    state.getCombo().reset();  // Reset combo no início
}

// moved to src/render/LayoutHelpers.cpp

// Função comum para eliminar duplicação
// DEPRECATED functions removed (processPieceFall, handleInput, checkTensionSound, updateGame)
// All functionality moved to GameState methods

// Funções especializadas extraídas da main
static bool initializeSDL() {
    // Inicializar cada subsistema separadamente para maior compatibilidade
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        DebugLogger::error("SDL_INIT_VIDEO error: " + std::string(SDL_GetError()));
        return false;
    }
    
    if (SDL_Init(SDL_INIT_TIMER) != 0) {
        DebugLogger::error("SDL_INIT_TIMER error: " + std::string(SDL_GetError()));
        return false;
    }
    
    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        DebugLogger::error("SDL_INIT_EVENTS error: " + std::string(SDL_GetError()));
        return false;
    }
    
    // Áudio e gamepad são opcionais
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        SDL_Log("Warning: SDL_INIT_AUDIO failed: %s", SDL_GetError());
    }
    
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("Warning: SDL_INIT_GAMECONTROLLER failed: %s", SDL_GetError());
    }
    
    if (SDL_Init(SDL_INIT_JOYSTICK) != 0) {
        SDL_Log("Warning: SDL_INIT_JOYSTICK failed: %s", SDL_GetError());
    }
    
    return true;
}

static void applyConfigToJoystick(InputManager& inputManager, const InputConfig& config) {
    // Find JoystickInput handler in InputManager
    for (auto& handler : inputManager.getHandlers()) {
        if (auto* joystickInput = dynamic_cast<JoystickInput*>(handler.get())) {
            JoystickConfig& joystickConfig = joystickInput->getConfig();
            
            // Apply button mappings
            joystickConfig.buttonLeft = config.buttonLeft;
            joystickConfig.buttonRight = config.buttonRight;
            joystickConfig.buttonDown = config.buttonDown;
            joystickConfig.buttonUp = config.buttonUp;
            joystickConfig.buttonRotateCCW = config.buttonRotateCCW;
            joystickConfig.buttonRotateCW = config.buttonRotateCW;
            joystickConfig.buttonSoftDrop = config.buttonSoftDrop;
            joystickConfig.buttonHardDrop = config.buttonHardDrop;
            joystickConfig.buttonPause = config.buttonPause;
            joystickConfig.buttonStart = config.buttonStart;
            joystickConfig.buttonQuit = config.buttonQuit;
            
            // Apply analog settings
            joystickConfig.analogDeadzone = config.analogDeadzone;
            joystickConfig.analogSensitivity = config.analogSensitivity;
            joystickConfig.invertYAxis = config.invertYAxis;
            
            // Apply timing settings
            joystickConfig.moveRepeatDelay = config.moveRepeatDelay;
            joystickConfig.softDropRepeatDelay = config.softDropRepeatDelay;
            
            return;
        }
    }
    
    DebugLogger::warning("No JoystickInput handler found for configuration");
}

bool initializeGame(GameState& state, AudioSystem& audio, ConfigManager& configManager, InputManager& inputManager) {
    // Load configuration using new system
    if (!configManager.loadAll()) {
        DebugLogger::error("Failed to load configuration");
        return false;
    }
    
    // Set dependencies for GameState (legacy compatibility)
    state.setDependencies(&audio, &themeManager, &pieceManager, &inputManager, &configManager);
    
    // Apply configuration to existing systems
    applyConfigToAudio(audio, configManager.getAudio());
    applyConfigToTheme(configManager.getVisual());
    applyConfigToGame(state, configManager.getGame());
    applyConfigToPieces(configManager.getPieces());
    
    // Apply joystick configuration to InputManager
    applyConfigToJoystick(inputManager, configManager.getInput());
    
    // Carregar peças
    bool piecesOk = loadPiecesFile();
    if (!piecesOk) {
        seedPiecesFallback();
    }
    
    // Aplicar tema
    applyThemePieceColors();
    
    // Initialize PieceManager after PIECES is loaded
    state.getPieces().initialize();
    
    char piecesInfo[256];
    snprintf(piecesInfo, sizeof(piecesInfo), "Pieces: %zu, PreviewGrid=%d, Randomizer=%s, BagSize=%d",
           PIECES.size(), pieceManager.getPreviewGrid(), (pieceManager.getRandomizerType() == RandType::BAG ? "bag" : "simple"), pieceManager.getRandBagSize());
    DebugLogger::info(piecesInfo);
    
    char audioInfo[256];
    snprintf(audioInfo, sizeof(audioInfo), "Audio: Master=%.1f, SFX=%.1f, Ambient=%.1f", 
           audio.masterVolume, audio.sfxVolume, audio.ambientVolume);
    DebugLogger::info(audioInfo);
    
    return true;
}

static bool initializeWindow(SDL_Window*& win, SDL_Renderer*& ren) {
    SDL_DisplayMode dm; 
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        DebugLogger::error("Failed to get display mode: " + std::string(SDL_GetError()));
        return false;
    }
    int SW = dm.w, SH = dm.h;
    
    win = SDL_CreateWindow("DropBlocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SW, SH,
                          SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) { 
        DebugLogger::error("Failed to create window: " + std::string(SDL_GetError()));
        return false; 
    }
    
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { 
        DebugLogger::error("Failed to create renderer: " + std::string(SDL_GetError()));
        return false; 
    }
    
    // Esconder cursor do mouse
    SDL_ShowCursor(SDL_DISABLE);
    
    return true;
}

// Declaração forward para initializeRandomizer
static void initializeRandomizer(GameState& state);

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
    // Instanciar classes de gerenciamento
    GameInitializer initializer;
    GameLoop gameLoop;
    GameCleanup cleanup;

    // Objetos do jogo
    AudioSystem audio;
    InputManager inputManager;
    ConfigManager configManager;
    GameState state;
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;

    // Inicialização completa usando GameInitializer
    printf("🚀 DropBlocks v%s - %s\n", DROPBLOCKS_VERSION, DROPBLOCKS_BUILD_INFO);
    printf("   Features: %s\n\n", DROPBLOCKS_FEATURES);
    printf("🔧 Testing Complete GameInitializer...\n");
    if (!initializer.initializeComplete(audio, inputManager, configManager, state, win, ren)) {
        printf("❌ GameInitializer complete test failed!\n");
        return 1;
    }
    printf("✅ GameInitializer complete test passed!\n");

    // Verificar status de inicialização
    printf("\n🔍 INITIALIZATION STATUS:\n");
    printf("%s SDL2: %s\n", initializer.isSDLInitialized() ? "✅" : "❌", initializer.isSDLInitialized() ? "OK" : "FAILED");
    printf("%s Audio: %s\n", initializer.isAudioInitialized() ? "✅" : "❌", initializer.isAudioInitialized() ? "OK" : "FAILED");
    printf("%s Input: %s\n", initializer.isInputInitialized() ? "✅" : "❌", initializer.isInputInitialized() ? "OK" : "FAILED");
    printf("%s Config: %s\n", initializer.isConfigInitialized() ? "✅" : "❌", initializer.isConfigInitialized() ? "OK" : "FAILED");
    printf("%s Fullscreen Window: %s\n", initializer.isWindowInitialized() ? "✅" : "❌", initializer.isWindowInitialized() ? "OK" : "FAILED");
    printf("%s GameState: %s\n", initializer.isGameStateInitialized() ? "✅" : "❌", initializer.isGameStateInitialized() ? "OK" : "FAILED");

    // Configurar render manager
    RenderManager renderManager(ren);
    
    // Adicionar camadas de renderização
    renderManager.addLayer(std::make_unique<BackgroundLayer>());
    renderManager.addLayer(std::make_unique<BannerLayer>(&audio));
    renderManager.addLayer(std::make_unique<BoardLayer>());
    renderManager.addLayer(std::make_unique<HUDLayer>());
    renderManager.addLayer(std::make_unique<OverlayLayer>());
    renderManager.addLayer(std::make_unique<PostEffectsLayer>(&audio));
    
    // Inicializar randomizador
    initializeRandomizer(state);

    printf("\n🎉 Initialization completed successfully!\n");

    // Loop principal usando GameLoop
    gameLoop.run(state, renderManager, ren);

    // Limpeza usando GameCleanup
    cleanup.cleanupAll(audio, inputManager, renderManager, win, ren);

    return 0;
}
