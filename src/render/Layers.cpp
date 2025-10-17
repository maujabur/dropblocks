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

// BackgroundLayer
void BackgroundLayer::render(SDL_Renderer* renderer, const GameState&, const LayoutCache&) {
    SDL_SetRenderDrawColor(renderer, themeManager.getTheme().bg_r, themeManager.getTheme().bg_g, themeManager.getTheme().bg_b, 255);
    SDL_RenderClear(renderer);
}
int BackgroundLayer::getZOrder() const { return 0; }
std::string BackgroundLayer::getName() const { return "Background"; }

// BannerLayer
BannerLayer::BannerLayer(AudioSystem* audio) : audio_(audio) {}
void BannerLayer::render(SDL_Renderer* renderer, const GameState&, const LayoutCache& layout) {
    drawRoundedFilled(renderer, layout.BX, layout.BY, layout.BW, layout.BH, 10,
                      themeManager.getTheme().banner_bg_r, themeManager.getTheme().banner_bg_g, themeManager.getTheme().banner_bg_b, 255);
    drawRoundedOutline(renderer, layout.BX, layout.BY, layout.BW, layout.BH, 10, 2,
                       themeManager.getTheme().banner_outline_r, themeManager.getTheme().banner_outline_g, themeManager.getTheme().banner_outline_b, themeManager.getTheme().banner_outline_a);

    int bty = layout.BY + 10, cxText = layout.BX + (layout.BW - 5 * layout.scale) / 2;
    for (size_t i = 0; i < TITLE_TEXT.size(); ++i) {
        char ch = TITLE_TEXT[i];
        if (ch == ' ') { bty += 6 * layout.scale; continue; }
        ch = (char)std::toupper((unsigned char)ch);
        if (ch < 'A' || ch > 'Z') ch = ' ';
        drawPixelText(renderer, cxText, bty, std::string(1, ch), layout.scale,
                      themeManager.getTheme().banner_text_r, themeManager.getTheme().banner_text_g, themeManager.getTheme().banner_text_b);
        bty += 9 * layout.scale;
    }

    const auto& vis = db_getVisualEffects();
    if (vis.bannerSweep) {
        SDL_Rect clip{layout.BX, layout.BY, layout.BW, layout.BH};
        SDL_RenderSetClipRect(renderer, &clip);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        int bandH = vis.sweepBandHS * layout.scale;
        int total = layout.BH + bandH;
        float tsec = SDL_GetTicks() / 1000.0f;
        int sweepY = (int)std::fmod(tsec * vis.sweepSpeedPxps, (float)total) - bandH;
        for (int i = 0; i < bandH; ++i) {
            float normalizedPos = (float)i / (float)bandH;
            float center = 0.5f;
            float distance = (normalizedPos - center) * 2.0f;
            float sigma = 0.3f + (1.0f - vis.sweepSoftness) * 0.4f;
            float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
            Uint8 a = (Uint8)std::round(vis.sweepAlphaMax * softness);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, a);
            SDL_Rect line{layout.BX, layout.BY + sweepY + i, layout.BW, 1};
            SDL_RenderFillRect(renderer, &line);
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_RenderSetClipRect(renderer, nullptr);
        if (audio_) audio_->playSweepEffect();
    }
}
int BannerLayer::getZOrder() const { return 1; }
std::string BannerLayer::getName() const { return "Banner"; }

// BoardLayer
void BoardLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    for (int y = 0; y < layout.GH / layout.cellBoard; ++y) {
        for (int x = 0; x < layout.GW / layout.cellBoard; ++x) {
            SDL_Rect r{layout.GX + x * layout.cellBoard, layout.GY + y * layout.cellBoard, layout.cellBoard - 1, layout.cellBoard - 1};
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
                        SDL_Rect rr{layout.GX + x * layout.cellBoard, layout.GY + y * layout.cellBoard, layout.cellBoard - 1, layout.cellBoard - 1};
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
            SDL_Rect rr{layout.GX + gx * layout.cellBoard,
                        layout.GY + gy * layout.cellBoard,
                        layout.cellBoard - 1, layout.cellBoard - 1};
            SDL_SetRenderDrawColor(renderer, pc.r, pc.g, pc.b, 255);
            SDL_RenderFillRect(renderer, &rr);
            ++drawn;
        }
        if (drawn == 0) {
            DebugLogger::info("BoardLayer: active piece not drawn (offboard?)");
        }
    }
}
int BoardLayer::getZOrder() const { return 2; }
std::string BoardLayer::getName() const { return "Board"; }

// HUDLayer
void HUDLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    drawRoundedFilled(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH, 12,
                      themeManager.getTheme().panel_fill_r, themeManager.getTheme().panel_fill_g, themeManager.getTheme().panel_fill_b, 255);
    drawRoundedOutline(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH, 12, 2,
                       themeManager.getTheme().panel_outline_r, themeManager.getTheme().panel_outline_g, themeManager.getTheme().panel_outline_b, themeManager.getTheme().panel_outline_a);
    int tx = layout.panelX + 14, ty = layout.panelY + 14;
    auto fmtScore = [](int s){ char b[32]; std::snprintf(b,sizeof(b), "%d", s); return std::string(b); };
    int score = db_getScore(state);
    int lines = db_getLines(state);
    int level = db_getLevel(state);
    drawPixelText(renderer, tx, ty, "SCORE", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); ty += 10 * layout.scale;
    drawPixelText(renderer, tx, ty, fmtScore(score), layout.scale + 1, themeManager.getTheme().hud_score_r, themeManager.getTheme().hud_score_g, themeManager.getTheme().hud_score_b); ty += 12 * (layout.scale + 1);
    drawPixelText(renderer, tx, ty, "LINES", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); ty += 8 * layout.scale;
    drawPixelText(renderer, tx, ty, std::to_string(lines), layout.scale, themeManager.getTheme().hud_lines_r, themeManager.getTheme().hud_lines_g, themeManager.getTheme().hud_lines_b); ty += 10 * layout.scale;
    drawPixelText(renderer, tx, ty, "LEVEL", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); ty += 8 * layout.scale;
    drawPixelText(renderer, tx, ty, std::to_string(level), layout.scale, themeManager.getTheme().hud_level_r, themeManager.getTheme().hud_level_g, themeManager.getTheme().hud_level_b); ty += 10 * layout.scale;

    // NEXT preview (mesma escala do tabuleiro) com grid de fundo
    ty += 6 * layout.scale;
    drawPixelText(renderer, tx, ty, "NEXT", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b);
    ty += 10 * layout.scale;
    int nextIdx = 0; if (!db_getNextIdx(state, nextIdx)) return;
    int gridCols = std::max(4, std::min(10, pieceManager.getPreviewGrid()));
    int gridRows = gridCols;
    int cellMini = layout.cellBoard; // mesma escala do tabuleiro
    int gridW = gridCols * cellMini, gridH = gridRows * cellMini;
    int gridX = layout.panelX + (layout.panelW - gridW) / 2;
    int gridY = ty;
    // Caixa arredondada para o NEXT
    {
        int pad = 10;
        int boxX = gridX - pad;
        int boxY = gridY - pad;
        int boxW = gridW + pad * 2;
        int boxH = gridH + pad * 2;
        drawRoundedFilled(renderer, boxX, boxY, boxW, boxH, 10,
                          themeManager.getTheme().next_fill_r, themeManager.getTheme().next_fill_g, themeManager.getTheme().next_fill_b, 255);
        drawRoundedOutline(renderer, boxX, boxY, boxW, boxH, 10, 2,
                           themeManager.getTheme().next_outline_r, themeManager.getTheme().next_outline_g, themeManager.getTheme().next_outline_b, themeManager.getTheme().next_outline_a);
    }
    // Desenha grid xadrez
    for (int gy = 0; gy < gridRows; ++gy) {
        for (int gx = 0; gx < gridCols; ++gx) {
            SDL_Rect q{gridX + gx * cellMini, gridY + gy * cellMini, cellMini - 1, cellMini - 1};
            bool isLight = ((gx + gy) & 1) != 0;
            if (themeManager.getTheme().next_grid_use_rgb) {
                if (isLight)
                    SDL_SetRenderDrawColor(renderer, themeManager.getTheme().next_grid_light_r, themeManager.getTheme().next_grid_light_g, themeManager.getTheme().next_grid_light_b, 255);
                else
                    SDL_SetRenderDrawColor(renderer, themeManager.getTheme().next_grid_dark_r, themeManager.getTheme().next_grid_dark_g, themeManager.getTheme().next_grid_dark_b, 255);
            } else {
                Uint8 v = isLight ? themeManager.getTheme().next_grid_light : themeManager.getTheme().next_grid_dark;
                SDL_SetRenderDrawColor(renderer, v, v, v, 255);
            }
            SDL_RenderFillRect(renderer, &q);
        }
    }
    // Desenha peça centralizada no grid
    if (nextIdx >= 0 && nextIdx < (int)PIECES.size()) {
        const auto& pc = PIECES[nextIdx];
        if (!pc.rot.empty() && !pc.rot[0].empty()) {
            int minx = 999, maxx = -999, miny = 999, maxy = -999;
            for (auto [px,py] : pc.rot[0]) { if (px < minx) minx = px; if (px > maxx) maxx = px; if (py < miny) miny = py; if (py > maxy) maxy = py; }
            int blocksW = (maxx - minx + 1);
            int blocksH = (maxy - miny + 1);
            int startX = gridX + (gridW - blocksW * cellMini) / 2 - minx * cellMini;
            int startY = gridY + (gridH - blocksH * cellMini) / 2 - miny * cellMini;
            for (auto [px,py] : pc.rot[0]) {
                SDL_Rect rr{ startX + px * cellMini, startY + py * cellMini, cellMini - 1, cellMini - 1 };
                SDL_SetRenderDrawColor(renderer, pc.r, pc.g, pc.b, 255);
                SDL_RenderFillRect(renderer, &rr);
            }
        }
    }
}
int HUDLayer::getZOrder() const { return 3; }
std::string HUDLayer::getName() const { return "HUD"; }

// OverlayLayer
void OverlayLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    const bool isPaused = db_isPaused(state);
    const bool isGameOver = db_isGameOver(state);
    if (isGameOver || isPaused) {
        const std::string topText = isPaused ? "PAUSE" : "GAME OVER";
        const std::string subText = isPaused ? "" : "PRESS START";
        int topW = textWidthPx(topText, layout.scale + 2);
        int subW = subText.empty() ? 0 : textWidthPx(subText, layout.scale);
        int textW = std::max(topW, subW);
        int padX = 24, padY = 20;
        int textH = 7 * (layout.scale + 2) + (subText.empty() ? 0 : (8 * layout.scale + 7 * layout.scale));
        int ow = textW + padX * 2, oh = textH + padY * 2;
        int ox = layout.GX + (layout.GW - ow) / 2, oy = layout.GY + (layout.GH - oh) / 2;
        drawRoundedFilled(renderer, ox, oy, ow, oh, 14, themeManager.getTheme().overlay_fill_r, themeManager.getTheme().overlay_fill_g, themeManager.getTheme().overlay_fill_b, themeManager.getTheme().overlay_fill_a);
        drawRoundedOutline(renderer, ox, oy, ow, oh, 14, 2, themeManager.getTheme().overlay_outline_r, themeManager.getTheme().overlay_outline_g, themeManager.getTheme().overlay_outline_b, themeManager.getTheme().overlay_outline_a);
        int txc = ox + (ow - topW) / 2, tyc = oy + padY;
        drawPixelTextOutlined(renderer, txc, tyc, topText, layout.scale + 2, themeManager.getTheme().overlay_top_r, themeManager.getTheme().overlay_top_g, themeManager.getTheme().overlay_top_b, 0, 0, 0);
        if (!subText.empty()) {
            int sx = ox + (ow - subW) / 2, sy = tyc + 7 * (layout.scale + 2) + 8 * layout.scale;
            drawPixelTextOutlined(renderer, sx, sy, subText, layout.scale, themeManager.getTheme().overlay_sub_r, themeManager.getTheme().overlay_sub_g, themeManager.getTheme().overlay_sub_b, 0, 0, 0);
        }
    }
}
int OverlayLayer::getZOrder() const { return 4; }
std::string OverlayLayer::getName() const { return "Overlay"; }

// PostEffectsLayer
PostEffectsLayer::PostEffectsLayer(AudioSystem* audio) : audio_(audio) {}
void PostEffectsLayer::render(SDL_Renderer* renderer, const GameState&, const LayoutCache& layout) {
    if (layout.SWr <= 0 || layout.SHr <= 0) { return; }
    const auto& vis = db_getVisualEffects();
    if (vis.scanlineAlpha > 0) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)vis.scanlineAlpha);
        for (int y = 0; y < layout.SHr; y += 2) {
            SDL_Rect sl{0, y, layout.SWr, 1};
            SDL_RenderFillRect(renderer, &sl);
        }
        if (audio_) audio_->playScanlineEffect();
    }
    if (vis.globalSweep) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        float tsec = SDL_GetTicks() / 1000.0f;
        int bandH = vis.sweepGBandHPx;
        if (bandH < 1) { SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); DebugLogger::info("PostEffectsLayer: skip sweep (bandH<1)"); return; }
        if (bandH > layout.SHr) bandH = layout.SHr;
        if (bandH > 1024) bandH = 1024; // safety cap to keep frame responsive
        float speed = (float)vis.sweepGSpeedPxps;
        if (speed < 1.0f) speed = 1.0f;
        if (speed > 4000.0f) speed = 4000.0f;
        int total = layout.SHr + bandH;
        int sweepY = (int)std::fmod(tsec * speed, (float)total) - bandH;
        for (int i = 0; i < bandH; ++i) {
            float normalizedPos = (float)i / (float)bandH;
            float center = 0.5f;
            float distance = (normalizedPos - center) * 2.0f;
            float sigma = 0.3f + (1.0f - vis.sweepGSoftness) * 0.4f;
            float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
            Uint8 a = (Uint8)std::round(vis.sweepGAlphaMax * softness);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, a);
            int yy = sweepY + i;
            if (yy >= 0 && yy < layout.SHr) {
                SDL_Rect line{0, yy, layout.SWr, 1};
                SDL_RenderFillRect(renderer, &line);
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
}
int PostEffectsLayer::getZOrder() const { return 5; }
std::string PostEffectsLayer::getName() const { return "PostEffects"; }


