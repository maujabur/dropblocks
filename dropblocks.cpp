/**
 * DropBlocks (SDL2) - VERSION 5.4 - Complete Timer System
 * =======================================================
 * 
 * FEATURES:
 * - Modular architecture: Config, Theme, Audio, Pieces, Render layers, App loop
 * - Visual effects via bridge (no global externs)  
 * - Centralized input (quit/pause/restart) via InputManager
 * - Guideline-like rotations (SRS) with fallback pieces
 * - Countdown Timer System with kiosk mode support:
 *   * Configurable duration, position, and colors via .cfg
 *   * Pause integration - timer freezes when game is paused
 *   * T-key toggle to enable/disable timer completely
 *   * Visual warnings at 30s and 10s remaining
 *   * Progress bar with enhanced visibility
 *   * Rounded borders matching game UI style
 *   * Game over on timer expiration
 * - Comprehensive joystick support with analog and button mapping
 * - Multiple configuration profiles (generic, neon-noir, rainbow, etc.)
 * - Virtual layout system with AUTO/STRETCH/NATIVE scaling modes
 * - Piece statistics tracking and display
 * - Score system with levels and line clearing
 * - Audio system with SFX and ambient sounds
 * - Debug overlay with performance metrics
 *
 * CONTROLS:
 * - Keyboard: Arrow keys, Z/X/Up, Space, P (pause), Enter, ESC, F12, T (timer toggle), R (restart), D (debug)
 * - Joystick: D-pad, buttons B0/B1/B8/B9, analog sticks (configurable deadzone)
 *
 * KIOSK MODE:
 * - Set TIMER_ENABLED=1 in .cfg file
 * - Configure TIMER_DURATION_SECONDS for session length
 * - Position with TIMER_X, TIMER_Y, TIMER_WIDTH, TIMER_HEIGHT
 * - Customize colors with TIMER_FILL, TIMER_TEXT_COLOR, warning colors
 *
 * BUILD:
 * - g++ -std=c++17 -Wall -Wextra -O2 -I./include dropblocks.cpp src/*.cpp src/app/*.cpp src/audio/*.cpp src/config/*.cpp src/di/*.cpp src/game/*.cpp src/input/*.cpp src/pieces/*.cpp src/render/*.cpp src/timer/*.cpp src/util/*.cpp `pkg-config --cflags --libs sdl2` -o dropblocks
 * 
 * CHANGELOG v5.4:
 * - Fixed timer pause functionality - now correctly freezes during game pause
 * - Improved T-key toggle - completely enables/disables timer visibility
 * - Enhanced progress bar visibility and thickness
 * - Removed outline parameters, using fill-only design
 * - Added comprehensive debug logging for troubleshooting
 * - Updated virtual layout integration with rounded borders
 * 
 * See docs/ for detailed configuration and development notes
 */

// SDL2
#include <SDL2/SDL.h>

// Application modules
#include "app/GameInitializer.hpp"
#include "app/GameLoop.hpp"
#include "app/GameCleanup.hpp"
#include "app/GameState.hpp"

// Rendering
#include "render/RenderManager.hpp"
#include "render/Layers.hpp"
#include "render/TimerRenderLayer.hpp"

// Input
#include "input/InputManager.hpp"
#include "audio/AudioSystem.hpp"

// Configuration
#include "ConfigManager.hpp"

// Utilities
#include "DebugLogger.hpp"

// Pieces and Theme (for global instances)
#include "pieces/PieceManager.hpp"
#include "render/GameStateBridge.hpp"
#include "pieces/Piece.hpp"
#include "ThemeManager.hpp"
#include "ConfigTypes.hpp"

// DI Container
#include "Interfaces.hpp"
#include "di/DependencyContainer.hpp"

// STL
#include <vector>
#include <string>
#include <memory>

// ===========================
//   DEFINIÇÕES DE VERSÃO
// ===========================
#define DROPBLOCKS_VERSION "9.2.4"
#define DROPBLOCKS_BUILD_INFO "Timer System - Themed Colors Complete"
#define DROPBLOCKS_FEATURES "Countdown Timer, Complete Themed Colors, Progress Bar Theming, Pause, T-Toggle"

// Math constant (if not defined by system)
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
std::string TITLE_TEXT  = "__H A C K T R I S";  // Vertical text (A-Z and space)
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
LayoutConfig layoutConfig;          // Virtual layout configuration


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
    renderManager.addLayer(std::make_unique<TimerRenderLayer>());  // Timer layer (early - before game elements)
    renderManager.addLayer(std::make_unique<BannerLayer>());
    renderManager.addLayer(std::make_unique<PieceStatsLayer>());
    renderManager.addLayer(std::make_unique<BoardLayer>());
    renderManager.addLayer(std::make_unique<HUDLayer>());
    renderManager.addLayer(std::make_unique<NextLayer>());
    renderManager.addLayer(std::make_unique<ScoreLayer>());
    renderManager.addLayer(std::make_unique<OverlayLayer>());
    renderManager.addLayer(std::make_unique<PostEffectsLayer>(&audio));
    
    // Initialize game randomizer
    GameInit::initializeRandomizer(state);
    
    // Run game loop
    GameLoop gameLoop;
    gameLoop.run(state, renderManager, ren, configManager, inputManager);
    
    // Cleanup
    GameCleanup cleanup;
    cleanup.cleanupAll(audio, inputManager, renderManager, win, ren);
    
    return 0;
}
