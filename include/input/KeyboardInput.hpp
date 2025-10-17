#pragma once

#include <SDL2/SDL.h>
#include "InputHandler.hpp"

class KeyboardInput : public InputHandler {
private:
    bool keyStates[SDL_NUM_SCANCODES] = {false};
    bool lastKeyStates[SDL_NUM_SCANCODES] = {false};
    Uint32 lastMoveTime = 0;
    Uint32 moveRepeatDelay = 200;

    bool isKeyPressed(SDL_Scancode key) {
        return keyStates[key] && !lastKeyStates[key];
    }

public:
    bool shouldMoveLeft() override { return isKeyPressed(SDL_SCANCODE_LEFT)  && SDL_GetTicks() - lastMoveTime > moveRepeatDelay; }
    bool shouldMoveRight() override { return isKeyPressed(SDL_SCANCODE_RIGHT) && SDL_GetTicks() - lastMoveTime > moveRepeatDelay; }
    bool shouldSoftDrop() override { return isKeyPressed(SDL_SCANCODE_DOWN); }
    bool shouldHardDrop() override { return isKeyPressed(SDL_SCANCODE_SPACE); }
    bool shouldRotateCCW() override { return isKeyPressed(SDL_SCANCODE_Z) || isKeyPressed(SDL_SCANCODE_UP); }
    bool shouldRotateCW() override { return isKeyPressed(SDL_SCANCODE_X); }
    bool shouldPause() override { return isKeyPressed(SDL_SCANCODE_P); }
    bool shouldRestart() override { return isKeyPressed(SDL_SCANCODE_RETURN); }
    bool shouldQuit() override { return isKeyPressed(SDL_SCANCODE_ESCAPE); }
    bool shouldScreenshot() override { return isKeyPressed(SDL_SCANCODE_F12); }

    void update() override {
        for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
            lastKeyStates[i] = keyStates[i];
            keyStates[i] = SDL_GetKeyboardState(nullptr)[i];
        }
    }
    bool isConnected() override { return true; }
    void resetTimers() override { lastMoveTime = SDL_GetTicks(); }
};


