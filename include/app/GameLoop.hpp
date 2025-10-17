#pragma once

class GameState;
class RenderManager;
struct SDL_Renderer;
struct LayoutCache;

class GameLoop {
private:
    bool running_ = false;
    LayoutCache* layoutCachePtr_ = nullptr; // forward-only; managed in cpp
public:
    void run(GameState& state, RenderManager& renderManager, SDL_Renderer* ren);
    void stop();
    bool isRunning() const { return running_; }
};


