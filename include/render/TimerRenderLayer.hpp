#pragma once

#include "render/RenderLayer.hpp"
#include <SDL2/SDL.h>
#include <string>

// Forward declarations
class TimerSystem;
class GameState;
struct LayoutCache;

/**
 * @brief Camada de renderização para o countdown timer
 * 
 * Renderiza o timer com formatação visual, cores responsivas e efeitos visuais.
 * Suporte para diferentes estados (normal, warning, critical) e piscante.
 */
class TimerRenderLayer : public RenderLayer {
private:
    static const int Z_ORDER = 100; // Alta prioridade (sobre outros elementos)
    
    // Cache para evitar recálculos
    mutable std::string lastFormattedTime_;
    mutable SDL_Texture* textTexture_;
    mutable int textWidth_;
    mutable int textHeight_;
    mutable bool textureNeedsUpdate_;
    
    // Estado visual
    mutable Uint32 lastBlinkTime_;
    mutable bool blinkState_;
    
    void updateTextTexture(SDL_Renderer* renderer, const TimerSystem& timer, const LayoutCache& layout) const;
    void renderBackground(SDL_Renderer* renderer, const TimerSystem& timer, const LayoutCache& layout) const;
    void renderText(SDL_Renderer* renderer, const TimerSystem& timer, const LayoutCache& layout) const;
    void renderProgressBar(SDL_Renderer* renderer, const TimerSystem& timer, const LayoutCache& layout) const;
    
    // Conversão de coordenadas virtuais para físicas
    SDL_Rect getPhysicalRect(const TimerSystem& timer, const LayoutCache& layout) const;
    
    // Cores com efeitos
    SDL_Color getBlinkColor(const TimerSystem& timer) const;
    
public:
    TimerRenderLayer();
    ~TimerRenderLayer();
    
    // Interface RenderLayer
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override;
    std::string getName() const override { return "Timer"; }
    int getZOrder() const override { return Z_ORDER; }
    
    // Cleanup
    void cleanup();
};