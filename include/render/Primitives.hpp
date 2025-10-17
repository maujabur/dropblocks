#pragma once

#include <string>
#include <SDL2/SDL.h>

void drawRoundedFilled(SDL_Renderer* r, int x, int y, int w, int h, int rad,
                       Uint8 R, Uint8 G, Uint8 B, Uint8 A);
void drawRoundedOutline(SDL_Renderer* r, int x, int y, int w, int h, int rad, int thickness,
                        Uint8 R, Uint8 G, Uint8 B, Uint8 A);
void drawPixelText(SDL_Renderer* r, int x, int y, const std::string& text, int scale,
                   Uint8 R, Uint8 G, Uint8 B);
void drawPixelTextOutlined(SDL_Renderer* r, int x, int y, const std::string& text, int scale,
                           Uint8 R, Uint8 G, Uint8 B,
                           Uint8 oR, Uint8 oG, Uint8 oB);
int textWidthPx(const std::string& text, int scale);


