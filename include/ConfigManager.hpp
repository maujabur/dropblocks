#pragma once

#include <string>
#include <vector>
#include <map>

#include "ConfigTypes.hpp"
#include "Interfaces.hpp"

class ConfigManager : public IGameConfig {
private:
    VisualConfig visual_;
    AudioConfig audio_;
    InputConfig input_;
    PiecesConfig pieces_;
    GameConfig game_;
    LayoutConfig layout_;

    std::vector<std::string> configPaths_;
    std::map<std::string, std::string> overrides_;
    bool loaded_ = false;

public:
    // IGameConfig (const getters)
    const VisualConfig& getVisual() const override { return visual_; }
    const AudioConfig& getAudio() const override { return audio_; }
    const InputConfig& getInput() const override { return input_; }
    const PiecesConfig& getPieces() const override { return pieces_; }
    const GameConfig& getGame() const override { return game_; }

    // Layout getters (new)
    const LayoutConfig& getLayout() const { return layout_; }
    LayoutConfig& getLayout() { return layout_; }

    // Mutating getters (for internal configuration flow)
    VisualConfig& getVisual() { return visual_; }
    AudioConfig& getAudio() { return audio_; }
    InputConfig& getInput() { return input_; }
    PiecesConfig& getPieces() { return pieces_; }
    GameConfig& getGame() { return game_; }

    // Loading methods
    bool loadFromFile(const std::string& path) override;
    bool loadFromEnvironment() override;
    bool loadFromCommandLine(int argc, char* argv[]);
    bool loadAll();

    // Override system
    void setOverride(const std::string& key, const std::string& value) override;
    void clearOverrides();

    // Validation
    bool validate() const override;

    // Status
    bool isLoaded() const { return loaded_; }
    const std::vector<std::string>& getConfigPaths() const { return configPaths_; }
};



