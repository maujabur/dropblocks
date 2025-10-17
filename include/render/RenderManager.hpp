#pragma once

#include <memory>
#include <string>
#include <vector>
#include "RenderLayer.hpp"

class GameState;
class LayoutCache;
class RenderLayer;
struct SDL_Renderer;

class RenderManager {
private:
    std::vector<std::unique_ptr<RenderLayer>> layers_;
    SDL_Renderer* renderer_ = nullptr;

public:
    explicit RenderManager(SDL_Renderer* renderer);

    void addLayer(std::unique_ptr<RenderLayer> layer);
    void render(const GameState& state, const LayoutCache& layout);
    void setLayerEnabled(const std::string& name, bool enabled);
    void cleanup();
    RenderLayer* getLayer(const std::string& name);
    std::vector<std::string> getLayerNames() const;
};


