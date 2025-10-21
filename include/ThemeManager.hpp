#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <string>

#include "ConfigTypes.hpp"
#include "Interfaces.hpp"

// Forward declaration to avoid heavy includes
struct Piece;

struct Theme {
    // background
    Uint8 bg_r=8, bg_g=8, bg_b=12;

    // board
    Uint8 board_empty_r=28, board_empty_g=28, board_empty_b=36;

    // panel (HUD)
    Uint8 panel_fill_r=24, panel_fill_g=24, panel_fill_b=32;
    Uint8 panel_outline_r=90, panel_outline_g=90, panel_outline_b=120;

    // banner
    Uint8 banner_bg_r=0, banner_bg_g=40, banner_bg_b=0;
    Uint8 banner_outline_r=0, banner_outline_g=60, banner_outline_b=0;
    Uint8 banner_text_r=120, banner_text_g=255, banner_text_b=120;

    // HUD texts
    Uint8 hud_label_r=200, hud_label_g=200, hud_label_b=220;
    Uint8 hud_score_r=255, hud_score_g=240, hud_score_b=120;
    Uint8 hud_lines_r=180, hud_lines_g=255, hud_lines_b=180;
    Uint8 hud_level_r=180, hud_level_g=200, hud_level_b=255;

    // NEXT
    Uint8 next_fill_r=18, next_fill_g=18, next_fill_b=26;
    Uint8 next_outline_r=80, next_outline_g=80, next_outline_b=110;
    Uint8 next_label_r=220, next_label_g=220, next_label_b=220;
    
    // Score box colors
    Uint8 score_fill_r=18, score_fill_g=18, score_fill_b=26;
    Uint8 score_outline_r=80, score_outline_g=80, score_outline_b=110;
    Uint8 next_grid_dark=24, next_grid_light=30; // grayscale fallback
    // Colored grid (prioritary if defined)
    Uint8 next_grid_dark_r=24, next_grid_dark_g=24, next_grid_dark_b=24;
    Uint8 next_grid_light_r=30, next_grid_light_g=30, next_grid_light_b=30;
    bool  next_grid_use_rgb=false;
    
    // PIECE STATS
    Uint8 stats_fill_r=18, stats_fill_g=18, stats_fill_b=26;
    Uint8 stats_outline_r=80, stats_outline_g=80, stats_outline_b=110;
    Uint8 stats_label_r=200, stats_label_g=200, stats_label_b=220;
    Uint8 stats_count_r=255, stats_count_g=255, stats_count_b=180;

    // overlay
    Uint8 overlay_fill_r=0, overlay_fill_g=0, overlay_fill_b=0, overlay_fill_a=200;
    Uint8 overlay_outline_r=200, overlay_outline_g=200, overlay_outline_b=220, overlay_outline_a=120;
    Uint8 overlay_top_r=255, overlay_top_g=160, overlay_top_b=160;
    Uint8 overlay_sub_r=220, overlay_sub_g=220, overlay_sub_b=220;

    // piece colors (dynamic, no fixed size)
    std::vector<RGB> piece_colors;
};

class ThemeManager : public IThemeManager {
private:
    Theme theme_;

public:
    const Theme& getTheme() const { return theme_; }
    Theme& getTheme() { return theme_; }

    void initDefaultPieceColors();

    // Non-const version used internally to mutate piece colors
    void applyPieceColors(std::vector<Piece>& pieces);
    // Interface requirement; delegates to non-const version
    void applyPieceColors(const std::vector<Piece>& pieces) override;

    bool loadFromConfig(const std::string& key, const std::string& value) override;
};


