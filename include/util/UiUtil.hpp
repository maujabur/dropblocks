#pragma once
#include <string>
struct SDL_Renderer;

std::string fmtScore(int value);
bool saveScreenshot(SDL_Renderer* renderer, const char* path);


