#include "render/LayoutCache.hpp"
#include "render/GameStateBridge.hpp"
#include <SDL2/SDL.h>
#include <algorithm>

extern const int COLS;
extern const int ROWS;
extern int BORDER;
extern int GAP1_SCALE;
extern int GAP2_SCALE;
extern int HUD_FIXED_SCALE;
extern float ASPECT_CORRECTION_FACTOR;

void db_layoutCalculate(LayoutCache& layout) {
    SDL_DisplayMode dmNow;
    SDL_GetCurrentDisplayMode(0, &dmNow);
    layout.SWr = dmNow.w;
    layout.SHr = dmNow.h;
    
    if (layout.SWr * 9 >= layout.SHr * 16) {
        layout.CH = layout.SHr;
        layout.CW = (layout.CH * 16) / 9;
        layout.CX = (layout.SWr - layout.CW) / 2;
        layout.CY = 0;
    } else {
        layout.CW = layout.SWr;
        layout.CH = (layout.CW * 9) / 16;
        layout.CX = 0;
        layout.CY = (layout.SHr - layout.CH) / 2;
    }
    
    layout.scale = HUD_FIXED_SCALE;
    layout.GAP1 = BORDER + GAP1_SCALE * layout.scale;
    layout.GAP2 = BORDER + GAP2_SCALE * layout.scale;
    layout.bannerW = 8 * layout.scale + 24;
    layout.panelTarget = (int)(layout.CW * 0.28);
    layout.usableLeftW = layout.CW - (BORDER + layout.bannerW + layout.GAP1) - layout.panelTarget - layout.GAP2;
    
    int cellW = layout.usableLeftW / COLS;
    int cellH = (layout.CH - 2 * BORDER) / ROWS;
    cellH = (int)(cellH * ASPECT_CORRECTION_FACTOR);
    layout.cellBoard = std::min(std::max(8, cellW), cellH);
    
    layout.GW = layout.cellBoard * COLS;
    layout.GH = layout.cellBoard * ROWS;
    layout.BX = layout.CX + BORDER;
    layout.BY = layout.CY + (layout.CH - layout.GH) / 2;
    layout.BW = layout.bannerW;
    layout.BH = layout.GH;
    layout.GX = layout.BX + layout.BW + layout.GAP1;
    layout.GY = layout.BY;
    layout.panelX = layout.GX + layout.GW + layout.GAP2;
    layout.panelW = layout.CX + layout.CW - layout.panelX - BORDER;
    layout.panelY = layout.BY;
    layout.panelH = layout.GH;
}

