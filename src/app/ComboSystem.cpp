#include "app/ComboSystem.hpp"
#include "audio/AudioSystem.hpp"

void ComboSystem::onLineClear(AudioSystem& audio) {
    Uint32 now = SDL_GetTicks();
    if (now - lastClear < 2000) {
        combo++;
    } else {
        combo = 1;
    }
    lastClear = now;
    audio.playComboSound(combo);
}

void ComboSystem::reset() {
    combo = 0;
    lastClear = 0;
}


