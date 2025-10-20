#pragma once

#include <SDL2/SDL.h>
#include "InputHandler.hpp"

class KeyboardInput : public InputHandler {
private:
    bool keyStates[SDL_NUM_SCANCODES] = {false};
    bool lastKeyStates[SDL_NUM_SCANCODES] = {false};
    Uint32 lastMoveTime = 0;
    Uint32 leftPressTime = 0;
    Uint32 rightPressTime = 0;
    Uint32 moveRepeatDelayDAS = 170;  // DAS: Delayed Auto Shift (initial delay)
    Uint32 moveRepeatDelayARR = 50;   // ARR: Auto Repeat Rate (repeat interval)

    bool isKeyPressed(SDL_Scancode key) {
        return keyStates[key] && !lastKeyStates[key];
    }
    
    bool shouldMoveHorizontal(SDL_Scancode key, Uint32& pressTime) {
        Uint32 now = SDL_GetTicks();
        
        // Just pressed
        if (isKeyPressed(key)) {
            pressTime = now;
            lastMoveTime = now;
            return true;
        }
        
        // Held down - check DAS then ARR
        if (keyStates[key]) {
            Uint32 heldTime = now - pressTime;
            
            // After DAS delay, use ARR for continuous movement
            if (heldTime > moveRepeatDelayDAS && (now - lastMoveTime) > moveRepeatDelayARR) {
                lastMoveTime = now;
                return true;
            }
        }
        
        return false;
    }

public:
    bool shouldMoveLeft() override { return shouldMoveHorizontal(SDL_SCANCODE_LEFT, leftPressTime); }
    bool shouldMoveRight() override { return shouldMoveHorizontal(SDL_SCANCODE_RIGHT, rightPressTime); }
    bool shouldSoftDrop() override { return isKeyPressed(SDL_SCANCODE_DOWN); }
    bool shouldHardDrop() override { return isKeyPressed(SDL_SCANCODE_SPACE); }
    bool shouldRotateCCW() override { return isKeyPressed(SDL_SCANCODE_Z) || isKeyPressed(SDL_SCANCODE_UP); }
    bool shouldRotateCW() override { return isKeyPressed(SDL_SCANCODE_X); }
    bool shouldPause() override { return isKeyPressed(SDL_SCANCODE_P); }
    bool shouldRestart() override { return isKeyPressed(SDL_SCANCODE_RETURN); }
    bool shouldForceRestart() override { return isKeyPressed(SDL_SCANCODE_R); }
    bool shouldQuit() override { return isKeyPressed(SDL_SCANCODE_ESCAPE); }
    bool shouldScreenshot() override { return isKeyPressed(SDL_SCANCODE_F12); }
    bool shouldToggleDebug() { return isKeyPressed(SDL_SCANCODE_D); }

    void update() override {
        for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
            lastKeyStates[i] = keyStates[i];
            keyStates[i] = SDL_GetKeyboardState(nullptr)[i];
        }
    }
    bool isConnected() override { return true; }
    void resetTimers() override { lastMoveTime = SDL_GetTicks(); }
};


