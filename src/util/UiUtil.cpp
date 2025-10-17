#include "util/UiUtil.hpp"
#include <algorithm>
#include <SDL2/SDL.h>

std::string fmtScore(int value){
    std::string s = std::to_string(value), out; int count = 0;
    for(int i = (int)s.size()-1; i >= 0; --i){ out.push_back(s[i]); if(++count==3 && i>0){ out.push_back(' '); count=0; } }
    std::reverse(out.begin(), out.end()); return out;
}

bool saveScreenshot(SDL_Renderer* renderer, const char* path) {
    int w, h; SDL_GetRendererOutputSize(renderer, &w, &h);
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 24, SDL_PIXELFORMAT_BGR24);
    if (!surf) return false;
    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_BGR24, surf->pixels, surf->pitch) != 0) {
        SDL_FreeSurface(surf); return false;
    }
    int rc = SDL_SaveBMP(surf, path);
    SDL_FreeSurface(surf);
    return (rc == 0);
}


