#include "app/GameLoop.hpp"
#include "render/LayoutCache.hpp"
#include "render/TextureCache.hpp"
#include "DebugOverlay.hpp"
#include "DebugLogger.hpp"
#include "ThemeManager.hpp"
#include "input/KeyboardInput.hpp"
#include <SDL2/SDL.h>
#include "render/RenderManager.hpp"
#include "render/GameStateBridge.hpp"

class GameState; // defined in main TU
extern ThemeManager themeManager;

void GameLoop::run(GameState& state, RenderManager& renderManager, SDL_Renderer* ren) {
    if (running_) { DebugLogger::warning("Game loop is already running"); return; }
    running_ = true;
    LayoutCache layoutCache;
    TextureCache textureCache;
    DebugOverlay debugOverlay;
    
    // Calculate layout once at startup
    db_layoutCalculate(layoutCache, ren);
    int lastWidth = layoutCache.SWr;
    int lastHeight = layoutCache.SHr;
    
    // Pre-render static textures
    textureCache.update(ren, layoutCache, themeManager);
    
    // Frame timing
    Uint32 lastFrameTime = SDL_GetTicks();
    
    while (db_isRunning(state) && running_) {
        if (!ren) { DebugLogger::error("Renderer is null; aborting main loop"); break; }
        
        // Measure frame start time
        Uint32 frameStart = SDL_GetTicks();
        
        // Check for debug toggle (before state.update() to avoid consuming the key)
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        static bool lastDKeyState = false;
        bool currentDKeyState = keyState[SDL_SCANCODE_D];
        if (currentDKeyState && !lastDKeyState) {
            debugOverlay.toggle();
            DebugLogger::info(debugOverlay.isEnabled() ? "Debug overlay enabled" : "Debug overlay disabled");
        }
        lastDKeyState = currentDKeyState;
        
        // Only recalculate layout if window size changed
        int currentWidth, currentHeight;
        SDL_GetRendererOutputSize(ren, &currentWidth, &currentHeight);
        if (currentWidth != lastWidth || currentHeight != lastHeight) {
            db_layoutCalculate(layoutCache, ren);
            textureCache.update(ren, layoutCache, themeManager);
            lastWidth = currentWidth;
            lastHeight = currentHeight;
        }
        
        db_update(state, ren);
        db_render(state, renderManager, layoutCache);
        
        // Render debug overlay
        if (debugOverlay.isEnabled()) {
            debugOverlay.render(ren, currentWidth, currentHeight);
        }
        
        SDL_RenderPresent(ren);
        
        // Calculate frame time
        Uint32 frameEnd = SDL_GetTicks();
        float deltaMs = (frameEnd - frameStart);
        debugOverlay.update(deltaMs);
        
        SDL_Delay(1);
        lastFrameTime = frameEnd;
    }
    
    textureCache.cleanup();
    running_ = false;
    DebugLogger::info("Main game loop ended");
}

void GameLoop::stop() { running_ = false; DebugLogger::info("Game loop stop requested"); }


