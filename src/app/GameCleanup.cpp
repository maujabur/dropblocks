#include "app/GameCleanup.hpp"
#include "DebugLogger.hpp"
#include <SDL2/SDL.h>
#include "audio/AudioSystem.hpp"
#include "input/InputManager.hpp"
#include "render/RenderManager.hpp"

void GameCleanup::cleanupAudio(AudioSystem& audio) { audio.cleanup(); DebugLogger::info("Audio system cleaned up"); }
void GameCleanup::cleanupInput(InputManager& inputManager) { inputManager.cleanup(); DebugLogger::info("Input system cleaned up"); }
void GameCleanup::cleanupWindow(SDL_Window* win, SDL_Renderer* ren) {
    if (ren) { SDL_DestroyRenderer(ren); DebugLogger::info("Renderer destroyed"); }
    if (win) { SDL_DestroyWindow(win); DebugLogger::info("Window destroyed"); }
}
void GameCleanup::cleanupRender(RenderManager& renderManager) { renderManager.cleanup(); DebugLogger::info("Render system cleaned up"); }
void GameCleanup::cleanupSDL() { SDL_Quit(); DebugLogger::info("SDL2 cleaned up"); }
void GameCleanup::cleanupAll(AudioSystem& audio, InputManager& inputManager, RenderManager& renderManager, SDL_Window* win, SDL_Renderer* ren) {
    if (cleaned_) return;
    cleanupRender(renderManager);
    cleanupInput(inputManager);
    cleanupAudio(audio);
    cleanupWindow(win, ren);
    cleanupSDL();
    cleaned_ = true;
    DebugLogger::info("Game cleanup completed");
}


