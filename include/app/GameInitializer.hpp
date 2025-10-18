#pragma once

#include <SDL2/SDL.h>

class AudioSystem;
class InputManager;
class ConfigManager;
class GameState;

/**
 * @brief Utility functions for game initialization
 * 
 * Standalone functions that can be used independently or via GameInitializer class
 */
namespace GameInit {
    /**
     * @brief Initialize SDL subsystems (video, audio, input, timer)
     */
    bool initializeSDL();
    
    /**
     * @brief Initialize window and renderer
     */
    bool initializeWindow(SDL_Window*& win, SDL_Renderer*& ren);
    
    /**
     * @brief Initialize game configuration and apply to all systems
     */
    bool initializeGame(GameState& state, AudioSystem& audio, ConfigManager& configManager, InputManager& inputManager);
    
    /**
     * @brief Initialize randomizer and set first piece
     */
    void initializeRandomizer(GameState& state);
}

class GameInitializer {
private:
    bool sdlInitialized_ = false;
    bool audioInitialized_ = false;
    bool inputInitialized_ = false;
    bool configInitialized_ = false;
    bool windowInitialized_ = false;
    bool gameStateInitialized_ = false;

public:
    bool initializeSDL();
    bool initializeBasic();
    bool initializeAudio(AudioSystem& audio);
    bool initializeInput(InputManager& inputManager);
    bool initializeConfig(ConfigManager& configManager);
    bool initializeWindow(SDL_Window*& win, SDL_Renderer*& ren);
    bool initializeGameState(GameState& state, AudioSystem& audio, ConfigManager& configManager, InputManager& inputManager);
    bool initializeComplete(AudioSystem& audio, InputManager& inputManager, ConfigManager& configManager, GameState& state, SDL_Window*& win, SDL_Renderer*& ren);

    bool isSDLInitialized() const { return sdlInitialized_; }
    bool isAudioInitialized() const { return audioInitialized_; }
    bool isInputInitialized() const { return inputInitialized_; }
    bool isConfigInitialized() const { return configInitialized_; }
    bool isWindowInitialized() const { return windowInitialized_; }
    bool isGameStateInitialized() const { return gameStateInitialized_; }
};


