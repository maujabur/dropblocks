#pragma once

#include <SDL2/SDL.h>

/**
 * @brief Unified Input Timing Manager
 * 
 * Centralizes DAS/ARR timing logic for both keyboard and joystick inputs.
 * Eliminates code duplication and ensures consistent timing behavior across all input devices.
 * 
 * DAS (Delayed Auto Shift): Initial delay before auto-repeat begins
 * ARR (Auto Repeat Rate): Interval between subsequent repeats
 */
class InputTimingManager {
public:
    /**
     * @brief Timer state for a single direction/action
     */
    struct DirectionTimer {
        Uint32 pressTime = 0;        // When the action was first activated
        Uint32 lastTriggerTime = 0;  // When the action was last triggered
        bool wasActive = false;      // Previous frame state for press detection
        
        void reset() {
            pressTime = 0;
            lastTriggerTime = 0;
            wasActive = false;
        }
    };
    
    /**
     * @brief Timing configuration
     */
    struct TimingConfig {
        Uint32 DAS = 170;  // Delayed Auto Shift (ms) - initial delay
        Uint32 ARR = 50;   // Auto Repeat Rate (ms) - repeat interval
        Uint32 softDropDelay = 100;  // Special timing for soft drop (ms)
    };

private:
    TimingConfig config_;
    
    // Direction timers for different actions
    DirectionTimer leftTimer_;
    DirectionTimer rightTimer_;
    DirectionTimer downTimer_;
    
public:
    /**
     * @brief Constructor with default professional timing
     */
    InputTimingManager();
    
    /**
     * @brief Constructor with custom timing
     */
    explicit InputTimingManager(const TimingConfig& config);
    
    /**
     * @brief Check if horizontal movement should trigger (with DAS/ARR)
     * @param isActive Current state (true if button/stick is active)
     * @param isLeft True for left movement, false for right
     * @return True if movement should be triggered this frame
     */
    bool shouldTriggerHorizontal(bool isActive, bool isLeft);
    
    /**
     * @brief Check if vertical movement (soft drop) should trigger
     * @param isActive Current state (true if button/stick is active)
     * @return True if soft drop should be triggered this frame
     */
    bool shouldTriggerVertical(bool isActive);
    
    /**
     * @brief Check if action should trigger once on press (no auto-repeat)
     * Used for rotations, hard drop, pause, etc.
     * @param isActive Current state
     * @param timer Timer to track press state
     * @return True if action should trigger (on press only)
     */
    bool shouldTriggerOnce(bool isActive, DirectionTimer& timer);
    
    /**
     * @brief Reset all timers (called when game state changes)
     */
    void resetAllTimers();
    
    /**
     * @brief Get current timing configuration
     */
    const TimingConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update timing configuration
     */
    void setConfig(const TimingConfig& config);
    
    /**
     * @brief Update DAS/ARR values
     */
    void setTiming(Uint32 das, Uint32 arr);
    
    /**
     * @brief Update soft drop timing
     */
    void setSoftDropTiming(Uint32 delay);

private:
    /**
     * @brief Core timing logic with DAS/ARR system
     * @param isActive Current input state
     * @param timer Direction timer to update
     * @param useSpecialDelay If true, uses softDropDelay instead of ARR
     * @return True if action should trigger
     */
    bool shouldTriggerWithTiming(bool isActive, DirectionTimer& timer, bool useSpecialDelay = false);
};