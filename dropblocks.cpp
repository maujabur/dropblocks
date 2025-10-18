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
 * 
 * See docs/DEVELOPMENT_NOTES.md for TODOs and future improvements
 */

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
#include "config/ConfigApplicator.hpp"
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
#define DROPBLOCKS_VERSION "6.25"
#define DROPBLOCKS_BUILD_INFO "Phase 14.1: Cleanup piece loading wrappers"
#define DROPBLOCKS_FEATURES "Removed unnecessary piece loading wrappers; direct pieceManager calls"

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
VisualEffectsView g_visualView{};

// RandType moved to include/pieces/PieceManager.hpp
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
int LEVEL_STEP    = 10;

// Forward declarations moved to include/config/ConfigProcessors.hpp


// Global manager instances (temporary during migration)
GameConfig gameConfig;
ThemeManager themeManager;

// Forward declarations for classes
class PieceManager;


std::vector<Piece> PIECES;


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

// processBasicConfigs moved to src/config/ConfigProcessors.cpp

// processThemeColors moved to src/config/ConfigProcessors.cpp

// Declaração forward para AudioSystem e JoystickSystem (from modules)
struct AudioSystem;
class JoystickSystem;

// processAudioConfigs and processJoystickConfigs moved to ConfigProcessors

// processSpecialConfigs moved to src/config/ConfigProcessors.cpp

// loadConfigFromStream, loadConfigPath, loadConfigFile removed - replaced by ConfigManager system




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

// applyThemePieceColors moved to src/config/ConfigApplicator.cpp

// ===========================
//   FUNÇÕES DE APLICAÇÃO DE CONFIGURAÇÃO
// ===========================
// All apply* functions moved to src/config/ConfigApplicator.cpp

// applyConfigToAudio moved to src/config/ConfigApplicator.cpp
// applyConfigToTheme moved to src/config/ConfigApplicator.cpp
// processAudioConfigs moved to src/config/ConfigProcessors.cpp

// Global piece manager instance
PieceManager pieceManager;

// Piece loading wrappers removed - use pieceManager methods directly
// pm_loadPiecesFromStream moved to src/pieces/PieceManager.cpp
// processJoystickConfigs moved to src/config/ConfigProcessors.cpp

// applyConfigToGame moved to src/config/ConfigApplicator.cpp

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

// applyConfigToPieces moved to src/config/ConfigApplicator.cpp

// initializeRandomizer moved to src/app/GameInitializer.cpp (GameInit namespace)

// moved to src/render/LayoutHelpers.cpp

// Função comum para eliminar duplicação
// DEPRECATED functions removed (processPieceFall, handleInput, checkTensionSound, updateGame)
// All functionality moved to GameState methods

// initializeSDL moved to src/app/GameInitializer.cpp (GameInit namespace)

// applyConfigToJoystick moved to src/config/ConfigApplicator.cpp

// initializeGame moved to src/app/GameInitializer.cpp (GameInit namespace)
// initializeWindow moved to src/app/GameInitializer.cpp (GameInit namespace)

// Declaração forward para initializeRandomizer removed (duplicate)

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
    GameInit::initializeRandomizer(state);

    printf("\n🎉 Initialization completed successfully!\n");

    // Loop principal usando GameLoop
    gameLoop.run(state, renderManager, ren);

    // Limpeza usando GameCleanup
    cleanup.cleanupAll(audio, inputManager, renderManager, win, ren);

    return 0;
}
