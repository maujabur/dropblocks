// Implement full rendering primitives here (moved from dropblocks.cpp)
#include "render/Primitives.hpp"
#include <algorithm>
#include <cmath>
#include <cctype>

extern int ROUNDED_PANELS; // global visual flag from config

// 5x7 pixel font glyphs
static bool glyph5x7(char c, int x, int y){
    auto at=[&](const char* rows[7]){ return rows[y][x]=='#'; };
    static const char* NUM[10][7] = {
        {" ### ","#   #","#  ##","# # #","##  #","#   #"," ### "},
        {"  #  "," ##  ","  #  ","  #  ","  #  ","  #  "," ### "},
        {" ### ","#   #","    #","   # ","  #  "," #   ","#####"},
        {" ### ","#   #","    #"," ### ","    #","#   #"," ### "},
        {"   # ","  ## "," # # ","#  # ","#####","   # ","   # "},
        {"#####","#    ","#    ","#### ","    #","#   #"," ### "},
        {" ### ","#   #","#    ","#### ","#   #","#   #"," ### "},
        {"#####","    #","   # ","  #  ","  #  ","  #  ","  #  "},
        {" ### ","#   #","#   #"," ### ","#   #","#   #"," ### "},
        {" ### ","#   #","#   #"," ####","    #","#   #"," ### "}
    };
    static const char* A_[7] = {
        " ### ",
        "#   #",
        "#   #",
        "#####",
        "#   #",
        "#   #",
        "#   #"};
    static const char* B_[7] = {
        "#### ",
        "#   #",
        "#   #",
        "#### ",
        "#   #",
        "#   #",
        "#### "};
    static const char* C_[7] = {" ### ","#   #","#    ","#    ","#    ","#   #"," ### "};
    static const char* D_[7] = {"#### ","#   #","#   #","#   #","#   #","#   #","#### "};
    static const char* E_[7] = {"#####","#    ","#    ","#### ","#    ","#    ","#####"};
    static const char* F_[7] = {"#####","#    ","#    ","#### ","#    ","#    ","#    "};
    static const char* G_[7] = {" ### ","#   #","#    ","# ###","#   #","#   #"," ### "};
    static const char* H_[7] = {"#   #","#   #","#   #","#####","#   #","#   #","#   #"};
    static const char* I_[7] = {"#####","  #  ","  #  ","  #  ","  #  ","  #  ","#####"};
    static const char* J_[7] = {"  ###","   # ","   # ","   # ","#  # ","#  # "," ##  "};
    static const char* K_[7] = {"#   #","#  # ","# #  ","##   ","# #  ","#  # ","#   #"};
    static const char* L_[7] = {"#    ","#    ","#    ","#    ","#    ","#    ","#####"};
    static const char* M_[7] = {"#   #","## ##","# # #","#   #","#   #","#   #","#   #"};
    static const char* N_[7] = {"#   #","##  #","# # #","#  ##","#   #","#   #","#   #"};
    static const char* O_[7] = {" ### ","#   #","#   #","#   #","#   #","#   #"," ### "};
    static const char* P_[7] = {"#### ","#   #","#   #","#### ","#    ","#    ","#    "};
    static const char* Q_[7] = {" ### ","#   #","#   #","#   #","# # #","#  # "," ## #"};
    static const char* R_[7] = {"#### ","#   #","#   #","#### ","# #  ","#  # ","#   #"};
    static const char* S_[7] = {" ####","#    ","#    "," ### ","    #","    #","#### "};
    static const char* T_[7] = {"#####","  #  ","  #  ","  #  ","  #  ","  #  ","  #  "};
    static const char* U_[7] = {"#   #","#   #","#   #","#   #","#   #","#   #"," ### "};
    static const char* V_[7] = {"#   #","#   #","#   #","#   #","#   #"," # # ","  #  "};
    static const char* W_[7] = {"#   #","#   #","#   #","# # #","# # #","## ##","#   #"};
    static const char* X_[7] = {"#   #","#   #"," # # ","  #  "," # # ","#   #","#   #"};
    static const char* Y_[7] = {"#   #","#   #"," # # ","  #  ","  #  ","  #  ","  #  "};
    static const char* Z_[7] = {"#####","    #","   # ","  #  "," #   ","#    ","#####"};
    static const char* dash[7] = {"     ","     ","     "," ### ","     ","     ","     "};
    static const char* colon[7] = {"     ","  #  ","     ","     ","     ","  #  ","     "};
    if(c>='0'&&c<='9') return NUM[c-'0'][y][x]=='#';
    c=(char)std::toupper((unsigned char)c);
#define GL(CH,ARR) if(c==CH) return at(ARR);
    GL('A',A_) GL('B',B_) GL('C',C_) GL('D',D_) GL('E',E_) GL('F',F_) GL('G',G_)
    GL('H',H_) GL('I',I_) GL('J',J_) GL('K',K_) GL('L',L_) GL('M',M_) GL('N',N_)
    GL('O',O_) GL('P',P_) GL('Q',Q_) GL('R',R_) GL('S',S_) GL('T',T_) GL('U',U_)
    GL('V',V_) GL('W',W_) GL('X',X_) GL('Y',Y_) GL('Z',Z_) GL('-',dash) GL(':',colon)
#undef GL
    return false;
}

void drawPixelText(SDL_Renderer* ren, int x, int y, const std::string& s, int scale, Uint8 r, Uint8 g, Uint8 b){
    SDL_Rect px; SDL_SetRenderDrawColor(ren, r,g,b,255);
    int cx=x;
    for(char c : s){
        if(c=='\n'){ y += (7*scale + scale*2); cx=x; continue; }
        for(int yy=0; yy<7; ++yy) for(int xx=0; xx<5; ++xx)
            if(glyph5x7(c,xx,yy)){ px = { cx + xx*scale, y + yy*scale, scale, scale }; SDL_RenderFillRect(ren, &px); }
        cx += 6*scale;
    }
}

int textWidthPx(const std::string& s, int scale){
    if(s.empty()) return 0; return (int)s.size() * 6 * scale - scale;
}

void drawPixelTextOutlined(SDL_Renderer* ren, int x, int y, const std::string& s, int scale,
                           Uint8 fr, Uint8 fg, Uint8 fb, Uint8 or_, Uint8 og, Uint8 ob){
    const int d = std::max(1, scale/2);
    drawPixelText(ren, x-d, y,   s, scale, or_,og,ob);
    drawPixelText(ren, x+d, y,   s, scale, or_,og,ob);
    drawPixelText(ren, x,   y-d, s, scale, or_,og,ob);
    drawPixelText(ren, x,   y+d, s, scale, or_,og,ob);
    drawPixelText(ren, x-d, y-d, s, scale, or_,og,ob);
    drawPixelText(ren, x+d, y-d, s, scale, or_,og,ob);
    drawPixelText(ren, x-d, y+d, s, scale, or_,og,ob);
    drawPixelText(ren, x+d, y+d, s, scale, or_,og,ob);
    drawPixelText(ren, x,   y,   s, scale, fr,fg,fb);
}

void drawRoundedFilled(SDL_Renderer* r, int x, int y, int w, int h, int rad, Uint8 R, Uint8 G, Uint8 B, Uint8 A){
    if(!ROUNDED_PANELS){ SDL_SetRenderDrawColor(r, R,G,B,A); SDL_Rect rr{ x,y,w,h }; SDL_RenderFillRect(r,&rr); return; }
    rad = std::max(0, std::min(rad, std::min(w,h)/2));
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, R,G,B,A);
    for (int yy=0; yy<h; ++yy){
        int left = x, right = x + w - 1;
        if (yy < rad){
            int dy = rad-1-yy; int dx = (int)std::floor(std::sqrt((double)rad*rad - (double)dy*dy));
            left = x + rad - dx; right = x + w - rad + dx - 1;
        } else if (yy >= h - rad){
            int dy = yy - (h - rad); int dx = (int)std::floor(std::sqrt((double)rad*rad - (double)dy*dy));
            left = x + rad - dx; right = x + w - rad + dx - 1;
        }
        SDL_Rect line{ left, y + yy, right - left + 1, 1 };
        SDL_RenderFillRect(r, &line);
    }
}

void drawRoundedOutline(SDL_Renderer* r, int x, int y, int w, int h, int rad, int thick, Uint8 R, Uint8 G, Uint8 B, Uint8 A){
    for(int i=0;i<thick;i++){
        drawRoundedFilled(r, x+i, y+i, w-2*i, h-2*i, std::max(0,rad-i), R,G,B,A);
    }
}

