#include "app/GameInitializer.hpp"
#include "DebugLogger.hpp"
#include <SDL2/SDL.h>
#include "audio/AudioSystem.hpp"
#include "input/InputManager.hpp"
#include "ConfigManager.hpp"
#include "render/RenderLayer.hpp"
#include "render/RenderManager.hpp"
#include "input/KeyboardInput.hpp"
#include "input/InputHandler.hpp"
#include "ThemeManager.hpp"

#include "render/GameStateBridge.hpp"
class GameState; // defined in main TU
extern bool initializeGame(GameState& state, AudioSystem& audio, ConfigManager& configManager, InputManager& inputManager);

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
    if (!initializeGame(state, audio, configManager, inputManager)) return false;
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


