#pragma once

class AudioSystem;
class InputManager;
class RenderManager;
struct SDL_Window;
struct SDL_Renderer;

class GameCleanup {
private:
    bool cleaned_ = false;
public:
    void cleanupAudio(AudioSystem& audio);
    void cleanupInput(InputManager& inputManager);
    void cleanupWindow(SDL_Window* win, SDL_Renderer* ren);
    void cleanupRender(RenderManager& renderManager);
    void cleanupSDL();
    void cleanupAll(AudioSystem& audio, InputManager& inputManager, RenderManager& renderManager, SDL_Window* win, SDL_Renderer* ren);
    bool isCleaned() const { return cleaned_; }
};


