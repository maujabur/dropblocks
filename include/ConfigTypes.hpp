#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>

struct RGB { unsigned char r = 0, g = 0, b = 0; };

// Scale mode for virtual layout
enum class ScaleMode {
    AUTO,      // Uniform scale, maintain aspect ratio with black bars
    STRETCH,   // Independent X/Y scale, fills screen (may distort)
    NATIVE     // 1:1 scale, no transformation, render from (0,0)
};

// Layout configuration for a UI element (in virtual coordinates)
struct ElementLayout {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    RGB backgroundColor{0, 0, 0};
    RGB outlineColor{0, 0, 0};
    RGB textColor{255, 255, 255};
    unsigned char backgroundAlpha = 255;
    unsigned char outlineAlpha = 255;
    bool enabled = true;
};

// Global layout configuration
struct LayoutConfig {
    int virtualWidth = 1620;
    int virtualHeight = 1080;
    ScaleMode scaleMode = ScaleMode::AUTO;
    int borderRadius = 10;
    int borderThickness = 2;
    int offsetX = 0;  // Used only in NATIVE mode
    int offsetY = 0;  // Used only in NATIVE mode
    
    // UI elements (in virtual coordinates) - with default positions
    ElementLayout banner{0, 0, 150, 1080, {0,40,0}, {0,60,0}, {120,255,120}, 255, 180, true};
    ElementLayout stats{180, 0, 350, 1080, {18,18,26}, {80,80,110}, {220,220,220}, 255, 160, true};
    ElementLayout board{560, 0, 540, 1080, {0,0,0}, {0,0,0}, {255,255,255}, 255, 255, true};  // Container for the game board
    ElementLayout hud{1130, 0, 490, 1080, {24,24,32}, {90,90,120}, {200,200,220}, 255, 200, true};
    ElementLayout next{1245, 550, 260, 300, {18,18,26}, {80,80,110}, {220,220,220}, 255, 160, true};
    ElementLayout score{1200, 50, 350, 450, {18,18,26}, {80,80,110}, {220,220,220}, 255, 160, true};
};

struct VisualConfig {
    struct Colors {
        RGB background{8,8,12};
        RGB boardEmpty{28,28,36};
        RGB panelFill{24,24,32};
        RGB panelOutline{90,90,120};
        unsigned char panelOutlineAlpha = 200;

        // Banner
        RGB bannerBg{0,40,0};
        RGB bannerOutline{0,60,0};
        unsigned char bannerOutlineAlpha = 180;
        RGB bannerText{120,255,120};

        // HUD
        RGB hudLabel{200,200,220};
        RGB hudScore{255,240,120};
        RGB hudLines{180,255,180};
        RGB hudLevel{180,200,255};

        // SCORE
        RGB scoreFill{18,18,26};
        RGB scoreOutline{80,80,110};
        unsigned char scoreOutlineAlpha = 160;

        // NEXT
        RGB nextFill{18,18,26};
        RGB nextOutline{80,80,110};
        unsigned char nextOutlineAlpha = 160;
        RGB nextLabel{220,220,220};
        RGB nextGridDark{24,24,24};
        RGB nextGridLight{30,30,30};
        bool nextGridUseRgb = false;

        // Overlay
        RGB overlayFill{0,0,0};
        unsigned char overlayFillAlpha = 200;
        RGB overlayOutline{200,200,220};
        unsigned char overlayOutlineAlpha = 120;
        RGB overlayTop{255,160,160};
        RGB overlaySub{220,220,220};

        // Statistics
        RGB statsFill{18,18,26};
        RGB statsOutline{80,80,110};
        unsigned char statsOutlineAlpha = 160;
        RGB statsLabel{200,200,220};
        RGB statsCount{255,255,180};
    } colors;

    struct Effects {
        bool bannerSweep = true;
        bool globalSweep = true;
        float sweepSpeedPxps = 15.0f;
        int sweepBandHS = 30;
        int sweepAlphaMax = 100;
        float sweepSoftness = 0.7f;
        float sweepGSpeedPxps = 20.0f;
        int sweepGBandHPx = 100;
        int sweepGAlphaMax = 50;
        float sweepGSoftness = 0.9f;
        int scanlineAlpha = 20;
    } effects;

    struct Layout {
        int roundedPanels = 1;
        int hudFixedScale = 6;
    } layout;

    std::string titleText = "__H A C K T R I S";
};

struct AudioConfig {
    float masterVolume = 1.0f;
    float sfxVolume = 0.75f;
    float ambientVolume = 0.2f;
    bool enableMovementSounds = true;
    bool enableAmbientSounds = true;
    bool enableComboSounds = true;
    bool enableLevelUpSounds = true;

    bool loadFromConfig(const std::string& key, const std::string& value) {
        if (key == "MASTER_VOLUME") { masterVolume = std::clamp((float)std::atof(value.c_str()), 0.0f, 1.0f); return true; }
        if (key == "SFX_VOLUME") { sfxVolume = std::clamp((float)std::atof(value.c_str()), 0.0f, 1.0f); return true; }
        if (key == "AMBIENT_VOLUME") { ambientVolume = std::clamp((float)std::atof(value.c_str()), 0.0f, 1.0f); return true; }
        if (key == "ENABLE_MOVEMENT_SOUNDS") { enableMovementSounds = (value == "1" || value == "true"); return true; }
        if (key == "ENABLE_AMBIENT_SOUNDS") { enableAmbientSounds = (value == "1" || value == "true"); return true; }
        if (key == "ENABLE_COMBO_SOUNDS") { enableComboSounds = (value == "1" || value == "true"); return true; }
        if (key == "ENABLE_LEVEL_UP_SOUNDS") { enableLevelUpSounds = (value == "1" || value == "true"); return true; }
        return false;
    }
};

struct InputConfig {
    int buttonLeft = 13, buttonRight = 11, buttonDown = 14, buttonUp = 12;
    int buttonRotateCCW = 0, buttonRotateCW = 1, buttonSoftDrop = 2, buttonHardDrop = 3;
    int buttonPause = 6, buttonStart = 7, buttonQuit = 8;
    float analogDeadzone = 0.3f; float analogSensitivity = 1.0f; bool invertYAxis = false;
    unsigned int moveRepeatDelay = 200; // ms
    unsigned int softDropRepeatDelay = 100; // ms
};

struct PiecesConfig {
    std::string piecesFilePath = "";
    int previewGrid = 6;
    std::string randomizerType = "simple";
    int randBagSize = 0;
    std::vector<RGB> pieceColors;
};

struct GameConfig { int tickMsStart = 400; int tickMsMin = 80; int speedAcceleration = 50; int levelStep = 10; };


