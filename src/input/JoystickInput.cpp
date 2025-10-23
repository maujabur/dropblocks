#include "input/JoystickInput.hpp"
#include <cmath>
#include <stdexcept>

JoystickInput::JoystickInput() 
    : joystickSystem_(std::make_unique<JoystickSystem>()) {}

bool JoystickInput::shouldMoveLeft() {
    return joystickSystem_ ? joystickSystem_->shouldMoveLeft() : false;
}

bool JoystickInput::shouldMoveRight() {
    return joystickSystem_ ? joystickSystem_->shouldMoveRight() : false;
}

bool JoystickInput::shouldSoftDrop() {
    return joystickSystem_ ? joystickSystem_->shouldSoftDrop() : false;
}

bool JoystickInput::shouldHardDrop() {
    return joystickSystem_ ? joystickSystem_->shouldHardDrop() : false;
}

bool JoystickInput::shouldRotateCCW() {
    return joystickSystem_ ? joystickSystem_->shouldRotateCCW() : false;
}

bool JoystickInput::shouldRotateCW() {
    return joystickSystem_ ? joystickSystem_->shouldRotateCW() : false;
}

bool JoystickInput::shouldPause() {
    return joystickSystem_ ? joystickSystem_->shouldPause() : false;
}

bool JoystickInput::shouldRestart() {
    return joystickSystem_ ? joystickSystem_->shouldRestart() : false;
}

bool JoystickInput::shouldForceRestart() {
    // Joystick force restart not implemented (keyboard only for now)
    return false;
}

bool JoystickInput::shouldQuit() {
    return joystickSystem_ ? joystickSystem_->shouldQuit() : false;
}

bool JoystickInput::shouldScreenshot() {
    return false; // Screenshot not supported on joystick
}

bool JoystickInput::shouldToggleDebug() {
    return false; // Debug toggle not supported on joystick (keyboard only)
}

bool JoystickInput::shouldToggleTimer() {
    return false; // Timer toggle not supported on joystick (keyboard only)
}

void JoystickInput::update() {
    if (joystickSystem_) {
        joystickSystem_->update();
    }
}

bool JoystickInput::isConnected() {
    return joystickSystem_ ? joystickSystem_->isConnected() : false;
}

void JoystickInput::resetTimers() {
    if (joystickSystem_) {
        joystickSystem_->resetTimers();
    }
}

bool JoystickInput::initialize() {
    return joystickSystem_ ? joystickSystem_->initialize() : false;
}

void JoystickInput::cleanup() {
    if (joystickSystem_) {
        joystickSystem_->cleanup();
    }
}

// Configuration access
JoystickConfig& JoystickInput::getConfig() {
    if (!joystickSystem_) {
        throw std::runtime_error("JoystickSystem not initialized");
    }
    return joystickSystem_->getConfig();
}

// Check if joystick has active input (for fallback to keyboard)
bool JoystickInput::hasActiveInput() {
    if (!joystickSystem_) return false;
    
    const auto& state = joystickSystem_->getState();
    const auto& config = joystickSystem_->getConfig();
    
    // Verificar se há botões pressionados ou movimento analógico
    for (int i = 0; i < 32; i++) {
        if (state.buttonStates[i]) return true;
    }
    
    // Verificar movimento analógico
    if (std::abs(state.leftStickX) > config.analogDeadzone || 
        std::abs(state.leftStickY) > config.analogDeadzone) {
        return true;
    }
    
    return false;
}

