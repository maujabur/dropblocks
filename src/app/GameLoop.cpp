#include "app/GameLoop.hpp"
#include "render/LayoutCache.hpp"
#include "DebugLogger.hpp"
#include <SDL2/SDL.h>
#include "render/RenderManager.hpp"
#include "render/GameStateBridge.hpp"

class GameState; // defined in main TU

void GameLoop::run(GameState& state, RenderManager& renderManager, SDL_Renderer* ren) {
    if (running_) { DebugLogger::warning("Game loop is already running"); return; }
    running_ = true;
    LayoutCache layoutCache; // main TU defines calculate/dirty
    while (db_isRunning(state) && running_) {
        if (!ren) { DebugLogger::error("Renderer is null; aborting main loop"); break; }
        // temporary: disable mouse cursor
        SDL_ShowCursor(SDL_DISABLE);
        
        // Force recalc every frame; the function in main TU handles dirty flag and geometry
        db_layoutCalculate(layoutCache);
        db_update(state, ren);
        db_render(state, renderManager, layoutCache);
        SDL_RenderPresent(ren);
        SDL_Delay(1);
    }
    running_ = false;
    DebugLogger::info("Main game loop ended");
}

void GameLoop::stop() { running_ = false; DebugLogger::info("Game loop stop requested"); }


