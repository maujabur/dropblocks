#include "../../include/render/RenderManager.hpp"
#include "../../include/render/RenderLayer.hpp"
#include "../../include/DebugLogger.hpp"

#include <algorithm>

class GameState;
class LayoutCache;
class RenderLayer; // forward declare
struct SDL_Renderer;

RenderManager::RenderManager(SDL_Renderer* renderer) : renderer_(renderer) {}

void RenderManager::addLayer(std::unique_ptr<RenderLayer> layer) {
    layers_.push_back(std::move(layer));
    std::sort(layers_.begin(), layers_.end(),
              [](const std::unique_ptr<RenderLayer>& a, const std::unique_ptr<RenderLayer>& b) {
                  return a->getZOrder() < b->getZOrder();
              });
}

void RenderManager::render(const GameState& state, const LayoutCache& layout) {
    for (auto& layer : layers_) {
        if (layer->isEnabled()) {
            layer->render(renderer_, state, layout);
        }
    }
}

void RenderManager::setLayerEnabled(const std::string& name, bool enabled) {
    for (auto& layer : layers_) {
        if (layer->getName() == name) {
            layer->setEnabled(enabled);
            break;
        }
    }
}

void RenderManager::cleanup() {
    layers_.clear();
}

RenderLayer* RenderManager::getLayer(const std::string& name) {
    for (auto& layer : layers_) {
        if (layer->getName() == name) {
            return layer.get();
        }
    }
    return nullptr;
}

std::vector<std::string> RenderManager::getLayerNames() const {
    std::vector<std::string> names;
    names.reserve(layers_.size());
    for (const auto& layer : layers_) {
        names.push_back(layer->getName());
    }
    return names;
}


