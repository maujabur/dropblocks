#include "input/InputTimingManager.hpp"

// ===========================
//   InputTimingManager
// ===========================

InputTimingManager::InputTimingManager() {
    // Conservative timing values for debugging (ensuring uniformity)
    config_.DAS = 250;  // Delayed Auto Shift (more conservative)
    config_.ARR = 100;  // Auto Repeat Rate (slower repeat)
    config_.softDropDelay = 100;  // Soft drop timing
}

InputTimingManager::InputTimingManager(const TimingConfig& config) 
    : config_(config) {
}

bool InputTimingManager::shouldTriggerHorizontal(bool isActive, bool isLeft) {
    DirectionTimer& timer = isLeft ? leftTimer_ : rightTimer_;
    return shouldTriggerWithTiming(isActive, timer, false);
}

bool InputTimingManager::shouldTriggerVertical(bool isActive) {
    return shouldTriggerWithTiming(isActive, downTimer_, true);
}

bool InputTimingManager::shouldTriggerOnce(bool isActive, DirectionTimer& timer) {
    Uint32 now = SDL_GetTicks();
    
    // Detect press (transition from inactive to active)
    bool justPressed = isActive && !timer.wasActive;
    timer.wasActive = isActive;
    
    if (justPressed) {
        timer.pressTime = now;
        timer.lastTriggerTime = now;
        return true;
    }
    
    return false;
}

void InputTimingManager::resetAllTimers() {
    leftTimer_.reset();
    rightTimer_.reset();
    downTimer_.reset();
}

void InputTimingManager::setConfig(const TimingConfig& config) {
    config_ = config;
}

void InputTimingManager::setTiming(Uint32 das, Uint32 arr) {
    config_.DAS = das;
    config_.ARR = arr;
}

void InputTimingManager::setSoftDropTiming(Uint32 delay) {
    config_.softDropDelay = delay;
}

bool InputTimingManager::shouldTriggerWithTiming(bool isActive, DirectionTimer& timer, bool useSpecialDelay) {
    Uint32 now = SDL_GetTicks();
    
    // Detect press (transition from inactive to active)
    bool justPressed = isActive && !timer.wasActive;
    timer.wasActive = isActive;
    
    if (!isActive) {
        // Reset timer when input becomes inactive
        timer.pressTime = 0;
        return false;
    }
    
    // Just pressed - immediate response
    if (justPressed) {
        timer.pressTime = now;
        timer.lastTriggerTime = now;
        return true;
    }
    
    // Held down - apply DAS/ARR logic
    if (timer.pressTime > 0) {
        Uint32 heldTime = now - timer.pressTime;
        Uint32 repeatDelay = useSpecialDelay ? config_.softDropDelay : config_.ARR;
        
        // After DAS delay, check ARR for continuous triggering
        if (heldTime > config_.DAS && (now - timer.lastTriggerTime) > repeatDelay) {
            timer.lastTriggerTime = now;
            return true;
        }
    }
    
    return false;
}