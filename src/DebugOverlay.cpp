#include "DebugOverlay.hpp"
#include "render/Primitives.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

void DebugOverlay::update(float deltaMs) {
    // Store frame time sample
    frameSamples_[sampleIndex_] = deltaMs;
    sampleIndex_ = (sampleIndex_ + 1) % SAMPLE_COUNT;
    
    // Calculate average frame time
    float sum = 0.0f;
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        sum += frameSamples_[i];
    }
    frameTimeMs_ = sum / SAMPLE_COUNT;
    
    // Calculate FPS from average frame time
    if (frameTimeMs_ > 0.0f) {
        fps_ = 1000.0f / frameTimeMs_;
    }
}

void DebugOverlay::render(SDL_Renderer* renderer, int screenWidth, int screenHeight) {
    if (!enabled_ || !renderer) return;
    
    // Position in top-right corner
    int x = screenWidth - 250;
    int y = 10;
    int lineHeight = 30;
    int scale = 2;
    
    // Semi-transparent background (increased height for extra line)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect bg = {x - 10, y - 5, 240, 230};
    SDL_RenderFillRect(renderer, &bg);
    
    // Border
    SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    SDL_RenderDrawRect(renderer, &bg);
    
    // Title
    drawPixelText(renderer, x, y, "DEBUG INFO", scale, 100, 255, 100);
    y += lineHeight;
    
    // FPS
    {
        std::ostringstream oss;
        oss << "FPS: " << std::fixed << std::setprecision(1) << fps_;
        Uint8 color = fps_ >= 58.0f ? 100 : (fps_ >= 30.0f ? 255 : 255);
        Uint8 g = fps_ >= 58.0f ? 255 : (fps_ >= 30.0f ? 200 : 100);
        drawPixelText(renderer, x, y, oss.str(), scale, color, g, 100);
    }
    y += lineHeight;
    
    // Frame Time
    {
        std::ostringstream oss;
        oss << "Frame: " << std::fixed << std::setprecision(2) << frameTimeMs_ << "ms";
        Uint8 color = frameTimeMs_ <= 16.7f ? 100 : (frameTimeMs_ <= 33.0f ? 255 : 255);
        Uint8 g = frameTimeMs_ <= 16.7f ? 255 : (frameTimeMs_ <= 33.0f ? 200 : 100);
        drawPixelText(renderer, x, y, oss.str(), scale, color, g, 100);
    }
    y += lineHeight;
    
    // Target frame time reference (split into 2 lines to avoid overflow)
    drawPixelText(renderer, x, y, "Target: 16.67ms", scale, 150, 150, 150);
    y += lineHeight;
    drawPixelText(renderer, x, y, "        (60 FPS)", scale, 150, 150, 150);
    y += lineHeight;
    
    // Custom values
    if (!customName1_.empty()) {
        std::string line = customName1_ + ": " + customValue1_;
        drawPixelText(renderer, x, y, line, scale, 200, 200, 255);
        y += lineHeight;
    }
    
    if (!customName2_.empty()) {
        std::string line = customName2_ + ": " + customValue2_;
        drawPixelText(renderer, x, y, line, scale, 200, 200, 255);
        y += lineHeight;
    }
}

void DebugOverlay::setCustomValue(const std::string& name, const std::string& value) {
    if (customName1_.empty()) {
        customName1_ = name;
        customValue1_ = value;
    } else if (customName1_ == name) {
        customValue1_ = value;
    } else {
        customName2_ = name;
        customValue2_ = value;
    }
}

