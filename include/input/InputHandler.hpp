#pragma once

#include <memory>

class SDL_Renderer; // forward

class InputHandler {
public:
    virtual ~InputHandler() = default;

    virtual bool shouldMoveLeft() = 0;
    virtual bool shouldMoveRight() = 0;
    virtual bool shouldSoftDrop() = 0;
    virtual bool shouldHardDrop() = 0;
    virtual bool shouldRotateCCW() = 0;
    virtual bool shouldRotateCW() = 0;
    virtual bool shouldPause() = 0;
    virtual bool shouldRestart() = 0;
    virtual bool shouldForceRestart() = 0;  // Restart even before game over
    virtual bool shouldQuit() = 0;
    virtual bool shouldScreenshot() = 0;

    virtual void update() = 0;
    virtual bool isConnected() = 0;
    virtual void resetTimers() = 0;
};


