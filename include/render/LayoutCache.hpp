#pragma once

#include <SDL2/SDL.h>
#include "ConfigTypes.hpp"

struct LayoutCache {
    // Physical screen dimensions
    int SWr, SHr;
    
    // New: Physical rectangles for each UI element (calculated from virtual coords)
    SDL_Rect bannerRect;
    SDL_Rect statsRect;
    SDL_Rect boardContainerRect;  // Container for the game board
    SDL_Rect hudRect;
    SDL_Rect nextRect;
    SDL_Rect scoreRect;
    
    // Element configurations (colors, etc)
    ElementLayout bannerConfig;
    ElementLayout statsConfig;
    ElementLayout boardConfig;
    ElementLayout hudConfig;
    ElementLayout nextConfig;
    ElementLayout scoreConfig;
    
    // Global border settings
    int borderRadius;
    int borderThickness;
    int borderRadiusX;  // For STRETCH mode (elliptical corners)
    int borderRadiusY;  // For STRETCH mode (elliptical corners)
    
    // Layout debug info (for DebugOverlay)
    int virtualWidth;
    int virtualHeight;
    float scaleX;
    float scaleY;
    float scaleTextX;  // Text scale (for STRETCH mode distortion)
    float scaleTextY;  // Text scale (for STRETCH mode distortion)
    int offsetX;
    int offsetY;
    ScaleMode scaleMode;
    
    // Legacy fields (kept for compatibility during transition)
    int CW, CH, CX, CY;
    int scale;
    int bannerW, panelTarget, usableLeftW;
    int cellBoard, GW, GH;
    float cellBoardW, cellBoardH;  // Separate cell dimensions (STRETCH can have W != H)
    int BX, BY, BW, BH, GX, GY;
    int panelX, panelW, panelY, panelH;
    int statsBoxW, statsMargin;
};

