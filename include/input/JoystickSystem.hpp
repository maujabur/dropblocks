#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <memory>

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
    
    // Timing settings (matching keyboard DAS/ARR system)
    Uint32 moveRepeatDelayDAS = 170;     // DAS: Delayed Auto Shift (initial delay)
    Uint32 moveRepeatDelayARR = 50;      // ARR: Auto Repeat Rate (repeat interval)
    Uint32 softDropRepeatDelay = 100;    // ms entre soft drops repetidos
    
    // Configuration methods
    void setButtonMapping(int left, int right, int down, int up, int rotCCW, int rotCW, 
                         int softDrop, int hardDrop, int pause, int start, int quit);
    void setAnalogSettings(float deadzone, float sensitivity, bool invertY);
    void setTiming(Uint32 moveDelayDAS, Uint32 moveDelayARR, Uint32 softDropDelay);
};

/**
 * @brief Joystick state management
 * 
 * Tracks current state of buttons and analog inputs
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
    
    // Timers for DAS/ARR system (matching keyboard)
    Uint32 lastMoveTime = 0;
    Uint32 lastSoftDropTime = 0;
    Uint32 leftPressTime = 0;     // When left movement started
    Uint32 rightPressTime = 0;    // When right movement started
    Uint32 downPressTime = 0;     // When down movement started
    
    // State management
    void updateButtonStates(const JoystickDevice& device, const JoystickConfig& config);
    bool isButtonPressed(int button) const;
    void resetTimers();
    
    // DAS/ARR timing functions (matching keyboard implementation)
    bool shouldMoveHorizontalAnalog(float analogValue, Uint32& pressTime, const JoystickConfig& config, bool positive);
    bool shouldMoveVerticalAnalog(float analogValue, Uint32& pressTime, const JoystickConfig& config, bool positive);
    
    // Analog press detection (for rotation)
    bool isAnalogPressed(float currentValue, float lastValue, float deadzone, bool checkNegative) const;
};

/**
 * @brief Joystick input processor
 * 
 * Processes joystick input and converts to game actions
 */
class JoystickInputProcessor {
private:
    const JoystickConfig& config_;
    const JoystickState& state_;
    const JoystickDevice& device_;
    
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
    
    // Timer management
    void resetTimers();
};

