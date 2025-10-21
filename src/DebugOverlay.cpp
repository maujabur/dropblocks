#include "DebugOverlay.hpp"
#include "render/Primitives.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <vector>

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
    
    // Position in top-right corner of VIRTUAL area (not screen)
    // Use virtual area bounds to keep debug overlay inside clipped region
    int virtualAreaX = offsetX_;
    int virtualAreaY = offsetY_;
    int virtualAreaW = (int)(virtualW_ * scaleX_);
    int virtualAreaH = (int)(virtualH_ * scaleY_);
    
    // Debug overlay dimensions
    int debugWidth = 240;
    int debugMargin = 10;
    
    // Position relative to virtual area, with margin from edges
    // Ensure it stays within virtual bounds
    int x = virtualAreaX + virtualAreaW - debugWidth - debugMargin;
    if (x < virtualAreaX + debugMargin) x = virtualAreaX + debugMargin;  // Clamp to virtual area
    int y = virtualAreaY + 10;
    int lineHeight = 30;
    int scale = 2;
    
    // Semi-transparent background (increased height for layout and config info)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect bg = {x - 10, y - 5, 240, 450};
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
    
    // Layout information
    if (virtualW_ > 0) {
        y += 5; // Small gap
        drawPixelText(renderer, x, y, "LAYOUT:", scale, 255, 200, 100);
        y += lineHeight;
        
        {
            std::ostringstream oss;
            oss << "Virt: " << virtualW_ << "x" << virtualH_;
            drawPixelText(renderer, x, y, oss.str(), scale, 200, 200, 200);
        }
        y += lineHeight;
        
        {
            std::ostringstream oss;
            oss << "Phys: " << physicalW_ << "x" << physicalH_;
            // Highlight if different
            Uint8 r = (physicalW_ == virtualW_ && physicalH_ == virtualH_) ? 200 : 255;
            Uint8 g = (physicalW_ == virtualW_ && physicalH_ == virtualH_) ? 200 : 100;
            drawPixelText(renderer, x, y, oss.str(), scale, r, g, 100);
        }
        y += lineHeight;
        
        {
            std::ostringstream oss;
            oss << "Mode: " << scaleMode_;
            drawPixelText(renderer, x, y, oss.str(), scale, 200, 200, 200);
        }
        y += lineHeight;
        
        {
            std::ostringstream oss;
            oss << "Scl: " << std::fixed << std::setprecision(3) << scaleX_ << "," << scaleY_;
            // Highlight if scale is not 1:1
            Uint8 r = (scaleX_ == 1.0f && scaleY_ == 1.0f) ? 200 : 255;
            Uint8 g = (scaleX_ == 1.0f && scaleY_ == 1.0f) ? 200 : 100;
            drawPixelText(renderer, x, y, oss.str(), scale, r, g, 100);
        }
        y += lineHeight;
        
        {
            std::ostringstream oss;
            oss << "Off: " << offsetX_ << "," << offsetY_;
            // Highlight if offset is not 0,0
            Uint8 r = (offsetX_ == 0 && offsetY_ == 0) ? 200 : 255;
            Uint8 g = (offsetX_ == 0 && offsetY_ == 0) ? 200 : 100;
            drawPixelText(renderer, x, y, oss.str(), scale, r, g, 100);
        }
        y += lineHeight;
    }
    
    // Configuration info
    y += 5; // Small gap
    drawPixelText(renderer, x, y, "CONFIG:", scale, 255, 200, 100);
    y += lineHeight;
    
    drawPixelText(renderer, x, y, configFiles_, scale, 200, 200, 200);
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

void DebugOverlay::setLayoutInfo(int virtualW, int virtualH, int physicalW, int physicalH,
                                  float scaleX, float scaleY, int offsetX, int offsetY,
                                  const std::string& scaleMode) {
    virtualW_ = virtualW;
    virtualH_ = virtualH;
    physicalW_ = physicalW;
    physicalH_ = physicalH;
    scaleX_ = scaleX;
    scaleY_ = scaleY;
    offsetX_ = offsetX;
    offsetY_ = offsetY;
    scaleMode_ = scaleMode;
}

void DebugOverlay::setConfigInfo(const std::vector<std::string>& configPaths) {
    if (configPaths.empty()) {
        configFiles_ = "None";
        return;
    }
    
    // Extract just the filename from the first path (most recently loaded)
    std::string firstPath = configPaths.back();  // Last loaded file
    size_t lastSlash = firstPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        configFiles_ = firstPath.substr(lastSlash + 1);
    } else {
        configFiles_ = firstPath;
    }
    
    // If multiple files, add count
    if (configPaths.size() > 1) {
        configFiles_ += " (+" + std::to_string(configPaths.size() - 1) + ")";
    }
}

