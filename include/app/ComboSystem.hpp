#pragma once
#include <SDL2/SDL.h>

class AudioSystem;

struct ComboSystem {
    int combo = 0;
    Uint32 lastClear = 0;

    void onLineClear(AudioSystem& audio);
    void reset();
};


