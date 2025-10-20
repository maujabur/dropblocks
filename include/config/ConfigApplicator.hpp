#pragma once

#include "ConfigTypes.hpp"
#include <vector>

// Forward declarations
class AudioSystem;
class ThemeManager;
class GameState;
class InputManager;
struct Piece;
struct VisualEffectsView;

/**
 * @brief Configuration application utilities
 * 
 * Functions to apply configuration structures to various game systems
 */
namespace ConfigApplicator {

/**
 * @brief Apply theme piece colors to PIECES vector
 */
void applyThemePieceColors(ThemeManager& themeManager, std::vector<Piece>& pieces);

/**
 * @brief Apply audio configuration to AudioSystem
 */
void applyConfigToAudio(AudioSystem& audio, const AudioConfig& config);

/**
 * @brief Apply visual configuration to theme and globals
 */
void applyConfigToTheme(const VisualConfig& config, ThemeManager& themeManager, VisualEffectsView& visualView);

/**
 * @brief Apply game configuration to GameState and globals
 */
void applyConfigToGame(GameState& state, const GameConfig& config);

/**
 * @brief Apply pieces configuration to theme
 */
void applyConfigToPieces(const PiecesConfig& config, ThemeManager& themeManager);

/**
 * @brief Apply input configuration to InputManager (joystick)
 */
void applyConfigToJoystick(InputManager& inputManager, const InputConfig& config);

/**
 * @brief Apply layout configuration to global layoutConfig
 */
void applyConfigToLayout(const LayoutConfig& config);

} // namespace ConfigApplicator

