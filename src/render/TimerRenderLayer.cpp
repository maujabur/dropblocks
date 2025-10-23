#include "render/TimerRenderLayer.hpp"
#include "render/Primitives.hpp"
#include "render/LayoutCache.hpp"
#include "timer/TimerSystem.hpp"
#include "app/GameState.hpp"
#include "DebugLogger.hpp"
#include <cmath>

TimerRenderLayer::TimerRenderLayer() 
    : textTexture_(nullptr), textWidth_(0), textHeight_(0), textureNeedsUpdate_(true),
      lastBlinkTime_(0), blinkState_(false) {
}

TimerRenderLayer::~TimerRenderLayer() {
    cleanup();
}

void TimerRenderLayer::cleanup() {
    if (textTexture_) {
        SDL_DestroyTexture(textTexture_);
        textTexture_ = nullptr;
    }
    textureNeedsUpdate_ = true;
}

SDL_Rect TimerRenderLayer::getPhysicalRect(const TimerSystem& timer, const LayoutCache& layout) const {
    const auto& timerLayout = timer.getLayout();
    
    // Converter coordenadas virtuais para físicas
    SDL_Rect rect;
    rect.x = static_cast<int>(timerLayout.x * layout.scaleX + layout.offsetX);
    rect.y = static_cast<int>(timerLayout.y * layout.scaleY + layout.offsetY);
    rect.w = static_cast<int>(timerLayout.width * layout.scaleX);
    rect.h = static_cast<int>(timerLayout.height * layout.scaleY);
    
    return rect;
}

SDL_Color TimerRenderLayer::getBlinkColor(const TimerSystem& timer) const {
    RGB color = timer.getCurrentColor();
    
    // Efeito piscante apenas no estado crítico
    if (timer.isCritical()) {
        Uint32 currentTime = SDL_GetTicks();
        
        // Atualizar estado do piscar a cada 500ms
        if (currentTime - lastBlinkTime_ >= 500) {
            blinkState_ = !blinkState_;
            lastBlinkTime_ = currentTime;
        }
        
        // Se está piscando e no estado "off", use uma cor mais escura
        if (blinkState_) {
            color.r = static_cast<unsigned char>(color.r * 0.3f);
            color.g = static_cast<unsigned char>(color.g * 0.3f);
            color.b = static_cast<unsigned char>(color.b * 0.3f);
        }
    }
    
    return {color.r, color.g, color.b, 255};
}

void TimerRenderLayer::renderBackground(SDL_Renderer* renderer, const TimerSystem& timer, const LayoutCache& layout) const {
    if (!timer.isEnabled() || !timer.getLayout().enabled) return;
    
    SDL_Rect rect = getPhysicalRect(timer, layout);
    const auto& timerLayout = timer.getLayout();
    
    // Renderizar fundo
    if (timerLayout.backgroundAlpha > 0) {
        drawRoundedFilled(renderer, rect.x, rect.y, rect.w, rect.h, 
                         layout.borderRadiusX, layout.borderRadiusY,
                         timerLayout.backgroundColor.r, 
                         timerLayout.backgroundColor.g, 
                         timerLayout.backgroundColor.b, 
                         timerLayout.backgroundAlpha);
    }
}

void TimerRenderLayer::renderProgressBar(SDL_Renderer* renderer, const TimerSystem& timer, const LayoutCache& layout) const {
    if (!timer.isEnabled()) return;
    
    SDL_Rect rect = getPhysicalRect(timer, layout);
    float progress = timer.getProgress();
    
    // Barra de progresso mais visível - posicionada mais acima
    int barHeight = static_cast<int>(12 * layout.scaleY); // Altura da barra
    int barY = rect.y + rect.h - barHeight - static_cast<int>(12 * layout.scaleY); // Mais espaço da borda
    int barX = rect.x + static_cast<int>(6 * layout.scaleX);
    int barMaxWidth = rect.w - static_cast<int>(12 * layout.scaleX);
    
    // Renderizar fundo da barra (usando cor configurável)
    const auto& timerConfig = timer.getConfig();
    SDL_SetRenderDrawColor(renderer, timerConfig.progressBarBg.r, timerConfig.progressBarBg.g, timerConfig.progressBarBg.b, 180);
    SDL_Rect backgroundRect = { barX, barY, barMaxWidth, barHeight };
    SDL_RenderFillRect(renderer, &backgroundRect);
    
    // Renderizar borda da barra de progresso (usando cor configurável)
    SDL_SetRenderDrawColor(renderer, timerConfig.progressBarBorder.r, timerConfig.progressBarBorder.g, timerConfig.progressBarBorder.b, 255);
    SDL_RenderDrawRect(renderer, &backgroundRect);
    
    // Renderizar progresso restante
    int barWidth = static_cast<int>(barMaxWidth * (1.0f - progress));
    if (barWidth > 2) {
        SDL_Color color = getBlinkColor(timer);
        
        // Barra mais opaca e colorida
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 220);
        SDL_Rect progressRect = { barX + 1, barY + 1, barWidth - 2, barHeight - 2 };
        SDL_RenderFillRect(renderer, &progressRect);
        
        // Adicionar brilho na barra quando critical
        if (timer.isCritical()) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
            SDL_Rect glowRect = { barX + 1, barY + 1, barWidth - 2, 2 };
            SDL_RenderFillRect(renderer, &glowRect);
        }
    }
}

void TimerRenderLayer::renderText(SDL_Renderer* renderer, const TimerSystem& timer, const LayoutCache& layout) const {
    if (!timer.isEnabled()) return;
    
    SDL_Rect rect = getPhysicalRect(timer, layout);
    std::string timeText = timer.getFormattedTime();
    
    // Calcular escala baseada no tamanho do container
    float textScale = std::min(layout.scaleX, layout.scaleY) * 2.0f;
    
    // Calcular posição central considerando a altura do box e a barra de progresso
    int textWidth = textWidthPx(timeText, textScale);
    int textHeight = static_cast<int>(8 * textScale); // Altura aproximada do texto
    int barHeight = static_cast<int>(12 * layout.scaleY);
    int availableHeight = rect.h - barHeight - static_cast<int>(12 * layout.scaleY); // Espaço disponível acima da barra
    
    int textX = rect.x + (rect.w - textWidth) / 2;
    int textY = rect.y + (availableHeight - textHeight) / 2; // Centralizado na área disponível
    
    // Obter cor com efeito piscante se necessário
    SDL_Color color = getBlinkColor(timer);
    
    // Renderizar texto com contorno para melhor legibilidade
    drawPixelTextOutlined(renderer, textX, textY, timeText, textScale, textScale,
                         color.r, color.g, color.b,
                         0, 0, 0); // Contorno preto
}

void TimerRenderLayer::render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) {
    const TimerSystem& timer = state.getTimer();
    
    if (!timer.isEnabled()) return;
    
    // Renderizar componentes do timer
    renderBackground(renderer, timer, layout);
    renderText(renderer, timer, layout);
    renderProgressBar(renderer, timer, layout);
}