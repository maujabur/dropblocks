#include "config/ConfigProcessors.hpp"
#include "audio/AudioSystem.hpp"
#include "input/JoystickSystem.hpp"
#include "ThemeManager.hpp"
#include "pieces/PieceManager.hpp"
#include <cctype>
#include <algorithm>
#include <cstdlib>

// External globals from dropblocks.cpp
extern int ROUNDED_PANELS;
extern int HUD_FIXED_SCALE;
extern std::string TITLE_TEXT;
extern std::string PIECES_FILE_PATH;
extern int SPEED_ACCELERATION;
extern GameConfig gameConfig;
extern ThemeManager themeManager;

namespace ConfigProcessors {

void trim(std::string& s) {
    // Remove espaços no início
    size_t start = 0;
    while (start < s.size() && std::isspace((unsigned char)s[start])) {
        start++;
    }
    if (start > 0) {
        s.erase(0, start);
    }
    
    // Remove espaços no final
    while (!s.empty() && std::isspace((unsigned char)s.back())) {
        s.pop_back();
    }
}

std::string parseConfigLine(const std::string& line) {
    // Comentários: ; sempre; # só se vier antes do '='
    size_t eq_probe = line.find('=');
    size_t semicol = line.find(';');
    size_t hash = line.find('#');
    size_t cut = std::string::npos;
    
    if (semicol != std::string::npos) cut = semicol;
    if (hash != std::string::npos && (eq_probe == std::string::npos || hash < eq_probe))
        cut = (cut == std::string::npos ? hash : std::min(cut, hash));
    if (cut != std::string::npos) {
        std::string result = line;
        result.resize(cut);
        return result;
    }
    return line;
}

bool parseHexColor(const std::string& color, Uint8& r, Uint8& g, Uint8& b) {
    if (color.size() != 7 || color[0] != '#') return false;
    auto cv = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    auto hx = [&](char a, char b) {
        int A = cv(a), B = cv(b);
        return (A < 0 || B < 0) ? -1 : (A * 16 + B);
    };
    int R = hx(color[1], color[2]), G = hx(color[3], color[4]), B = hx(color[5], color[6]);
    if (R < 0 || G < 0 || B < 0) return false;
    r = (Uint8)R; g = (Uint8)G; b = (Uint8)B;
    return true;
}

bool processBasicConfigs(const std::string& key, const std::string& val, int& processedLines) {
    auto setb = [&](const char* K, bool& ref) {
        if (key == K) { 
            std::string v = val; 
            for (char& c : v) c = (char)std::tolower((unsigned char)c);
            ref = (v == "1" || v == "true" || v == "on" || v == "yes"); 
            return true; 
        } 
        return false; 
    };
    auto seti = [&](const char* K, int& ref) { 
        if (key == K) { ref = std::atoi(val.c_str()); return true; } 
        return false; 
    };
    auto setf = [&](const char* K, float& ref) { 
        if (key == K) { ref = (float)std::atof(val.c_str()); return true; } 
        return false; 
    };

    // Visual effect keys now handled via ConfigManager VisualConfig -> applyConfigToTheme/db_getVisualEffects
    // Layout shortcuts kept for backward compatibility
    if (seti("ROUNDED_PANELS", ROUNDED_PANELS)) { processedLines++; return true; }
    if (seti("HUD_FIXED_SCALE", HUD_FIXED_SCALE)) { processedLines++; return true; }
    
    return false;
}

bool processThemeColors(const std::string& key, const std::string& val, int& processedLines, ThemeManager& tm) {
    auto setrgb = [&](const char* K, Uint8& R, Uint8& G, Uint8& B) {
        if (key == K) { 
            Uint8 r, g, b; 
            if (parseHexColor(val, r, g, b)) { 
                R = r; G = g; B = b; 
                return true; 
            } 
        } 
        return false; 
    };
    auto seta = [&](const char* K, Uint8& ref) {
        if (key == K) { 
            int v = std::atoi(val.c_str()); 
            if (v < 0) v = 0; 
            if (v > 255) v = 255; 
            ref = (Uint8)v; 
            return true; 
        } 
        return false; 
    };

    // Cores básicas
    if (setrgb("BG", tm.getTheme().bg_r, tm.getTheme().bg_g, tm.getTheme().bg_b)) { processedLines++; return true; }
    if (setrgb("BOARD_EMPTY", tm.getTheme().board_empty_r, tm.getTheme().board_empty_g, tm.getTheme().board_empty_b)) { processedLines++; return true; }
    if (setrgb("PANEL_FILL", tm.getTheme().panel_fill_r, tm.getTheme().panel_fill_g, tm.getTheme().panel_fill_b)) { processedLines++; return true; }
    if (setrgb("PANEL_OUTLINE", tm.getTheme().panel_outline_r, tm.getTheme().panel_outline_g, tm.getTheme().panel_outline_b)) { processedLines++; return true; }
    if (setrgb("BANNER_BG", tm.getTheme().banner_bg_r, tm.getTheme().banner_bg_g, tm.getTheme().banner_bg_b)) { processedLines++; return true; }
    if (setrgb("BANNER_OUTLINE", tm.getTheme().banner_outline_r, tm.getTheme().banner_outline_g, tm.getTheme().banner_outline_b)) { processedLines++; return true; }
    if (setrgb("BANNER_TEXT", tm.getTheme().banner_text_r, tm.getTheme().banner_text_g, tm.getTheme().banner_text_b)) { processedLines++; return true; }

    // Cores HUD
    if (setrgb("HUD_LABEL", tm.getTheme().hud_label_r, tm.getTheme().hud_label_g, tm.getTheme().hud_label_b)) { processedLines++; return true; }
    if (setrgb("HUD_SCORE", tm.getTheme().hud_score_r, tm.getTheme().hud_score_g, tm.getTheme().hud_score_b)) { processedLines++; return true; }
    if (setrgb("HUD_LINES", tm.getTheme().hud_lines_r, tm.getTheme().hud_lines_g, tm.getTheme().hud_lines_b)) { processedLines++; return true; }
    if (setrgb("HUD_LEVEL", tm.getTheme().hud_level_r, tm.getTheme().hud_level_g, tm.getTheme().hud_level_b)) { processedLines++; return true; }

    // Cores NEXT
    if (setrgb("NEXT_FILL", tm.getTheme().next_fill_r, tm.getTheme().next_fill_g, tm.getTheme().next_fill_b)) { processedLines++; return true; }
    if (setrgb("NEXT_OUTLINE", tm.getTheme().next_outline_r, tm.getTheme().next_outline_g, tm.getTheme().next_outline_b)) { processedLines++; return true; }
    if (setrgb("NEXT_LABEL", tm.getTheme().next_label_r, tm.getTheme().next_label_g, tm.getTheme().next_label_b)) { processedLines++; return true; }

    // Cores OVERLAY
    if (setrgb("OVERLAY_FILL", tm.getTheme().overlay_fill_r, tm.getTheme().overlay_fill_g, tm.getTheme().overlay_fill_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_OUTLINE", tm.getTheme().overlay_outline_r, tm.getTheme().overlay_outline_g, tm.getTheme().overlay_outline_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_TOP", tm.getTheme().overlay_top_r, tm.getTheme().overlay_top_g, tm.getTheme().overlay_top_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_SUB", tm.getTheme().overlay_sub_r, tm.getTheme().overlay_sub_g, tm.getTheme().overlay_sub_b)) { processedLines++; return true; }

    // Alpha values (only overlay needs transparency)
    if (seta("OVERLAY_FILL_A", tm.getTheme().overlay_fill_a)) { processedLines++; return true; }
    if (seta("OVERLAY_OUTLINE_A", tm.getTheme().overlay_outline_a)) { processedLines++; return true; }

    return false;
}

bool processSpecialConfigs(const std::string& key, const std::string& val, int& processedLines, ThemeManager& tm) {
    // Configurações especiais
    if (key == "TITLE_TEXT") { TITLE_TEXT = val; processedLines++; return true; }
    if (key == "PIECES_FILE") { PIECES_FILE_PATH = val; processedLines++; return true; }
    
    // Grid colors
    if (key == "NEXT_GRID_DARK") { 
        *(int*)&tm.getTheme().next_grid_dark = std::atoi(val.c_str()); 
        processedLines++; 
        return true; 
    }
    if (key == "NEXT_GRID_LIGHT") { 
        *(int*)&tm.getTheme().next_grid_light = std::atoi(val.c_str()); 
        processedLines++; 
        return true; 
    }
    
    // Grid colors RGB
    if (key == "NEXT_GRID_DARK_COLOR") {
        Uint8 r, g, b;
        if (parseHexColor(val, r, g, b)) {
            tm.getTheme().next_grid_dark_r = r; tm.getTheme().next_grid_dark_g = g; tm.getTheme().next_grid_dark_b = b;
            tm.getTheme().next_grid_use_rgb = true;
            processedLines++;
        }
        return true;
    }
    if (key == "NEXT_GRID_LIGHT_COLOR") {
        Uint8 r, g, b;
        if (parseHexColor(val, r, g, b)) {
            tm.getTheme().next_grid_light_r = r; tm.getTheme().next_grid_light_g = g; tm.getTheme().next_grid_light_b = b;
            tm.getTheme().next_grid_use_rgb = true;
            processedLines++;
        }
        return true;
    }
    
    return false;
}

bool processAudioConfigs(const std::string& key, const std::string& val, int& processedLines, AudioSystem& audio) {
    // Delegate to the new AudioConfig system
    if (audio.loadFromConfig(key, val)) {
        processedLines++;
        return true;
    }
    
    // Legacy compatibility - update old fields for backward compatibility
    auto setb = [&](const char* K, bool& ref) {
        if (key == K) { 
            std::string v = val; 
            for (char& c : v) c = (char)std::tolower((unsigned char)c);
            ref = (v == "1" || v == "true" || v == "on" || v == "yes"); 
            return true; 
        } 
        return false; 
    };
    auto setf = [&](const char* K, float& ref) { 
        if (key == K) { 
            float v = (float)std::atof(val.c_str());
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
            ref = v; 
            return true; 
        } 
        return false; 
    };

    // Update legacy fields for backward compatibility
    if (setf("AUDIO_MASTER_VOLUME", audio.masterVolume)) { processedLines++; return true; }
    if (setf("AUDIO_SFX_VOLUME", audio.sfxVolume)) { processedLines++; return true; }
    if (setf("AUDIO_AMBIENT_VOLUME", audio.ambientVolume)) { processedLines++; return true; }
    if (setb("ENABLE_MOVEMENT_SOUNDS", audio.enableMovementSounds)) { processedLines++; return true; }
    if (setb("ENABLE_AMBIENT_SOUNDS", audio.enableAmbientSounds)) { processedLines++; return true; }
    if (setb("ENABLE_COMBO_SOUNDS", audio.enableComboSounds)) { processedLines++; return true; }
    if (setb("ENABLE_LEVEL_UP_SOUNDS", audio.enableLevelUpSounds)) { processedLines++; return true; }
    
    return false;
}

bool processJoystickConfigs(const std::string& key, const std::string& val, int& processedLines, JoystickSystem& joystick, PieceManager& pm) {
    auto seti = [&](const char* K, int& ref) { 
        if (key == K) { 
            int v = std::atoi(val.c_str());
            if (v >= 0 && v < 32) {
                ref = v; 
                return true; 
            }
        } 
        return false; 
    };
    auto setf = [&](const char* K, float& ref) { 
        if (key == K) { 
            float v = (float)std::atof(val.c_str());
            if (v >= 0.0f && v <= 1.0f) {
                ref = v; 
                return true; 
            }
        } 
        return false; 
    };

    // Get reference to config for easier access
    JoystickConfig& config = joystick.getConfig();

    // Configurações de mapeamento de botões
    if (seti("JOYSTICK_BUTTON_LEFT", config.buttonLeft)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_RIGHT", config.buttonRight)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_DOWN", config.buttonDown)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_UP", config.buttonUp)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_ROTATE_CCW", config.buttonRotateCCW)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_ROTATE_CW", config.buttonRotateCW)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_SOFT_DROP", config.buttonSoftDrop)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_HARD_DROP", config.buttonHardDrop)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_PAUSE", config.buttonPause)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_START", config.buttonStart)) { processedLines++; return true; }
    if (seti("JOYSTICK_BUTTON_QUIT", config.buttonQuit)) { processedLines++; return true; }
    
    // Configurações de analógico
    if (setf("JOYSTICK_ANALOG_DEADZONE", config.analogDeadzone)) { processedLines++; return true; }
    if (setf("JOYSTICK_ANALOG_SENSITIVITY", config.analogSensitivity)) { processedLines++; return true; }
    if (key == "JOYSTICK_INVERT_Y_AXIS") {
        int v = std::atoi(val.c_str());
        config.invertYAxis = (v != 0);
        processedLines++;
        return true;
    }
    
    // Configurações de timing (DAS/ARR system)
    if (key == "JOYSTICK_MOVE_REPEAT_DELAY_DAS") {
        int v = std::atoi(val.c_str());
        if (v >= 50 && v <= 1000) {
            config.moveRepeatDelayDAS = v;
            processedLines++;
            return true;
        }
    }
    if (key == "JOYSTICK_MOVE_REPEAT_DELAY_ARR") {
        int v = std::atoi(val.c_str());
        if (v >= 10 && v <= 200) {
            config.moveRepeatDelayARR = v;
            processedLines++;
            return true;
        }
    }
    // Legacy support: JOYSTICK_MOVE_REPEAT_DELAY sets DAS value
    if (key == "JOYSTICK_MOVE_REPEAT_DELAY") {
        int v = std::atoi(val.c_str());
        if (v >= 50 && v <= 1000) {
            config.moveRepeatDelayDAS = v;
            processedLines++;
            return true;
        }
    }
    if (key == "JOYSTICK_SOFT_DROP_DELAY") {
        int v = std::atoi(val.c_str());
        if (v >= 50 && v <= 500) {
            config.softDropRepeatDelay = v;
            processedLines++;
            return true;
        }
    }
    
    // Configurações de velocidade do jogo
    if (key == "GAME_SPEED_START_MS") {
        int v = std::atoi(val.c_str());
        if (v > 0) {
            gameConfig.tickMsStart = v;
            processedLines++;
            return true;
        }
    }
    if (key == "GAME_SPEED_MIN_MS") {
        int v = std::atoi(val.c_str());
        if (v > 0) {
            gameConfig.tickMsMin = v;
            processedLines++;
            return true;
        }
    }
    if (key == "GAME_SPEED_ACCELERATION") {
        int v = std::atoi(val.c_str());
        if (v > 0) {
            SPEED_ACCELERATION = v;
            processedLines++;
            return true;
        }
    }
    
    // Configurações de renderização
    if (key == "PREVIEW_GRID") {
        int v = std::atoi(val.c_str());
        if (v > 0 && v <= 10) {
            pm.setPreviewGrid(v);
            processedLines++;
            return true;
        }
    }
    
    return false;
}

bool processTimerConfigs(const std::string& key, const std::string& val, int& processedLines, TimerConfig& timerConfig) {
    if (timerConfig.loadFromConfig(key, val)) {
        processedLines++;
        return true;
    }
    return false;
}

} // namespace ConfigProcessors

