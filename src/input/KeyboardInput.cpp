#include "input/KeyboardInput.hpp"

void KeyboardInput::handleKeyEvent(const SDL_KeyboardEvent& event) {
    // Only handle key press/release, ignore repeat events from OS
    if (event.repeat == 0) {
        SDL_Scancode scancode = event.keysym.scancode;
        if (scancode >= 0 && scancode < SDL_NUM_SCANCODES) {
            keyStates[scancode] = (event.type == SDL_KEYDOWN);
        }
    }
}