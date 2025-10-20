#pragma once

#include <SDL2/SDL.h>
#include "ConfigTypes.hpp"

/**
 * @brief Transforms virtual coordinates to physical screen coordinates
 * 
 * Supports two modes:
 * - AUTO: Uniform scaling, maintains aspect ratio with black bars
 * - STRETCH: Independent X/Y scaling, fills screen (may distort)
 */
class VirtualLayout {
public:
    VirtualLayout();
    
    /**
     * @brief Configure virtual resolution and scale mode
     */
    void configure(int virtualWidth, int virtualHeight, ScaleMode mode);
    
    /**
     * @brief Set custom offsets (used in NATIVE mode)
     */
    void setCustomOffsets(int offsetX, int offsetY);
    
    /**
     * @brief Calculate transformation based on physical screen size
     */
    void calculate(int screenW, int screenH);
    
    /**
     * @brief Transform virtual rectangle to physical coordinates
     */
    SDL_Rect toPhysical(int vx, int vy, int vw, int vh) const;
    
    /**
     * @brief Transform virtual X coordinate to physical
     */
    int toPhysicalX(int vx) const;
    
    /**
     * @brief Transform virtual Y coordinate to physical
     */
    int toPhysicalY(int vy) const;
    
    /**
     * @brief Transform virtual width to physical
     */
    int toPhysicalW(int vw) const;
    
    /**
     * @brief Transform virtual height to physical
     */
    int toPhysicalH(int vh) const;
    
    /**
     * @brief Get physical screen dimensions
     */
    void getPhysicalSize(int& w, int& h) const { w = physicalW_; h = physicalH_; }
    
    /**
     * @brief Get virtual dimensions
     */
    void getVirtualSize(int& w, int& h) const { w = virtualW_; h = virtualH_; }
    
    /**
     * @brief Get black bar offsets (for AUTO mode)
     */
    void getOffsets(int& x, int& y) const { x = offsetX_; y = offsetY_; }
    
private:
    int virtualW_;
    int virtualH_;
    int physicalW_;
    int physicalH_;
    float scaleX_;
    float scaleY_;
    int offsetX_;
    int offsetY_;
    int customOffsetX_;  // Custom offsets for NATIVE mode
    int customOffsetY_;  // Custom offsets for NATIVE mode
    ScaleMode mode_;
};

