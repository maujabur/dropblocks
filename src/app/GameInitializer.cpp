#include "app/GameInitializer.hpp"
#include "DebugLogger.hpp"
#include <SDL2/SDL.h>
#include "audio/AudioSystem.hpp"
#include "input/InputManager.hpp"
#include "ConfigManager.hpp"
#include "config/ConfigApplicator.hpp"
#include "render/RenderLayer.hpp"
#include "render/RenderManager.hpp"
#include "input/KeyboardInput.hpp"
#include "input/InputHandler.hpp"
#include "ThemeManager.hpp"
#include "pieces/PieceManager.hpp"
#include "pieces/Piece.hpp"
#include "app/GameState.hpp"
#include "app/GameHelpers.hpp"
#include "render/GameStateBridge.hpp"
#include <cstdio>

// External globals from dropblocks.cpp
extern ThemeManager themeManager;
extern PieceManager pieceManager;
extern VisualEffectsView g_visualView;
extern std::vector<Piece> PIECES;

bool GameInitializer::initializeSDL() {
    if (sdlInitialized_) return true;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
        DebugLogger::error(std::string("SDL could not initialize: ") + SDL_GetError());
        return false;
    }
    sdlInitialized_ = true;
    return true;
}

bool GameInitializer::initializeBasic() {
    return initializeSDL();
}

bool GameInitializer::initializeAudio(AudioSystem& audio) {
    if (audioInitialized_) return true;
    if (!audio.initialize()) {
        DebugLogger::warning("Audio initialization failed, continuing without sound");
    }
    audioInitialized_ = true;
    return true;
}

bool GameInitializer::initializeInput(InputManager& inputManager) {
    if (inputInitialized_) return true;
    auto keyboardInput = std::make_unique<KeyboardInput>();
    inputManager.addHandler(std::move(keyboardInput));
    inputInitialized_ = true;
    return true;
}

bool GameInitializer::initializeConfig(ConfigManager& /*configManager*/) {
    if (configInitialized_) return true;
    configInitialized_ = true;
    return true;
}

bool GameInitializer::initializeWindow(SDL_Window*& win, SDL_Renderer*& ren) {
    if (windowInitialized_) return true;
    SDL_DisplayMode dm; if (SDL_GetCurrentDisplayMode(0, &dm) != 0) return false;
    int SW = dm.w, SH = dm.h;
    win = SDL_CreateWindow("DropBlocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SW, SH, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) return false;
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { SDL_DestroyWindow(win); return false; }
    windowInitialized_ = true;
    return true;
}

bool GameInitializer::initializeGameState(GameState& state, AudioSystem& audio, ConfigManager& configManager, InputManager& inputManager) {
    if (gameStateInitialized_) return true;
    if (!GameInit::initializeGame(state, audio, configManager, inputManager)) return false;
    gameStateInitialized_ = true;
    return true;
}

bool GameInitializer::initializeComplete(AudioSystem& audio, InputManager& inputManager, ConfigManager& configManager, GameState& state, SDL_Window*& win, SDL_Renderer*& ren) {
    if (!initializeSDL()) return false;
    if (!initializeAudio(audio)) return false;
    if (!initializeInput(inputManager)) return false;
    if (!initializeConfig(configManager)) return false;
    if (!initializeGameState(state, audio, configManager, inputManager)) return false;
    if (!initializeWindow(win, ren)) return false;
    return true;
}

// ============================================================================
// GameInit namespace - standalone initialization functions
// ============================================================================

namespace GameInit {

bool initializeSDL() {
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

bool initializeWindow(SDL_Window*& win, SDL_Renderer*& ren) {
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

bool initializeGame(GameState& state, AudioSystem& audio, ConfigManager& configManager, InputManager& inputManager) {
    // Load configuration using new system
    if (!configManager.loadAll()) {
        DebugLogger::error("Failed to load configuration");
        return false;
    }
    
    // Set dependencies for GameState (legacy compatibility)
    state.setDependencies(&audio, &themeManager, &pieceManager, &inputManager, &configManager);
    
    // Apply configuration to existing systems using ConfigApplicator
    ConfigApplicator::applyConfigToAudio(audio, configManager.getAudio());
    ConfigApplicator::applyConfigToTheme(configManager.getVisual(), themeManager, g_visualView);
    ConfigApplicator::applyConfigToGame(state, configManager.getGame());
    ConfigApplicator::applyConfigToPieces(configManager.getPieces(), themeManager);
    ConfigApplicator::applyConfigToLayout(configManager.getLayout());
    
    // Apply joystick configuration to InputManager
    ConfigApplicator::applyConfigToJoystick(inputManager, configManager.getInput());
    
    // Carregar peças
    bool piecesOk = pieceManager.loadPiecesFile();
    if (!piecesOk) {
        pieceManager.seedFallback();
    }
    
    // Aplicar tema
    ConfigApplicator::applyThemePieceColors(themeManager, PIECES);
    
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

void initializeRandomizer(GameState& state) {
    pieceManager.initializeRandomizer();
    
    // Reset statistics before starting new game
    state.resetPieceStats();
    
    // Use the new PieceManager system
    state.getPieces().reset();
    
    // Get the first piece and set it as active
    int firstPiece = state.getPieces().getNextPiece();
    newActive(state.getActivePiece(), firstPiece);
    
    // Incrementar estatísticas para a primeira peça
    state.incrementPieceStat(firstPiece);
    
    // Set the next piece
    state.getPieces().setNextPiece(state.getPieces().getNextPiece());
    
    state.setLastTick(SDL_GetTicks());
    state.getCombo().reset();  // Reset combo no início
}

} // namespace GameInit


