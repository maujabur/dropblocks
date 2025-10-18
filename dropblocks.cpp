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
#define DROPBLOCKS_VERSION "7.0"
#define DROPBLOCKS_BUILD_INFO "Major Refactoring Complete"
#define DROPBLOCKS_FEATURES "Modular architecture: 661→328 lines (-50%); ConfigApplicator, GameInit, cleaned globals"

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
//   GLOBAL CONFIGURATION
// ===========================
// These globals are managed by ConfigManager and applied via ConfigApplicator.
// They remain global for performance (avoiding indirection) and backward compatibility.
// Future: Consider moving to a GlobalConfig singleton if more encapsulation is needed.

// Layout parameters (synced from VisualConfig.layout via ConfigApplicator)
int   ROUNDED_PANELS = 1;           // 1 = rounded; 0 = rectangle
int   HUD_FIXED_SCALE   = 6;        // Fixed HUD scale
std::string TITLE_TEXT  = "---H A C K T R I S";  // Vertical text (A-Z and space)
int   GAP1_SCALE        = 10;       // banner ↔ board (x scale)
int   GAP2_SCALE        = 10;       // board ↔ panel (x scale)

// Pieces configuration (managed by PieceManager)
std::string PIECES_FILE_PATH = "";  // Optional path to pieces file from config

// Visual effects bridge (provides read-only access to visual config)
VisualEffectsView g_visualView{};

// ===========================
//   GAME MECHANICS CONSTANTS
// ===========================
// These constants define core game behavior and are synced from GameConfig.

/** @brief Number of columns in the game board (constant) */
const int COLS = 10;
/** @brief Number of rows in the game board (constant) */
const int ROWS = 20;
/** @brief Border size around the game board (pixels) */
int BORDER = 10;
/** @brief Speed acceleration per level (ms reduction per level) */
int SPEED_ACCELERATION = 50;
/** @brief Aspect ratio correction factor for LED screen distortion */
float ASPECT_CORRECTION_FACTOR = 0.75f;
/** @brief Lines required to advance to next level */
int LEVEL_STEP = 10;

// ===========================
//   GLOBAL MANAGER INSTANCES
// ===========================
// These are the main subsystem managers. They coordinate configuration,
// theme, and piece management across the application.

GameConfig gameConfig;              // Game timing and mechanics configuration
ThemeManager themeManager;          // Visual theme and color management
std::vector<Piece> PIECES;          // Active piece set (loaded from .pieces file)


// Global piece manager instance
PieceManager pieceManager;

// Note: All bridge functions (db_*) moved to src/render/GameStateBridge.cpp

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
    // Display version info
    DebugLogger::info("DropBlocks v" + std::string(DROPBLOCKS_VERSION) + " - " + DROPBLOCKS_BUILD_INFO);
    DebugLogger::info("Features: " + std::string(DROPBLOCKS_FEATURES));
    
    // Create game objects
    AudioSystem audio;
    InputManager inputManager;
    ConfigManager configManager;
    GameState state;
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;
    
    // Initialize all systems
    GameInitializer initializer;
    if (!initializer.initializeComplete(audio, inputManager, configManager, state, win, ren)) {
        DebugLogger::error("Initialization failed");
        return 1;
    }
    
    // Setup rendering pipeline
    RenderManager renderManager(ren);
    renderManager.addLayer(std::make_unique<BackgroundLayer>());
    renderManager.addLayer(std::make_unique<BannerLayer>(&audio));
    renderManager.addLayer(std::make_unique<BoardLayer>());
    renderManager.addLayer(std::make_unique<HUDLayer>());
    renderManager.addLayer(std::make_unique<OverlayLayer>());
    renderManager.addLayer(std::make_unique<PostEffectsLayer>(&audio));
    
    // Initialize game randomizer
    GameInit::initializeRandomizer(state);
    
    DebugLogger::info("Initialization completed successfully");
    
    // Run game loop
    GameLoop gameLoop;
    gameLoop.run(state, renderManager, ren);
    
    // Cleanup
    GameCleanup cleanup;
    cleanup.cleanupAll(audio, inputManager, renderManager, win, ren);
    
    return 0;
}
