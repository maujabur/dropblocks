#pragma once

#include <memory>
#include <vector>
#include <SDL2/SDL.h>
#include "InputHandler.hpp"
#include "IInputManager.hpp"

class InputManager : public IInputManager {
private:
    std::vector<std::unique_ptr<InputHandler>> handlers;
    InputHandler* primaryHandler = nullptr;
    bool quitRequested = false;

public:
    void addHandler(std::unique_ptr<InputHandler> handler) {
        handlers.push_back(std::move(handler));
        if (!primaryHandler) primaryHandler = handlers.back().get();
    }
    void setPrimaryHandler(InputHandler* handler) { primaryHandler = handler; }
    void update() override {
        // Drain SDL events for global quit/screenshot triggers
        SDL_Event e; while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quitRequested = true;
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) quitRequested = true;
            else if (e.type == SDL_KEYDOWN) {
                const SDL_Keymod mods = (SDL_Keymod)(e.key.keysym.mod);
                if ((mods & KMOD_ALT) && e.key.keysym.sym == SDLK_F4) quitRequested = true;
            }
        }
        for (auto& h : handlers) h->update();
    }
    std::vector<std::unique_ptr<InputHandler>>& getHandlers() { return handlers; }
    InputHandler* getActiveHandler() {
        if (primaryHandler && primaryHandler->isConnected()) return primaryHandler;
        for (auto& h : handlers) if (h->isConnected()) return h.get();
        return nullptr;
    }
    bool shouldMoveLeft() override { for (auto& h : handlers) if (h->isConnected() && h->shouldMoveLeft()) return true; return false; }
    bool shouldMoveRight() override { for (auto& h : handlers) if (h->isConnected() && h->shouldMoveRight()) return true; return false; }
    bool shouldSoftDrop() override { for (auto& h : handlers) if (h->isConnected() && h->shouldSoftDrop()) return true; return false; }
    bool shouldHardDrop() override { for (auto& h : handlers) if (h->isConnected() && h->shouldHardDrop()) return true; return false; }
    bool shouldRotateCCW() override { for (auto& h : handlers) if (h->isConnected() && h->shouldRotateCCW()) return true; return false; }
    bool shouldRotateCW() override { for (auto& h : handlers) if (h->isConnected() && h->shouldRotateCW()) return true; return false; }
    bool shouldPause() override { for (auto& h : handlers) if (h->isConnected() && h->shouldPause()) return true; return false; }
    bool shouldRestart() override { for (auto& h : handlers) if (h->isConnected() && h->shouldRestart()) return true; return false; }
    bool shouldForceRestart() override { for (auto& h : handlers) if (h->isConnected() && h->shouldForceRestart()) return true; return false; }
    bool shouldQuit() override {
        if (quitRequested) return true;
        for (auto& h : handlers) if (h->isConnected() && h->shouldQuit()) return true; return false;
    }
    bool shouldScreenshot() override { for (auto& h : handlers) if (h->isConnected() && h->shouldScreenshot()) return true; return false; }
    void resetTimers() override { auto h = getActiveHandler(); if (h) h->resetTimers(); }
    void cleanup() { quitRequested = false; handlers.clear(); primaryHandler = nullptr; }
};


