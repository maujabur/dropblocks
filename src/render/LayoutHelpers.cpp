#include "render/LayoutCache.hpp"
#include "render/GameStateBridge.hpp"
#include "render/Primitives.hpp"
#include "render/VirtualLayout.hpp"
#include <SDL2/SDL.h>
#include <algorithm>

extern const int COLS;
extern const int ROWS;
extern int BORDER;
extern int GAP1_SCALE;
extern int GAP2_SCALE;
extern int HUD_FIXED_SCALE;
extern LayoutConfig layoutConfig;

// Initialize default layout if not configured
static void initializeDefaultLayout(LayoutConfig& config) {
    // Always ensure we have valid virtual dimensions
    if (config.virtualWidth <= 0) config.virtualWidth = 1920;
    if (config.virtualHeight <= 0) config.virtualHeight = 1080;
    
    // Set defaults for any element that has width == 0 (not configured)
    if (config.banner.width == 0) {
        config.banner = {10, 100, 80, 880, {0,40,0}, {0,60,0}, {120,255,120}, 255, 180, true};
    }
    
    if (config.stats.width == 0) {
        // Enable stats by default if not explicitly configured
        config.stats = {95, 100, 200, 880, {18,18,26}, {80,80,110}, {220,220,220}, 255, 160, true};
    }
    
    if (config.board.width == 0) {
        config.board = {300, 100, 600, 880, {0,0,0}, {0,0,0}, {255,255,255}, 255, 255, true};
    }
    
    if (config.hud.width == 0) {
        config.hud = {920, 100, 500, 880, {24,24,32}, {90,90,120}, {200,200,220}, 255, 200, true};
    }
    
    if (config.next.width == 0) {
        // Don't set defaults for NEXT - it should be explicitly configured
        // to be rendered as an independent layer
        config.next = {0, 0, 0, 0, {18,18,26}, {80,80,110}, {220,220,220}, 255, 160, false};
    }
    
    if (config.score.width == 0) {
        // Don't set defaults for SCORE - it should be explicitly configured
        // to be rendered as an independent layer
        config.score = {0, 0, 0, 0, {18,18,26}, {80,80,110}, {220,220,220}, 255, 160, false};
    }
}

void db_layoutCalculate(LayoutCache& layout, SDL_Renderer* renderer) {
    // Use SDL_GetRendererOutputSize to get the actual rendering resolution
    // This is important when SDL_WINDOW_ALLOW_HIGHDPI is used
    int w, h;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    layout.SWr = w;
    layout.SHr = h;
    
    // Initialize default layout if needed
    initializeDefaultLayout(layoutConfig);
    
    // Setup virtual layout transformer
    VirtualLayout virtualLayout;
    virtualLayout.configure(layoutConfig.virtualWidth, layoutConfig.virtualHeight, layoutConfig.scaleMode);
    virtualLayout.setCustomOffsets(layoutConfig.offsetX, layoutConfig.offsetY);  // Used only in NATIVE mode
    virtualLayout.calculate(w, h);
    
    // Transform virtual coordinates to physical for each element
    layout.bannerRect = virtualLayout.toPhysical(layoutConfig.banner.x, layoutConfig.banner.y, 
                                                  layoutConfig.banner.width, layoutConfig.banner.height);
    layout.statsRect = virtualLayout.toPhysical(layoutConfig.stats.x, layoutConfig.stats.y,
                                                 layoutConfig.stats.width, layoutConfig.stats.height);
    layout.boardContainerRect = virtualLayout.toPhysical(layoutConfig.board.x, layoutConfig.board.y,
                                                          layoutConfig.board.width, layoutConfig.board.height);
    layout.hudRect = virtualLayout.toPhysical(layoutConfig.hud.x, layoutConfig.hud.y,
                                              layoutConfig.hud.width, layoutConfig.hud.height);
    layout.nextRect = virtualLayout.toPhysical(layoutConfig.next.x, layoutConfig.next.y,
                                               layoutConfig.next.width, layoutConfig.next.height);
    layout.scoreRect = virtualLayout.toPhysical(layoutConfig.score.x, layoutConfig.score.y,
                                                layoutConfig.score.width, layoutConfig.score.height);
    
    // Copy element configurations
    layout.bannerConfig = layoutConfig.banner;
    layout.statsConfig = layoutConfig.stats;
    layout.boardConfig = layoutConfig.board;
    layout.hudConfig = layoutConfig.hud;
    layout.nextConfig = layoutConfig.next;
    layout.scoreConfig = layoutConfig.score;
    
    // Copy global border settings
    layout.borderRadius = layoutConfig.borderRadius;
    layout.borderThickness = layoutConfig.borderThickness;
    
    // Store layout debug info
    layout.virtualWidth = layoutConfig.virtualWidth;
    layout.virtualHeight = layoutConfig.virtualHeight;
    virtualLayout.getOffsets(layout.offsetX, layout.offsetY);
    layout.scaleX = 0; // Will be set below
    layout.scaleY = 0; // Will be set below
    layout.scaleMode = layoutConfig.scaleMode;
    
    // Calculate scales from transformation
    if (layoutConfig.virtualWidth > 0 && layoutConfig.virtualHeight > 0) {
        int physicalW = virtualLayout.toPhysicalW(layoutConfig.virtualWidth);
        int physicalH = virtualLayout.toPhysicalH(layoutConfig.virtualHeight);
        layout.scaleX = (float)physicalW / (float)layoutConfig.virtualWidth;
        layout.scaleY = (float)physicalH / (float)layoutConfig.virtualHeight;
    }
    
    // Calculate text scale (needs to be done AFTER layout.scale is set below)
    // This will be calculated after HUD_FIXED_SCALE is assigned to layout.scale
    layout.scaleTextX = 1.0f;  // Temporary, will be recalculated below
    layout.scaleTextY = 1.0f;  // Temporary, will be recalculated below
    
    // Calculate elliptical border radius (for STRETCH mode)
    if (layoutConfig.scaleMode == ScaleMode::STRETCH) {
        layout.borderRadiusX = (int)(layoutConfig.borderRadius * layout.scaleX);
        layout.borderRadiusY = (int)(layoutConfig.borderRadius * layout.scaleY);
    } else {
        // AUTO/NATIVE: circular corners
        float uniformScale = std::min(layout.scaleX, layout.scaleY);
        layout.borderRadiusX = (int)(layoutConfig.borderRadius * uniformScale);
        layout.borderRadiusY = (int)(layoutConfig.borderRadius * uniformScale);
    }
    
    // Calculate game board position/size within the board container
    // Board is 10×20, centered within container with black bars if needed
    
    // Get VIRTUAL container size
    int containerVirtW = layoutConfig.board.width;
    int containerVirtH = layoutConfig.board.height;
    
    // Calculate base cell size in VIRTUAL coordinates (square cells)
    int cellVirtW = containerVirtW / COLS;
    int cellVirtH = containerVirtH / ROWS;
    int cellVirtSize = std::min(cellVirtW, cellVirtH);
    cellVirtSize = std::max(8, cellVirtSize);  // Minimum 8px virtual cells
    
    // Now apply transformation to get PHYSICAL cell sizes
    float finalCellW, finalCellH;
    if (layoutConfig.scaleMode == ScaleMode::STRETCH) {
        // STRETCH: Cells distort according to independent X/Y scales
        finalCellW = cellVirtSize * layout.scaleX;
        finalCellH = cellVirtSize * layout.scaleY;
    } else {
        // AUTO/NATIVE: Cells remain square with uniform scale
        float uniformScale = std::min(layout.scaleX, layout.scaleY);
        finalCellW = cellVirtSize * uniformScale;
        finalCellH = cellVirtSize * uniformScale;
    }
    
    int boardW = (int)(finalCellW * COLS);
    int boardH = (int)(finalCellH * ROWS);
    
    // Center board within physical container
    int containerW = layout.boardContainerRect.w;
    int containerH = layout.boardContainerRect.h;
    layout.GX = layout.boardContainerRect.x + (containerW - boardW) / 2;
    layout.GY = layout.boardContainerRect.y + (containerH - boardH) / 2;
    layout.GW = boardW;
    layout.GH = boardH;
    layout.cellBoard = (int)std::min(finalCellW, finalCellH);  // Legacy field (use min for compatibility)
    layout.cellBoardW = finalCellW;
    layout.cellBoardH = finalCellH;
    
    // Legacy fields for compatibility
    int offsetX, offsetY;
    virtualLayout.getOffsets(offsetX, offsetY);
    layout.CX = offsetX;
    layout.CY = offsetY;
    
    virtualLayout.getVirtualSize(layout.CW, layout.CH);
    layout.CW = virtualLayout.toPhysicalW(layout.CW);
    layout.CH = virtualLayout.toPhysicalH(layout.CH);
    
    layout.scale = HUD_FIXED_SCALE;
    
    // NOW calculate text scale based on virtual transformation
    // Text must be scaled according to the virtual->physical transformation
    if (layoutConfig.scaleMode == ScaleMode::STRETCH) {
        // STRETCH: text distorts with independent X/Y scales
        layout.scaleTextX = layout.scale * layout.scaleX;
        layout.scaleTextY = layout.scale * layout.scaleY;
    } else {
        // AUTO/NATIVE: text scales uniformly
        float uniformScale = std::min(layout.scaleX, layout.scaleY);
        layout.scaleTextX = layout.scale * uniformScale;
        layout.scaleTextY = layout.scale * uniformScale;
    }
    
    layout.GAP1 = BORDER + GAP1_SCALE * layout.scale;
    layout.GAP2 = BORDER + GAP2_SCALE * layout.scale;
    layout.bannerW = layout.bannerRect.w;
    layout.BX = layout.bannerRect.x;
    layout.BY = layout.bannerRect.y;
    layout.BW = layout.bannerRect.w;
    layout.BH = layout.bannerRect.h;
    layout.statsBoxW = layout.statsRect.w;
    layout.statsMargin = 5;
    layout.panelX = layout.hudRect.x;
    layout.panelY = layout.hudRect.y;
    layout.panelW = layout.hudRect.w;
    layout.panelH = layout.hudRect.h;
}

