#pragma once

#include <SDL2/SDL.h>
#include <memory>

// Forward declarations
struct LayoutCache;
class ThemeManager;

/**
 * @brief Manages pre-rendered textures for static UI panels
 * 
 * This cache stores textures for panels that don't change frequently
 * (banner, stats box, HUD panel, NEXT box). Pre-rendering these to
 * textures is much faster than redrawing them every frame.
 */
class TextureCache {
public:
    TextureCache() = default;
    ~TextureCache();
    
    // Disable copy, allow move
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;
    TextureCache(TextureCache&&) = default;
    TextureCache& operator=(TextureCache&&) = default;
    
    /**
     * @brief Initialize/update all cached textures
     * @param renderer SDL renderer
     * @param layout Current layout cache
     * @param themeManager Theme manager for colors
     */
    void update(SDL_Renderer* renderer, const LayoutCache& layout, ThemeManager& themeManager);
    
    /**
     * @brief Check if cache is valid
     */
    bool isValid() const { return valid_; }
    
    /**
     * @brief Invalidate cache (forces regeneration on next update)
     */
    void invalidate() { valid_ = false; }
    
    /**
     * @brief Get cached banner texture
     */
    SDL_Texture* getBannerTexture() const { return bannerTexture_; }
    
    /**
     * @brief Get cached stats box texture
     */
    SDL_Texture* getStatsBoxTexture() const { return statsBoxTexture_; }
    
    /**
     * @brief Get cached HUD panel texture
     */
    SDL_Texture* getHudPanelTexture() const { return hudPanelTexture_; }
    
    /**
     * @brief Get cached NEXT box background texture
     */
    SDL_Texture* getNextBoxTexture() const { return nextBoxTexture_; }
    
    /**
     * @brief Free all textures
     */
    void cleanup();
    
private:
    SDL_Texture* bannerTexture_ = nullptr;
    SDL_Texture* statsBoxTexture_ = nullptr;
    SDL_Texture* hudPanelTexture_ = nullptr;
    SDL_Texture* nextBoxTexture_ = nullptr;
    bool valid_ = false;
    
    // Helper to create a texture
    SDL_Texture* createTexture(SDL_Renderer* renderer, int w, int h);
};

