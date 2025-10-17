#pragma once

#include <string>
#include <SDL2/SDL.h>

// Forward declarations
class AudioSystem;
class JoystickSystem;
class ThemeManager;
class PieceManager;

/**
 * @brief Configuration processing utilities
 * 
 * Collection of functions to process configuration key-value pairs
 * and apply them to various game systems.
 */
namespace ConfigProcessors {

/**
 * @brief Parse configuration line (remove comments)
 */
std::string parseConfigLine(const std::string& line);

/**
 * @brief Parse hex color string (#RRGGBB)
 */
bool parseHexColor(const std::string& s, Uint8& r, Uint8& g, Uint8& b);

/**
 * @brief Trim whitespace from string
 */
void trim(std::string& s);

/**
 * @brief Process basic configuration keys (layout, visual effects)
 */
bool processBasicConfigs(const std::string& key, const std::string& val, int& processedLines);

/**
 * @brief Process theme color configuration keys
 */
bool processThemeColors(const std::string& key, const std::string& val, int& processedLines, ThemeManager& themeManager);

/**
 * @brief Process special configuration keys (TITLE_TEXT, PIECES_FILE, grid colors)
 */
bool processSpecialConfigs(const std::string& key, const std::string& val, int& processedLines, ThemeManager& themeManager);

/**
 * @brief Process audio configuration keys
 */
bool processAudioConfigs(const std::string& key, const std::string& val, int& processedLines, AudioSystem& audio);

/**
 * @brief Process joystick configuration keys
 */
bool processJoystickConfigs(const std::string& key, const std::string& val, int& processedLines, JoystickSystem& joystick, PieceManager& pieceManager);

} // namespace ConfigProcessors

