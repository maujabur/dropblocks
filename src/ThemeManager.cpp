#include "ThemeManager.hpp"
#include "ConfigTypes.hpp"
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
    if (key == "BG") {
        Uint8 r, g, b; if (parseHexColor(value, r, g, b)) { theme_.bg_r=r; theme_.bg_g=g; theme_.bg_b=b; return true; }
    }
    return false;
}


