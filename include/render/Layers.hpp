#pragma once

#include "RenderLayer.hpp"
#include <string>

class GameState;
class LayoutCache;
struct SDL_Renderer;
class AudioSystem;

class BackgroundLayer : public RenderLayer {
public:
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override;
    int getZOrder() const override;
    std::string getName() const override;
};

class BannerLayer : public RenderLayer {
private:
    AudioSystem* audio_ = nullptr;
public:
    explicit BannerLayer(AudioSystem* audio);
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override;
    int getZOrder() const override;
    std::string getName() const override;
};

class BoardLayer : public RenderLayer {
public:
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override;
    int getZOrder() const override;
    std::string getName() const override;
};

class HUDLayer : public RenderLayer {
public:
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override;
    int getZOrder() const override;
    std::string getName() const override;
};

class OverlayLayer : public RenderLayer {
public:
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override;
    int getZOrder() const override;
    std::string getName() const override;
};

class PostEffectsLayer : public RenderLayer {
private:
    AudioSystem* audio_ = nullptr;
public:
    explicit PostEffectsLayer(AudioSystem* audio);
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override;
    int getZOrder() const override;
    std::string getName() const override;
};

 