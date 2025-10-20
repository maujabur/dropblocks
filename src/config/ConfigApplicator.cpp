#include "config/ConfigApplicator.hpp"
#include "audio/AudioSystem.hpp"
#include "ThemeManager.hpp"
#include "app/GameState.hpp"
#include "input/InputManager.hpp"
#include "input/JoystickInput.hpp"
#include "pieces/Piece.hpp"
#include "render/GameStateBridge.hpp"

// External globals from dropblocks.cpp
extern int ROUNDED_PANELS;
extern int HUD_FIXED_SCALE;
extern int GAP1_SCALE;
extern int GAP2_SCALE;
extern std::string TITLE_TEXT;
extern int SPEED_ACCELERATION;
extern int LEVEL_STEP;
extern GameConfig gameConfig;
extern LayoutConfig layoutConfig;

namespace ConfigApplicator {

void applyThemePieceColors(ThemeManager& themeManager, std::vector<Piece>& pieces) {
    themeManager.applyPieceColors(pieces);
}

void applyConfigToAudio(AudioSystem& audio, const AudioConfig& config) {
    audio.masterVolume = config.masterVolume;
    audio.sfxVolume = config.sfxVolume;
    audio.ambientVolume = config.ambientVolume;
    audio.enableMovementSounds = config.enableMovementSounds;
    audio.enableAmbientSounds = config.enableAmbientSounds;
    audio.enableComboSounds = config.enableComboSounds;
    audio.enableLevelUpSounds = config.enableLevelUpSounds;
}

void applyConfigToTheme(const VisualConfig& config, ThemeManager& themeManager, VisualEffectsView& visualView) {
    // Apply colors
    themeManager.getTheme().bg_r = config.colors.background.r;
    themeManager.getTheme().bg_g = config.colors.background.g;
    themeManager.getTheme().bg_b = config.colors.background.b;
    
    themeManager.getTheme().board_empty_r = config.colors.boardEmpty.r;
    themeManager.getTheme().board_empty_g = config.colors.boardEmpty.g;
    themeManager.getTheme().board_empty_b = config.colors.boardEmpty.b;
    
    themeManager.getTheme().panel_fill_r = config.colors.panelFill.r;
    themeManager.getTheme().panel_fill_g = config.colors.panelFill.g;
    themeManager.getTheme().panel_fill_b = config.colors.panelFill.b;
    
    themeManager.getTheme().panel_outline_r = config.colors.panelOutline.r;
    themeManager.getTheme().panel_outline_g = config.colors.panelOutline.g;
    themeManager.getTheme().panel_outline_b = config.colors.panelOutline.b;
    themeManager.getTheme().panel_outline_a = config.colors.panelOutlineAlpha;
    
    // Banner
    themeManager.getTheme().banner_bg_r = config.colors.bannerBg.r;
    themeManager.getTheme().banner_bg_g = config.colors.bannerBg.g;
    themeManager.getTheme().banner_bg_b = config.colors.bannerBg.b;
    
    themeManager.getTheme().banner_outline_r = config.colors.bannerOutline.r;
    themeManager.getTheme().banner_outline_g = config.colors.bannerOutline.g;
    themeManager.getTheme().banner_outline_b = config.colors.bannerOutline.b;
    themeManager.getTheme().banner_outline_a = config.colors.bannerOutlineAlpha;
    
    themeManager.getTheme().banner_text_r = config.colors.bannerText.r;
    themeManager.getTheme().banner_text_g = config.colors.bannerText.g;
    themeManager.getTheme().banner_text_b = config.colors.bannerText.b;
    
    // HUD
    themeManager.getTheme().hud_label_r = config.colors.hudLabel.r;
    themeManager.getTheme().hud_label_g = config.colors.hudLabel.g;
    themeManager.getTheme().hud_label_b = config.colors.hudLabel.b;
    
    themeManager.getTheme().hud_score_r = config.colors.hudScore.r;
    themeManager.getTheme().hud_score_g = config.colors.hudScore.g;
    themeManager.getTheme().hud_score_b = config.colors.hudScore.b;
    
    themeManager.getTheme().hud_lines_r = config.colors.hudLines.r;
    themeManager.getTheme().hud_lines_g = config.colors.hudLines.g;
    themeManager.getTheme().hud_lines_b = config.colors.hudLines.b;
    
    themeManager.getTheme().hud_level_r = config.colors.hudLevel.r;
    themeManager.getTheme().hud_level_g = config.colors.hudLevel.g;
    themeManager.getTheme().hud_level_b = config.colors.hudLevel.b;
    
    // NEXT
    themeManager.getTheme().next_fill_r = config.colors.nextFill.r;
    themeManager.getTheme().next_fill_g = config.colors.nextFill.g;
    themeManager.getTheme().next_fill_b = config.colors.nextFill.b;
    
    themeManager.getTheme().next_outline_r = config.colors.nextOutline.r;
    themeManager.getTheme().next_outline_g = config.colors.nextOutline.g;
    themeManager.getTheme().next_outline_b = config.colors.nextOutline.b;
    themeManager.getTheme().next_outline_a = config.colors.nextOutlineAlpha;
    
    themeManager.getTheme().next_label_r = config.colors.nextLabel.r;
    themeManager.getTheme().next_label_g = config.colors.nextLabel.g;
    themeManager.getTheme().next_label_b = config.colors.nextLabel.b;
    
    themeManager.getTheme().next_grid_dark_r = config.colors.nextGridDark.r;
    themeManager.getTheme().next_grid_dark_g = config.colors.nextGridDark.g;
    themeManager.getTheme().next_grid_dark_b = config.colors.nextGridDark.b;
    
    themeManager.getTheme().next_grid_light_r = config.colors.nextGridLight.r;
    themeManager.getTheme().next_grid_light_g = config.colors.nextGridLight.g;
    themeManager.getTheme().next_grid_light_b = config.colors.nextGridLight.b;
    themeManager.getTheme().next_grid_use_rgb = config.colors.nextGridUseRgb;
    
    // Overlay
    themeManager.getTheme().overlay_fill_r = config.colors.overlayFill.r;
    themeManager.getTheme().overlay_fill_g = config.colors.overlayFill.g;
    themeManager.getTheme().overlay_fill_b = config.colors.overlayFill.b;
    themeManager.getTheme().overlay_fill_a = config.colors.overlayFillAlpha;
    
    themeManager.getTheme().overlay_outline_r = config.colors.overlayOutline.r;
    themeManager.getTheme().overlay_outline_g = config.colors.overlayOutline.g;
    themeManager.getTheme().overlay_outline_b = config.colors.overlayOutline.b;
    themeManager.getTheme().overlay_outline_a = config.colors.overlayOutlineAlpha;
    
    themeManager.getTheme().overlay_top_r = config.colors.overlayTop.r;
    themeManager.getTheme().overlay_top_g = config.colors.overlayTop.g;
    themeManager.getTheme().overlay_top_b = config.colors.overlayTop.b;
    
    themeManager.getTheme().overlay_sub_r = config.colors.overlaySub.r;
    themeManager.getTheme().overlay_sub_g = config.colors.overlaySub.g;
    themeManager.getTheme().overlay_sub_b = config.colors.overlaySub.b;
    
    // Apply effects
    // Visual effects now flow via g_visualView and bridge only
    visualView.bannerSweep = config.effects.bannerSweep;
    visualView.globalSweep = config.effects.globalSweep;
    visualView.sweepSpeedPxps = config.effects.sweepSpeedPxps;
    visualView.sweepBandHS = config.effects.sweepBandHS;
    visualView.sweepAlphaMax = config.effects.sweepAlphaMax;
    visualView.sweepSoftness = config.effects.sweepSoftness;
    visualView.sweepGSpeedPxps = config.effects.sweepGSpeedPxps;
    visualView.sweepGBandHPx = config.effects.sweepGBandHPx;
    visualView.sweepGAlphaMax = config.effects.sweepGAlphaMax;
    visualView.sweepGSoftness = config.effects.sweepGSoftness;
    visualView.scanlineAlpha = config.effects.scanlineAlpha;
    
    // Apply layout
    ROUNDED_PANELS = config.layout.roundedPanels;
    HUD_FIXED_SCALE = config.layout.hudFixedScale;
    GAP1_SCALE = config.layout.gap1Scale;
    GAP2_SCALE = config.layout.gap2Scale;
    
    // Apply text
    TITLE_TEXT = config.titleText;
}

void applyConfigToGame(GameState& state, const GameConfig& config) {
    gameConfig.tickMsStart = config.tickMsStart;
    gameConfig.tickMsMin = config.tickMsMin;
    SPEED_ACCELERATION = config.speedAcceleration;
    LEVEL_STEP = config.levelStep;
    
    // Apply configuration to the new modular systems
    state.getScore().setTickMs(config.tickMsStart);
}

void applyConfigToPieces(const PiecesConfig& config, ThemeManager& themeManager) {
    // Apply piece colors
    if (!config.pieceColors.empty()) {
        themeManager.getTheme().piece_colors.clear();
        for (const auto& color : config.pieceColors) {
            themeManager.getTheme().piece_colors.push_back({color.r, color.g, color.b});
        }
    }
}

void applyConfigToJoystick(InputManager& inputManager, const InputConfig& config) {
    // Find JoystickInput handler in InputManager
    for (auto& handler : inputManager.getHandlers()) {
        if (auto* joystickInput = dynamic_cast<JoystickInput*>(handler.get())) {
            JoystickConfig& joystickConfig = joystickInput->getConfig();
            
            // Apply button mappings
            joystickConfig.buttonLeft = config.buttonLeft;
            joystickConfig.buttonRight = config.buttonRight;
            joystickConfig.buttonDown = config.buttonDown;
            joystickConfig.buttonUp = config.buttonUp;
            joystickConfig.buttonRotateCCW = config.buttonRotateCCW;
            joystickConfig.buttonRotateCW = config.buttonRotateCW;
            joystickConfig.buttonSoftDrop = config.buttonSoftDrop;
            joystickConfig.buttonHardDrop = config.buttonHardDrop;
            joystickConfig.buttonPause = config.buttonPause;
            joystickConfig.buttonStart = config.buttonStart;
            joystickConfig.buttonQuit = config.buttonQuit;
            
            // Apply analog settings
            joystickConfig.analogDeadzone = config.analogDeadzone;
            joystickConfig.analogSensitivity = config.analogSensitivity;
            joystickConfig.invertYAxis = config.invertYAxis;
            
            // Apply timing settings
            joystickConfig.moveRepeatDelay = config.moveRepeatDelay;
            joystickConfig.softDropRepeatDelay = config.softDropRepeatDelay;
            
            break; // Only configure the first joystick found
        }
    }
}

void applyConfigToLayout(const LayoutConfig& config) {
    // Copy layout configuration to global layoutConfig
    layoutConfig = config;
}

} // namespace ConfigApplicator

