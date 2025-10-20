#include "render/TextureCache.hpp"
#include "render/LayoutCache.hpp"
#include "render/Primitives.hpp"
#include "ThemeManager.hpp"
#include "DebugLogger.hpp"

extern ThemeManager themeManager;

TextureCache::~TextureCache() {
    cleanup();
}

SDL_Texture* TextureCache::createTexture(SDL_Renderer* renderer, int w, int h) {
    if (!renderer || w <= 0 || h <= 0) return nullptr;
    
    SDL_Texture* texture = SDL_CreateTexture(renderer, 
                                            SDL_PIXELFORMAT_RGBA8888, 
                                            SDL_TEXTUREACCESS_TARGET, 
                                            w, h);
    if (texture) {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    } else {
        DebugLogger::error("Failed to create texture: " + std::string(SDL_GetError()));
    }
    return texture;
}

void TextureCache::update(SDL_Renderer* renderer, const LayoutCache& layout, ThemeManager& theme) {
    if (!renderer) return;
    
    // Clean up old textures
    cleanup();
    
    // Use new layout system if configured, otherwise fall back to legacy
    int bannerW = layout.bannerRect.w > 0 ? layout.bannerRect.w : layout.BW;
    int bannerH = layout.bannerRect.w > 0 ? layout.bannerRect.h : layout.BH;
    int statsW = layout.statsRect.w > 0 ? layout.statsRect.w : layout.statsBoxW;
    int statsH = layout.statsRect.w > 0 ? layout.statsRect.h : layout.GH;
    int hudW = layout.hudRect.w > 0 ? layout.hudRect.w : layout.panelW;
    int hudH = layout.hudRect.w > 0 ? layout.hudRect.h : layout.panelH;
    
    // 1. Create Banner Texture
    if (layout.bannerConfig.enabled) {
        bannerTexture_ = createTexture(renderer, bannerW, bannerH);
        if (bannerTexture_) {
            SDL_SetRenderTarget(renderer, bannerTexture_);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            
            // Draw banner background
            drawRoundedFilled(renderer, 0, 0, bannerW, bannerH, layout.borderRadius,
                             theme.getTheme().banner_bg_r, 
                             theme.getTheme().banner_bg_g, 
                             theme.getTheme().banner_bg_b, 255);
            drawRoundedOutline(renderer, 0, 0, bannerW, bannerH, layout.borderRadius, layout.borderThickness,
                              theme.getTheme().banner_outline_r, 
                              theme.getTheme().banner_outline_g, 
                              theme.getTheme().banner_outline_b, 
                              theme.getTheme().banner_outline_a);
            
            SDL_SetRenderTarget(renderer, nullptr);
        }
    }
    
    // 2. Create Stats Box Texture
    if (layout.statsConfig.enabled) {
        statsBoxTexture_ = createTexture(renderer, statsW, statsH);
        if (statsBoxTexture_) {
            SDL_SetRenderTarget(renderer, statsBoxTexture_);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            
            drawRoundedFilled(renderer, 0, 0, statsW, statsH, layout.borderRadius,
                             theme.getTheme().next_fill_r, 
                             theme.getTheme().next_fill_g, 
                             theme.getTheme().next_fill_b, 255);
            drawRoundedOutline(renderer, 0, 0, statsW, statsH, layout.borderRadius, layout.borderThickness,
                              theme.getTheme().next_outline_r, 
                              theme.getTheme().next_outline_g, 
                              theme.getTheme().next_outline_b, 
                              theme.getTheme().next_outline_a);
            
            SDL_SetRenderTarget(renderer, nullptr);
        }
    }
    
    // 3. Create HUD Panel Texture
    if (layout.hudConfig.enabled) {
        hudPanelTexture_ = createTexture(renderer, hudW, hudH);
        if (hudPanelTexture_) {
            SDL_SetRenderTarget(renderer, hudPanelTexture_);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            
            drawRoundedFilled(renderer, 0, 0, hudW, hudH, layout.borderRadius,
                             theme.getTheme().panel_fill_r, 
                             theme.getTheme().panel_fill_g, 
                             theme.getTheme().panel_fill_b, 255);
            drawRoundedOutline(renderer, 0, 0, hudW, hudH, layout.borderRadius, layout.borderThickness,
                              theme.getTheme().panel_outline_r, 
                              theme.getTheme().panel_outline_g, 
                              theme.getTheme().panel_outline_b, 
                              theme.getTheme().panel_outline_a);
            
            SDL_SetRenderTarget(renderer, nullptr);
        }
    }
    
    valid_ = true;
    DebugLogger::info("Texture cache updated successfully");
}

void TextureCache::cleanup() {
    if (bannerTexture_) {
        SDL_DestroyTexture(bannerTexture_);
        bannerTexture_ = nullptr;
    }
    if (statsBoxTexture_) {
        SDL_DestroyTexture(statsBoxTexture_);
        statsBoxTexture_ = nullptr;
    }
    if (hudPanelTexture_) {
        SDL_DestroyTexture(hudPanelTexture_);
        hudPanelTexture_ = nullptr;
    }
    if (nextBoxTexture_) {
        SDL_DestroyTexture(nextBoxTexture_);
        nextBoxTexture_ = nullptr;
    }
    valid_ = false;
}

