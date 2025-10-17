#pragma once

#include <string>

class GameState;
class LayoutCache;
struct SDL_Renderer;

class RenderLayer {
public:
    virtual ~RenderLayer() = default;

    virtual void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) = 0;

    virtual int getZOrder() const = 0;
    virtual bool isEnabled() const { return enabled_; }
    virtual void setEnabled(bool enabled) { enabled_ = enabled; }

    virtual std::string getName() const = 0;

protected:
    bool enabled_ = true;
};


