#pragma once

#include <SDL2/SDL.h>
#include "InputHandler.hpp"
#include "InputTimingManager.hpp"

class KeyboardInput : public InputHandler {
private:
    // Clean key states (no OS auto-repeat)
    bool keyStates[SDL_NUM_SCANCODES] = {false};
    
    // Unified timing manager (same as joystick for uniformity)
    InputTimingManager timingManager_;
    
    // Timers for single-press actions
    InputTimingManager::DirectionTimer rotateCCWTimer_;
    InputTimingManager::DirectionTimer rotateCWTimer_;
    InputTimingManager::DirectionTimer hardDropTimer_;
    InputTimingManager::DirectionTimer pauseTimer_;
    InputTimingManager::DirectionTimer restartTimer_;
    InputTimingManager::DirectionTimer forceRestartTimer_;
    InputTimingManager::DirectionTimer quitTimer_;
    InputTimingManager::DirectionTimer screenshotTimer_;
    InputTimingManager::DirectionTimer debugTimer_;
    InputTimingManager::DirectionTimer timerToggleTimer_;

    bool isKeyActive(SDL_Scancode key) {
        return keyStates[key];
    }

public:
    // Movement with DAS/ARR (uniform with joystick)
    bool shouldMoveLeft() override { 
        return timingManager_.shouldTriggerHorizontal(isKeyActive(SDL_SCANCODE_LEFT), true); 
    }
    
    bool shouldMoveRight() override { 
        return timingManager_.shouldTriggerHorizontal(isKeyActive(SDL_SCANCODE_RIGHT), false); 
    }
    
    bool shouldSoftDrop() override { 
        // Soft drop with repeat (uniform with joystick)
        return timingManager_.shouldTriggerVertical(isKeyActive(SDL_SCANCODE_DOWN)); 
    }
    
    // Single-press actions
    bool shouldHardDrop() override { 
        return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_SPACE), hardDropTimer_); 
    }
    
    bool shouldRotateCCW() override { 
        bool isActive = isKeyActive(SDL_SCANCODE_Z) || isKeyActive(SDL_SCANCODE_UP);
        return timingManager_.shouldTriggerOnce(isActive, rotateCCWTimer_); 
    }
    
    bool shouldRotateCW() override { 
        return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_X), rotateCWTimer_); 
    }
    
    bool shouldPause() override { 
        return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_P), pauseTimer_); 
    }
    
    bool shouldRestart() override { 
        return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_RETURN), restartTimer_); 
    }
    
    bool shouldForceRestart() override { 
        return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_R), forceRestartTimer_); 
    }
    
    bool shouldQuit() override { 
        return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_ESCAPE), quitTimer_); 
    }
    
    bool shouldScreenshot() override { 
        return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_F12), screenshotTimer_); 
    }
    
    bool shouldToggleDebug() { 
        return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_D), debugTimer_); 
    }
    
    bool shouldToggleTimer() override { 
        return timingManager_.shouldTriggerOnce(isKeyActive(SDL_SCANCODE_T), timerToggleTimer_); 
    }

    // Handle SDL events to get clean key press/release (no OS auto-repeat)
    void handleKeyEvent(const SDL_KeyboardEvent& event);
    
    void update() override {
        // Update is now handled by SDL events, not polling
    }
    
    bool isConnected() override { return true; }
    
    void resetTimers() override { 
        timingManager_.resetAllTimers(); 
    }
    
    // Access to timing manager for configuration
    InputTimingManager& getTimingManager() { return timingManager_; }
    const InputTimingManager& getTimingManager() const { return timingManager_; }
};


