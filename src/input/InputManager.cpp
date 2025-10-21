#include "input/InputManager.hpp"
#include "input/KeyboardInput.hpp"
#include <typeinfo>

void InputManager::addHandler(std::unique_ptr<InputHandler> handler) {
    // Check if this is a KeyboardInput to store direct reference
    KeyboardInput* keyboardPtr = dynamic_cast<KeyboardInput*>(handler.get());
    if (keyboardPtr) {
        keyboardHandler = keyboardPtr;
    }
    
    handlers.push_back(std::move(handler));
    if (!primaryHandler) primaryHandler = handlers.back().get();
}

void InputManager::update() {
    // Process SDL events and forward to appropriate handlers
    SDL_Event e; 
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quitRequested = true;
        } else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
            quitRequested = true;
        } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            // Handle global quit shortcuts
            if (e.type == SDL_KEYDOWN) {
                const SDL_Keymod mods = (SDL_Keymod)(e.key.keysym.mod);
                if ((mods & KMOD_ALT) && e.key.keysym.sym == SDLK_F4) {
                    quitRequested = true;
                    continue;
                }
            }
            
            // Forward keyboard events to KeyboardInput handler
            handleKeyboardEvent(e.key);
        }
    }
    
    // Update all handlers (but KeyboardInput no longer polls SDL_GetKeyboardState)
    for (auto& h : handlers) h->update();
}

void InputManager::handleKeyboardEvent(const SDL_KeyboardEvent& event) {
    if (keyboardHandler) {
        keyboardHandler->handleKeyEvent(event);
    }
}