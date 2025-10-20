#include "render/Layers.hpp"
#include "render/RenderLayer.hpp"
#include "render/LayoutCache.hpp"
#include "ThemeManager.hpp"
#include "DebugLogger.hpp"
#include "audio/AudioSystem.hpp"
#include "pieces/Piece.hpp"
#include "pieces/PieceManager.hpp"
#include "render/Primitives.hpp"
#include "render/GameStateBridge.hpp"

#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>
#include <string>

// Externals from main translation unit
extern ThemeManager themeManager;
extern std::vector<Piece> PIECES;
extern PieceManager pieceManager;

// Helpers declared in main TU
void drawRoundedFilled(SDL_Renderer* r, int x, int y, int w, int h, int rad, Uint8 R, Uint8 G, Uint8 B, Uint8 A);
void drawRoundedOutline(SDL_Renderer* r, int x, int y, int w, int h, int rad, int thickness, Uint8 R, Uint8 G, Uint8 B, Uint8 A);
void drawPixelText(SDL_Renderer* r, int x, int y, const std::string& text, int scale, Uint8 R, Uint8 G, Uint8 B);
void drawPixelTextOutlined(SDL_Renderer* r, int x, int y, const std::string& text, int scale, Uint8 R, Uint8 G, Uint8 B, Uint8 oR, Uint8 oG, Uint8 oB);
int textWidthPx(const std::string& text, int scale);

// Globals configured in config (temporary; will migrate via bridge)
extern int ROUNDED_PANELS;
extern std::string TITLE_TEXT;

class GameState;

// ============================================================================
// HELPER FUNCTIONS: Scale offsets/dimensions consistently
// ============================================================================
// 
// IMPORTANT: In STRETCH mode, scaleX and scaleY can differ, causing distortion.
// All hardcoded offsets, padding, and spacing MUST be scaled to maintain
// correct proportions across different virtual resolutions.
//
// Usage guide:
//   scaleOffsetX/Y:      For panel padding, margins, initial positions
//   scaleTextSpacing:    For spacing between text lines (proportional to text size)
//   scaleCellSpacing:    For cell gaps (minimum 1px)
//
// Examples:
//   int x = baseX + scaleOffsetX(14, layout);      // Panel padding
//   int y = baseY + scaleOffsetY(10, layout);      // Panel margin
//   y += scaleTextSpacing(12, layout);             // Space after text line
//   int gap = scaleCellSpacing(1, layout.scaleX);  // Cell spacing
//
namespace {
    // Scale a virtual X offset to physical coordinates (for panel spacing)
    inline int scaleOffsetX(int virtualOffset, const LayoutCache& layout) {
        return (int)(virtualOffset * layout.scaleX);
    }
    
    // Scale a virtual Y offset to physical coordinates (for panel spacing)
    inline int scaleOffsetY(int virtualOffset, const LayoutCache& layout) {
        return (int)(virtualOffset * layout.scaleY);
    }
    
    // Scale text spacing (proportional to text size, not screen size)
    inline int scaleTextSpacing(int virtualSpacing, const LayoutCache& layout) {
        return (int)(virtualSpacing * layout.scaleTextY);
    }
    
    // Scale a cell spacing value (minimum 1 pixel)
    inline int scaleCellSpacing(int virtualSpacing, float scale) {
        return std::max(1, (int)(virtualSpacing * scale));
    }
}

// BackgroundLayer
void BackgroundLayer::render(SDL_Renderer* renderer, const GameState&, const LayoutCache& layout) {
    // ALWAYS render black background first (full screen)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Then render configured BG color ONLY in the virtual area (transformed to physical)
    // This applies to ALL modes (AUTO, STRETCH, NATIVE)
    int virtualAreaX = layout.offsetX;
    int virtualAreaY = layout.offsetY;
    int virtualAreaW = (int)(layout.virtualWidth * layout.scaleX);
    int virtualAreaH = (int)(layout.virtualHeight * layout.scaleY);
    
    SDL_Rect virtualArea = {virtualAreaX, virtualAreaY, virtualAreaW, virtualAreaH};
    SDL_SetRenderDrawColor(renderer, themeManager.getTheme().bg_r, themeManager.getTheme().bg_g, themeManager.getTheme().bg_b, 255);
    SDL_RenderFillRect(renderer, &virtualArea);
    
    // Set clipping to virtual area - all subsequent layers will respect this
    SDL_RenderSetClipRect(renderer, &virtualArea);
}
int BackgroundLayer::getZOrder() const { return 0; }
std::string BackgroundLayer::getName() const { return "Background"; }

// BannerLayer
void BannerLayer::render(SDL_Renderer* renderer, const GameState&, const LayoutCache& layout) {
    if (!layout.bannerConfig.enabled) return;
    
    // Use new layout system if configured, otherwise fall back to legacy
    int x = layout.bannerRect.w > 0 ? layout.bannerRect.x : layout.BX;
    int y = layout.bannerRect.w > 0 ? layout.bannerRect.y : layout.BY;
    int w = layout.bannerRect.w > 0 ? layout.bannerRect.w : layout.BW;
    int h = layout.bannerRect.w > 0 ? layout.bannerRect.h : layout.BH;
    
    // Draw banner background (rounded with elliptical corners in STRETCH mode)
    // Uses ThemeManager for colors (proper separation of concerns)
    drawRoundedFilled(renderer, x, y, w, h, layout.borderRadiusX, layout.borderRadiusY,
                      themeManager.getTheme().banner_bg_r,
                      themeManager.getTheme().banner_bg_g,
                      themeManager.getTheme().banner_bg_b, 255);
    
    // Draw outline (before text so text is not covered)
    drawRoundedOutline(renderer, x, y, w, h, layout.borderRadiusX, layout.borderRadiusY, layout.borderThickness,
                       themeManager.getTheme().banner_outline_r,
                       themeManager.getTheme().banner_outline_g,
                       themeManager.getTheme().banner_outline_b,
                       themeManager.getTheme().banner_outline_a);
    
    // Draw text (vertical) - distorts in STRETCH mode - LAST so it's on top
    int bty = y + scaleOffsetY(10, layout);
    int cxText = x + (int)(w - 5 * layout.scaleTextX) / 2;
    
    for (size_t i = 0; i < TITLE_TEXT.size(); ++i) {
        char ch = TITLE_TEXT[i];
        if (ch == ' ') { bty += scaleTextSpacing(6, layout); continue; }
        ch = (char)std::toupper((unsigned char)ch);
        // Allow A-Z, 0-9, and special chars (-, :, .)
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == ':' || ch == '.')) {
            ch = ' ';
        }
        drawPixelText(renderer, cxText, bty, std::string(1, ch), layout.scaleTextX, layout.scaleTextY,
                      themeManager.getTheme().banner_text_r,
                      themeManager.getTheme().banner_text_g,
                      themeManager.getTheme().banner_text_b);
        bty += scaleTextSpacing(9, layout);
    }
}
int BannerLayer::getZOrder() const { return 1; }
std::string BannerLayer::getName() const { return "Banner"; }

// BoardLayer
void BoardLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    // Use separate W/H for cells (STRETCH mode can have rectangular cells)
    int cellW = (int)layout.cellBoardW;
    int cellH = (int)layout.cellBoardH;
    int cellSpacingW = scaleCellSpacing(1, layout.scaleX);
    int cellSpacingH = scaleCellSpacing(1, layout.scaleY);
    
    for (int y = 0; y < layout.GH / cellH; ++y) {
        for (int x = 0; x < layout.GW / cellW; ++x) {
            SDL_Rect r{layout.GX + x * cellW, layout.GY + y * cellH, cellW - cellSpacingW, cellH - cellSpacingH};
            SDL_SetRenderDrawColor(renderer, themeManager.getTheme().board_empty_r, themeManager.getTheme().board_empty_g, themeManager.getTheme().board_empty_b, 255);
            SDL_RenderFillRect(renderer, &r);
        }
    }
    {
        int rows=0, cols=0; if (db_getBoardSize(state, rows, cols)) {
            for (int y = 0; y < rows; ++y) {
                for (int x = 0; x < cols; ++x) {
                    Uint8 r,g,b; bool occ; if (!db_getBoardCell(state, x, y, r, g, b, occ)) continue;
                    if (occ) {
                        SDL_Rect rr{layout.GX + x * cellW, layout.GY + y * cellH, cellW - cellSpacingW, cellH - cellSpacingH};
                        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                        SDL_RenderFillRect(renderer, &rr);
                    }
                }
            }
        }
    }
    if (PIECES.empty()) return;
    {
        int idx=0, rot=0, ax=0, ay=0; db_getActive(state, idx, rot, ax, ay);
        if (idx < 0 || idx >= (int)PIECES.size()) return;
        auto& pc = PIECES[idx];
        if (pc.rot.empty()) return;
        // Clamp rot because pieces from fallback MUST have 4 rotations
        rot = ((rot % 4) + 4) % 4;
        int rows=0, cols=0; db_getBoardSize(state, rows, cols);
        int drawn = 0;
        for (auto pr : pc.rot[rot]) {
            int gx = ax + pr.first;
            int gy = ay + pr.second;
            if (gx < 0 || gx >= cols || gy < 0 || gy >= rows) continue;
            SDL_Rect rr{layout.GX + gx * cellW,
                        layout.GY + gy * cellH,
                        cellW - cellSpacingW, cellH - cellSpacingH};
            SDL_SetRenderDrawColor(renderer, pc.r, pc.g, pc.b, 255);
            SDL_RenderFillRect(renderer, &rr);
            ++drawn;
        }
        if (drawn == 0) {
            DebugLogger::info("BoardLayer: active piece not drawn (offboard?)");
        }
    }
}
std::string BoardLayer::getName() const { return "Board"; }

// HUDLayer
void HUDLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    if (!layout.hudConfig.enabled) return;
    
    // Use new layout system if configured, otherwise fall back to legacy
    int x = layout.hudRect.w > 0 ? layout.hudRect.x : layout.panelX;
    int y = layout.hudRect.w > 0 ? layout.hudRect.y : layout.panelY;
    int w = layout.hudRect.w > 0 ? layout.hudRect.w : layout.panelW;
    int h = layout.hudRect.w > 0 ? layout.hudRect.h : layout.panelH;
    
    // Painel principal do HUD (à direita) - elliptical corners in STRETCH mode
    drawRoundedFilled(renderer, x, y, w, h, layout.borderRadiusX, layout.borderRadiusY,
                      themeManager.getTheme().panel_fill_r, themeManager.getTheme().panel_fill_g, themeManager.getTheme().panel_fill_b, 255);
    drawRoundedOutline(renderer, x, y, w, h, layout.borderRadiusX, layout.borderRadiusY, layout.borderThickness,
                       themeManager.getTheme().panel_outline_r, themeManager.getTheme().panel_outline_g, themeManager.getTheme().panel_outline_b, themeManager.getTheme().panel_outline_a);
}

// NextLayer (independent preview box)
void NextLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    if (!layout.nextConfig.enabled) return;
    
    // Get next piece index
    int nextIdx = 0;
    if (!db_getNextIdx(state, nextIdx)) return;
    
    // Use new layout system if configured, otherwise fall back to legacy (inside HUD)
    int x = layout.nextRect.w > 0 ? layout.nextRect.x : -1;
    int y = layout.nextRect.w > 0 ? layout.nextRect.y : -1;
    int w = layout.nextRect.w > 0 ? layout.nextRect.w : -1;
    int h = layout.nextRect.w > 0 ? layout.nextRect.h : -1;
    
    // Skip if not configured (will render in HUD as legacy)
    if (x < 0 || w <= 0) return;
    
    // Calculate grid size
    int gridCols = std::max(4, std::min(10, pieceManager.getPreviewGrid()));
    int gridRows = gridCols;
    // In STRETCH mode, cells distort like board cells; in AUTO/NATIVE they stay square
    int cellMiniW = (int)(layout.cellBoardW * 0.6f);  // Smaller than board cells
    int cellMiniH = (int)(layout.cellBoardH * 0.6f);
    int gridW = gridCols * cellMiniW;
    int gridH = gridRows * cellMiniH;
    int pad = scaleOffsetY(10, layout);
    int labelH = (int)(10 * layout.scaleTextY);
    
    // Use the configured rectangle
    int boxX = x;
    int boxY = y;
    int boxW = w;
    int boxH = h;
    
    // Draw background - elliptical corners in STRETCH mode
    drawRoundedFilled(renderer, boxX, boxY, boxW, boxH, layout.borderRadiusX, layout.borderRadiusY,
                      themeManager.getTheme().next_fill_r, themeManager.getTheme().next_fill_g, 
                      themeManager.getTheme().next_fill_b, 255);
    drawRoundedOutline(renderer, boxX, boxY, boxW, boxH, layout.borderRadiusX, layout.borderRadiusY, layout.borderThickness,
                       themeManager.getTheme().next_outline_r, themeManager.getTheme().next_outline_g, 
                       themeManager.getTheme().next_outline_b, themeManager.getTheme().next_outline_a);
    
    // Draw "NEXT" label - text distorts in STRETCH mode
    std::string nextText = "NEXT";
    int nextW = textWidthPx(nextText, layout.scaleTextX);
    int textX = boxX + (boxW - nextW) / 2;
    int textY = boxY + pad*2;  // Aumentado de pad/2 para pad (mais para baixo)
    drawPixelText(renderer, textX, textY, nextText, layout.scaleTextX, layout.scaleTextY,
                 themeManager.getTheme().next_label_r,
                 themeManager.getTheme().next_label_g,
                 themeManager.getTheme().next_label_b);
    
    // Calculate grid position (centered in box)
    int gridX = boxX + (boxW - gridW) / 2;
    int gridY = boxY + labelH + pad*2;
    
    // Adjust grid to fit if box is too small
    if (gridY + gridH > boxY + boxH - pad) {
        gridY = boxY + boxH - gridH - pad;
    }
    
    // Draw checkerboard grid (distorts in STRETCH mode)
    for (int gy = 0; gy < gridRows; ++gy) {
        for (int gx = 0; gx < gridCols; ++gx) {
            // IMPORTANT: Cell spacing must be scaled too!
                SDL_Rect q{gridX + gx * cellMiniW, gridY + gy * cellMiniH,
                           cellMiniW - scaleCellSpacing(1, layout.scaleX), 
                           cellMiniH - scaleCellSpacing(1, layout.scaleY)};
            bool isLight = ((gx + gy) & 1) != 0;
            if (themeManager.getTheme().next_grid_use_rgb) {
                if (isLight)
                    SDL_SetRenderDrawColor(renderer, themeManager.getTheme().next_grid_light_r, 
                                          themeManager.getTheme().next_grid_light_g, 
                                          themeManager.getTheme().next_grid_light_b, 255);
                else
                    SDL_SetRenderDrawColor(renderer, themeManager.getTheme().next_grid_dark_r, 
                                          themeManager.getTheme().next_grid_dark_g, 
                                          themeManager.getTheme().next_grid_dark_b, 255);
            } else {
                Uint8 v = isLight ? themeManager.getTheme().next_grid_light : themeManager.getTheme().next_grid_dark;
                SDL_SetRenderDrawColor(renderer, v, v, v, 255);
            }
            SDL_RenderFillRect(renderer, &q);
        }
    }
    
    // Draw centered piece (distorts in STRETCH mode)
    if (nextIdx >= 0 && nextIdx < (int)PIECES.size()) {
        const auto& pc = PIECES[nextIdx];
        if (!pc.rot.empty() && !pc.rot[0].empty()) {
            int minx = 999, maxx = -999, miny = 999, maxy = -999;
            for (auto [px,py] : pc.rot[0]) {
                if (px < minx) minx = px;
                if (px > maxx) maxx = px;
                if (py < miny) miny = py;
                if (py > maxy) maxy = py;
            }
            int blocksW = (maxx - minx + 1);
            int blocksH = (maxy - miny + 1);
            int startX = gridX + (gridW - blocksW * cellMiniW) / 2 - minx * cellMiniW;
            int startY = gridY + (gridH - blocksH * cellMiniH) / 2 - miny * cellMiniH;
            for (auto [px,py] : pc.rot[0]) {
            SDL_Rect rr{startX + px * cellMiniW, startY + py * cellMiniH,
                           cellMiniW - scaleCellSpacing(1, layout.scaleX), 
                           cellMiniH - scaleCellSpacing(1, layout.scaleY)};
                SDL_SetRenderDrawColor(renderer, pc.r, pc.g, pc.b, 255);
                SDL_RenderFillRect(renderer, &rr);
            }
        }
    }
}

// PieceStatsLayer
void PieceStatsLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    const std::vector<int>* pieceStats = nullptr;
    if (!db_getPieceStats(state, pieceStats) || pieceStats == nullptr || PIECES.empty()) return;
    
    // Calcular tamanho da caixa de estatísticas (distorce em STRETCH mode)
    // Peças maiores (50% do tamanho do board, não 33%)
    int miniCellW = std::max(2, (int)(layout.cellBoardW / 2));
    int miniCellH = std::max(2, (int)(layout.cellBoardH / 2));
    int cellSizeW = (int)(miniCellW * 4.5f);  // Espaço para 4 blocos + margem
    int cellSizeH = (int)(miniCellH * 4.5f);
    int rowHeight = cellSizeH + scaleOffsetY(4, layout);
    int numPieces = (int)PIECES.size();
    int statsPad = scaleOffsetY(10, layout);
    
    if (!layout.statsConfig.enabled) return;
    
    // Use new layout system if configured, otherwise fall back to legacy
    int statsBoxX = layout.statsRect.w > 0 ? layout.statsRect.x : (layout.BX + layout.BW + layout.statsMargin);
    int statsBoxY = layout.statsRect.w > 0 ? layout.statsRect.y : layout.GY;
    int statsBoxW = layout.statsRect.w > 0 ? layout.statsRect.w : layout.statsBoxW;
    int statsBoxH = layout.statsRect.w > 0 ? layout.statsRect.h : layout.GH;
    
    // Desenhar caixa ao redor das estatísticas - elliptical corners in STRETCH mode
    drawRoundedFilled(renderer, statsBoxX, statsBoxY, statsBoxW, statsBoxH, layout.borderRadiusX, layout.borderRadiusY,
                      themeManager.getTheme().next_fill_r, 
                      themeManager.getTheme().next_fill_g, 
                      themeManager.getTheme().next_fill_b, 255);
    drawRoundedOutline(renderer, statsBoxX, statsBoxY, statsBoxW, statsBoxH, layout.borderRadiusX, layout.borderRadiusY, layout.borderThickness,
                       themeManager.getTheme().next_outline_r, 
                       themeManager.getTheme().next_outline_g, 
                       themeManager.getTheme().next_outline_b, 
                       themeManager.getTheme().next_outline_a);
    
    // Renderizar estatísticas de cada peça (sem título, peças centralizadas)
    int statY = statsBoxY + statsPad;
    int statX = statsBoxX + (statsBoxW - cellSizeW) / 2;  // Centralizar horizontalmente
    
    for (size_t i = 0; i < PIECES.size(); ++i) {
        int count = 0;
        if (i < pieceStats->size()) {
            count = (*pieceStats)[i];
        }
        
        const auto& pc = PIECES[i];
        
        // Desenhar miniatura da peça CENTRALIZADA no bloco (distorce em STRETCH mode)
        if (!pc.rot.empty() && !pc.rot[0].empty()) {
            int minx = 999, maxx = -999, miny = 999, maxy = -999;
            for (auto [px,py] : pc.rot[0]) {
                if (px < minx) minx = px;
                if (px > maxx) maxx = px;
                if (py < miny) miny = py;
                if (py > maxy) maxy = py;
            }
            int blocksW = (maxx - minx + 1);
            int blocksH = (maxy - miny + 1);
            
            // Centralizar peça no bloco completo
            int startX = statX + (cellSizeW - blocksW * miniCellW) / 2 - minx * miniCellW;
            int startY = statY + (cellSizeH - blocksH * miniCellH) / 2 - miny * miniCellH;
            
            for (auto [px,py] : pc.rot[0]) {
                SDL_Rect rr{startX + px * miniCellW, startY + py * miniCellH, 
                           miniCellW - scaleCellSpacing(1, layout.scaleX), 
                           miniCellH - scaleCellSpacing(1, layout.scaleY)};
                SDL_SetRenderDrawColor(renderer, pc.r, pc.g, pc.b, 255);
                SDL_RenderFillRect(renderer, &rr);
            }
        }
        
        // Contagem CENTRALIZADA SOBRE a peça com outline preto
        std::string countStr = std::to_string(count);
        float numberScaleX = layout.scaleTextX * 0.8f;  // Um pouco maior que antes
        float numberScaleY = layout.scaleTextY * 0.8f;
        int countW = textWidthPx(countStr, numberScaleX);
        int countX = statX + (cellSizeW - countW) / 2;  // Centralizar
        int countY = statY + (cellSizeH - (int)(7 * numberScaleY)) / 2;  // Centralizar verticalmente
        
        drawPixelTextOutlined(renderer, countX, countY, countStr, numberScaleX, numberScaleY,
                             themeManager.getTheme().stats_count_r,
                             themeManager.getTheme().stats_count_g,
                             themeManager.getTheme().stats_count_b,
                             0, 0, 0);  // Outline preto
        
        statY += rowHeight;
    }
}
int PieceStatsLayer::getZOrder() const { return 2; }
std::string PieceStatsLayer::getName() const { return "PieceStats"; }

// BoardLayer
int BoardLayer::getZOrder() const { return 3; } // Mudado de 2 para 3

// HUDLayer
int HUDLayer::getZOrder() const { return 4; } // Mudado de 3 para 4
std::string HUDLayer::getName() const { return "HUD"; }

// NextLayer
int NextLayer::getZOrder() const { return 5; } // Independent NEXT preview
std::string NextLayer::getName() const { return "Next"; }

// ScoreLayer (independent score/lines/level box)
void ScoreLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    if (!layout.scoreConfig.enabled) return;
    
    // Get score data
    auto fmtScore = [](int s){ char b[32]; std::snprintf(b,sizeof(b), "%d", s); return std::string(b); };
    int score = db_getScore(state);
    int lines = db_getLines(state);
    int level = db_getLevel(state);
    
    // Use new layout system if configured
    int boxX = layout.scoreRect.w > 0 ? layout.scoreRect.x : -1;
    int boxY = layout.scoreRect.w > 0 ? layout.scoreRect.y : -1;
    int boxW = layout.scoreRect.w > 0 ? layout.scoreRect.w : -1;
    int boxH = layout.scoreRect.w > 0 ? layout.scoreRect.h : -1;
    
    // Skip if not configured
    if (boxX < 0 || boxW <= 0) return;
    
    // Draw box background
    drawRoundedFilled(renderer, boxX, boxY, boxW, boxH, layout.borderRadiusX, layout.borderRadiusY,
                      themeManager.getTheme().score_fill_r, themeManager.getTheme().score_fill_g, 
                      themeManager.getTheme().score_fill_b, 255);
    drawRoundedOutline(renderer, boxX, boxY, boxW, boxH, layout.borderRadiusX, layout.borderRadiusY, layout.borderThickness,
                       themeManager.getTheme().score_outline_r, themeManager.getTheme().score_outline_g, 
                       themeManager.getTheme().score_outline_b, themeManager.getTheme().score_outline_a);
    
    // Same top margin as NEXT (pad*2 for consistency)
    int pad = scaleOffsetY(10, layout);
    int textPad = scaleOffsetX(20, layout);  // Larger right margin for numbers
    int ty = boxY + pad*2;  // Same top margin as NEXT
    
    // SCORE (centered label, right-aligned number)
    std::string scoreLabel = "SCORE";
    int scoreLabelW = textWidthPx(scoreLabel, layout.scaleTextX);
    int labelX = boxX + (boxW - scoreLabelW) / 2;  // Centered
    drawPixelText(renderer, labelX, ty, scoreLabel, layout.scaleTextX, layout.scaleTextY, 
                  themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); 
    ty += scaleTextSpacing(10, layout);
    
    std::string scoreStr = fmtScore(score);
    int scoreW = textWidthPx(scoreStr, layout.scaleTextX);
    int scoreX = boxX + boxW - scoreW - textPad;  // Right-aligned
    drawPixelText(renderer, scoreX, ty, scoreStr, layout.scaleTextX, layout.scaleTextY,
                  themeManager.getTheme().hud_score_r, themeManager.getTheme().hud_score_g, themeManager.getTheme().hud_score_b); 
    ty += scaleTextSpacing(14, layout);
    
    // LINES (centered label, right-aligned number)
    std::string linesLabel = "LINES";
    int linesLabelW = textWidthPx(linesLabel, layout.scaleTextX);
    labelX = boxX + (boxW - linesLabelW) / 2;
    drawPixelText(renderer, labelX, ty, linesLabel, layout.scaleTextX, layout.scaleTextY,
                  themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); 
    ty += scaleTextSpacing(10, layout);
    
    std::string linesStr = std::to_string(lines);
    int linesW = textWidthPx(linesStr, layout.scaleTextX);
    int linesX = boxX + boxW - linesW - textPad;
    drawPixelText(renderer, linesX, ty, linesStr, layout.scaleTextX, layout.scaleTextY,
                  themeManager.getTheme().hud_lines_r, themeManager.getTheme().hud_lines_g, themeManager.getTheme().hud_lines_b); 
    ty += scaleTextSpacing(14, layout);
    
    // LEVEL (centered label, right-aligned number)
    std::string levelLabel = "LEVEL";
    int levelLabelW = textWidthPx(levelLabel, layout.scaleTextX);
    labelX = boxX + (boxW - levelLabelW) / 2;
    drawPixelText(renderer, labelX, ty, levelLabel, layout.scaleTextX, layout.scaleTextY,
                  themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); 
    ty += scaleTextSpacing(10, layout);
    
    std::string levelStr = std::to_string(level);
    int levelW = textWidthPx(levelStr, layout.scaleTextX);
    int levelX = boxX + boxW - levelW - textPad;
    drawPixelText(renderer, levelX, ty, levelStr, layout.scaleTextX, layout.scaleTextY,
                  themeManager.getTheme().hud_level_r, themeManager.getTheme().hud_level_g, themeManager.getTheme().hud_level_b); 
}
int ScoreLayer::getZOrder() const { return 5; } // Same Z as Next
std::string ScoreLayer::getName() const { return "Score"; }

// OverlayLayer
void OverlayLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    const bool isPaused = db_isPaused(state);
    const bool isGameOver = db_isGameOver(state);
    if (isGameOver || isPaused) {
        const std::string topText = isPaused ? "PAUSE" : "GAME OVER";
        const std::string subText = isPaused ? "" : "PRESS START";
        // Text distorts in STRETCH mode
        float topScaleX = layout.scaleTextX * (layout.scale + 2) / layout.scale;
        float topScaleY = layout.scaleTextY * (layout.scale + 2) / layout.scale;
        int topW = textWidthPx(topText, topScaleX);
        int subW = subText.empty() ? 0 : textWidthPx(subText, layout.scaleTextX);
        int textW = std::max(topW, subW);
        int padX = scaleOffsetX(24, layout);
        int padY = scaleOffsetY(20, layout);
        int textH = (int)(7 * topScaleY) + (subText.empty() ? 0 : (int)(8 * layout.scaleTextY + 7 * layout.scaleTextY));
        int ow = textW + padX * 2, oh = textH + padY * 2;
        int ox = layout.GX + (layout.GW - ow) / 2, oy = layout.GY + (layout.GH - oh) / 2;
        // Elliptical corners in STRETCH mode - calculate from virtual 14 radius
        int overlayRadX = scaleOffsetX(14, layout);
        int overlayRadY = scaleOffsetY(14, layout);
        drawRoundedFilled(renderer, ox, oy, ow, oh, overlayRadX, overlayRadY, themeManager.getTheme().overlay_fill_r, themeManager.getTheme().overlay_fill_g, themeManager.getTheme().overlay_fill_b, themeManager.getTheme().overlay_fill_a);
        drawRoundedOutline(renderer, ox, oy, ow, oh, overlayRadX, overlayRadY, 2, themeManager.getTheme().overlay_outline_r, themeManager.getTheme().overlay_outline_g, themeManager.getTheme().overlay_outline_b, themeManager.getTheme().overlay_outline_a);
        int txc = ox + (ow - topW) / 2, tyc = oy + padY;
        drawPixelTextOutlined(renderer, txc, tyc, topText, topScaleX, topScaleY, themeManager.getTheme().overlay_top_r, themeManager.getTheme().overlay_top_g, themeManager.getTheme().overlay_top_b, 0, 0, 0);
        if (!subText.empty()) {
            int sx = ox + (ow - subW) / 2, sy = tyc + (int)(7 * topScaleY + 8 * layout.scaleTextY);
            drawPixelTextOutlined(renderer, sx, sy, subText, layout.scaleTextX, layout.scaleTextY, themeManager.getTheme().overlay_sub_r, themeManager.getTheme().overlay_sub_g, themeManager.getTheme().overlay_sub_b, 0, 0, 0);
        }
    }
}
int OverlayLayer::getZOrder() const { return 6; } // Mudado de 4 para 6 (NextLayer is 5)
std::string OverlayLayer::getName() const { return "Overlay"; }

// PostEffectsLayer
PostEffectsLayer::PostEffectsLayer(AudioSystem* audio) : audio_(audio) {}
void PostEffectsLayer::render(SDL_Renderer* renderer, const GameState&, const LayoutCache& layout) {
    if (layout.SWr <= 0 || layout.SHr <= 0) { return; }
    
    // Calculate virtual area bounds (same as BackgroundLayer)
    int virtualAreaX = layout.offsetX;
    int virtualAreaY = layout.offsetY;
    int virtualAreaW = (int)(layout.virtualWidth * layout.scaleX);
    int virtualAreaH = (int)(layout.virtualHeight * layout.scaleY);
    
    const auto& vis = db_getVisualEffects();
    if (vis.scanlineAlpha > 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)vis.scanlineAlpha);
        // Scanlines only in virtual area
        for (int y = virtualAreaY; y < virtualAreaY + virtualAreaH; y += 2) {
            SDL_Rect sl{virtualAreaX, y, virtualAreaW, 1};
            SDL_RenderFillRect(renderer, &sl);
        }
        if (audio_) audio_->playScanlineEffect();
    }
    if (vis.globalSweep) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        float tsec = SDL_GetTicks() / 1000.0f;
        int bandH = (int)(vis.sweepGBandHPx * layout.scaleY); // Scale sweep band with virtual area
        if (bandH < 1) { SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); DebugLogger::info("PostEffectsLayer: skip sweep (bandH<1)"); return; }
        if (bandH > virtualAreaH) bandH = virtualAreaH;
        if (bandH > 1024) bandH = 1024; // safety cap to keep frame responsive
        float speed = (float)vis.sweepGSpeedPxps * layout.scaleY; // Scale speed with virtual area
        if (speed < 1.0f) speed = 1.0f;
        if (speed > 4000.0f) speed = 4000.0f;
        int total = virtualAreaH + bandH;
        int sweepY = (int)std::fmod(tsec * speed, (float)total) - bandH;
        for (int i = 0; i < bandH; ++i) {
            float normalizedPos = (float)i / (float)bandH;
            float center = 0.5f;
            float distance = (normalizedPos - center) * 2.0f;
            float sigma = 0.3f + (1.0f - vis.sweepGSoftness) * 0.4f;
            float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
            Uint8 a = (Uint8)std::round(vis.sweepGAlphaMax * softness);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, a);
            int yy = virtualAreaY + sweepY + i;
            if (yy >= virtualAreaY && yy < virtualAreaY + virtualAreaH) {
                SDL_Rect line{virtualAreaX, yy, virtualAreaW, 1};
                SDL_RenderFillRect(renderer, &line);
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        
        // Play sweep sound effect
        if (audio_) audio_->playSweepEffect();
    }
}
int PostEffectsLayer::getZOrder() const { return 7; } // Mudado de 5 para 7 (NextLayer is 5)
std::string PostEffectsLayer::getName() const { return "PostEffects"; }


