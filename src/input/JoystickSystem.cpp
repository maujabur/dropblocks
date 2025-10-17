#include "input/JoystickSystem.hpp"
#include "DebugLogger.hpp"
#include <cmath>

// ===========================
//   JoystickDevice
// ===========================

JoystickDevice::~JoystickDevice() { 
    cleanup(); 
}

bool JoystickDevice::initialize() {
    // Verificar se SDL joystick subsystem está inicializado
    if (!SDL_WasInit(SDL_INIT_JOYSTICK)) {
        DebugLogger::error("SDL_INIT_JOYSTICK not initialized!");
        return false;
    }
    
    int numJoysticks = SDL_NumJoysticks();
    if (numJoysticks > 0) {
        if (SDL_IsGameController(0)) {
            controller_ = SDL_GameControllerOpen(0);
            if (controller_) {
                joystick_ = SDL_GameControllerGetJoystick(controller_);
                joystickId_ = 0;
                isConnected_ = true;
                deviceName_ = SDL_GameControllerName(controller_);
                DebugLogger::info("Game controller connected: " + deviceName_);
                return true;
            } else {
                DebugLogger::error("Failed to open game controller: " + std::string(SDL_GetError()));
            }
        }
        
        joystick_ = SDL_JoystickOpen(0);
        if (joystick_) {
            joystickId_ = 0;
            isConnected_ = true;
            deviceName_ = SDL_JoystickName(joystick_);
            DebugLogger::info("Joystick connected: " + deviceName_);
            return true;
        } else {
            DebugLogger::error("Failed to open joystick: " + std::string(SDL_GetError()));
        }
    }
    
    DebugLogger::warning("No joystick/controller found");
    return false;
}

void JoystickDevice::cleanup() {
    if (controller_) {
        SDL_GameControllerClose(controller_);
        controller_ = nullptr;
    }
    if (joystick_) {
        SDL_JoystickClose(joystick_);
        joystick_ = nullptr;
    }
    isConnected_ = false;
    deviceName_.clear();
}

// ===========================
//   JoystickConfig
// ===========================

void JoystickConfig::setButtonMapping(int left, int right, int down, int up, int rotCCW, int rotCW, 
                     int softDrop, int hardDrop, int pause, int start, int quit) {
    buttonLeft = left; buttonRight = right; buttonDown = down; buttonUp = up;
    buttonRotateCCW = rotCCW; buttonRotateCW = rotCW;
    buttonSoftDrop = softDrop; buttonHardDrop = hardDrop;
    buttonPause = pause; buttonStart = start; buttonQuit = quit;
}

void JoystickConfig::setAnalogSettings(float deadzone, float sensitivity, bool invertY) {
    analogDeadzone = deadzone;
    analogSensitivity = sensitivity;
    invertYAxis = invertY;
}

void JoystickConfig::setTiming(Uint32 moveDelay, Uint32 softDropDelay) {
    moveRepeatDelay = moveDelay;
    softDropRepeatDelay = softDropDelay;
}

// ===========================
//   JoystickState
// ===========================

void JoystickState::updateButtonStates(const JoystickDevice& device, const JoystickConfig& config) {
    // Copy current states to last states
    for (int i = 0; i < 32; i++) {
        lastButtonStates[i] = buttonStates[i];
    }
    
    // Update current button states
    // Always use joystick API for generic joysticks, even if detected as game controller
    if (device.getJoystick()) {
        // Regular joystick buttons - this works for both generic joysticks and game controllers
        for (int i = 0; i < 32; i++) {
            buttonStates[i] = SDL_JoystickGetButton(device.getJoystick(), i);
        }
    }
    
    // Update analog states
    // Always use joystick API for generic joysticks, even if detected as game controller
    if (device.getJoystick()) {
        // Regular joystick axes - this works for both generic joysticks and game controllers
        leftStickX = SDL_JoystickGetAxis(device.getJoystick(), 0) / 32767.0f;
        leftStickY = SDL_JoystickGetAxis(device.getJoystick(), 1) / 32767.0f;
        rightStickX = SDL_JoystickGetAxis(device.getJoystick(), 2) / 32767.0f;
        rightStickY = SDL_JoystickGetAxis(device.getJoystick(), 3) / 32767.0f;
        
        // Apply sensitivity and invert Y if needed
        leftStickX *= config.analogSensitivity;
        leftStickY *= config.analogSensitivity;
        rightStickX *= config.analogSensitivity;
        rightStickY *= config.analogSensitivity;
        
        if (config.invertYAxis) {
            leftStickY = -leftStickY;
            rightStickY = -rightStickY;
        }
    }
}

bool JoystickState::isButtonPressed(int button) const {
    if (button < 0 || button >= 32) return false;
    return buttonStates[button] && !lastButtonStates[button];
}

void JoystickState::resetTimers() {
    lastMoveTime = SDL_GetTicks();
}

// ===========================
//   JoystickInputProcessor
// ===========================

JoystickInputProcessor::JoystickInputProcessor(const JoystickConfig& config, const JoystickState& state, const JoystickDevice& device)
    : config_(config), state_(state), device_(device) {}

bool JoystickInputProcessor::shouldMoveLeft() const {
    return state_.isButtonPressed(config_.buttonLeft) || 
           (state_.leftStickX < -config_.analogDeadzone && SDL_GetTicks() - state_.lastMoveTime > config_.moveRepeatDelay) ||
           (device_.getController() && SDL_GameControllerGetButton(device_.getController(), SDL_CONTROLLER_BUTTON_DPAD_LEFT));
}

bool JoystickInputProcessor::shouldMoveRight() const {
    return state_.isButtonPressed(config_.buttonRight) || 
           (state_.leftStickX > config_.analogDeadzone && SDL_GetTicks() - state_.lastMoveTime > config_.moveRepeatDelay) ||
           (device_.getController() && SDL_GameControllerGetButton(device_.getController(), SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
}

bool JoystickInputProcessor::shouldSoftDrop() const {
    return state_.isButtonPressed(config_.buttonSoftDrop) || 
           (state_.leftStickY > config_.analogDeadzone && SDL_GetTicks() - state_.lastSoftDropTime > config_.softDropRepeatDelay) ||
           (device_.getController() && SDL_GameControllerGetButton(device_.getController(), SDL_CONTROLLER_BUTTON_DPAD_DOWN));
}

bool JoystickInputProcessor::shouldHardDrop() const {
    return state_.isButtonPressed(config_.buttonHardDrop);
}

bool JoystickInputProcessor::shouldRotateCCW() const {
    return state_.isButtonPressed(config_.buttonRotateCCW) || 
           (state_.leftStickY < -config_.analogDeadzone && SDL_GetTicks() - state_.lastMoveTime > config_.moveRepeatDelay) ||
           (device_.getController() && SDL_GameControllerGetButton(device_.getController(), SDL_CONTROLLER_BUTTON_DPAD_UP));
}

bool JoystickInputProcessor::shouldRotateCW() const {
    return state_.isButtonPressed(config_.buttonRotateCW) || 
           (state_.rightStickX > config_.analogDeadzone);
}

bool JoystickInputProcessor::shouldPause() const {
    return state_.isButtonPressed(config_.buttonPause);
}

bool JoystickInputProcessor::shouldRestart() const {
    return state_.isButtonPressed(config_.buttonStart);
}

bool JoystickInputProcessor::shouldQuit() const {
    return state_.isButtonPressed(config_.buttonQuit);
}

bool JoystickInputProcessor::shouldScreenshot() const {
    return false; // Screenshot not supported on joystick
}

// ===========================
//   JoystickSystem
// ===========================

JoystickSystem::JoystickSystem() {
    processor_ = std::make_unique<JoystickInputProcessor>(config_, state_, device_);
}

bool JoystickSystem::initialize() {
    return device_.initialize();
}

void JoystickSystem::cleanup() {
    device_.cleanup();
}

void JoystickSystem::update() {
    if (device_.isConnected()) {
        state_.updateButtonStates(device_, config_);
    }
}

bool JoystickSystem::shouldMoveLeft() const { return processor_->shouldMoveLeft(); }
bool JoystickSystem::shouldMoveRight() const { return processor_->shouldMoveRight(); }
bool JoystickSystem::shouldSoftDrop() const { return processor_->shouldSoftDrop(); }
bool JoystickSystem::shouldHardDrop() const { return processor_->shouldHardDrop(); }
bool JoystickSystem::shouldRotateCCW() const { return processor_->shouldRotateCCW(); }
bool JoystickSystem::shouldRotateCW() const { return processor_->shouldRotateCW(); }
bool JoystickSystem::shouldPause() const { return processor_->shouldPause(); }
bool JoystickSystem::shouldRestart() const { return processor_->shouldRestart(); }
bool JoystickSystem::shouldQuit() const { return processor_->shouldQuit(); }
bool JoystickSystem::shouldScreenshot() const { return processor_->shouldScreenshot(); }

bool JoystickSystem::isConnected() const { 
    return device_.isConnected(); 
}

void JoystickSystem::resetTimers() { 
    state_.resetTimers(); 
}

