#pragma once

#include <string>
#include <vector>

// Forward decls for types
struct Piece;
struct VisualConfig;
struct AudioConfig;
struct InputConfig;
struct PiecesConfig;
struct GameConfig;
enum class RandType; // forward declare to use in interface

class IAudioSystem {
public:
    virtual ~IAudioSystem() = default;
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
    virtual void playBeep(double freq, int ms, float vol = 0.25f, bool square = true) = 0;
    virtual void playChord(double baseFreq, int notes[], int count, int ms, float vol = 0.15f) = 0;
    // Align with concrete AudioSystem currently used in dropblocks.cpp
    virtual void playMovementSound() = 0;
    virtual void playRotationSound(bool clockwise) = 0;
    virtual void playSoftDropSound() = 0;
    virtual void playHardDropSound() = 0;
    virtual void playKickSound() = 0;
    virtual void playLevelUpSound() = 0;
    virtual void playGameOverSound() = 0;
    virtual void playComboSound(int combo) = 0;
    virtual void playTetrisSound() = 0;
    virtual void playBackgroundMelody(int level) = 0;
    virtual void playTensionSound(int tension) = 0;
    virtual void playSweepEffect() = 0;
    virtual void playScanlineEffect() = 0;
    virtual bool loadFromConfig(const std::string& key, const std::string& value) = 0;
};

class IThemeManager {
public:
    virtual ~IThemeManager() = default;
    virtual void initDefaultPieceColors() = 0;
    virtual void applyPieceColors(const std::vector<Piece>& pieces) = 0;
    virtual bool loadFromConfig(const std::string& key, const std::string& value) = 0;
};

class IPieceManager {
public:
    virtual ~IPieceManager() = default;
    virtual void initialize() = 0;
    virtual void reset() = 0;
    virtual void initializeRandomizer() = 0;
    virtual int getPreviewGrid() const = 0;
    virtual void setPreviewGrid(int grid) = 0;
    virtual int getNextPiece() = 0;
    virtual void setNextPiece(int id) = 0;
    virtual void setRandomizerType(RandType type) = 0;
    virtual void setRandBagSize(int size) = 0;
    virtual bool loadPiecesFile() = 0;
    virtual void seedFallback() = 0;
    virtual int getCurrentNextPiece() const = 0;
};

class IInputManager {
public:
    virtual ~IInputManager() = default;
    // Input actions
    virtual bool shouldMoveLeft() = 0;
    virtual bool shouldMoveRight() = 0;
    virtual bool shouldSoftDrop() = 0;
    virtual bool shouldHardDrop() = 0;
    virtual bool shouldRotateCCW() = 0;
    virtual bool shouldRotateCW() = 0;
    virtual bool shouldPause() = 0;
    virtual bool shouldRestart() = 0;
    virtual bool shouldForceRestart() = 0;
    virtual bool shouldQuit() = 0;
    virtual bool shouldScreenshot() = 0;
    // System methods
    virtual void update() = 0;
    virtual void resetTimers() = 0;
};

class IGameConfig {
public:
    virtual ~IGameConfig() = default;
    virtual const VisualConfig& getVisual() const = 0;
    virtual const AudioConfig& getAudio() const = 0;
    virtual const InputConfig& getInput() const = 0;
    virtual const PiecesConfig& getPieces() const = 0;
    virtual const GameConfig& getGame() const = 0;
    virtual bool loadFromFile(const std::string& path) = 0;
    virtual bool loadFromEnvironment() = 0;
    virtual bool validate() const = 0;
    virtual void setOverride(const std::string& key, const std::string& value) = 0;
};


