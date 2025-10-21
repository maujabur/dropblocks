#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <memory>
#include "InputTimingManager.hpp"

/**
 * @brief Joystick device management
 * 
 * Handles SDL joystick/controller detection and connection
 */
class JoystickDevice {
private:
    SDL_Joystick* joystick_ = nullptr;
    SDL_GameController* controller_ = nullptr;
    int joystickId_ = -1;
    bool isConnected_ = false;
    std::string deviceName_;
    
public:
    ~JoystickDevice();
    
    // Device management
    bool initialize();
    void cleanup();
    
    // Getters
    SDL_Joystick* getJoystick() const { return joystick_; }
    SDL_GameController* getController() const { return controller_; }
    int getJoystickId() const { return joystickId_; }
    bool isConnected() const { return isConnected_; }
    const std::string& getDeviceName() const { return deviceName_; }
};

/**
 * @brief Joystick configuration management
 * 
 * Handles button mapping and analog settings
 */
class JoystickConfig {
public:
    // Button mapping
    int buttonLeft = 13;      // D-pad left (padrão)
    int buttonRight = 11;     // D-pad right (padrão)
    int buttonDown = 14;      // D-pad down (padrão)
    int buttonUp = 12;        // D-pad up (padrão)
    int buttonRotateCCW = 0;  // A button (padrão)
    int buttonRotateCW = 1;   // B button (padrão)
    int buttonSoftDrop = 2;   // X button (padrão)
    int buttonHardDrop = 3;   // Y button (padrão)
    int buttonPause = 6;      // Back button (padrão)
    int buttonStart = 7;      // Start button (padrão)
    int buttonQuit = 8;       // Guide button (padrão)
    
    // Analog settings
    float analogDeadzone = 0.3f;     // Zona morta para analógico
    float analogSensitivity = 1.0f;  // Sensibilidade do analógico
    bool invertYAxis = false;        // Inverter eixo Y (padrão: false)
    
    // Timing settings (unified with InputTimingManager)
    Uint32 moveRepeatDelayDAS = 170;     // DAS: Delayed Auto Shift (initial delay)
    Uint32 moveRepeatDelayARR = 50;      // ARR: Auto Repeat Rate (repeat interval)
    Uint32 softDropRepeatDelay = 100;    // ms entre soft drops repetidos
    
    // Configuration methods
    void setButtonMapping(int left, int right, int down, int up, int rotCCW, int rotCW, 
                         int softDrop, int hardDrop, int pause, int start, int quit);
    void setAnalogSettings(float deadzone, float sensitivity, bool invertY);
    void setTiming(Uint32 moveDelayDAS, Uint32 moveDelayARR, Uint32 softDropDelay);
    
    // Get timing configuration for InputTimingManager
    InputTimingManager::TimingConfig getTimingConfig() const {
        InputTimingManager::TimingConfig config;
        config.DAS = moveRepeatDelayDAS;
        config.ARR = moveRepeatDelayARR;
        config.softDropDelay = softDropRepeatDelay;
        return config;
    }
};

/**
 * @brief Joystick state management
 * 
 * Tracks current state of buttons and analog inputs (simplified - timing handled by InputTimingManager)
 */
class JoystickState {
public:
    // Button states (for press detection)
    bool buttonStates[32] = {false};
    bool lastButtonStates[32] = {false};
    
    // Analog states
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;
    
    // Previous analog states (for press detection)
    float lastLeftStickX = 0.0f;
    float lastLeftStickY = 0.0f;
    
    // State management
    void updateButtonStates(const JoystickDevice& device, const JoystickConfig& config);
    bool isButtonPressed(int button) const;
    
    // Analog press detection (for rotation - single press only)
    bool isAnalogPressed(float currentValue, float lastValue, float deadzone, bool checkNegative) const;
};

/**
 * @brief Joystick input processor
 * 
 * Processes joystick input and converts to game actions using unified timing system
 */
class JoystickInputProcessor {
private:
    const JoystickConfig& config_;
    const JoystickState& state_;
    const JoystickDevice& device_;
    
    // Unified timing manager (replaces duplicated DAS/ARR logic)
    mutable InputTimingManager timingManager_;
    
    // Timers for single-press actions
    mutable InputTimingManager::DirectionTimer rotateCCWTimer_;
    mutable InputTimingManager::DirectionTimer rotateCWTimer_;
    mutable InputTimingManager::DirectionTimer hardDropTimer_;
    mutable InputTimingManager::DirectionTimer pauseTimer_;
    mutable InputTimingManager::DirectionTimer restartTimer_;
    
public:
    JoystickInputProcessor(const JoystickConfig& config, const JoystickState& state, const JoystickDevice& device);
    
    bool shouldMoveLeft() const;
    bool shouldMoveRight() const;
    bool shouldSoftDrop() const;
    bool shouldHardDrop() const;
    bool shouldRotateCCW() const;
    bool shouldRotateCW() const;
    bool shouldPause() const;
    bool shouldRestart() const;
    bool shouldQuit() const;
    bool shouldScreenshot() const;
    
    // Access to timing manager for configuration
    InputTimingManager& getTimingManager() const { return timingManager_; }
};

/**
 * @brief Main joystick system coordinator
 * 
 * Coordinates joystick device, configuration, state, and input processing
 */
class JoystickSystem {
private:
    JoystickDevice device_;
    JoystickConfig config_;
    JoystickState state_;
    std::unique_ptr<JoystickInputProcessor> processor_;
    
public:
    JoystickSystem();
    
    // System management
    bool initialize();
    void cleanup();
    void update();
    
    // Configuration
    JoystickConfig& getConfig() { return config_; }
    const JoystickConfig& getConfig() const { return config_; }
    
    // State access
    const JoystickState& getState() const { return state_; }
    const JoystickDevice& getDevice() const { return device_; }
    
    // Input processing
    bool shouldMoveLeft() const;
    bool shouldMoveRight() const;
    bool shouldSoftDrop() const;
    bool shouldHardDrop() const;
    bool shouldRotateCCW() const;
    bool shouldRotateCW() const;
    bool shouldPause() const;
    bool shouldRestart() const;
    bool shouldQuit() const;
    bool shouldScreenshot() const;
    
    // Connection status
    bool isConnected() const;
    
    // Timer management (unified)
    void resetTimers();
    
    // Access to timing manager for configuration
    InputTimingManager& getTimingManager() const;
};

