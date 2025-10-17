#pragma once

#include <SDL2/SDL.h>
#include <string>

#include "Interfaces.hpp"
#include "ConfigTypes.hpp"

// Forward declarations for private impl types are kept in the .cpp

class AudioSystem : public IAudioSystem {
public:
    AudioSystem();
    ~AudioSystem() override = default;

    // Lifecycle
    bool initialize() override;
    void cleanup() override;

    // Synthesis
    void playBeep(double freq, int ms, float vol = 0.25f, bool square = true) override;
    void playChord(double baseFreq, int notes[], int count, int ms, float vol = 0.15f) override;
    void playRotationSound(bool clockwise) override;
    void playMovementSound() override;
    void playSoftDropSound() override;
    void playHardDropSound() override;
    void playKickSound() override;
    void playLevelUpSound() override;
    void playGameOverSound() override;
    void playComboSound(int combo) override;
    void playTetrisSound() override;
    void playBackgroundMelody(int level) override;
    void playTensionSound(int tension) override;
    void playSweepEffect() override;
    void playScanlineEffect() override;
    bool loadFromConfig(const std::string& key, const std::string& value) override;

    // Configuration access used elsewhere in the app
    AudioConfig& getConfig();
    const AudioConfig& getConfig() const;

    // Legacy compatibility fields (kept while migrating code)
    float masterVolume = 1.0f;
    float sfxVolume = 0.6f;
    float ambientVolume = 0.3f;
    bool enableMovementSounds = true;
    bool enableAmbientSounds = true;
    bool enableComboSounds = true;
    bool enableLevelUpSounds = true;

private:
    struct Impl;
    Impl* impl_; // pimpl to keep internal types private
};


