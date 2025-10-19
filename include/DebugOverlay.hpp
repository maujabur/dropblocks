#pragma once

#include <SDL2/SDL.h>
#include <string>

/**
 * @brief Debug overlay for development
 * 
 * Shows FPS, frame time, and other debug info.
 * Toggle with 'D' key.
 */
class DebugOverlay {
public:
    DebugOverlay() = default;
    
    /**
     * @brief Toggle debug overlay on/off
     */
    void toggle() { enabled_ = !enabled_; }
    
    /**
     * @brief Check if overlay is enabled
     */
    bool isEnabled() const { return enabled_; }
    
    /**
     * @brief Set enabled state
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    
    /**
     * @brief Update frame timing statistics
     * @param deltaMs Frame time in milliseconds
     */
    void update(float deltaMs);
    
    /**
     * @brief Render debug overlay
     * @param renderer SDL renderer
     * @param screenWidth Screen width
     * @param screenHeight Screen height
     */
    void render(SDL_Renderer* renderer, int screenWidth, int screenHeight);
    
    /**
     * @brief Get current FPS
     */
    float getFPS() const { return fps_; }
    
    /**
     * @brief Get current frame time
     */
    float getFrameTime() const { return frameTimeMs_; }
    
    /**
     * @brief Add a custom debug value
     */
    void setCustomValue(const std::string& name, const std::string& value);
    
private:
    bool enabled_ = false;
    float fps_ = 0.0f;
    float frameTimeMs_ = 0.0f;
    
    // Running average for smoother display
    static constexpr int SAMPLE_COUNT = 60;
    float frameSamples_[SAMPLE_COUNT] = {0};
    int sampleIndex_ = 0;
    
    // Custom debug values
    std::string customName1_;
    std::string customValue1_;
    std::string customName2_;
    std::string customValue2_;
};

