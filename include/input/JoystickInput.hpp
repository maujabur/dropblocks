#pragma once

#include "input/InputHandler.hpp"
#include "input/JoystickSystem.hpp"
#include <memory>

/**
 * @brief Joystick input handler
 * 
 * Handles joystick/controller input with analog and digital support
 * Uses the modular JoystickSystem internally
 */
class JoystickInput : public InputHandler {
private:
    std::unique_ptr<JoystickSystem> joystickSystem_;
    
public:
    JoystickInput();
    
    bool shouldMoveLeft() override;
    bool shouldMoveRight() override;
    bool shouldSoftDrop() override;
    bool shouldHardDrop() override;
    bool shouldRotateCCW() override;
    bool shouldRotateCW() override;
    bool shouldPause() override;
    bool shouldRestart() override;
    bool shouldForceRestart() override;
    bool shouldQuit() override;
    bool shouldScreenshot() override;
    
    void update() override;
    bool isConnected() override;
    void resetTimers() override;
    
    bool initialize();
    void cleanup();
    
    // Configuration access
    JoystickConfig& getConfig();
    
    // Check if joystick has active input (for fallback to keyboard)
    bool hasActiveInput();
};

