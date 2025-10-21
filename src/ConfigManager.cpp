#include <fstream>
#include <cctype>
#include <SDL2/SDL.h>

#include "DebugLogger.hpp"
#include "ConfigManager.hpp"
#include "ConfigTypes.hpp"

// Local parser interfaces and implementations (extracted from dropblocks.cpp)
class ConfigParser {
public:
    virtual ~ConfigParser() = default;
    virtual bool parse(const std::string& key, const std::string& value) = 0;
    virtual std::string getCategory() const = 0;
    virtual bool validate() const = 0;
};

class VisualConfigParser : public ConfigParser {
private:
    VisualConfig& config_;
    bool parseBool(const std::string& value) const {
        std::string v = value; for (char& c : v) c = (char)std::tolower((unsigned char)c);
        return (v == "1" || v == "true" || v == "on" || v == "yes");
    }
    int parseInt(const std::string& value) const { return std::atoi(value.c_str()); }
    float parseFloat(const std::string& value) const { return (float)std::atof(value.c_str()); }
    bool parseHexColor(const std::string& value, RGB& color) const {
        std::string hex = value; if (hex.size() && hex[0] == '#') hex = hex.substr(1);
        if (hex.length() != 6) return false; 
        try {
            color.r = (Uint8)std::stoi(hex.substr(0,2), nullptr, 16);
            color.g = (Uint8)std::stoi(hex.substr(2,2), nullptr, 16);
            color.b = (Uint8)std::stoi(hex.substr(4,2), nullptr, 16);
            return true;
        } catch (...) { 
            return false; 
        }
    }
public:
    VisualConfigParser(VisualConfig& config) : config_(config) {}
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "visual"; }
    bool validate() const override;
};

class AudioConfigParser : public ConfigParser {
private:
    AudioConfig* config_;
    bool parseBool(const std::string& value) const {
        std::string v = value; for (char& c : v) c = (char)std::tolower((unsigned char)c);
        return (v == "1" || v == "true" || v == "on" || v == "yes");
    }
    float parseFloat(const std::string& value) const {
        float v = (float)std::atof(value.c_str()); if (v < 0.0f) v = 0.0f; if (v > 1.0f) v = 1.0f; return v;
    }
public:
    AudioConfigParser(AudioConfig* config) : config_(config) {}
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "audio"; }
    bool validate() const override { return config_ && (config_->masterVolume >= 0.0f && config_->masterVolume <= 1.0f) && (config_->sfxVolume >= 0.0f && config_->sfxVolume <= 1.0f) && (config_->ambientVolume >= 0.0f && config_->ambientVolume <= 1.0f); }
};

class InputConfigParser : public ConfigParser {
private:
    InputConfig& config_;
    bool parseBool(const std::string& value) const { std::string v = value; for (char& c : v) c = (char)std::tolower((unsigned char)c); return (v == "1" || v == "true" || v == "on" || v == "yes"); }
    int parseInt(const std::string& value) const { return std::atoi(value.c_str()); }
    float parseFloat(const std::string& value) const { return (float)std::atof(value.c_str()); }
public:
    InputConfigParser(InputConfig& config) : config_(config) {}
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "input"; }
    bool validate() const override;
};

class PiecesConfigParser : public ConfigParser {
private:
    PiecesConfig& config_;
    int parseInt(const std::string& value) const { return std::atoi(value.c_str()); }
    bool parseHexColor(const std::string& value, RGB& color) const {
        std::string hex = value; if (hex.size() && hex[0] == '#') hex = hex.substr(1); if (hex.length() != 6) return false;
        try { color.r = (Uint8)std::stoi(hex.substr(0,2), nullptr, 16); color.g = (Uint8)std::stoi(hex.substr(2,2), nullptr, 16); color.b = (Uint8)std::stoi(hex.substr(4,2), nullptr, 16); return true; } catch (...) { return false; }
    }
public:
    PiecesConfigParser(PiecesConfig& config) : config_(config) {}
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "pieces"; }
    bool validate() const override { return (config_.previewGrid >= 4 && config_.previewGrid <= 12) && (config_.randBagSize >= 0 && config_.randBagSize <= 20) && (config_.randomizerType == "simple" || config_.randomizerType == "bag"); }
};

class GameConfigParser : public ConfigParser {
private:
    GameConfig& config_;
    int parseInt(const std::string& value) const { return std::atoi(value.c_str()); }
    float parseFloat(const std::string& value) const { return (float)std::atof(value.c_str()); }
public:
    GameConfigParser(GameConfig& config) : config_(config) {}
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "game"; }
    bool validate() const override { return (config_.tickMsStart > 0) && (config_.tickMsMin > 0) && (config_.speedAcceleration > 0) && (config_.levelStep > 0); }
};

class LayoutConfigParser : public ConfigParser {
private:
    LayoutConfig& config_;
    int parseInt(const std::string& value) const { return std::atoi(value.c_str()); }
    bool parseHexColor(const std::string& value, RGB& color) const {
        std::string hex = value; if (hex.size() && hex[0] == '#') hex = hex.substr(1);
        if (hex.length() != 6) return false;
        try {
            color.r = (Uint8)std::stoi(hex.substr(0,2), nullptr, 16);
            color.g = (Uint8)std::stoi(hex.substr(2,2), nullptr, 16);
            color.b = (Uint8)std::stoi(hex.substr(4,2), nullptr, 16);
            return true;
        } catch (...) { return false; }
    }
    ScaleMode parseScaleMode(const std::string& value) const {
        std::string v = value;
        for (char& c : v) c = (char)std::toupper((unsigned char)c);
        if (v == "STRETCH") return ScaleMode::STRETCH;
        if (v == "NATIVE") return ScaleMode::NATIVE;
        return ScaleMode::AUTO;
    }
public:
    LayoutConfigParser(LayoutConfig& config) : config_(config) {}
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "layout"; }
    bool validate() const override { return (config_.virtualWidth > 0) && (config_.virtualHeight > 0); }
};

// ---- VisualConfigParser impl ----
bool VisualConfigParser::parse(const std::string& key, const std::string& value) {
    if (key == "BACKGROUND") return parseHexColor(value, config_.colors.background);
    if (key == "BOARD_EMPTY") return parseHexColor(value, config_.colors.boardEmpty);
    if (key == "PANEL_FILL") return parseHexColor(value, config_.colors.panelFill);
    if (key == "PANEL_OUTLINE") return parseHexColor(value, config_.colors.panelOutline);
    if (key == "PANEL_OUTLINE_A") { config_.colors.panelOutlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "BANNER_BG") return parseHexColor(value, config_.colors.bannerBg);
    if (key == "BANNER_OUTLINE") return parseHexColor(value, config_.colors.bannerOutline);
    if (key == "BANNER_OUTLINE_A") { config_.colors.bannerOutlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "BANNER_TEXT") return parseHexColor(value, config_.colors.bannerText);
    if (key == "HUD_LABEL") return parseHexColor(value, config_.colors.hudLabel);
    if (key == "HUD_SCORE") return parseHexColor(value, config_.colors.hudScore);
    if (key == "HUD_LINES") return parseHexColor(value, config_.colors.hudLines);
    if (key == "HUD_LEVEL") return parseHexColor(value, config_.colors.hudLevel);
    
    // SCORE
    if (key == "SCORE_FILL") return parseHexColor(value, config_.colors.scoreFill);
    if (key == "SCORE_OUTLINE") return parseHexColor(value, config_.colors.scoreOutline);
    if (key == "SCORE_OUTLINE_A") { config_.colors.scoreOutlineAlpha = (unsigned char)parseInt(value); return true; }
    
    if (key == "NEXT_FILL") return parseHexColor(value, config_.colors.nextFill);
    if (key == "NEXT_OUTLINE") return parseHexColor(value, config_.colors.nextOutline);
    if (key == "NEXT_OUTLINE_A") { config_.colors.nextOutlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "NEXT_LABEL") return parseHexColor(value, config_.colors.nextLabel);
    if (key == "NEXT_GRID_DARK") return parseHexColor(value, config_.colors.nextGridDark);
    if (key == "NEXT_GRID_LIGHT") return parseHexColor(value, config_.colors.nextGridLight);
    if (key == "NEXT_GRID_USE_RGB") { config_.colors.nextGridUseRgb = parseBool(value); return true; }
    if (key == "OVERLAY_FILL") return parseHexColor(value, config_.colors.overlayFill);
    if (key == "OVERLAY_FILL_A") { config_.colors.overlayFillAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "OVERLAY_OUTLINE") return parseHexColor(value, config_.colors.overlayOutline);
    if (key == "OVERLAY_OUTLINE_A") { config_.colors.overlayOutlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "OVERLAY_TOP") return parseHexColor(value, config_.colors.overlayTop);
    if (key == "OVERLAY_SUB") return parseHexColor(value, config_.colors.overlaySub);
    
    // Statistics
    if (key == "STATS_FILL") return parseHexColor(value, config_.colors.statsFill);
    if (key == "STATS_OUTLINE") return parseHexColor(value, config_.colors.statsOutline);
    if (key == "STATS_OUTLINE_A") { config_.colors.statsOutlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "STATS_LABEL") return parseHexColor(value, config_.colors.statsLabel);
    if (key == "STATS_COUNT") return parseHexColor(value, config_.colors.statsCount);
    
    if (key == "ENABLE_BANNER_SWEEP") { config_.effects.bannerSweep = parseBool(value); return true; }
    if (key == "ENABLE_GLOBAL_SWEEP") { config_.effects.globalSweep = parseBool(value); return true; }
    if (key == "SWEEP_SPEED_PXPS") { config_.effects.sweepSpeedPxps = parseFloat(value); return true; }
    if (key == "SWEEP_BAND_H_S") { config_.effects.sweepBandHS = parseInt(value); return true; }
    if (key == "SWEEP_ALPHA_MAX") { config_.effects.sweepAlphaMax = parseInt(value); return true; }
    if (key == "SWEEP_SOFTNESS") { config_.effects.sweepSoftness = parseFloat(value); return true; }
    if (key == "SWEEP_G_SPEED_PXPS") { config_.effects.sweepGSpeedPxps = parseFloat(value); return true; }
    if (key == "SWEEP_G_BAND_H_PX") { config_.effects.sweepGBandHPx = parseInt(value); return true; }
    if (key == "SWEEP_G_ALPHA_MAX") { config_.effects.sweepGAlphaMax = parseInt(value); return true; }
    if (key == "SWEEP_G_SOFTNESS") { config_.effects.sweepGSoftness = parseFloat(value); return true; }
    if (key == "SCANLINE_ALPHA") { config_.effects.scanlineAlpha = parseInt(value); return true; }
    if (key == "ROUNDED_PANELS") { config_.layout.roundedPanels = parseInt(value); return true; }
    if (key == "HUD_FIXED_SCALE") { config_.layout.hudFixedScale = parseInt(value); return true; }
    if (key == "TITLE_TEXT") { config_.titleText = value; return true; }
    return false;
}

bool VisualConfigParser::validate() const {
    if (config_.effects.sweepAlphaMax < 0 || config_.effects.sweepAlphaMax > 255) return false;
    if (config_.effects.sweepGAlphaMax < 0 || config_.effects.sweepGAlphaMax > 255) return false;
    if (config_.effects.scanlineAlpha < 0 || config_.effects.scanlineAlpha > 255) return false;
    if (config_.effects.sweepSoftness < 0.0f || config_.effects.sweepSoftness > 1.0f) return false;
    if (config_.effects.sweepGSoftness < 0.0f || config_.effects.sweepGSoftness > 1.0f) return false;
    if (config_.layout.roundedPanels < 0) return false; if (config_.layout.hudFixedScale < 1) return false;
    return true;
}

// ---- AudioConfigParser impl ----
bool AudioConfigParser::parse(const std::string& key, const std::string& value) {
    if (!config_) return false;
    if (key == "AUDIO_MASTER_VOLUME") { config_->masterVolume = parseFloat(value); return true; }
    if (key == "AUDIO_SFX_VOLUME") { config_->sfxVolume = parseFloat(value); return true; }
    if (key == "AUDIO_AMBIENT_VOLUME") { config_->ambientVolume = parseFloat(value); return true; }
    if (key == "ENABLE_MOVEMENT_SOUNDS") { config_->enableMovementSounds = parseBool(value); return true; }
    if (key == "ENABLE_AMBIENT_SOUNDS") { config_->enableAmbientSounds = parseBool(value); return true; }
    if (key == "ENABLE_COMBO_SOUNDS") { config_->enableComboSounds = parseBool(value); return true; }
    if (key == "ENABLE_LEVEL_UP_SOUNDS") { config_->enableLevelUpSounds = parseBool(value); return true; }
    return false;
}

// ---- InputConfigParser impl ----
bool InputConfigParser::parse(const std::string& key, const std::string& value) {
    if (key == "JOYSTICK_BUTTON_LEFT") { config_.buttonLeft = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_RIGHT") { config_.buttonRight = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_DOWN") { config_.buttonDown = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_UP") { config_.buttonUp = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_ROTATE_CCW") { config_.buttonRotateCCW = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_ROTATE_CW") { config_.buttonRotateCW = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_SOFT_DROP") { config_.buttonSoftDrop = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_HARD_DROP") { config_.buttonHardDrop = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_PAUSE") { config_.buttonPause = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_START") { config_.buttonStart = parseInt(value); return true; }
    if (key == "JOYSTICK_BUTTON_QUIT") { config_.buttonQuit = parseInt(value); return true; }
    if (key == "JOYSTICK_ANALOG_DEADZONE") { config_.analogDeadzone = parseFloat(value); return true; }
    if (key == "JOYSTICK_ANALOG_SENSITIVITY") { config_.analogSensitivity = parseFloat(value); return true; }
    if (key == "JOYSTICK_INVERT_Y_AXIS") { config_.invertYAxis = parseBool(value); return true; }
    if (key == "JOYSTICK_MOVE_REPEAT_DELAY_DAS") { config_.moveRepeatDelayDAS = (unsigned int)parseInt(value); return true; }
    if (key == "JOYSTICK_MOVE_REPEAT_DELAY_ARR") { config_.moveRepeatDelayARR = (unsigned int)parseInt(value); return true; }
    // Legacy support
    if (key == "JOYSTICK_MOVE_REPEAT_DELAY") { config_.moveRepeatDelayDAS = (unsigned int)parseInt(value); return true; }
    if (key == "JOYSTICK_SOFT_DROP_REPEAT_DELAY" || key == "JOYSTICK_SOFT_DROP_DELAY") { config_.softDropRepeatDelay = (unsigned int)parseInt(value); return true; }
    return false;
}

bool InputConfigParser::validate() const {
    return (config_.analogDeadzone >= 0.0f && config_.analogDeadzone <= 1.0f) && (config_.analogSensitivity >= 0.0f && config_.analogSensitivity <= 2.0f) && (config_.buttonLeft >= 0 && config_.buttonLeft < 32) && (config_.buttonRight >= 0 && config_.buttonRight < 32) && (config_.buttonDown >= 0 && config_.buttonDown < 32) && (config_.buttonUp >= 0 && config_.buttonUp < 32) && (config_.buttonRotateCCW >= 0 && config_.buttonRotateCCW < 32) && (config_.buttonRotateCW >= 0 && config_.buttonRotateCW < 32) && (config_.buttonSoftDrop >= 0 && config_.buttonSoftDrop < 32) && (config_.buttonHardDrop >= 0 && config_.buttonHardDrop < 32) && (config_.buttonPause >= 0 && config_.buttonPause < 32) && (config_.buttonStart >= 0 && config_.buttonStart < 32) && (config_.buttonQuit >= 0 && config_.buttonQuit < 32);
}

// ---- PiecesConfigParser impl ----
bool PiecesConfigParser::parse(const std::string& key, const std::string& value) {
    if (key == "PIECES_FILE") { config_.piecesFilePath = value; return true; }
    if (key == "PREVIEW_GRID") { config_.previewGrid = parseInt(value); return true; }
    if (key == "RAND_TYPE") { config_.randomizerType = value; return true; }
    if (key == "RAND_BAG_SIZE") { config_.randBagSize = parseInt(value); return true; }
    if (key.rfind("PIECE", 0) == 0) {
        std::string numStr = key.substr(5); int pieceIndex = -1; try { pieceIndex = std::stoi(numStr); } catch (...) { return false; }
        RGB color; if (parseHexColor(value, color)) { if (pieceIndex >= (int)config_.pieceColors.size()) { config_.pieceColors.resize(pieceIndex + 1, RGB{200,200,200}); } config_.pieceColors[pieceIndex] = color; return true; }
    }
    return false;
}

// ---- GameConfigParser impl ----
bool GameConfigParser::parse(const std::string& key, const std::string& value) {
    if (key == "TICK_MS_START" || key == "GAME_SPEED_START_MS") { config_.tickMsStart = parseInt(value); return true; }
    if (key == "TICK_MS_MIN" || key == "GAME_SPEED_MIN_MS") { config_.tickMsMin = parseInt(value); return true; }
    if (key == "SPEED_ACCELERATION" || key == "GAME_SPEED_ACCELERATION") { config_.speedAcceleration = parseInt(value); return true; }
    if (key == "LEVEL_STEP") { config_.levelStep = parseInt(value); return true; }
    return false;
}

// ---- LayoutConfigParser impl ----
bool LayoutConfigParser::parse(const std::string& key, const std::string& value) {
    // Global layout settings
    if (key == "LAYOUT_VIRTUAL_WIDTH") { config_.virtualWidth = parseInt(value); return true; }
    if (key == "LAYOUT_VIRTUAL_HEIGHT") { config_.virtualHeight = parseInt(value); return true; }
    if (key == "LAYOUT_SCALE_MODE") { config_.scaleMode = parseScaleMode(value); return true; }
    if (key == "LAYOUT_OFFSET_X") { config_.offsetX = parseInt(value); return true; }
    if (key == "LAYOUT_OFFSET_Y") { config_.offsetY = parseInt(value); return true; }
    if (key == "PANEL_BORDER_RADIUS") { config_.borderRadius = parseInt(value); return true; }
    if (key == "PANEL_BORDER_THICKNESS") { config_.borderThickness = parseInt(value); return true; }
    
    // Banner element
    if (key == "BANNER_X") { config_.banner.x = parseInt(value); return true; }
    if (key == "BANNER_Y") { config_.banner.y = parseInt(value); return true; }
    if (key == "BANNER_WIDTH") { config_.banner.width = parseInt(value); return true; }
    if (key == "BANNER_HEIGHT") { config_.banner.height = parseInt(value); return true; }
    if (key == "BANNER_BG_COLOR") { return parseHexColor(value, config_.banner.backgroundColor); }
    if (key == "BANNER_OUTLINE_COLOR") { return parseHexColor(value, config_.banner.outlineColor); }
    if (key == "BANNER_TEXT_COLOR") { return parseHexColor(value, config_.banner.textColor); }
    if (key == "BANNER_BG_ALPHA") { config_.banner.backgroundAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "BANNER_OUTLINE_ALPHA") { config_.banner.outlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "BANNER_ENABLED") { config_.banner.enabled = (parseInt(value) != 0); return true; }
    
    // Stats element
    if (key == "STATS_X") { config_.stats.x = parseInt(value); return true; }
    if (key == "STATS_Y") { config_.stats.y = parseInt(value); return true; }
    if (key == "STATS_WIDTH") { config_.stats.width = parseInt(value); return true; }
    if (key == "STATS_HEIGHT") { config_.stats.height = parseInt(value); return true; }
    if (key == "STATS_BG_COLOR") { return parseHexColor(value, config_.stats.backgroundColor); }
    if (key == "STATS_OUTLINE_COLOR") { return parseHexColor(value, config_.stats.outlineColor); }
    if (key == "STATS_TEXT_COLOR") { return parseHexColor(value, config_.stats.textColor); }
    if (key == "STATS_BG_ALPHA") { config_.stats.backgroundAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "STATS_OUTLINE_ALPHA") { config_.stats.outlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "STATS_ENABLED") { config_.stats.enabled = (parseInt(value) != 0); return true; }
    
    // Board element
    if (key == "BOARD_X") { config_.board.x = parseInt(value); return true; }
    if (key == "BOARD_Y") { config_.board.y = parseInt(value); return true; }
    if (key == "BOARD_WIDTH") { config_.board.width = parseInt(value); return true; }
    if (key == "BOARD_HEIGHT") { config_.board.height = parseInt(value); return true; }
    if (key == "BOARD_ENABLED") { config_.board.enabled = (parseInt(value) != 0); return true; }
    
    // HUD element
    if (key == "HUD_X") { config_.hud.x = parseInt(value); return true; }
    if (key == "HUD_Y") { config_.hud.y = parseInt(value); return true; }
    if (key == "HUD_WIDTH") { config_.hud.width = parseInt(value); return true; }
    if (key == "HUD_HEIGHT") { config_.hud.height = parseInt(value); return true; }
    if (key == "HUD_BG_COLOR") { return parseHexColor(value, config_.hud.backgroundColor); }
    if (key == "HUD_OUTLINE_COLOR") { return parseHexColor(value, config_.hud.outlineColor); }
    if (key == "HUD_TEXT_COLOR") { return parseHexColor(value, config_.hud.textColor); }
    if (key == "HUD_BG_ALPHA") { config_.hud.backgroundAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "HUD_OUTLINE_ALPHA") { config_.hud.outlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "HUD_ENABLED") { config_.hud.enabled = (parseInt(value) != 0); return true; }
    
    // Next element
    if (key == "NEXT_X") { config_.next.x = parseInt(value); return true; }
    if (key == "NEXT_Y") { config_.next.y = parseInt(value); return true; }
    if (key == "NEXT_WIDTH") { config_.next.width = parseInt(value); return true; }
    if (key == "NEXT_HEIGHT") { config_.next.height = parseInt(value); return true; }
    if (key == "NEXT_BG_COLOR") { return parseHexColor(value, config_.next.backgroundColor); }
    if (key == "NEXT_OUTLINE_COLOR") { return parseHexColor(value, config_.next.outlineColor); }
    if (key == "NEXT_TEXT_COLOR") { return parseHexColor(value, config_.next.textColor); }
    if (key == "NEXT_BG_ALPHA") { config_.next.backgroundAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "NEXT_OUTLINE_ALPHA") { config_.next.outlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "NEXT_ENABLED") { config_.next.enabled = (parseInt(value) != 0); return true; }
    
    // === SCORE ===
    if (key == "SCORE_X") { config_.score.x = parseInt(value); return true; }
    if (key == "SCORE_Y") { config_.score.y = parseInt(value); return true; }
    if (key == "SCORE_WIDTH") { config_.score.width = parseInt(value); return true; }
    if (key == "SCORE_HEIGHT") { config_.score.height = parseInt(value); return true; }
    if (key == "SCORE_BG_COLOR") { return parseHexColor(value, config_.score.backgroundColor); }
    if (key == "SCORE_OUTLINE_COLOR") { return parseHexColor(value, config_.score.outlineColor); }
    if (key == "SCORE_TEXT_COLOR") { return parseHexColor(value, config_.score.textColor); }
    if (key == "SCORE_BG_ALPHA") { config_.score.backgroundAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "SCORE_OUTLINE_ALPHA") { config_.score.outlineAlpha = (unsigned char)parseInt(value); return true; }
    if (key == "SCORE_ENABLED") { config_.score.enabled = (parseInt(value) != 0); return true; }
    
    return false;
}

// ---- ConfigManager impl ----
static void trim(std::string& s) {
    if (s.empty()) return;
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) { s.clear(); return; }
    s = s.substr(start, end - start + 1);
}

bool ConfigManager::loadFromFile(const std::string& path) {
    DebugLogger::info("Loading config file: " + path);
    std::ifstream file(path);
    if (!file.good()) { DebugLogger::error("Failed to open config file: " + path); return false; }
    configPaths_.push_back(path);

    VisualConfigParser visualParser(visual_);
    AudioConfigParser audioParser(&audio_);
    InputConfigParser inputParser(input_);
    PiecesConfigParser piecesParser(pieces_);
    GameConfigParser gameParser(game_);
    LayoutConfigParser layoutParser(layout_);
    std::vector<ConfigParser*> parsers = {&visualParser, &audioParser, &inputParser, &piecesParser, &gameParser, &layoutParser};

    std::string line; while (std::getline(file, line)) {
        // Remove comments (# preceded by whitespace or at start of line)
        size_t commentPos = std::string::npos;
        for (size_t i = 0; i < line.length(); i++) {
            if (line[i] == '#') {
                // If # is at start of line or preceded by whitespace, it's a comment
                if (i == 0 || std::isspace(line[i-1])) {
                    commentPos = i;
                    break;
                }
            }
        }
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Remove everything after semicolon
        size_t semiPos = line.find(';'); 
        if (semiPos != std::string::npos) line = line.substr(0, semiPos);
        
        trim(line); if (line.empty()) continue;
        size_t eq = line.find('='); if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq); std::string value = line.substr(eq + 1);
        trim(key); trim(value); if (key.empty()) continue;
        for (char& c : key) c = (char)std::toupper((unsigned char)c);
        bool parsed = false; for (auto* parser : parsers) { if (parser->parse(key, value)) { parsed = true; break; } }
        if (!parsed) { DebugLogger::warning("Unknown config key: " + key); }
    }

    loaded_ = true; return true;
}

bool ConfigManager::loadFromEnvironment() {
    if (const char* env = std::getenv("DROPBLOCKS_CFG")) { return loadFromFile(env); }
    return false;
}

bool ConfigManager::loadFromCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) { std::string arg = argv[i]; if (arg.length() > 4 && arg.substr(arg.length() - 4) == ".cfg") { return loadFromFile(arg); } }
    return false;
}

bool ConfigManager::loadAll() {
    if (loadFromEnvironment()) return true;
    if (loadFromCommandLine(0, nullptr)) return true;
    if (loadFromFile("default.cfg")) return true;
    if (loadFromFile("dropblocks.cfg")) return true;
    if (const char* home = std::getenv("HOME")) {
        std::string p = std::string(home) + "/.config/default.cfg"; if (loadFromFile(p)) return true;
        p = std::string(home) + "/.config/dropblocks.cfg"; if (loadFromFile(p)) return true;
    }
    loaded_ = true; return true;
}

void ConfigManager::setOverride(const std::string& key, const std::string& value) { overrides_[key] = value; }
void ConfigManager::clearOverrides() { overrides_.clear(); }

bool ConfigManager::validate() const {
    VisualConfigParser visualParser(const_cast<VisualConfig&>(visual_));
    AudioConfigParser audioParser(const_cast<AudioConfig*>(&audio_));
    InputConfigParser inputParser(const_cast<InputConfig&>(input_));
    PiecesConfigParser piecesParser(const_cast<PiecesConfig&>(pieces_));
    GameConfigParser gameParser(const_cast<GameConfig&>(game_));
    LayoutConfigParser layoutParser(const_cast<LayoutConfig&>(layout_));
    return visualParser.validate() && audioParser.validate() && inputParser.validate() && piecesParser.validate() && gameParser.validate() && layoutParser.validate();
}


