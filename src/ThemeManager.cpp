#include "ThemeManager.hpp"
#include "ConfigTypes.hpp"
#include "DebugLogger.hpp"
#include <vector>
#include <array>
#include <utility>
#include <string>

#include "pieces/Piece.hpp"

static bool parseHexColor(const std::string& s, Uint8& r, Uint8& g, Uint8& b){
    std::string color = s;
    if (color.size() == 6 && color[0] != '#') color = "#" + color;
    if (color.size()!=7 || color[0]!='#') return false;
    auto cv=[&](char c)->int{
        if(c>='0'&&c<='9') return c-'0'; c=(char)std::toupper((unsigned char)c);
        if(c>='A'&&c<='F') return 10+(c-'A'); return -1; };
    auto hx=[&](char a,char b){int A=cv(a),B=cv(b); return (A<0||B<0)?-1:(A*16+B);} ;
    int R=hx(color[1],color[2]), G=hx(color[3],color[4]), B=hx(color[5],color[6]);
    if(R<0||G<0||B<0) return false; r=(Uint8)R; g=(Uint8)G; b=(Uint8)B; return true;
}

void ThemeManager::initDefaultPieceColors() {
    if (theme_.piece_colors.empty()) {
        theme_.piece_colors = {
            {220, 80, 80},  { 80,180,120}, { 80,120,220}, {220,180, 80},
            {180, 80,220},  { 80,220,180}, {220,120, 80}, {160,160,160}
        };
    }
}

void ThemeManager::applyPieceColors(std::vector<Piece>& pieces) {
    initDefaultPieceColors();
    for (size_t i = 0; i < pieces.size(); ++i) {
        if (i < theme_.piece_colors.size()) {
            pieces[i].r = theme_.piece_colors[i].r;
            pieces[i].g = theme_.piece_colors[i].g;
            pieces[i].b = theme_.piece_colors[i].b;
        } else {
            if (pieces[i].r == 0 && pieces[i].g == 0 && pieces[i].b == 0) {
                size_t idx = i % 8;
                pieces[i].r = theme_.piece_colors[idx].r;
                pieces[i].g = theme_.piece_colors[idx].g;
                pieces[i].b = theme_.piece_colors[idx].b;
            }
        }
    }
}

void ThemeManager::applyPieceColors(const std::vector<Piece>& pieces) {
    applyPieceColors(const_cast<std::vector<Piece>&>(pieces));
}

bool ThemeManager::loadFromConfig(const std::string& key, const std::string& value) {
    if (key == "BG" || key == "BACKGROUND") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.bg_r=r; theme_.bg_g=g; theme_.bg_b=b; return true; }
    }
    if (key == "BOARD_EMPTY") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.board_empty_r=r; theme_.board_empty_g=g; theme_.board_empty_b=b; return true; }
    }
    if (key == "PANEL_FILL") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.panel_fill_r=r; theme_.panel_fill_g=g; theme_.panel_fill_b=b; return true; }
    }
    if (key == "PANEL_OUTLINE") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.panel_outline_r=r; theme_.panel_outline_g=g; theme_.panel_outline_b=b; return true; }
    }
    
    // === BANNER ===
    if (key == "BANNER_BG") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.banner_bg_r=r; theme_.banner_bg_g=g; theme_.banner_bg_b=b; return true; }
    }
    if (key == "BANNER_OUTLINE") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.banner_outline_r=r; theme_.banner_outline_g=g; theme_.banner_outline_b=b; return true; }
    }
    if (key == "BANNER_TEXT") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.banner_text_r=r; theme_.banner_text_g=g; theme_.banner_text_b=b; return true; }
    }
    
    // === HUD (novo!) ===
    if (key == "HUD_LABEL") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.hud_label_r=r; theme_.hud_label_g=g; theme_.hud_label_b=b; return true; }
    }
    if (key == "HUD_SCORE") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.hud_score_r=r; theme_.hud_score_g=g; theme_.hud_score_b=b; return true; }
    }
    if (key == "HUD_LINES") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.hud_lines_r=r; theme_.hud_lines_g=g; theme_.hud_lines_b=b; return true; }
    }
    if (key == "HUD_LEVEL") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.hud_level_r=r; theme_.hud_level_g=g; theme_.hud_level_b=b; return true; }
    }
    
    // === NEXT (novo!) ===
    if (key == "NEXT_FILL") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.next_fill_r=r; theme_.next_fill_g=g; theme_.next_fill_b=b; return true; }
    }
    if (key == "NEXT_OUTLINE") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.next_outline_r=r; theme_.next_outline_g=g; theme_.next_outline_b=b; return true; }
    }
    if (key == "NEXT_LABEL") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.next_label_r=r; theme_.next_label_g=g; theme_.next_label_b=b; return true; }
    }
    
    // === SCORE ===
    if (key == "SCORE_FILL") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.score_fill_r=r; theme_.score_fill_g=g; theme_.score_fill_b=b; return true; }
    }
    if (key == "SCORE_OUTLINE") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.score_outline_r=r; theme_.score_outline_g=g; theme_.score_outline_b=b; return true; }
    }
    
    // === STATS (já existente, mantido) ===
    if (key == "STATS_LABEL") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.stats_label_r=r; theme_.stats_label_g=g; theme_.stats_label_b=b; return true; }
    }
    if (key == "STATS_FILL") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.stats_fill_r=r; theme_.stats_fill_g=g; theme_.stats_fill_b=b; return true; }
    }
    if (key == "STATS_OUTLINE") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.stats_outline_r=r; theme_.stats_outline_g=g; theme_.stats_outline_b=b; return true; }
    }
    if (key == "STATS_COUNT") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.stats_count_r=r; theme_.stats_count_g=g; theme_.stats_count_b=b; return true; }
    }
    
    // === OVERLAY (novo!) ===
    if (key == "OVERLAY_FILL") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.overlay_fill_r=r; theme_.overlay_fill_g=g; theme_.overlay_fill_b=b; return true; }
    }
    if (key == "OVERLAY_FILL_ALPHA") {
        theme_.overlay_fill_a = (Uint8)std::stoi(value); return true;
    }
    if (key == "OVERLAY_OUTLINE") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.overlay_outline_r=r; theme_.overlay_outline_g=g; theme_.overlay_outline_b=b; return true; }
    }
    if (key == "OVERLAY_OUTLINE_ALPHA") {
        theme_.overlay_outline_a = (Uint8)std::stoi(value); return true;
    }
    if (key == "OVERLAY_TOP") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.overlay_top_r=r; theme_.overlay_top_g=g; theme_.overlay_top_b=b; return true; }
    }
    if (key == "OVERLAY_SUB") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.overlay_sub_r=r; theme_.overlay_sub_g=g; theme_.overlay_sub_b=b; return true; }
    }
    
    return false;
}


