/**
 * @file dropblocks.cpp
 * @brief DropBlocks - SDL2 single-file falling blocks game (not Tetris®)
 * @author DropBlocks Team
 * @version 1.0
 * @date 2025
 * 
 * A modern, customizable falling blocks game inspired by Tetris®.
 * Features advanced visual effects, customizable themes, and multiple piece sets.
 * 
 * @section controls Controls
 * 
 * KEYBOARD:
 * - ← → : Move piece left/right
 * - ↓ : Soft drop
 * - Z/↑ : Rotate counter-clockwise
 * - X : Rotate clockwise
 * - SPACE : Hard drop
 * - P : Pause
 * - ENTER : Restart (after Game Over)
 * - ESC : Quit
 * - F12 : Screenshot
 * 
 * JOYSTICK/CONTROLLER:
 * - D-pad Left/Right : Move piece left/right
 * - D-pad Down : Soft drop
 * - D-pad Up : Rotate (CCW/CW)
 * - A/X Button : Rotate counter-clockwise
 * - B/Circle Button : Rotate clockwise
 * - X/Square Button : Soft drop
 * - Y/Triangle Button : Hard drop
 * - Start Button : Restart (after Game Over)
 * - Back/Select Button : Pause
 * - Analog sticks : Movement and rotation with deadzone
 * - Guide/PS Button : Disabled in kiosk mode
 * 
 * @section build Build
 * g++ dropblocks.cpp -o dropblocks `sdl2-config --cflags --libs` -O2
 * 
 * @section dependencies Dependencies
 * - SDL2 (graphics and audio)
 * - C++17 compatible compiler
 */

// TODO: add a configurable countdown timer for use in expositions
// TODO: check hard drop moving pieces to the right
// TODO: add piece statistics
// TODO: make regions configurable by position and size
// TODO: enhance resolution ans screen ratio system
// TODO: 

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <functional>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <chrono>

// ===========================
//   DEFINIÇÕES DE VERSÃO
// ===========================
#define DROPBLOCKS_VERSION "6.13"
#define DROPBLOCKS_BUILD_INFO "Phase 6: GameInitializer Basic Implementation Complete"
#define DROPBLOCKS_FEATURES "DependencyContainer with lifecycle management - GameState refactored for DI - Abstract interfaces with concrete implementations - Complete system decoupling - GameInitializer basic implementation"

// ===========================
//   FORWARD DECLARATIONS
// ===========================
// Forward declarations for structures
struct Theme;
struct Piece;
enum class RandType;
struct VisualConfig;
struct AudioConfig;
struct InputConfig;
struct PiecesConfig;
struct GameConfig;

// ===========================
//   INTERFACES ABSTRATAS
// ===========================

/**
 * @brief Interface for audio system
 * 
 * Defines the contract for audio functionality
 */
class IAudioSystem {
public:
    virtual ~IAudioSystem() = default;
    
    // Initialization and cleanup
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
    
    // Basic synthesis
    virtual void playBeep(double freq, int ms, float vol = 0.25f, bool square = true) = 0;
    virtual void playChord(double baseFreq, int notes[], int count, int ms, float vol = 0.15f) = 0;
    virtual void playArpeggio(double baseFreq, int notes[], int count, int noteMs, float vol = 0.12f) = 0;
    virtual void playSweep(double startFreq, double endFreq, int ms, float vol = 0.10f) = 0;
    
    // Game-specific sounds
    virtual void playMovementSound() = 0;
    virtual void playRotationSound(bool cw = true) = 0;
    virtual void playSoftDropSound() = 0;
    virtual void playHardDropSound() = 0;
    virtual void playKickSound() = 0;
    virtual void playLevelUpSound() = 0;
    virtual void playGameOverSound() = 0;
    virtual void playComboSound(int combo) = 0;
    virtual void playTetrisSound() = 0;
    virtual void playBackgroundMelody(int level) = 0;
    virtual void playTensionSound(int filledRows) = 0;
    virtual void playSweepEffect() = 0;
    virtual void playScanlineEffect() = 0;
    
    // Configuration
    virtual bool loadFromConfig(const std::string& key, const std::string& value) = 0;
};

/**
 * @brief Interface for theme management
 * 
 * Defines the contract for theme functionality
 */
class IThemeManager {
public:
    virtual ~IThemeManager() = default;
    
    // Theme access
    virtual const Theme& getTheme() const = 0;
    virtual Theme& getTheme() = 0;
    
    // Theme management
    virtual void initDefaultPieceColors() = 0;
    virtual void applyPieceColors(const std::vector<Piece>& pieces) = 0;
    virtual bool loadFromConfig(const std::string& key, const std::string& value) = 0;
};

/**
 * @brief Interface for piece management
 * 
 * Defines the contract for piece functionality
 */
class IPieceManager {
public:
    virtual ~IPieceManager() = default;
    
    // Piece generation
    virtual int getNextPiece() = 0;
    virtual int getCurrentNextPiece() const = 0;
    virtual void setNextPiece(int pieceIdx) = 0;
    
    // Initialization and reset
    virtual void initialize() = 0;
    virtual void reset() = 0;
    virtual void initializeRandomizer() = 0;
    
    // Configuration
    virtual int getPreviewGrid() const = 0;
    virtual void setPreviewGrid(int grid) = 0;
    virtual RandType getRandomizerType() const = 0;
    virtual void setRandomizerType(RandType type) = 0;
    virtual int getRandBagSize() const = 0;
    virtual void setRandBagSize(int size) = 0;
    
    // Piece loading
    virtual bool loadPiecesFile() = 0;
    virtual void seedFallback() = 0;
};

/**
 * @brief Interface for input management
 * 
 * Defines the contract for input functionality
 */
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
    virtual bool shouldQuit() = 0;
    virtual bool shouldScreenshot() = 0;
    
    // System methods
    virtual void update() = 0;
    virtual void resetTimers() = 0;
};

/**
 * @brief Interface for game configuration
 * 
 * Defines the contract for configuration functionality
 */
class IGameConfig {
public:
    virtual ~IGameConfig() = default;
    
    // Configuration access
    virtual const VisualConfig& getVisual() const = 0;
    virtual const AudioConfig& getAudio() const = 0;
    virtual const InputConfig& getInput() const = 0;
    virtual const PiecesConfig& getPieces() const = 0;
    virtual const GameConfig& getGame() const = 0;
    
    // Configuration management
    virtual bool loadFromFile(const std::string& path) = 0;
    virtual bool loadFromEnvironment() = 0;
    virtual bool validate() const = 0;
    virtual void setOverride(const std::string& key, const std::string& value) = 0;
};

// ===========================
//   DEPENDENCY INJECTION
// ===========================

/**
 * @brief Dependency injection container
 * 
 * Manages dependency registration and resolution with lifecycle support
 */
class DependencyContainer {
public:
    enum class Lifecycle {
        Singleton,  // Single instance per container
        Transient   // New instance per resolution
    };

private:
    struct ServiceMetadata {
        std::string name;
        std::string typeName;
        std::vector<std::string> dependencies;
        std::chrono::steady_clock::time_point createdTime;
        std::chrono::steady_clock::time_point lastAccessTime;
        size_t accessCount = 0;
        bool isHealthy = true;
        std::string lastError;
    };
    
    struct ServiceRegistration {
        std::function<void*()> factory;
        Lifecycle lifecycle;
        void* instance = nullptr;
        bool isInitialized = false;
        ServiceMetadata metadata;
    };
    
    std::map<std::string, ServiceRegistration> services_;
    std::vector<std::string> resolutionStack_; // For circular dependency detection
    std::map<std::string, std::vector<std::string>> dependencyGraph_; // For visualization
    
public:
    DependencyContainer() = default;
    ~DependencyContainer() {
        clear();
    }
    
    // Non-copyable, non-movable
    DependencyContainer(const DependencyContainer&) = delete;
    DependencyContainer& operator=(const DependencyContainer&) = delete;
    DependencyContainer(DependencyContainer&&) = delete;
    DependencyContainer& operator=(DependencyContainer&&) = delete;
    
    /**
     * @brief Register a service with factory function
     * @param name Service name/identifier
     * @param factory Factory function to create instance
     * @param lifecycle Singleton or Transient
     * @param dependencies List of service dependencies
     */
    template<typename T>
    void registerService(const std::string& name, std::function<T*()> factory, 
                        Lifecycle lifecycle = Lifecycle::Singleton,
                        const std::vector<std::string>& dependencies = {}) {
        auto now = std::chrono::steady_clock::now();
        services_[name] = {
            [factory]() -> void* { return static_cast<void*>(factory()); },
            lifecycle,
            nullptr,
            false,
            {
                name,
                typeid(T).name(),
                dependencies,
                now,
                now,
                0,
                true,
                ""
            }
        };
        
        // Update dependency graph
        for (const auto& dep : dependencies) {
            dependencyGraph_[name].push_back(dep);
        }
        
        // Service registered successfully
    }
    
    /**
     * @brief Register a singleton instance
     * @param name Service name/identifier
     * @param instance Pre-created instance
     */
    template<typename T>
    void registerInstance(const std::string& name, T* instance) {
        services_[name] = {
            nullptr,
            Lifecycle::Singleton,
            static_cast<void*>(instance),
            true
        };
    }
    
    /**
     * @brief Resolve a service by name
     * @param name Service name/identifier
     * @return Pointer to service instance
     */
    template<typename T>
    T* resolve(const std::string& name) {
        // Check for circular dependencies
        if (std::find(resolutionStack_.begin(), resolutionStack_.end(), name) != resolutionStack_.end()) {
            std::string cycle = name;
            for (auto it = resolutionStack_.rbegin(); it != resolutionStack_.rend(); ++it) {
                cycle += " -> " + *it;
            }
            throw std::runtime_error("Circular dependency detected: " + cycle);
        }
        
        auto it = services_.find(name);
        if (it == services_.end()) {
            throw std::runtime_error("Service not registered: " + name + 
                                   ". Available services: " + getRegisteredServicesList());
        }
        
        auto& registration = it->second;
        auto now = std::chrono::steady_clock::now();
        
        // Update access tracking
        registration.metadata.lastAccessTime = now;
        registration.metadata.accessCount++;
        
        // Return existing singleton instance
        if (registration.lifecycle == Lifecycle::Singleton && registration.isInitialized) {
            return static_cast<T*>(registration.instance);
        }
        
        // Create new instance
        resolutionStack_.push_back(name);
        T* instance = nullptr;
        
        try {
            // Validate dependencies first
            validateDependencies(name);
            
            if (registration.factory) {
                instance = static_cast<T*>(registration.factory());
            } else if (registration.instance) {
                instance = static_cast<T*>(registration.instance);
            } else {
                throw std::runtime_error("No factory or instance available for: " + name);
            }
            
            // Store singleton instance
            if (registration.lifecycle == Lifecycle::Singleton) {
                registration.instance = static_cast<void*>(instance);
                registration.isInitialized = true;
                registration.metadata.createdTime = now;
            }
            
            // Mark as healthy
            registration.metadata.isHealthy = true;
            registration.metadata.lastError = "";
            
        } catch (const std::exception& e) {
            registration.metadata.isHealthy = false;
            registration.metadata.lastError = e.what();
            resolutionStack_.pop_back();
            throw;
        }
        
        resolutionStack_.pop_back();
        return instance;
    }
    
    /**
     * @brief Check if service is registered
     * @param name Service name/identifier
     * @return True if registered
     */
    bool isRegistered(const std::string& name) const {
        return services_.find(name) != services_.end();
    }
    
    /**
     * @brief Get all registered service names
     * @return Vector of service names
     */
    std::vector<std::string> getRegisteredServices() const {
        std::vector<std::string> names;
        for (const auto& pair : services_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    /**
     * @brief Clear all services and instances
     */
    void clear() {
        // Note: We don't delete instances here as they might be managed elsewhere
        // This is a design choice - the container doesn't own the instances
        services_.clear();
        resolutionStack_.clear();
    }
    
    /**
     * @brief Get container statistics
     * @return String with registration info
     */
    std::string getStats() const {
        int singletonCount = 0;
        int transientCount = 0;
        int initializedCount = 0;
        int healthyCount = 0;
        size_t totalAccessCount = 0;
        
        for (const auto& pair : services_) {
            if (pair.second.lifecycle == Lifecycle::Singleton) {
                singletonCount++;
            } else {
                transientCount++;
            }
            if (pair.second.isInitialized) {
                initializedCount++;
            }
            if (pair.second.metadata.isHealthy) {
                healthyCount++;
            }
            totalAccessCount += pair.second.metadata.accessCount;
        }
        
        return "Services: " + std::to_string(services_.size()) + 
               " (Singletons: " + std::to_string(singletonCount) + 
               ", Transients: " + std::to_string(transientCount) + 
               ", Initialized: " + std::to_string(initializedCount) + 
               ", Healthy: " + std::to_string(healthyCount) + 
               ", Total Access: " + std::to_string(totalAccessCount) + ")";
    }
    
    /**
     * @brief Get detailed service information
     * @param name Service name
     * @return Detailed service info string
     */
    std::string getServiceInfo(const std::string& name) const {
        auto it = services_.find(name);
        if (it == services_.end()) {
            return "Service not found: " + name;
        }
        
        const auto& reg = it->second;
        const auto& meta = reg.metadata;
        
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - meta.createdTime).count();
        auto lastAccess = std::chrono::duration_cast<std::chrono::milliseconds>(now - meta.lastAccessTime).count();
        
        return "Service: " + name + 
               "\n  Type: " + meta.typeName +
               "\n  Lifecycle: " + (reg.lifecycle == Lifecycle::Singleton ? "Singleton" : "Transient") +
               "\n  Initialized: " + (reg.isInitialized ? "Yes" : "No") +
               "\n  Healthy: " + (meta.isHealthy ? "Yes" : "No") +
               "\n  Access Count: " + std::to_string(meta.accessCount) +
               "\n  Age: " + std::to_string(age) + "ms" +
               "\n  Last Access: " + std::to_string(lastAccess) + "ms ago" +
               "\n  Dependencies: " + (meta.dependencies.empty() ? "None" : join(meta.dependencies, ", ")) +
               (meta.lastError.empty() ? "" : "\n  Last Error: " + meta.lastError);
    }
    
    /**
     * @brief Get dependency graph as string
     * @return Dependency graph visualization
     */
    std::string getDependencyGraph() const {
        std::string result = "Dependency Graph:\n";
        for (const auto& pair : dependencyGraph_) {
            result += "  " + pair.first + " -> ";
            if (pair.second.empty()) {
                result += "[]\n";
            } else {
                result += "[" + join(pair.second, ", ") + "]\n";
            }
        }
        return result;
    }
    
    /**
     * @brief Validate all services
     * @return True if all services are valid
     */
    bool validateAllServices() const {
        bool allValid = true;
        for (const auto& pair : services_) {
            if (!pair.second.metadata.isHealthy) {
                // Service is unhealthy - error details in metadata
                allValid = false;
            }
        }
        return allValid;
    }
    
    /**
     * @brief Get list of registered service names
     * @return Comma-separated list of service names
     */
    std::string getRegisteredServicesList() const {
        std::vector<std::string> names;
        for (const auto& pair : services_) {
            names.push_back(pair.first);
        }
        return join(names, ", ");
    }

private:
    /**
     * @brief Validate dependencies for a service
     * @param serviceName Service to validate
     */
    void validateDependencies(const std::string& serviceName) const {
        auto it = services_.find(serviceName);
        if (it == services_.end()) return;
        
        const auto& deps = it->second.metadata.dependencies;
        for (const auto& dep : deps) {
            if (services_.find(dep) == services_.end()) {
                throw std::runtime_error("Dependency not found: " + dep + " (required by " + serviceName + ")");
            }
        }
    }
    
    /**
     * @brief Join vector of strings with separator
     * @param vec Vector of strings
     * @param sep Separator
     * @return Joined string
     */
    std::string join(const std::vector<std::string>& vec, const std::string& sep) const {
        if (vec.empty()) return "";
        
        std::string result = vec[0];
        for (size_t i = 1; i < vec.size(); ++i) {
            result += sep + vec[i];
        }
        return result;
    }
};
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <random>
#include <array>
#include <memory>
#include <map>

// ===========================
//   SISTEMA DE DEBUG
// ===========================

/**
 * @brief Debug logging system
 * 
 * Centralized debug logging with levels and automatic flushing
 */
class DebugLogger {
private:
    static bool enabled_;
    static int level_;
    
public:
    enum Level {
        ERROR = 0,
        WARNING = 1,
        INFO = 2,
        DEBUG = 3
    };
    
    static void setEnabled(bool enabled) { enabled_ = enabled; }
    static void setLevel(int level) { level_ = level; }
    
    static void log(Level level, const std::string& message) {
        if (!enabled_ || level > level_) return;
        
        const char* prefix = "";
        switch (level) {
            case ERROR: prefix = "ERROR"; break;
            case WARNING: prefix = "WARNING"; break;
            case INFO: prefix = "INFO"; break;
            case DEBUG: prefix = "DEBUG"; break;
        }
        
        printf("[%s] %s\n", prefix, message.c_str());
        fflush(stdout);
    }
    
    static void error(const std::string& message) { log(ERROR, message); }
    static void warning(const std::string& message) { log(WARNING, message); }
    static void info(const std::string& message) { log(INFO, message); }
    static void debug(const std::string& message) { log(DEBUG, message); }
};

bool DebugLogger::enabled_ = true;
int DebugLogger::level_ = DebugLogger::DEBUG;

// ===========================
//   SISTEMA DE CONFIGURAÇÃO MODULAR
// ===========================

/**
 * @brief RGB color structure
 */
struct RGB { Uint8 r,g,b; };

/**
 * @brief Visual configuration structure
 * 
 * Contains all visual theme settings including colors, effects, and layout
 */
struct VisualConfig {
    // Colors
    struct Colors {
        RGB background{8, 8, 12};
        RGB boardEmpty{28, 28, 36};
        RGB panelFill{24, 24, 32};
        RGB panelOutline{90, 90, 120};
        Uint8 panelOutlineAlpha = 200;
        
        // Banner
        RGB bannerBg{0, 40, 0};
        RGB bannerOutline{0, 60, 0};
        Uint8 bannerOutlineAlpha = 180;
        RGB bannerText{120, 255, 120};
        
        // HUD
        RGB hudLabel{200, 200, 220};
        RGB hudScore{255, 240, 120};
        RGB hudLines{180, 255, 180};
        RGB hudLevel{180, 200, 255};
        
        // NEXT
        RGB nextFill{18, 18, 26};
        RGB nextOutline{80, 80, 110};
        Uint8 nextOutlineAlpha = 160;
        RGB nextLabel{220, 220, 220};
        RGB nextGridDark{24, 24, 24};
        RGB nextGridLight{30, 30, 30};
        bool nextGridUseRgb = false;
        
        // Overlay
        RGB overlayFill{0, 0, 0};
        Uint8 overlayFillAlpha = 200;
        RGB overlayOutline{200, 200, 220};
        Uint8 overlayOutlineAlpha = 120;
        RGB overlayTop{255, 160, 160};
        RGB overlaySub{220, 220, 220};
    } colors;
    
    // Effects
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
    
    // Layout
    struct Layout {
        int roundedPanels = 1;
        int hudFixedScale = 4;
        int gap1Scale = 10;
        int gap2Scale = 10;
    } layout;
    
    // Text
    std::string titleText = "---H A C K T R I S";
};

/**
 * @brief Audio configuration and settings
 * 
 * Manages all audio-related configuration options
 */
class AudioConfig {
public:
    float masterVolume = 1.0f;
    float sfxVolume = 0.6f;
    float ambientVolume = 0.3f;
    bool enableMovementSounds = true;
    bool enableAmbientSounds = true;
    bool enableComboSounds = true;
    bool enableLevelUpSounds = true;
    
    // Configuration loading
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

/**
 * @brief Input configuration structure
 * 
 * Contains all input settings including joystick and keyboard
 */
struct InputConfig {
    // Joystick button mapping
    int buttonLeft = 13;
    int buttonRight = 11;
    int buttonDown = 14;
    int buttonUp = 12;
    int buttonRotateCCW = 0;
    int buttonRotateCW = 1;
    int buttonSoftDrop = 2;
    int buttonHardDrop = 3;
    int buttonPause = 6;
    int buttonStart = 7;
    int buttonQuit = 8;
    
    // Analog settings
    float analogDeadzone = 0.3f;
    float analogSensitivity = 1.0f;
    bool invertYAxis = false;
    
    // Timing
    Uint32 moveRepeatDelay = 200;
    Uint32 softDropRepeatDelay = 100;
};

/**
 * @brief Pieces configuration structure
 * 
 * Contains all piece-related settings
 */
struct PiecesConfig {
    std::string piecesFilePath = "";
    int previewGrid = 6;
    std::string randomizerType = "simple";
    int randBagSize = 0;
    std::vector<RGB> pieceColors;
};

/**
 * @brief Game configuration structure
 * 
 * Contains all game mechanics settings
 */
struct GameConfig {
    int tickMsStart = 400;
    int tickMsMin = 80;
    int speedAcceleration = 50;
    int levelStep = 10;
    float aspectCorrectionFactor = 0.75f;
};

/**
 * @brief Centralized configuration manager
 * 
 * Manages all configuration categories and provides unified access
 */
class ConfigManager : public IGameConfig {
private:
    VisualConfig visual_;
    AudioConfig audio_;
    InputConfig input_;
    PiecesConfig pieces_;
    GameConfig game_;
    
    std::vector<std::string> configPaths_;
    std::map<std::string, std::string> overrides_;
    bool loaded_ = false;
    
public:
    // Getters
    const VisualConfig& getVisual() const { return visual_; }
    const AudioConfig& getAudio() const { return audio_; }
    const InputConfig& getInput() const { return input_; }
    const PiecesConfig& getPieces() const { return pieces_; }
    const GameConfig& getGame() const { return game_; }
    
    // Setters
    VisualConfig& getVisual() { return visual_; }
    AudioConfig& getAudio() { return audio_; }
    InputConfig& getInput() { return input_; }
    PiecesConfig& getPieces() { return pieces_; }
    GameConfig& getGame() { return game_; }
    
    // Loading methods
    bool loadFromFile(const std::string& path);
    bool loadFromEnvironment();
    bool loadFromCommandLine(int argc, char* argv[]);
    bool loadAll();
    
    // Override system
    void setOverride(const std::string& key, const std::string& value);
    void clearOverrides();
    
    // Validation
    bool validate() const;
    
    // Status
    bool isLoaded() const { return loaded_; }
    const std::vector<std::string>& getConfigPaths() const { return configPaths_; }
};

/**
 * @brief Abstract configuration parser interface
 * 
 * Provides a unified interface for parsing different configuration categories
 */
class ConfigParser {
public:
    virtual ~ConfigParser() = default;
    virtual bool parse(const std::string& key, const std::string& value) = 0;
    virtual std::string getCategory() const = 0;
    virtual bool validate() const = 0;
};

/**
 * @brief Visual configuration parser
 * 
 * Handles parsing of all visual-related configuration options
 */
class VisualConfigParser : public ConfigParser {
private:
    VisualConfig& config_;
    
    // Helper functions
    bool parseBool(const std::string& value) const;
    int parseInt(const std::string& value) const;
    float parseFloat(const std::string& value) const;
    bool parseHexColor(const std::string& value, RGB& color) const;
    
public:
    VisualConfigParser(VisualConfig& config) : config_(config) {}
    
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "visual"; }
    bool validate() const override;
};

/**
 * @brief Audio configuration parser
 * 
 * Handles parsing of all audio-related configuration options
 */
class AudioConfigParser : public ConfigParser {
private:
    AudioConfig* config_;
    
    bool parseBool(const std::string& value) const;
    float parseFloat(const std::string& value) const;
    
public:
    AudioConfigParser(AudioConfig* config) : config_(config) {}
    
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "audio"; }
    bool validate() const override;
};

/**
 * @brief Input configuration parser
 * 
 * Handles parsing of all input-related configuration options
 */
class InputConfigParser : public ConfigParser {
private:
    InputConfig& config_;
    
    bool parseBool(const std::string& value) const;
    int parseInt(const std::string& value) const;
    float parseFloat(const std::string& value) const;
    
public:
    InputConfigParser(InputConfig& config) : config_(config) {}
    
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "input"; }
    bool validate() const override;
};

/**
 * @brief Pieces configuration parser
 * 
 * Handles parsing of all piece-related configuration options
 */
class PiecesConfigParser : public ConfigParser {
private:
    PiecesConfig& config_;
    
    int parseInt(const std::string& value) const;
    bool parseHexColor(const std::string& value, RGB& color) const;
    
public:
    PiecesConfigParser(PiecesConfig& config) : config_(config) {}
    
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "pieces"; }
    bool validate() const override;
};

/**
 * @brief Game configuration parser
 * 
 * Handles parsing of all game mechanics configuration options
 */
class GameConfigParser : public ConfigParser {
private:
    GameConfig& config_;
    
    int parseInt(const std::string& value) const;
    float parseFloat(const std::string& value) const;
    
public:
    GameConfigParser(GameConfig& config) : config_(config) {}
    
    bool parse(const std::string& key, const std::string& value) override;
    std::string getCategory() const override { return "game"; }
    bool validate() const override;
};

/**
 * @brief Configuration inheritance system
 * 
 * Manages configuration inheritance chain and priority-based loading
 */
class ConfigInheritance {
private:
    std::vector<std::string> inheritanceChain_;
    std::map<std::string, std::string> overrides_;
    
public:
    // Inheritance chain management
    void addBaseConfig(const std::string& path);
    void addOverrideConfig(const std::string& path);
    void clearChain();
    
    // Override management
    void addOverride(const std::string& key, const std::string& value);
    void clearOverrides();
    
    // Loading
    bool loadInheritedConfigs(ConfigManager& manager);
    
    // Getters
    const std::vector<std::string>& getInheritanceChain() const { return inheritanceChain_; }
    const std::map<std::string, std::string>& getOverrides() const { return overrides_; }
};

/**
 * @brief Configuration validator
 * 
 * Validates configuration values and provides error reporting
 */
class ConfigValidator {
public:
    struct ValidationError {
        std::string category;
        std::string key;
        std::string value;
        std::string message;
    };
    
    static std::vector<ValidationError> validate(const ConfigManager& config);
    static bool isValid(const ConfigManager& config);
    static void printErrors(const std::vector<ValidationError>& errors);
};

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// ===========================
//   PARÂMETROS & THEME
// ===========================

static int   ROUNDED_PANELS = 1;           // 1 = arredondado; 0 = retângulo
static int   HUD_FIXED_SCALE   = 6;        // escala fixa do HUD
static std::string TITLE_TEXT  = "---H A C K T R I S";// texto vertical (A–Z e espaço)
static int   GAP1_SCALE        = 10;       // banner ↔ tabuleiro (x scale)
static int   GAP2_SCALE        = 10;       // tabuleiro ↔ painel  (x scale)

static bool  ENABLE_BANNER_SWEEP = true;   // sweep local no banner
static bool  ENABLE_GLOBAL_SWEEP = true;   // sweep global (clareia a tela)

static float SWEEP_SPEED_PXPS  = 15.0f;    // px/s (banner)
static int   SWEEP_BAND_H_S    = 30;       // altura do banner sweep (em scale)
static int   SWEEP_ALPHA_MAX   = 100;      // 0..255 (banner)
static float SWEEP_SOFTNESS    = 0.7f;     // <1 = mais suave

static float SWEEP_G_SPEED_PXPS = 20.0f;   // px/s (global)
static int   SWEEP_G_BAND_H_PX  = 100;     // px
static int   SWEEP_G_ALPHA_MAX  = 50;      // 0..255
static float SWEEP_G_SOFTNESS   = 0.9f;    // suavidade

static int   SCANLINE_ALPHA     = 20;      // 0..255 (0 desativa)

// Caminho opcional indicado no cfg para o arquivo de peças
static std::string PIECES_FILE_PATH = "";

// Config do set de peças
static int PREVIEW_GRID = 6;               // NxN no NEXT (padrão 6)

/**
 * @brief Randomizer type enumeration
 * 
 * Defines the different piece randomization algorithms available.
 */
enum class RandType { 
    SIMPLE,  /**< Simple random selection */
    BAG      /**< Bag-based randomizer (7-bag system) */
};
static RandType RAND_TYPE = RandType::SIMPLE;
static int RAND_BAG_SIZE  = 0;             // 0 => tamanho do set

/**
 * @brief Theme configuration structure
 * 
 * Contains all visual theme settings including colors, transparency values,
 * and visual effect parameters for customizing the game's appearance.
 */
struct Theme {
    // fundo
    Uint8 bg_r=8, bg_g=8, bg_b=12;

    // tabuleiro
    Uint8 board_empty_r=28, board_empty_g=28, board_empty_b=36;

    // painel (HUD)
    Uint8 panel_fill_r=24, panel_fill_g=24, panel_fill_b=32;
    Uint8 panel_outline_r=90, panel_outline_g=90, panel_outline_b=120;
    Uint8 panel_outline_a=200;

    // banner
    Uint8 banner_bg_r=0, banner_bg_g=40, banner_bg_b=0;
    Uint8 banner_outline_r=0, banner_outline_g=60, banner_outline_b=0;
    Uint8 banner_outline_a=180;
    Uint8 banner_text_r=120, banner_text_g=255, banner_text_b=120;

    // HUD textos
    Uint8 hud_label_r=200, hud_label_g=200, hud_label_b=220;
    Uint8 hud_score_r=255, hud_score_g=240, hud_score_b=120;
    Uint8 hud_lines_r=180, hud_lines_g=255, hud_lines_b=180;
    Uint8 hud_level_r=180, hud_level_g=200, hud_level_b=255;

    // NEXT
    Uint8 next_fill_r=18, next_fill_g=18, next_fill_b=26;
    Uint8 next_outline_r=80, next_outline_g=80, next_outline_b=110;
    Uint8 next_outline_a=160;
    Uint8 next_label_r=220, next_label_g=220, next_label_b=220;
    Uint8 next_grid_dark=24, next_grid_light=30; // cinza
    // Grid colorido (prioritário se definido)
    Uint8 next_grid_dark_r=24, next_grid_dark_g=24, next_grid_dark_b=24;
    Uint8 next_grid_light_r=30, next_grid_light_g=30, next_grid_light_b=30;
    bool  next_grid_use_rgb=false;

    // overlay
    Uint8 overlay_fill_r=0, overlay_fill_g=0, overlay_fill_b=0, overlay_fill_a=200;
    Uint8 overlay_outline_r=200, overlay_outline_g=200, overlay_outline_b=220, overlay_outline_a=120;
    Uint8 overlay_top_r=255, overlay_top_g=160, overlay_top_b=160;
    Uint8 overlay_sub_r=220, overlay_sub_g=220, overlay_sub_b=220;

    // cores das peças (array dinâmico, sem limitação)
    std::vector<RGB> piece_colors;
} THEME;

// ===========================
//   MECÂNICA / ESTRUTURAS
// ===========================

/** @brief Number of columns in the game board */
static const int COLS = 10;
/** @brief Number of rows in the game board */
static const int ROWS = 20;
/** @brief Border size around the game board */
static const int BORDER = 10;
/** @brief Initial game tick interval in milliseconds */
static int TICK_MS_START = 400;
/** @brief Minimum game tick interval in milliseconds */
static int TICK_MS_MIN   = 80;
/** @brief Speed acceleration per level (ms reduction) */
static int SPEED_ACCELERATION = 50;
/** @brief Aspect ratio correction factor for LED screen distortion */
static float ASPECT_CORRECTION_FACTOR = 0.75f;
/** @brief Lines required to advance to next level */
static int LEVEL_STEP    = 10;

/**
 * @brief Game board cell structure
 * 
 * Represents a single cell on the game board with color and occupancy information.
 */
struct Cell { Uint8 r,g,b; bool occ=false; };

/**
 * @brief Tetris piece structure
 * 
 * Contains all information about a tetris piece including its rotations,
 * kick data for SRS (Super Rotation System), and visual properties.
 */
struct Piece {
    std::string name;
    std::vector<std::vector<std::pair<int,int>>> rot; // 0..3
    Uint8 r=200,g=200,b=200;

    // --- NOVO: SRS por transição ---
    // kicks[dirIndex][fromState] -> vetor de offsets
    // dirIndex: 0 = CW (+1), 1 = CCW (-1)
    std::array<std::array<std::vector<std::pair<int,int>>,4>,2> kicksPerTrans;
    bool hasPerTransKicks=false;

    // --- Legado (fallback compatível) ---
    std::vector<std::pair<int,int>> kicksCW;
    std::vector<std::pair<int,int>> kicksCCW;
    bool hasKicks=false;
};

// ===========================
//   MANAGERS - FASE 1: ELIMINAR GLOBAIS
// ===========================

// Forward declaration
static bool parseHexColor(const std::string& s, Uint8& r, Uint8& g, Uint8& b);

/**
 * @brief Theme manager
 * 
 * Manages visual theme configuration including colors, transparency values,
 * and visual effect parameters.
 */
class ThemeManager : public IThemeManager {
private:
    Theme theme_;
    
public:
    // Getters
    const Theme& getTheme() const { return theme_; }
    Theme& getTheme() { return theme_; }
    
    // Theme management
    void initDefaultPieceColors() {
        if (theme_.piece_colors.empty()) {
            theme_.piece_colors = {
                {220, 80, 80},  // Vermelho
                { 80,180,120},  // Verde
                { 80,120,220},  // Azul
                {220,180, 80},  // Amarelo
                {180, 80,220},  // Roxo
                { 80,220,180},  // Ciano
                {220,120, 80},  // Laranja
                {160,160,160}   // Cinza
            };
        }
    }
    
    void applyPieceColors(std::vector<Piece>& pieces) {
        initDefaultPieceColors();
        
        // Aplica cores do tema APENAS para as peças que têm cor definida no CFG
        for (size_t i = 0; i < pieces.size(); ++i) {
            if (i < theme_.piece_colors.size()) {
                // Usa cor do tema se disponível (CFG tem prioridade)
                pieces[i].r = theme_.piece_colors[i].r;
                pieces[i].g = theme_.piece_colors[i].g;
                pieces[i].b = theme_.piece_colors[i].b;
            } else {
                // Para peças "extra", verifica se tem cor do arquivo
                if (pieces[i].r == 0 && pieces[i].g == 0 && pieces[i].b == 0) {
                    // Se a peça não tem cor definida no arquivo, usa cor padrão
                    size_t defaultIndex = i % 8; // Cicla pelas 8 cores padrão
                    pieces[i].r = theme_.piece_colors[defaultIndex].r;
                    pieces[i].g = theme_.piece_colors[defaultIndex].g;
                    pieces[i].b = theme_.piece_colors[defaultIndex].b;
                }
                // Se a peça já tem cor do arquivo, mantém ela (não sobrescreve)
            }
        }
    }
    
    // Const version for interface compliance
    void applyPieceColors(const std::vector<Piece>& pieces) override {
        // Cast away const for the non-const version
        applyPieceColors(const_cast<std::vector<Piece>&>(pieces));
    }
    
    // Configuration loading
    bool loadFromConfig(const std::string& key, const std::string& value) {
        // Background colors
        if (key == "BG") {
            Uint8 r, g, b;
            if (parseHexColor(value, r, g, b)) {
                theme_.bg_r = r; theme_.bg_g = g; theme_.bg_b = b;
                return true;
            }
        }
        // Add more theme configuration loading as needed
        return false;
    }
};

// Global manager instances (temporary during migration)
static GameConfig gameConfig;
static ThemeManager themeManager;

// Forward declarations for classes
class PieceManager;
class JoystickSystem;

// Forward declarations for functions that use pieceManager
static bool loadPiecesFromStream(std::istream& in);
static bool processJoystickConfigs(const std::string& key, const std::string& val, int& processedLines, JoystickSystem& joystick);

// Temporary global variables eliminated - now using PieceManager private fields

static std::vector<Piece> PIECES;

/**
 * @brief Active piece structure
 * 
 * Represents the currently falling piece with its position, rotation, and piece index.
 */
struct Active { int x=COLS/2, y=0, rot=0, idx=0; };

// ===========================
//   UTILS: STR / CORES / PARSING
// ===========================

/**
 * @brief Trim whitespace from string
 * 
 * Removes leading and trailing whitespace characters from a string.
 * 
 * @param s Reference to string to trim (modified in place)
 */
static void trim(std::string& s){
    size_t a=s.find_first_not_of(" \t\r\n");
    size_t b=s.find_last_not_of(" \t\r\n");
    if(a==std::string::npos){ s.clear(); return; }
    s=s.substr(a,b-a+1);
}
/**
 * @brief Parse hexadecimal color string
 * 
 * Parses a hexadecimal color string in format #RRGGBB or RRGGBB
 * and extracts the RGB components.
 * 
 * @param s Input color string (e.g., "#FF0000" or "FF0000")
 * @param r Reference to store red component (0-255)
 * @param g Reference to store green component (0-255)
 * @param b Reference to store blue component (0-255)
 * @return true if parsing successful, false otherwise
 */
static bool parseHexColor(const std::string& s, Uint8& r, Uint8& g, Uint8& b){
    // Aceita tanto #RRGGBB quanto RRGGBB
    std::string color = s;
    if (color.size() == 6 && color[0] != '#') {
        // Adiciona # se não tiver
        color = "#" + color;
    }
    if (color.size()!=7 || color[0]!='#') return false;
    auto cv=[&](char c)->int{
        if(c>='0'&&c<='9') return c-'0';
        c=(char)std::toupper((unsigned char)c);
        if(c>='A'&&c<='F') return 10+(c-'A');
        return -1;
    };
    auto hx=[&](char a,char b){int A=cv(a),B=cv(b); return (A<0||B<0)?-1:(A*16+B);};
    int R=hx(color[1],color[2]), G=hx(color[3],color[4]), B=hx(color[5],color[6]);
    if(R<0||G<0||B<0) return false; r=(Uint8)R; g=(Uint8)G; b=(Uint8)B; return true;
}
static bool parseInt(const std::string& s, int& out){
    char* e=nullptr; long v=strtol(s.c_str(), &e, 10);
    if(e==s.c_str()||*e!='\0') return false; out=(int)v; return true;
}
static bool parseCoordList(const std::string& val, std::vector<std::pair<int,int>>& out){
    out.clear();
    
    // Parse coordinates like "(0,0);(1,0);(0,1);(1,1)"
    size_t pos = 0;
    while(pos < val.size()){
        // Skip whitespace
        while(pos < val.size() && (val[pos] == ' ' || val[pos] == '\t')) pos++;
        if(pos >= val.size()) break;
        
        // Look for opening parenthesis
        if(val[pos] != '('){
            pos++;
            continue;
        }
        pos++; // skip '('
        
        // Parse x coordinate
        int x = 0, y = 0;
        int sign = 1;
        if(pos < val.size() && val[pos] == '-'){ sign = -1; pos++; }
        else if(pos < val.size() && val[pos] == '+'){ sign = 1; pos++; }
        
        while(pos < val.size() && std::isdigit(val[pos])){
            x = x * 10 + (val[pos] - '0');
            pos++;
        }
        x *= sign;
        
        // Skip comma
        if(pos < val.size() && val[pos] == ',') pos++;
        
        // Parse y coordinate
        sign = 1;
        if(pos < val.size() && val[pos] == '-'){ sign = -1; pos++; }
        else if(pos < val.size() && val[pos] == '+'){ sign = 1; pos++; }
        
        while(pos < val.size() && std::isdigit(val[pos])){
            y = y * 10 + (val[pos] - '0');
            pos++;
        }
        y *= sign;
        
        // Skip closing parenthesis
        if(pos < val.size() && val[pos] == ')') pos++;
        
        // Add coordinate
        out.push_back({x, y});
        
        // Skip semicolon
        if(pos < val.size() && val[pos] == ';') pos++;
    }
    
    return !out.empty();
}
static bool parseKicks(const std::string& v, std::vector<std::pair<int,int>>& out){ return parseCoordList(v,out); }
static void rotate90(std::vector<std::pair<int,int>>& pts){
    for(auto& p:pts){ int x=p.first,y=p.second; p.first=-y; p.second=x; }
}

// ===========================
//   CONFIGURAÇÃO E CARREGAMENTO
// ===========================

// Funções auxiliares para parsing de configuração
static std::string parseConfigLine(const std::string& line) {
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

static bool processBasicConfigs(const std::string& key, const std::string& val, int& processedLines) {
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

    if (setb("ENABLE_BANNER_SWEEP", ENABLE_BANNER_SWEEP)) { processedLines++; return true; }
    if (setb("ENABLE_GLOBAL_SWEEP", ENABLE_GLOBAL_SWEEP)) { processedLines++; return true; }
    if (seti("ROUNDED_PANELS", ROUNDED_PANELS)) { processedLines++; return true; }
    if (seti("HUD_FIXED_SCALE", HUD_FIXED_SCALE)) { processedLines++; return true; }
    if (seti("GAP1_SCALE", GAP1_SCALE)) { processedLines++; return true; }
    if (seti("GAP2_SCALE", GAP2_SCALE)) { processedLines++; return true; }
    if (seti("SWEEP_BAND_H_S", SWEEP_BAND_H_S)) { processedLines++; return true; }
    if (seti("SWEEP_ALPHA_MAX", SWEEP_ALPHA_MAX)) { processedLines++; return true; }
    if (seti("SWEEP_G_BAND_H_PX", SWEEP_G_BAND_H_PX)) { processedLines++; return true; }
    if (seti("SWEEP_G_ALPHA_MAX", SWEEP_G_ALPHA_MAX)) { processedLines++; return true; }
    if (seti("SCANLINE_ALPHA", SCANLINE_ALPHA)) { processedLines++; return true; }
    if (setf("SWEEP_SPEED_PXPS", SWEEP_SPEED_PXPS)) { processedLines++; return true; }
    if (setf("SWEEP_SOFTNESS", SWEEP_SOFTNESS)) { processedLines++; return true; }
    if (setf("SWEEP_G_SPEED_PXPS", SWEEP_G_SPEED_PXPS)) { processedLines++; return true; }
    if (setf("SWEEP_G_SOFTNESS", SWEEP_G_SOFTNESS)) { processedLines++; return true; }
    
    return false;
}

static bool processThemeColors(const std::string& key, const std::string& val, int& processedLines) {
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
    if (setrgb("BG", themeManager.getTheme().bg_r, themeManager.getTheme().bg_g, themeManager.getTheme().bg_b)) { processedLines++; return true; }
    if (setrgb("BOARD_EMPTY", themeManager.getTheme().board_empty_r, themeManager.getTheme().board_empty_g, themeManager.getTheme().board_empty_b)) { processedLines++; return true; }
    if (setrgb("PANEL_FILL", themeManager.getTheme().panel_fill_r, themeManager.getTheme().panel_fill_g, themeManager.getTheme().panel_fill_b)) { processedLines++; return true; }
    if (setrgb("PANEL_OUTLINE", themeManager.getTheme().panel_outline_r, themeManager.getTheme().panel_outline_g, themeManager.getTheme().panel_outline_b)) { processedLines++; return true; }
    if (setrgb("BANNER_BG", themeManager.getTheme().banner_bg_r, themeManager.getTheme().banner_bg_g, themeManager.getTheme().banner_bg_b)) { processedLines++; return true; }
    if (setrgb("BANNER_OUTLINE", themeManager.getTheme().banner_outline_r, themeManager.getTheme().banner_outline_g, themeManager.getTheme().banner_outline_b)) { processedLines++; return true; }
    if (setrgb("BANNER_TEXT", themeManager.getTheme().banner_text_r, themeManager.getTheme().banner_text_g, themeManager.getTheme().banner_text_b)) { processedLines++; return true; }

    // Cores HUD
    if (setrgb("HUD_LABEL", themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b)) { processedLines++; return true; }
    if (setrgb("HUD_SCORE", themeManager.getTheme().hud_score_r, themeManager.getTheme().hud_score_g, themeManager.getTheme().hud_score_b)) { processedLines++; return true; }
    if (setrgb("HUD_LINES", themeManager.getTheme().hud_lines_r, themeManager.getTheme().hud_lines_g, themeManager.getTheme().hud_lines_b)) { processedLines++; return true; }
    if (setrgb("HUD_LEVEL", themeManager.getTheme().hud_level_r, themeManager.getTheme().hud_level_g, themeManager.getTheme().hud_level_b)) { processedLines++; return true; }

    // Cores NEXT
    if (setrgb("NEXT_FILL", themeManager.getTheme().next_fill_r, themeManager.getTheme().next_fill_g, themeManager.getTheme().next_fill_b)) { processedLines++; return true; }
    if (setrgb("NEXT_OUTLINE", themeManager.getTheme().next_outline_r, themeManager.getTheme().next_outline_g, themeManager.getTheme().next_outline_b)) { processedLines++; return true; }
    if (setrgb("NEXT_LABEL", themeManager.getTheme().next_label_r, themeManager.getTheme().next_label_g, themeManager.getTheme().next_label_b)) { processedLines++; return true; }

    // Cores OVERLAY
    if (setrgb("OVERLAY_FILL", themeManager.getTheme().overlay_fill_r, themeManager.getTheme().overlay_fill_g, themeManager.getTheme().overlay_fill_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_OUTLINE", themeManager.getTheme().overlay_outline_r, themeManager.getTheme().overlay_outline_g, themeManager.getTheme().overlay_outline_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_TOP", themeManager.getTheme().overlay_top_r, themeManager.getTheme().overlay_top_g, themeManager.getTheme().overlay_top_b)) { processedLines++; return true; }
    if (setrgb("OVERLAY_SUB", themeManager.getTheme().overlay_sub_r, themeManager.getTheme().overlay_sub_g, themeManager.getTheme().overlay_sub_b)) { processedLines++; return true; }

    // Alpha values
    if (seta("PANEL_OUTLINE_A", themeManager.getTheme().panel_outline_a)) { processedLines++; return true; }
    if (seta("NEXT_OUTLINE_A", themeManager.getTheme().next_outline_a)) { processedLines++; return true; }
    if (seta("OVERLAY_FILL_A", themeManager.getTheme().overlay_fill_a)) { processedLines++; return true; }
    if (seta("OVERLAY_OUTLINE_A", themeManager.getTheme().overlay_outline_a)) { processedLines++; return true; }

    return false;
}

// Declaração forward para AudioSystem e JoystickSystem
struct AudioSystem;
struct JoystickSystem;

static bool processAudioConfigs(const std::string& key, const std::string& val, int& processedLines, AudioSystem& audio);
static bool processJoystickConfigs(const std::string& key, const std::string& val, int& processedLines, JoystickSystem& joystick);

static bool processSpecialConfigs(const std::string& key, const std::string& val, int& processedLines) {
    // Configurações especiais
    if (key == "TITLE_TEXT") { TITLE_TEXT = val; processedLines++; return true; }
    if (key == "PIECES_FILE") { PIECES_FILE_PATH = val; processedLines++; return true; }
    
    // Grid colors
    if (key == "NEXT_GRID_DARK") { 
        *(int*)&themeManager.getTheme().next_grid_dark = std::atoi(val.c_str()); 
        processedLines++; 
        return true; 
    }
    if (key == "NEXT_GRID_LIGHT") { 
        *(int*)&themeManager.getTheme().next_grid_light = std::atoi(val.c_str()); 
        processedLines++; 
        return true; 
    }
    
    // Grid colors RGB
    if (key == "NEXT_GRID_DARK_COLOR") {
        Uint8 r, g, b;
        if (parseHexColor(val, r, g, b)) {
            themeManager.getTheme().next_grid_dark_r = r; themeManager.getTheme().next_grid_dark_g = g; themeManager.getTheme().next_grid_dark_b = b;
            themeManager.getTheme().next_grid_use_rgb = true;
            processedLines++;
        }
        return true;
    }
    if (key == "NEXT_GRID_LIGHT_COLOR") {
        Uint8 r, g, b;
        if (parseHexColor(val, r, g, b)) {
            themeManager.getTheme().next_grid_light_r = r; themeManager.getTheme().next_grid_light_g = g; themeManager.getTheme().next_grid_light_b = b;
            themeManager.getTheme().next_grid_use_rgb = true;
            processedLines++;
        }
        return true;
    }

    // Cores das peças (PIECE0, PIECE1, etc.)
    if (key.rfind("PIECE", 0) == 0) {
        std::string numStr = key.substr(5);
        int pieceIndex = -1;
        try {
            pieceIndex = std::stoi(numStr);
        } catch (...) {
            return false;
        }
        
        Uint8 r, g, b;
        if (parseHexColor(val, r, g, b)) {
            if (pieceIndex >= (int)themeManager.getTheme().piece_colors.size()) {
                themeManager.getTheme().piece_colors.resize(pieceIndex + 1, {200, 200, 200});
            }
            themeManager.getTheme().piece_colors[pieceIndex] = {r, g, b};
            processedLines++;
        }
        return true;
    }
    
    return false;
}

static void loadConfigFromStream(std::istream& in, AudioSystem& audio, JoystickSystem& joystick) {
    
    std::string line;
    int lineNum = 0;
    int processedLines = 0;
    int skippedLines = 0;
    
    while (std::getline(in, line)) {
        lineNum++;
        
        // Parse da linha (remove comentários)
        line = parseConfigLine(line);
        trim(line);
        
        if (line.empty()) {
            skippedLines++;
            continue;
        }
        
        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            skippedLines++;
            continue;
        }
        
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        trim(key);
        trim(val);
        
        if (key.empty()) {
            skippedLines++;
            continue;
        }

        std::string KEY = key;
        for (char& c : KEY) c = (char)std::toupper((unsigned char)c);

        // Processar usando funções especializadas
        if (processBasicConfigs(KEY, val, processedLines)) continue;
        if (processThemeColors(KEY, val, processedLines)) continue;
        if (processAudioConfigs(KEY, val, processedLines, audio)) continue;
        if (processJoystickConfigs(KEY, val, processedLines, joystick)) continue;
        if (processSpecialConfigs(KEY, val, processedLines)) continue;
        
        // Linha não reconhecida
        skippedLines++;
    }
    
}
static bool loadConfigPath(const std::string& p, AudioSystem& audio, JoystickSystem& joystick){
    
    std::ifstream f(p.c_str()); 
    if(f.good()){ 
        loadConfigFromStream(f, audio, joystick);
        return true; 
    } 
    return false;
}
/**
 * @brief Load configuration file
 * 
 * Attempts to load configuration from multiple sources in order:
 * 1. Environment variable DROPBLOCKS_CFG
 * 2. default.cfg
 * 3. dropblocks.cfg
 * 4. Command line arguments
 * 
 * @param audio Audio system reference for audio configuration
 */
static void loadConfigFile(AudioSystem& audio, JoystickSystem& joystick){
    
    if(const char* env = std::getenv("DROPBLOCKS_CFG")){ 
        if(loadConfigPath(env, audio, joystick)) { 
            DebugLogger::info("Config carregado de: " + std::string(env)); 
            return; 
        } 
    }
    if(loadConfigPath("default.cfg", audio, joystick)) { 
        DebugLogger::info("Config carregado de: default.cfg"); 
        return; 
    }
    if(loadConfigPath("dropblocks.cfg", audio, joystick)) { 
        DebugLogger::info("Config carregado de: dropblocks.cfg"); 
        return; 
    } // fallback para compatibilidade
    if(const char* home = std::getenv("HOME")){
        std::string p = std::string(home) + "/.config/default.cfg";
        if(loadConfigPath(p, audio, joystick)) { 
            DebugLogger::info("Config carregado de: " + p); 
            return; 
        }
        std::string p2 = std::string(home) + "/.config/dropblocks.cfg";
        if(loadConfigPath(p2, audio, joystick)) { 
            DebugLogger::info("Config carregado de: " + p2); 
            return; 
        }
    }
    DebugLogger::info("Nenhum config encontrado; usando padrões.");
}



// Funções auxiliares para parsing de peças
static std::string parsePiecesLine(const std::string& line) {
    size_t semi = line.find(';');
    size_t cut = std::string::npos;
    
    // Cut at ; only if it's at the start of line (comment) or if it's after = but before coordinates
    if (semi != std::string::npos) {
        // If ; is at start of line, it's a comment
        if (semi == 0 || (semi > 0 && line[semi-1] == ' ')) {
            cut = semi;
        }
        // If ; is after =, check if it's before coordinates (has parentheses)
        else {
            size_t eq_probe = line.find('=');
            if (eq_probe != std::string::npos && semi > eq_probe) {
                // Look for parentheses after the ; to see if it's coordinate separation
                size_t paren_after_semi = line.find('(', semi);
                if (paren_after_semi == std::string::npos) {
                    // No parentheses after ;, so it's a comment
                    cut = semi;
                }
                // If there are parentheses after ;, it's coordinate separation, don't cut
            }
        }
    }
    
    if (cut != std::string::npos) {
        std::string result = line;
        result.resize(cut);
        return result;
    }
    return line;
}

static void buildPieceRotations(Piece& piece, const std::vector<std::pair<int,int>>& base, 
                               const std::vector<std::pair<int,int>>& rot0,
                               const std::vector<std::pair<int,int>>& rot1,
                               const std::vector<std::pair<int,int>>& rot2,
                               const std::vector<std::pair<int,int>>& rot3,
                               bool rotExplicit) {
    piece.rot.clear();
    
    if (rotExplicit) {
        if (!rot0.empty()) {
            piece.rot.push_back(rot0);
            piece.rot.push_back(rot1.empty() ? rot0 : rot1);
            piece.rot.push_back(rot2.empty() ? rot0 : rot2);
            piece.rot.push_back(rot3.empty() ? (rot1.empty() ? rot0 : rot1) : rot3);
        }
    } else {
        if (!base.empty()) {
            std::vector<std::pair<int,int>> r0 = base, r1 = base, r2 = base, r3 = base;
            rotate90(r1);
            r2 = r1; rotate90(r2);
            r3 = r2; rotate90(r3);
            piece.rot.push_back(r0);
            piece.rot.push_back(r1);
            piece.rot.push_back(r2);
            piece.rot.push_back(r3);
        }
    }
}

static bool processPieceProperty(Piece& cur, const std::string& key, const std::string& val,
                                std::vector<std::pair<int,int>>& base,
                                std::vector<std::pair<int,int>>& rot0,
                                std::vector<std::pair<int,int>>& rot1,
                                std::vector<std::pair<int,int>>& rot2,
                                std::vector<std::pair<int,int>>& rot3,
                                bool& rotExplicit) {
    if (key == "COLOR") { 
        Uint8 r, g, b; 
        if (parseHexColor(val, r, g, b)) { 
            cur.r = r; cur.g = g; cur.b = b; 
        } 
        return true; 
    }
    if (key == "ROTATIONS") { 
        std::string vv = val; 
        for (char& c : vv) c = (char)std::tolower((unsigned char)c); 
        rotExplicit = (vv == "explicit"); 
        return true; 
    }
    if (key == "BASE") { 
        parseCoordList(val, base); 
        return true; 
    }
    if (key == "ROT0") { 
        if (val.rfind("sameas:", 0) == 0) { /* usa rot0 */ } 
        else parseCoordList(val, rot0); 
        rotExplicit = true; 
        return true; 
    }
    if (key == "ROT1") { 
        if (val.rfind("sameas:", 0) == 0) { rot1 = rot0; } 
        else parseCoordList(val, rot1); 
        rotExplicit = true; 
        return true; 
    }
    if (key == "ROT2") { 
        if (val.rfind("sameas:", 0) == 0) { rot2 = rot0; } 
        else parseCoordList(val, rot2); 
        rotExplicit = true; 
        return true; 
    }
    if (key == "ROT3") { 
        if (val.rfind("sameas:", 0) == 0) { rot3 = rot1.empty() ? rot0 : rot1; } 
        else parseCoordList(val, rot3); 
        rotExplicit = true; 
        return true; 
    }
    if (key == "KICKS.CW") { 
        parseKicks(val, cur.kicksCW); 
        cur.hasKicks = true; 
        return true; 
    }
    if (key == "KICKS.CCW") { 
        parseKicks(val, cur.kicksCCW); 
        cur.hasKicks = true; 
        return true; 
    }
    
    auto setKPT = [&](int dirIdx, int fromState, const std::string& val) {
        std::vector<std::pair<int,int>> tmp;
        if (parseCoordList(val, tmp)) {
            cur.kicksPerTrans[dirIdx][fromState] = tmp;
            cur.hasPerTransKicks = true;
            return true;
        }
        return false;
    };

    // KICKS.CW.0TO1 / 1TO2 / 2TO3 / 3TO0
    if (key.rfind("KICKS.CW.", 0) == 0) {
        std::string t = key.substr(10); // depois de "KICKS.CW."
        if (t == "0TO1") { setKPT(0, 0, val); return true; }
        if (t == "1TO2") { setKPT(0, 1, val); return true; }
        if (t == "2TO3") { setKPT(0, 2, val); return true; }
        if (t == "3TO0") { setKPT(0, 3, val); return true; }
    }
    // KICKS.CCW.0TO3 / 3TO2 / 2TO1 / 1TO0
    if (key.rfind("KICKS.CCW.", 0) == 0) {
        std::string t = key.substr(11); // depois de "KICKS.CCW."
        if (t == "0TO3") { setKPT(1, 0, val); return true; }
        if (t == "3TO2") { setKPT(1, 3, val); return true; }
        if (t == "2TO1") { setKPT(1, 2, val); return true; }
        if (t == "1TO0") { setKPT(1, 1, val); return true; }
    }
    
    return false;
}

// loadPiecesFromStream moved after pieceManager definition

static bool loadPiecesPath(const std::string& p){
    std::ifstream f(p.c_str()); 
    if(!f.good()) {
        return false;
    }
    bool ok=loadPiecesFromStream(f);
    SDL_Log("Pieces carregado de: %s (%s)", p.c_str(), ok?"OK":"vazio/erro");
    return ok;
}
/**
 * @brief Load pieces file
 * 
 * Attempts to load piece definitions from multiple sources in order:
 * 1. PIECES_FILE_PATH from configuration
 * 2. default.pieces
 * 3. tetris_original.pieces
 * Falls back to default piece set if no file is found.
 * 
 * @return true if pieces loaded successfully, false otherwise
 */
// Funções movidas para depois da declaração do pieceManager

static void initDefaultPieceColors(){
    // Se não há cores definidas, inicializa com cores padrão
    if(themeManager.getTheme().piece_colors.empty()){
        themeManager.getTheme().piece_colors = {
            {220, 80, 80},  // Vermelho
            { 80,180,120},  // Verde
            { 80,120,220},  // Azul
            {220,160, 80},  // Laranja
            {160, 80,220},  // Roxo
            {200,200,200},  // Cinza claro
            {200,200,200},  // Cinza claro
            {200,200,200}   // Cinza claro
        };
    }
    
    // NÃO preenche automaticamente - deixa o CFG controlar quantas cores tem
}

static void applyThemePieceColors(){
    themeManager.applyPieceColors(PIECES);
}

// ===========================
//   MECÂNICA DO JOGO
// ===========================
/**
 * @brief Check if piece collides with board or other pieces
 * 
 * Tests if the active piece would collide with the board boundaries or
 * other locked pieces at the specified position and rotation.
 * 
 * @param a Active piece to test
 * @param g Game board grid
 * @param dx X offset to test
 * @param dy Y offset to test
 * @param drot Rotation offset to test
 * @return true if collision detected, false otherwise
 */
static bool collides(const Active& a, const std::vector<std::vector<Cell>>& g, int dx, int dy, int drot){
    int R = (a.rot + drot + 4)%4;
    for (auto [px,py] : PIECES[a.idx].rot[R]) {
        int x = a.x + dx + px, y = a.y + dy + py;
        if (x<0 || x>=COLS || y<0 || y>=ROWS) return true;
        if (g[y][x].occ) return true;
    }
    return false;
}
/**
 * @brief Lock piece to the board
 * 
 * Permanently places the active piece on the board at its current position.
 * 
 * @param a Active piece to lock
 * @param g Game board grid (modified in place)
 */
static void lockPiece(const Active& a, std::vector<std::vector<Cell>>& g){
    auto &pc = PIECES[a.idx];
    for (auto [px,py] : pc.rot[a.rot]) {
        int x=a.x+px, y=a.y+py;
        if (y>=0 && y<ROWS && x>=0 && x<COLS){
            g[y][x].occ=true; g[y][x].r=pc.r; g[y][x].g=pc.g; g[y][x].b=pc.b;
        }
    }
}
/**
 * @brief Clear completed lines
 * 
 * Removes all completed horizontal lines from the board and returns the count.
 * 
 * @param g Game board grid (modified in place)
 * @return Number of lines cleared
 */
static int clearLines(std::vector<std::vector<Cell>>& g){
    int cleared=0;
    for (int y=ROWS-1; y>=0; --y){
        bool full=true; for (int x=0;x<COLS;x++) if(!g[y][x].occ){ full=false; break; }
        if (full){ cleared++; for (int yy=y; yy>0; --yy) g[yy]=g[yy-1]; g[0]=std::vector<Cell>(COLS); y++; }
    }
    return cleared;
}
static void newActive(Active& a, int idx){ a.idx=idx; a.rot=0; a.x=COLS/2; a.y=0; }
// Declaração forward para rotateWithKicks
static void rotateWithKicks(Active& act, const std::vector<std::vector<Cell>>& grid, int dir, AudioSystem& audio);

// ===========================
//   FONTE 5x7 PIXEL
// ===========================
static bool glyph(char c, int x, int y){
    auto at=[&](const char* rows[7]){ return rows[y][x]=='#'; };
    static const char* NUM[10][7] = {
        {" ### ","#   #","#  ##","# # #","##  #","#   #"," ### "}, //0
        {"  #  "," ##  ","  #  ","  #  ","  #  ","  #  "," ### "}, //1
        {" ### ","#   #","    #","   # ","  #  "," #   ","#####"}, //2
        {" ### ","#   #","    #"," ### ","    #","#   #"," ### "}, //3
        {"   # ","  ## "," # # ","#  # ","#####","   # ","   # "}, //4
        {"#####","#    ","#    ","#### ","    #","#   #"," ### "}, //5
        {" ### ","#   #","#    ","#### ","#   #","#   #"," ### "}, //6
        {"#####","    #","   # ","  #  ","  #  ","  #  ","  #  "}, //7
        {" ### ","#   #","#   #"," ### ","#   #","#   #"," ### "}, //8
        {" ### ","#   #","#   #"," ####","    #","#   #"," ### "}  //9
    };
    static const char* A_[7]={" ### ","#   #","#   #","#####","#   #","#   #","#   #"};
    static const char* B_[7]={"#### ","#   #","#   #","#### ","#   #","#   #","#### "};
    static const char* C_[7]={" ### ","#   #","#    ","#    ","#    ","#   #"," ### "};
    static const char* D_[7]={"#### ","#   #","#   #","#   #","#   #","#   #","#### "};
    static const char* E_[7]={"#####","#    ","#    ","#### ","#    ","#    ","#####"};
    static const char* F_[7]={"#####","#    ","#    ","#### ","#    ","#    ","#    "};
    static const char* G_[7]={" ### ","#   #","#    ","# ###","#   #","#   #"," ### "};
    static const char* H_[7]={"#   #","#   #","#   #","#####","#   #","#   #","#   #"};
    static const char* I_[7]={"#####","  #  ","  #  ","  #  ","  #  ","  #  ","#####"};
    static const char* J_[7]={"  ###","   # ","   # ","   # ","#  # ","#  # "," ##  "};
    static const char* K_[7]={"#   #","#  # ","# #  ","##   ","# #  ","#  # ","#   #"};
    static const char* L_[7]={"#    ","#    ","#    ","#    ","#    ","#    ","#####"};
    static const char* M_[7]={"#   #","## ##","# # #","#   #","#   #","#   #","#   #"};
    static const char* N_[7]={"#   #","##  #","# # #","#  ##","#   #","#   #","#   #"};
    static const char* O_[7]={" ### ","#   #","#   #","#   #","#   #","#   #"," ### "};
    static const char* P_[7]={"#### ","#   #","#   #","#### ","#    ","#    ","#    "};
    static const char* Q_[7]={" ### ","#   #","#   #","#   #","# # #","#  # "," ## #"};
    static const char* R_[7]={"#### ","#   #","#   #","#### ","# #  ","#  # ","#   #"};
    static const char* S_[7]={" ####","#    ","#    "," ### ","    #","    #","#### "};
    static const char* T_[7]={"#####","  #  ","  #  ","  #  ","  #  ","  #  ","  #  "};
    static const char* U_[7]={"#   #","#   #","#   #","#   #","#   #","#   #"," ### "};
    static const char* V_[7]={"#   #","#   #","#   #","#   #","#   #"," # # ","  #  "};
    static const char* W_[7]={"#   #","#   #","#   #","# # #","# # #","## ##","#   #"};
    static const char* X_[7]={"#   #","#   #"," # # ","  #  "," # # ","#   #","#   #"};
    static const char* Y_[7]={"#   #","#   #"," # # ","  #  ","  #  ","  #  ","  #  "};
    static const char* Z_[7]={"#####","    #","   # ","  #  "," #   ","#    ","#####"};
    static const char* dash[7]={"     ","     ","     "," ### ","     ","     ","     "};
    static const char* colon[7]={"     ","  #  ","     ","     ","     ","  #  ","     "};

    if(c>='0'&&c<='9') return NUM[c-'0'][y][x]=='#';
    c=(char)std::toupper((unsigned char)c);
    #define GL(CH,ARR) if(c==CH) return at(ARR);
    GL('A',A_) GL('B',B_) GL('C',C_) GL('D',D_) GL('E',E_) GL('F',F_) GL('G',G_)
    GL('H',H_) GL('I',I_) GL('J',J_) GL('K',K_) GL('L',L_) GL('M',M_) GL('N',N_)
    GL('O',O_) GL('P',P_) GL('Q',Q_) GL('R',R_) GL('S',S_) GL('T',T_) GL('U',U_)
    GL('V',V_) GL('W',W_) GL('X',X_) GL('Y',Y_) GL('Z',Z_) GL('-',dash) GL(':',colon)
    #undef GL
    return false;
}
static void drawPixelText(SDL_Renderer* ren, int x, int y, const std::string& s, int scale, Uint8 r, Uint8 g, Uint8 b){
    SDL_Rect px; SDL_SetRenderDrawColor(ren, r,g,b,255);
    int cx=x;
    for(char c : s){
        if(c=='\n'){ y += (7*scale + scale*2); cx=x; continue; }
        for(int yy=0; yy<7; ++yy) for(int xx=0; xx<5; ++xx)
            if(glyph(c,xx,yy)){ px = { cx + xx*scale, y + yy*scale, scale, scale }; SDL_RenderFillRect(ren, &px); }
        cx += 6*scale;
    }
}
static int textWidthPx(const std::string& s, int scale){
    if(s.empty()) return 0; return (int)s.size() * 6 * scale - scale;
}
static void drawPixelTextOutlined(SDL_Renderer* ren, int x, int y, const std::string& s, int scale,
                                  Uint8 fr, Uint8 fg, Uint8 fb, Uint8 or_, Uint8 og, Uint8 ob){
    const int d = std::max(1, scale/2);
    drawPixelText(ren, x-d, y,   s, scale, or_,og,ob);
    drawPixelText(ren, x+d, y,   s, scale, or_,og,ob);
    drawPixelText(ren, x,   y-d, s, scale, or_,og,ob);
    drawPixelText(ren, x,   y+d, s, scale, or_,og,ob);
    drawPixelText(ren, x-d, y-d, s, scale, or_,og,ob);
    drawPixelText(ren, x+d, y-d, s, scale, or_,og,ob);
    drawPixelText(ren, x-d, y+d, s, scale, or_,og,ob);
    drawPixelText(ren, x+d, y+d, s, scale, or_,og,ob);
    drawPixelText(ren, x,   y,   s, scale, fr,fg,fb);
}

// ===========================
//   RENDERIZAÇÃO E INTERFACE
// ===========================
static void drawRoundedFilled(SDL_Renderer* r, int x, int y, int w, int h, int rad, Uint8 R, Uint8 G, Uint8 B, Uint8 A){
    if(!ROUNDED_PANELS){ SDL_SetRenderDrawColor(r, R,G,B,A); SDL_Rect rr{ x,y,w,h }; SDL_RenderFillRect(r,&rr); return; }
    rad = std::max(0, std::min(rad, std::min(w,h)/2));
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, R,G,B,A);
    for (int yy=0; yy<h; ++yy){
        int left = x, right = x + w - 1;
        if (yy < rad){
            int dy = rad-1-yy; int dx = (int)std::floor(std::sqrt((double)rad*rad - (double)dy*dy));
            left = x + rad - dx; right = x + w - rad + dx - 1;
        } else if (yy >= h - rad){
            int dy = yy - (h - rad); int dx = (int)std::floor(std::sqrt((double)rad*rad - (double)dy*dy));
            left = x + rad - dx; right = x + w - rad + dx - 1;
        }
        SDL_Rect line{ left, y + yy, right - left + 1, 1 };
        SDL_RenderFillRect(r, &line);
    }
}
static void drawRoundedOutline(SDL_Renderer* r, int x, int y, int w, int h, int rad, int thick, Uint8 R, Uint8 G, Uint8 B, Uint8 A){
    for(int i=0;i<thick;i++){
        drawRoundedFilled(r, x+i, y+i, w-2*i, h-2*i, std::max(0,rad-i), R,G,B,A);
    }
}

// ===========================
//   UTILITÁRIOS
// ===========================
static std::string fmtScore(int v){
    std::string s=std::to_string(v),o; int c=0;
    for(int i=(int)s.size()-1;i>=0;--i){ o.push_back(s[i]); if(++c==3 && i>0){ o.push_back(' '); c=0; } }
    std::reverse(o.begin(), o.end()); return o;
}
static bool saveScreenshot(SDL_Renderer* ren, const char* path) {
    int w, h; SDL_GetRendererOutputSize(ren, &w, &h);
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 24, SDL_PIXELFORMAT_BGR24);
    if (!surf) return false;
    if (SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_BGR24, surf->pixels, surf->pitch) != 0) {
        SDL_FreeSurface(surf); return false;
    }
    int rc = SDL_SaveBMP(surf, path);
    SDL_FreeSurface(surf);
    return (rc == 0);
}

// ===========================
//   SISTEMA DE ÁUDIO MODULAR
// ===========================

/**
 * @brief Audio hardware management and basic synthesis
 * 
 * Handles SDL audio device initialization, cleanup, and basic sound generation
 */
class AudioDevice {
private:
    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec spec_;
    
public:
    AudioDevice() = default;
    
    bool initialize() {
        spec_.freq = 44100; 
        spec_.format = AUDIO_F32SYS; 
        spec_.channels = 1; 
        spec_.samples = 1024;
        device_ = SDL_OpenAudioDevice(nullptr, 0, &spec_, &spec_, 0);
        if (device_) SDL_PauseAudioDevice(device_, 0);
        return device_ != 0;
    }
    
    void cleanup() {
        if (device_) SDL_CloseAudioDevice(device_);
        device_ = 0;
    }
    
    bool isInitialized() const { return device_ != 0; }
    const SDL_AudioSpec& getSpec() const { return spec_; }
    
    void playBeep(double freq, int ms, float vol = 0.25f, bool square = true) {
        if (!device_) return;
        int N = (int)(spec_.freq * (ms / 1000.0));
        std::vector<float> buf(N);
        double ph = 0, st = 2.0 * M_PI * freq / spec_.freq;
        for (int i = 0; i < N; i++) {
            float s = square ? (std::sin(ph) >= 0 ? 1.f : -1.f) : (float)std::sin(ph);
            buf[i] = s * vol; 
            ph += st; 
            if (ph > 2 * M_PI) ph -= 2 * M_PI;
        }
        SDL_QueueAudio(device_, buf.data(), (Uint32)(buf.size() * sizeof(float)));
    }
    
    void playChord(double baseFreq, int notes[], int count, int ms, float vol = 0.15f) {
        if (!device_) return;
        for (int i = 0; i < count; i++) {
            playBeep(baseFreq * notes[i], ms, vol, false);
        }
    }
    
    void playArpeggio(double baseFreq, int notes[], int count, int noteMs, float vol = 0.12f) {
        if (!device_) return;
        for (int i = 0; i < count; i++) {
            playBeep(baseFreq * notes[i], noteMs, vol, false);
        }
    }
    
    void playSweep(double startFreq, double endFreq, int ms, float vol = 0.10f) {
        if (!device_) return;
        int steps = 20;
        for (int i = 0; i <= steps; i++) {
            double freq = startFreq + (endFreq - startFreq) * (i / (double)steps);
            playBeep(freq, ms / steps, vol, false);
        }
    }
};

// AudioConfig definition moved to earlier in file

/**
 * @brief Game-specific sound effects
 * 
 * Handles all game-specific sound effects and ambient sounds
 */
class SoundEffects {
private:
    AudioDevice* device_;
    AudioConfig* config_;
    
    // Timing for ambient sounds
    Uint32 lastSweepSound_ = 0;
    Uint32 lastScanlineSound_ = 0;
    Uint32 lastMelody_ = 0;
    Uint32 lastTension_ = 0;
    
public:
    SoundEffects(AudioDevice* device, AudioConfig* config) 
        : device_(device), config_(config) {}
    
    // Movement sounds
    void playMovementSound() {
        if (config_->enableMovementSounds) {
            device_->playBeep(150.0, 8, 0.06f * config_->masterVolume, true);
        }
    }
    
    void playRotationSound(bool cw = true) {
        if (config_->enableMovementSounds) {
            device_->playBeep(cw ? 350.0 : 300.0, 15, 0.10f * config_->masterVolume, false);
        }
    }
    
    void playSoftDropSound() {
        if (config_->enableMovementSounds) {
            device_->playBeep(200.0, 12, 0.08f * config_->masterVolume, true);
        }
    }
    
    void playHardDropSound() {
        if (config_->enableMovementSounds) {
            device_->playBeep(400.0, 20, 0.12f * config_->masterVolume, true);
        }
    }
    
    void playKickSound() {
        if (config_->enableMovementSounds) {
            device_->playBeep(250.0, 15, 0.08f * config_->masterVolume, true);
        }
    }
    
    // Game state sounds
    void playLevelUpSound() {
        if (config_->enableLevelUpSounds) {
            float vol = 0.25f * config_->masterVolume;
            device_->playBeep(880.0, 100, vol, false);
            device_->playBeep(1320.0, 80, vol * 0.8f, false);
        }
    }
    
    void playGameOverSound() {
        if (config_->enableLevelUpSounds) {
            float vol = 0.3f * config_->masterVolume;
            device_->playBeep(440.0, 200, vol, false);
            device_->playBeep(392.0, 200, vol, false);
            device_->playBeep(349.0, 200, vol, false);
            device_->playBeep(294.0, 300, vol * 1.33f, false);
        }
    }
    
    void playComboSound(int combo) {
        if (config_->enableComboSounds && combo > 1) {
            double freq = 440.0 + (combo * 50.0);
            float vol = (0.15f + combo * 0.02f) * config_->masterVolume * config_->sfxVolume;
            device_->playBeep(freq, 100 + combo * 20, vol, true);
        }
    }
    
    void playTetrisSound() {
        if (config_->enableComboSounds) {
            int notes[] = {1, 5, 8, 12}; // C, E, G, C (oitava)
            float vol = 0.20f * config_->masterVolume * config_->sfxVolume;
            device_->playArpeggio(220.0, notes, 4, 50, vol);
        }
    }
    
    // Ambient sounds
    void playBackgroundMelody(int level) {
        if (!config_->enableAmbientSounds) return;
        Uint32 now = SDL_GetTicks();
        if (now - lastMelody_ > 3000) {  // A cada 3 segundos
            double baseFreq = 220.0 + (level * 20.0);
            double melody[] = {1.0, 1.25, 1.5, 1.875, 2.0};  // Pentatônica
            
            for (int i = 0; i < 3; i++) {
                double freq = baseFreq * melody[i % 5];
                float vol = 0.05f * config_->ambientVolume * config_->masterVolume;
                device_->playBeep(freq, 200, vol, false);
            }
            lastMelody_ = now;
        }
    }
    
    void playTensionSound(int filledRows) {
        if (!config_->enableAmbientSounds || filledRows < 5) return;
        Uint32 now = SDL_GetTicks();
        if (now - lastTension_ > 1000) {
            float vol = 0.08f * config_->ambientVolume * config_->masterVolume;
            device_->playBeep(80.0, 300, vol, true);
            lastTension_ = now;
        }
    }
    
    void playSweepEffect() {
        if (!config_->enableAmbientSounds) return;
        Uint32 now = SDL_GetTicks();
        if (now - lastSweepSound_ > 2000) {  // A cada 2 segundos
            float vol = 0.03f * config_->ambientVolume * config_->masterVolume;
            device_->playBeep(50.0, 100, vol, false);
            lastSweepSound_ = now;
        }
    }
    
    void playScanlineEffect() {
        if (!config_->enableAmbientSounds) return;
        Uint32 now = SDL_GetTicks();
        if (now - lastScanlineSound_ > 5000) {  // A cada 5 segundos
            float vol = 0.02f * config_->ambientVolume * config_->masterVolume;
            device_->playBeep(15.0, 200, vol, true);
            lastScanlineSound_ = now;
        }
    }
};

/**
 * @brief Main audio system coordinator
 * 
 * Coordinates AudioDevice, AudioConfig, and SoundEffects
 */
struct AudioSystem : public IAudioSystem {
private:
    AudioDevice device_;
    AudioConfig config_;
    SoundEffects effects_;
    
public:
    AudioSystem() : effects_(&device_, &config_) {}
    
    // Initialization and cleanup
    bool initialize() {
        return device_.initialize();
    }
    
    void cleanup() {
        device_.cleanup();
    }
    
    // Configuration
    AudioConfig& getConfig() { return config_; }
    const AudioConfig& getConfig() const { return config_; }
    
    // Basic synthesis (delegated to device)
    void playBeep(double freq, int ms, float vol = 0.25f, bool square = true) {
        device_.playBeep(freq, ms, vol * config_.masterVolume, square);
    }
    
    void playChord(double baseFreq, int notes[], int count, int ms, float vol = 0.15f) {
        device_.playChord(baseFreq, notes, count, ms, vol * config_.masterVolume * config_.sfxVolume);
    }
    
    void playArpeggio(double baseFreq, int notes[], int count, int noteMs, float vol = 0.12f) {
        device_.playArpeggio(baseFreq, notes, count, noteMs, vol * config_.masterVolume * config_.sfxVolume);
    }
    
    void playSweep(double startFreq, double endFreq, int ms, float vol = 0.10f) {
        device_.playSweep(startFreq, endFreq, ms, vol * config_.masterVolume * config_.sfxVolume);
    }
    
    // Game-specific sounds (delegated to effects)
    void playMovementSound() { effects_.playMovementSound(); }
    void playRotationSound(bool cw = true) { effects_.playRotationSound(cw); }
    void playSoftDropSound() { effects_.playSoftDropSound(); }
    void playHardDropSound() { effects_.playHardDropSound(); }
    void playKickSound() { effects_.playKickSound(); }
    void playLevelUpSound() { effects_.playLevelUpSound(); }
    void playGameOverSound() { effects_.playGameOverSound(); }
    void playComboSound(int combo) { effects_.playComboSound(combo); }
    void playTetrisSound() { effects_.playTetrisSound(); }
    void playBackgroundMelody(int level) { effects_.playBackgroundMelody(level); }
    void playTensionSound(int filledRows) { effects_.playTensionSound(filledRows); }
    void playSweepEffect() { effects_.playSweepEffect(); }
    void playScanlineEffect() { effects_.playScanlineEffect(); }
    
    // Configuration loading (delegated to config)
    bool loadFromConfig(const std::string& key, const std::string& value) {
        return config_.loadFromConfig(key, value);
    }
    
    // Legacy compatibility - direct access to old fields
    float masterVolume = 1.0f;
    float sfxVolume = 0.6f;
    float ambientVolume = 0.3f;
    bool enableMovementSounds = true;
    bool enableAmbientSounds = true;
    bool enableComboSounds = true;
    bool enableLevelUpSounds = true;
};

// ===========================
//   IMPLEMENTAÇÕES DO SISTEMA DE CONFIGURAÇÃO
// ===========================

// Implementação do VisualConfigParser
bool VisualConfigParser::parseBool(const std::string& value) const {
    std::string v = value;
    for (char& c : v) c = (char)std::tolower((unsigned char)c);
    return (v == "1" || v == "true" || v == "on" || v == "yes");
}

int VisualConfigParser::parseInt(const std::string& value) const {
    return std::atoi(value.c_str());
}

float VisualConfigParser::parseFloat(const std::string& value) const {
    return (float)std::atof(value.c_str());
}

bool VisualConfigParser::parseHexColor(const std::string& value, RGB& color) const {
    std::string hex = value;
    if (hex[0] == '#') hex = hex.substr(1);
    if (hex.length() != 6) {
        return false;
    }
    
    try {
        color.r = (Uint8)std::stoi(hex.substr(0, 2), nullptr, 16);
        color.g = (Uint8)std::stoi(hex.substr(2, 2), nullptr, 16);
        color.b = (Uint8)std::stoi(hex.substr(4, 2), nullptr, 16);
        return true;
    } catch (...) {
        return false;
    }
}

bool VisualConfigParser::parse(const std::string& key, const std::string& value) {
    
    // Colors
    if (key == "BG") return parseHexColor(value, config_.colors.background);
    if (key == "BOARD_EMPTY") return parseHexColor(value, config_.colors.boardEmpty);
    if (key == "PANEL_FILL") return parseHexColor(value, config_.colors.panelFill);
    if (key == "PANEL_OUTLINE") return parseHexColor(value, config_.colors.panelOutline);
    if (key == "PANEL_OUTLINE_A") { config_.colors.panelOutlineAlpha = parseInt(value); return true; }
    
    // Banner
    if (key == "BANNER_BG") return parseHexColor(value, config_.colors.bannerBg);
    if (key == "BANNER_OUTLINE") return parseHexColor(value, config_.colors.bannerOutline);
    if (key == "BANNER_OUTLINE_A") { config_.colors.bannerOutlineAlpha = parseInt(value); return true; }
    if (key == "BANNER_TEXT") return parseHexColor(value, config_.colors.bannerText);
    
    // HUD
    if (key == "HUD_LABEL") return parseHexColor(value, config_.colors.hudLabel);
    if (key == "HUD_SCORE") return parseHexColor(value, config_.colors.hudScore);
    if (key == "HUD_LINES") return parseHexColor(value, config_.colors.hudLines);
    if (key == "HUD_LEVEL") return parseHexColor(value, config_.colors.hudLevel);
    
    // NEXT
    if (key == "NEXT_FILL") return parseHexColor(value, config_.colors.nextFill);
    if (key == "NEXT_OUTLINE") return parseHexColor(value, config_.colors.nextOutline);
    if (key == "NEXT_OUTLINE_A") { config_.colors.nextOutlineAlpha = parseInt(value); return true; }
    if (key == "NEXT_LABEL") return parseHexColor(value, config_.colors.nextLabel);
    if (key == "NEXT_GRID_DARK") return parseHexColor(value, config_.colors.nextGridDark);
    if (key == "NEXT_GRID_LIGHT") return parseHexColor(value, config_.colors.nextGridLight);
    if (key == "NEXT_GRID_USE_RGB") { config_.colors.nextGridUseRgb = parseBool(value); return true; }
    
    // Overlay
    if (key == "OVERLAY_FILL") return parseHexColor(value, config_.colors.overlayFill);
    if (key == "OVERLAY_FILL_A") { config_.colors.overlayFillAlpha = parseInt(value); return true; }
    if (key == "OVERLAY_OUTLINE") return parseHexColor(value, config_.colors.overlayOutline);
    if (key == "OVERLAY_OUTLINE_A") { config_.colors.overlayOutlineAlpha = parseInt(value); return true; }
    if (key == "OVERLAY_TOP") return parseHexColor(value, config_.colors.overlayTop);
    if (key == "OVERLAY_SUB") return parseHexColor(value, config_.colors.overlaySub);
    
    // Effects
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
    
    // Layout
    if (key == "ROUNDED_PANELS") { config_.layout.roundedPanels = parseInt(value); return true; }
    if (key == "HUD_FIXED_SCALE") { config_.layout.hudFixedScale = parseInt(value); return true; }
    if (key == "GAP1_SCALE") { config_.layout.gap1Scale = parseInt(value); return true; }
    if (key == "GAP2_SCALE") { config_.layout.gap2Scale = parseInt(value); return true; }
    
    // Text
    if (key == "TITLE_TEXT") { config_.titleText = value; return true; }
    
    return false;
}

bool VisualConfigParser::validate() const {
    // Validate ranges
    if (config_.effects.sweepAlphaMax < 0 || config_.effects.sweepAlphaMax > 255) return false;
    if (config_.effects.sweepGAlphaMax < 0 || config_.effects.sweepGAlphaMax > 255) return false;
    if (config_.effects.scanlineAlpha < 0 || config_.effects.scanlineAlpha > 255) return false;
    if (config_.effects.sweepSoftness < 0.0f || config_.effects.sweepSoftness > 1.0f) return false;
    if (config_.effects.sweepGSoftness < 0.0f || config_.effects.sweepGSoftness > 1.0f) return false;
    if (config_.layout.roundedPanels < 0) return false;
    if (config_.layout.hudFixedScale < 1) return false;
    if (config_.layout.gap1Scale < 0) return false;
    if (config_.layout.gap2Scale < 0) return false;
    
    return true;
}

// Implementação do AudioConfigParser
bool AudioConfigParser::parseBool(const std::string& value) const {
    std::string v = value;
    for (char& c : v) c = (char)std::tolower((unsigned char)c);
    return (v == "1" || v == "true" || v == "on" || v == "yes");
}

float AudioConfigParser::parseFloat(const std::string& value) const {
    float v = (float)std::atof(value.c_str());
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    return v;
}

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

bool AudioConfigParser::validate() const {
    if (!config_) return false;
    return (config_->masterVolume >= 0.0f && config_->masterVolume <= 1.0f) &&
           (config_->sfxVolume >= 0.0f && config_->sfxVolume <= 1.0f) &&
           (config_->ambientVolume >= 0.0f && config_->ambientVolume <= 1.0f);
}

// Implementação do InputConfigParser
bool InputConfigParser::parseBool(const std::string& value) const {
    std::string v = value;
    for (char& c : v) c = (char)std::tolower((unsigned char)c);
    return (v == "1" || v == "true" || v == "on" || v == "yes");
}

int InputConfigParser::parseInt(const std::string& value) const {
    return std::atoi(value.c_str());
}

float InputConfigParser::parseFloat(const std::string& value) const {
    return (float)std::atof(value.c_str());
}

bool InputConfigParser::parse(const std::string& key, const std::string& value) {
    // Button mapping
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
    
    // Analog settings
    if (key == "JOYSTICK_ANALOG_DEADZONE") { config_.analogDeadzone = parseFloat(value); return true; }
    if (key == "JOYSTICK_ANALOG_SENSITIVITY") { config_.analogSensitivity = parseFloat(value); return true; }
    if (key == "JOYSTICK_INVERT_Y_AXIS") { config_.invertYAxis = parseBool(value); return true; }
    
    // Timing
    if (key == "JOYSTICK_MOVE_REPEAT_DELAY") { config_.moveRepeatDelay = parseInt(value); return true; }
    if (key == "JOYSTICK_SOFT_DROP_REPEAT_DELAY") { config_.softDropRepeatDelay = parseInt(value); return true; }
    
    return false;
}

bool InputConfigParser::validate() const {
    return (config_.analogDeadzone >= 0.0f && config_.analogDeadzone <= 1.0f) &&
           (config_.analogSensitivity >= 0.0f && config_.analogSensitivity <= 2.0f) &&
           (config_.buttonLeft >= 0 && config_.buttonLeft < 32) &&
           (config_.buttonRight >= 0 && config_.buttonRight < 32) &&
           (config_.buttonDown >= 0 && config_.buttonDown < 32) &&
           (config_.buttonUp >= 0 && config_.buttonUp < 32) &&
           (config_.buttonRotateCCW >= 0 && config_.buttonRotateCCW < 32) &&
           (config_.buttonRotateCW >= 0 && config_.buttonRotateCW < 32) &&
           (config_.buttonSoftDrop >= 0 && config_.buttonSoftDrop < 32) &&
           (config_.buttonHardDrop >= 0 && config_.buttonHardDrop < 32) &&
           (config_.buttonPause >= 0 && config_.buttonPause < 32) &&
           (config_.buttonStart >= 0 && config_.buttonStart < 32) &&
           (config_.buttonQuit >= 0 && config_.buttonQuit < 32);
}

// Implementação do PiecesConfigParser
int PiecesConfigParser::parseInt(const std::string& value) const {
    return std::atoi(value.c_str());
}

bool PiecesConfigParser::parseHexColor(const std::string& value, RGB& color) const {
    std::string hex = value;
    if (hex[0] == '#') hex = hex.substr(1);
    if (hex.length() != 6) return false;
    
    try {
        color.r = (Uint8)std::stoi(hex.substr(0, 2), nullptr, 16);
        color.g = (Uint8)std::stoi(hex.substr(2, 2), nullptr, 16);
        color.b = (Uint8)std::stoi(hex.substr(4, 2), nullptr, 16);
        return true;
    } catch (...) {
        return false;
    }
}

bool PiecesConfigParser::parse(const std::string& key, const std::string& value) {
    if (key == "PIECES_FILE") { config_.piecesFilePath = value; return true; }
    if (key == "PREVIEW_GRID") { config_.previewGrid = parseInt(value); return true; }
    if (key == "RAND_TYPE") { config_.randomizerType = value; return true; }
    if (key == "RAND_BAG_SIZE") { config_.randBagSize = parseInt(value); return true; }
    
    // Piece colors (PIECE0, PIECE1, etc.)
    if (key.rfind("PIECE", 0) == 0) {
        std::string numStr = key.substr(5);
        int pieceIndex = -1;
        try {
            pieceIndex = std::stoi(numStr);
        } catch (...) {
            return false;
        }
        
        RGB color;
        if (parseHexColor(value, color)) {
            if (pieceIndex >= (int)config_.pieceColors.size()) {
                config_.pieceColors.resize(pieceIndex + 1, {200, 200, 200});
            }
            config_.pieceColors[pieceIndex] = color;
            return true;
        }
    }
    
    return false;
}

bool PiecesConfigParser::validate() const {
    return (config_.previewGrid >= 4 && config_.previewGrid <= 12) &&
           (config_.randBagSize >= 0 && config_.randBagSize <= 20) &&
           (config_.randomizerType == "simple" || config_.randomizerType == "bag");
}

// Implementação do GameConfigParser
int GameConfigParser::parseInt(const std::string& value) const {
    return std::atoi(value.c_str());
}

float GameConfigParser::parseFloat(const std::string& value) const {
    return (float)std::atof(value.c_str());
}

bool GameConfigParser::parse(const std::string& key, const std::string& value) {
    if (key == "TICK_MS_START") { config_.tickMsStart = parseInt(value); return true; }
    if (key == "TICK_MS_MIN") { config_.tickMsMin = parseInt(value); return true; }
    if (key == "SPEED_ACCELERATION") { config_.speedAcceleration = parseInt(value); return true; }
    if (key == "LEVEL_STEP") { config_.levelStep = parseInt(value); return true; }
    if (key == "ASPECT_CORRECTION_FACTOR") { config_.aspectCorrectionFactor = parseFloat(value); return true; }
    
    return false;
}

bool GameConfigParser::validate() const {
    return (config_.tickMsStart > 0) &&
           (config_.tickMsMin > 0) &&
           (config_.speedAcceleration > 0) &&
           (config_.levelStep > 0) &&
           (config_.aspectCorrectionFactor > 0.0f && config_.aspectCorrectionFactor <= 2.0f);
}

// Implementação do ConfigManager
bool ConfigManager::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.good()) {
        DebugLogger::error("Failed to open config file: " + path);
        return false;
    }
    
    configPaths_.push_back(path);
    
    // Create parsers
    VisualConfigParser visualParser(visual_);
    AudioConfigParser audioParser(&audio_);
    InputConfigParser inputParser(input_);
    PiecesConfigParser piecesParser(pieces_);
    GameConfigParser gameParser(game_);
    
    std::vector<ConfigParser*> parsers = {&visualParser, &audioParser, &inputParser, &piecesParser, &gameParser};
    
    std::string line;
    int lineNum = 0;
    int processedLines = 0;
    int skippedLines = 0;
    
    while (std::getline(file, line)) {
        lineNum++;
        
        
        // Parse line (remove comments) - but only if # is at the beginning of the line or after whitespace
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            // Check if # is at the beginning or after only whitespace
            std::string beforeHash = line.substr(0, commentPos);
            beforeHash.erase(0, beforeHash.find_first_not_of(" \t"));
            if (beforeHash.empty()) {
                line = line.substr(0, commentPos);
            }
        }
        
        // Also remove ; comments (inline comments)
        size_t semiPos = line.find(';');
        if (semiPos != std::string::npos) {
            line = line.substr(0, semiPos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) {
            skippedLines++;
            continue;
        }
        
        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            skippedLines++;
            continue;
        }
        
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        
        
        // Trim key and value
        trim(key);
        trim(value);
        
        
        if (key.empty()) {
            skippedLines++;
            continue;
        }
        
        // Convert key to uppercase
        for (char& c : key) c = (char)std::toupper((unsigned char)c);
        
        // Try each parser
        bool parsed = false;
        for (auto* parser : parsers) {
            if (parser->parse(key, value)) {
                parsed = true;
                processedLines++;
                break;
            }
        }
        
        if (!parsed) {
            skippedLines++;
            DebugLogger::warning("Unknown config key: " + key);
        }
    }
    
    loaded_ = true;
    return true;
}

bool ConfigManager::loadFromEnvironment() {
    if (const char* env = std::getenv("DROPBLOCKS_CFG")) {
        return loadFromFile(env);
    }
    return false;
}

bool ConfigManager::loadFromCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.length() > 4 && arg.substr(arg.length() - 4) == ".cfg") {
            return loadFromFile(arg);
        }
    }
    return false;
}

bool ConfigManager::loadAll() {
    // Try environment variable first
    if (loadFromEnvironment()) return true;
    
    // Try command line
    if (loadFromCommandLine(0, nullptr)) return true;
    
    // Try default files
    if (loadFromFile("default.cfg")) return true;
    if (loadFromFile("dropblocks.cfg")) return true;
    
    // Try home directory
    if (const char* home = std::getenv("HOME")) {
        std::string homeConfig = std::string(home) + "/.config/default.cfg";
        if (loadFromFile(homeConfig)) return true;
        
        homeConfig = std::string(home) + "/.config/dropblocks.cfg";
        if (loadFromFile(homeConfig)) return true;
    }
    
    loaded_ = true;
    return true;
}

void ConfigManager::setOverride(const std::string& key, const std::string& value) {
    overrides_[key] = value;
}

void ConfigManager::clearOverrides() {
    overrides_.clear();
}

bool ConfigManager::validate() const {
    VisualConfigParser visualParser(const_cast<VisualConfig&>(visual_));
    AudioConfigParser audioParser(const_cast<AudioConfig*>(&audio_));
    InputConfigParser inputParser(const_cast<InputConfig&>(input_));
    PiecesConfigParser piecesParser(const_cast<PiecesConfig&>(pieces_));
    GameConfigParser gameParser(const_cast<GameConfig&>(game_));
    
    return visualParser.validate() && audioParser.validate() && 
           inputParser.validate() && piecesParser.validate() && 
           gameParser.validate();
}

// ===========================
//   FUNÇÕES DE APLICAÇÃO DE CONFIGURAÇÃO
// ===========================

/**
 * @brief Apply audio configuration to AudioSystem
 */
static void applyConfigToAudio(AudioSystem& audio, const AudioConfig& config) {
    audio.masterVolume = config.masterVolume;
    audio.sfxVolume = config.sfxVolume;
    audio.ambientVolume = config.ambientVolume;
    audio.enableMovementSounds = config.enableMovementSounds;
    audio.enableAmbientSounds = config.enableAmbientSounds;
    audio.enableComboSounds = config.enableComboSounds;
    audio.enableLevelUpSounds = config.enableLevelUpSounds;
}

/**
 * @brief Apply visual configuration to global theme
 */
static void applyConfigToTheme(const VisualConfig& config) {
    
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
    ENABLE_BANNER_SWEEP = config.effects.bannerSweep;
    ENABLE_GLOBAL_SWEEP = config.effects.globalSweep;
    SWEEP_SPEED_PXPS = config.effects.sweepSpeedPxps;
    SWEEP_BAND_H_S = config.effects.sweepBandHS;
    SWEEP_ALPHA_MAX = config.effects.sweepAlphaMax;
    SWEEP_SOFTNESS = config.effects.sweepSoftness;
    SWEEP_G_SPEED_PXPS = config.effects.sweepGSpeedPxps;
    SWEEP_G_BAND_H_PX = config.effects.sweepGBandHPx;
    SWEEP_G_ALPHA_MAX = config.effects.sweepGAlphaMax;
    SWEEP_G_SOFTNESS = config.effects.sweepGSoftness;
    SCANLINE_ALPHA = config.effects.scanlineAlpha;
    
    // Apply layout
    ROUNDED_PANELS = config.layout.roundedPanels;
    HUD_FIXED_SCALE = config.layout.hudFixedScale;
    GAP1_SCALE = config.layout.gap1Scale;
    GAP2_SCALE = config.layout.gap2Scale;
    
    // Apply text
    TITLE_TEXT = config.titleText;
}


// Implementação da função processAudioConfigs
static bool processAudioConfigs(const std::string& key, const std::string& val, int& processedLines, AudioSystem& audio) {
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


// Implementação da função rotateWithKicks
static void rotateWithKicks(Active& act, const std::vector<std::vector<Cell>>& grid, int dir, AudioSystem& audio){ // +1 = CW, -1 = CCW
    int from = act.rot;
    int to   = (from + dir + 4) % 4;
    auto& p  = PIECES[act.idx];

    // 1) SRS por transição (preferência)
    if(p.hasPerTransKicks){
        int dirIdx = (dir>0? 0 : 1); // 0=CW, 1=CCW
        const auto& lst = p.kicksPerTrans[dirIdx][from];
        for(auto [kx,ky] : lst){
            if(!collides(act, grid, kx, ky, dir)){
                act.x += kx; act.y += ky; act.rot = to; 
                if(kx != 0 || ky != 0) audio.playKickSound();  // Som de kick
                return;
            }
        }
    }

    // 2) Fallback: lista única por direção (legado)
    if(p.hasKicks){
        const auto& lst = (dir>0? p.kicksCW : p.kicksCCW);
        for(auto [kx,ky] : lst){
            if(!collides(act, grid, kx, ky, dir)){
                act.x += kx; act.y += ky; act.rot = to; 
                if(kx != 0 || ky != 0) audio.playKickSound();  // Som de kick
                return;
            }
        }
    }

    // 3) Fallback simples
    if(!collides(act, grid, 0, 0, dir)) { act.rot = to; return; }
    int sx = (dir>0?1:-1);
    if(!collides(act, grid, sx, 0, dir)) { act.x += sx; act.rot = to; return; }
    if(!collides(act, grid, 0,-1, dir)) { act.y -= 1; act.rot = to; return; }
}

// ===========================
//   FUNÇÕES AUXILIARES DO JOGO
// ===========================

// Estrutura para sistema de combos
struct ComboSystem {
    int combo = 0;
    Uint32 lastClear = 0;
    
    void onLineClear(AudioSystem& audio) {
        Uint32 now = SDL_GetTicks();
        if (now - lastClear < 2000) {  // Combo ativo (2 segundos)
            combo++;
        } else {
            combo = 1;
        }
        lastClear = now;
        
        // Som de combo
        audio.playComboSound(combo);
    }
    
    void reset() {
        combo = 0;
        lastClear = 0;
    }
};

// ===========================
//   SISTEMA DE INPUT UNIFICADO
// ===========================

/**
 * @brief Abstract input handler interface
 * 
 * Provides a unified interface for all input methods (keyboard, joystick, etc.)
 */
class InputHandler {
public:
    virtual ~InputHandler() = default;
    
    // Core input actions
    virtual bool shouldMoveLeft() = 0;
    virtual bool shouldMoveRight() = 0;
    virtual bool shouldSoftDrop() = 0;
    virtual bool shouldHardDrop() = 0;
    virtual bool shouldRotateCCW() = 0;
    virtual bool shouldRotateCW() = 0;
    virtual bool shouldPause() = 0;
    virtual bool shouldRestart() = 0;
    virtual bool shouldQuit() = 0;
    virtual bool shouldScreenshot() = 0;
    
    // System methods
    virtual void update() = 0;
    virtual bool isConnected() = 0;
    virtual void resetTimers() = 0;
};

/**
 * @brief Keyboard input handler
 * 
 * Handles keyboard input using SDL events
 */
class KeyboardInput : public InputHandler {
private:
    bool keyStates[SDL_NUM_SCANCODES] = {false};
    bool lastKeyStates[SDL_NUM_SCANCODES] = {false};
    Uint32 lastMoveTime = 0;
    Uint32 moveRepeatDelay = 200;
    
public:
    bool shouldMoveLeft() override {
        return isKeyPressed(SDL_SCANCODE_LEFT) && SDL_GetTicks() - lastMoveTime > moveRepeatDelay;
    }
    
    bool shouldMoveRight() override {
        return isKeyPressed(SDL_SCANCODE_RIGHT) && SDL_GetTicks() - lastMoveTime > moveRepeatDelay;
    }
    
    bool shouldSoftDrop() override {
        return isKeyPressed(SDL_SCANCODE_DOWN);
    }
    
    bool shouldHardDrop() override {
        return isKeyPressed(SDL_SCANCODE_SPACE);
    }
    
    bool shouldRotateCCW() override {
        return isKeyPressed(SDL_SCANCODE_Z) || isKeyPressed(SDL_SCANCODE_UP);
    }
    
    bool shouldRotateCW() override {
        return isKeyPressed(SDL_SCANCODE_X);
    }
    
    bool shouldPause() override {
        return isKeyPressed(SDL_SCANCODE_P);
    }
    
    bool shouldRestart() override {
        return isKeyPressed(SDL_SCANCODE_RETURN);
    }
    
    bool shouldQuit() override {
        return isKeyPressed(SDL_SCANCODE_ESCAPE);
    }
    
    bool shouldScreenshot() override {
        return isKeyPressed(SDL_SCANCODE_F12);
    }
    
    void update() override {
        // Update key states
        for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
            lastKeyStates[i] = keyStates[i];
            keyStates[i] = SDL_GetKeyboardState(nullptr)[i];
        }
    }
    
    bool isConnected() override {
        return true; // Keyboard is always available
    }
    
    void resetTimers() override {
        lastMoveTime = SDL_GetTicks();
    }
    
private:
    bool isKeyPressed(SDL_Scancode key) {
        return keyStates[key] && !lastKeyStates[key];
    }
};


/**
 * @brief Input manager for handling multiple input sources
 * 
 * Manages multiple input handlers and provides unified input processing
 */
class InputManager : public IInputManager {
private:
    std::vector<std::unique_ptr<InputHandler>> handlers;
    InputHandler* primaryHandler = nullptr;
    
public:
    void addHandler(std::unique_ptr<InputHandler> handler) {
        handlers.push_back(std::move(handler));
        if (!primaryHandler) {
            primaryHandler = handlers.back().get();
        }
    }
    
    void setPrimaryHandler(InputHandler* handler) {
        primaryHandler = handler;
    }
    
    void update() {
        for (auto& handler : handlers) {
            handler->update();
        }
    }
    
    // Access to handlers for configuration
    std::vector<std::unique_ptr<InputHandler>>& getHandlers() {
        return handlers;
    }
    
    // Unified input methods - use primary handler or first available
    InputHandler* getActiveHandler() {
        // Se o handler primário está conectado, use ele
        if (primaryHandler && primaryHandler->isConnected()) {
            return primaryHandler;
        }
        
        for (auto& handler : handlers) {
            if (handler->isConnected()) {
                return handler.get();
            }
        }
        
        return nullptr;
    }
    
    // Debug method to check which handler is active
    void debugActiveHandler() {
        // Debug info removed for verbosity reduction
    }
    
    bool shouldMoveLeft() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldMoveLeft()) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldMoveRight() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldMoveRight()) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldSoftDrop() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldSoftDrop()) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldHardDrop() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldHardDrop()) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldRotateCCW() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldRotateCCW()) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldRotateCW() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldRotateCW()) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldPause() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldPause()) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldRestart() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldRestart()) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldQuit() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldQuit()) {
                return true;
            }
        }
        return false;
    }
    
    bool shouldScreenshot() {
        // Verificar todos os handlers conectados
        for (auto& handler : handlers) {
            if (handler->isConnected() && handler->shouldScreenshot()) {
                return true;
            }
        }
        return false;
    }
    
    void resetTimers() {
        auto handler = getActiveHandler();
        if (handler) handler->resetTimers();
    }
    
    void cleanup(); // Declaração - implementação após definição de JoystickInput
};

// ===========================
//   SISTEMA DE JOYSTICK MODULAR
// ===========================

/**
 * @brief Joystick device management
 * 
 * Handles SDL joystick/controller detection and connection
 */
class JoystickDevice {
private:
    SDL_Joystick* joystick_ = nullptr;
    SDL_GameController* controller_ = nullptr;
    int joystickId_ = -1;
    bool isConnected_ = false;
    std::string deviceName_;
    
public:
    ~JoystickDevice() { cleanup(); }
    
    // Device management
    bool initialize() {
        // Verificar se SDL joystick subsystem está inicializado
        if (!SDL_WasInit(SDL_INIT_JOYSTICK)) {
            DebugLogger::error("SDL_INIT_JOYSTICK not initialized!");
            return false;
        }
        
        int numJoysticks = SDL_NumJoysticks();
        if (numJoysticks > 0) {
            if (SDL_IsGameController(0)) {
                controller_ = SDL_GameControllerOpen(0);
                if (controller_) {
                    joystick_ = SDL_GameControllerGetJoystick(controller_);
                    joystickId_ = 0;
                    isConnected_ = true;
                    deviceName_ = SDL_GameControllerName(controller_);
                    DebugLogger::info("Game controller connected: " + deviceName_);
                    return true;
                } else {
                    DebugLogger::error("Failed to open game controller: " + std::string(SDL_GetError()));
                }
            }
            
            joystick_ = SDL_JoystickOpen(0);
            if (joystick_) {
                joystickId_ = 0;
                isConnected_ = true;
                deviceName_ = SDL_JoystickName(joystick_);
                DebugLogger::info("Joystick connected: " + deviceName_);
                return true;
            } else {
                DebugLogger::error("Failed to open joystick: " + std::string(SDL_GetError()));
            }
        }
        
        DebugLogger::warning("No joystick/controller found");
        return false;
    }
    
    void cleanup() {
        if (controller_) {
            SDL_GameControllerClose(controller_);
            controller_ = nullptr;
        }
        if (joystick_) {
            SDL_JoystickClose(joystick_);
            joystick_ = nullptr;
        }
        isConnected_ = false;
        deviceName_.clear();
    }
    
    // Getters
    SDL_Joystick* getJoystick() const { return joystick_; }
    SDL_GameController* getController() const { return controller_; }
    int getJoystickId() const { return joystickId_; }
    bool isConnected() const { return isConnected_; }
    const std::string& getDeviceName() const { return deviceName_; }
};

/**
 * @brief Joystick configuration management
 * 
 * Handles button mapping and analog settings
 */
class JoystickConfig {
public:
    // Button mapping
    int buttonLeft = 13;      // D-pad left (padrão)
    int buttonRight = 11;     // D-pad right (padrão)
    int buttonDown = 14;      // D-pad down (padrão)
    int buttonUp = 12;        // D-pad up (padrão)
    int buttonRotateCCW = 0;  // A button (padrão)
    int buttonRotateCW = 1;   // B button (padrão)
    int buttonSoftDrop = 2;   // X button (padrão)
    int buttonHardDrop = 3;   // Y button (padrão)
    int buttonPause = 6;      // Back button (padrão)
    int buttonStart = 7;      // Start button (padrão)
    int buttonQuit = 8;       // Guide button (padrão)
    
    // Analog settings
    float analogDeadzone = 0.3f;     // Zona morta para analógico
    float analogSensitivity = 1.0f;  // Sensibilidade do analógico
    bool invertYAxis = false;        // Inverter eixo Y (padrão: false)
    
    // Timing settings
    Uint32 moveRepeatDelay = 200;        // ms entre movimentos repetidos
    Uint32 softDropRepeatDelay = 100;    // ms entre soft drops repetidos
    
    // Configuration methods
    void setButtonMapping(int left, int right, int down, int up, int rotCCW, int rotCW, 
                         int softDrop, int hardDrop, int pause, int start, int quit) {
        buttonLeft = left; buttonRight = right; buttonDown = down; buttonUp = up;
        buttonRotateCCW = rotCCW; buttonRotateCW = rotCW;
        buttonSoftDrop = softDrop; buttonHardDrop = hardDrop;
        buttonPause = pause; buttonStart = start; buttonQuit = quit;
    }
    
    void setAnalogSettings(float deadzone, float sensitivity, bool invertY) {
        analogDeadzone = deadzone;
        analogSensitivity = sensitivity;
        invertYAxis = invertY;
    }
    
    void setTiming(Uint32 moveDelay, Uint32 softDropDelay) {
        moveRepeatDelay = moveDelay;
        softDropRepeatDelay = softDropDelay;
    }
};

/**
 * @brief Joystick state management
 * 
 * Tracks current state of buttons and analog inputs
 */
class JoystickState {
public:
    // Button states (for press detection)
    bool buttonStates[32] = {false};
    bool lastButtonStates[32] = {false};
    
    // Analog states
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;
    
    // Timers
    Uint32 lastMoveTime = 0;
    Uint32 lastSoftDropTime = 0;
    
    // State management
    void updateButtonStates(const JoystickDevice& device, const JoystickConfig& config) {
        // Copy current states to last states
        for (int i = 0; i < 32; i++) {
            lastButtonStates[i] = buttonStates[i];
        }
        
        // Update current button states
        // Always use joystick API for generic joysticks, even if detected as game controller
        if (device.getJoystick()) {
            // Regular joystick buttons - this works for both generic joysticks and game controllers
            for (int i = 0; i < 32; i++) {
                buttonStates[i] = SDL_JoystickGetButton(device.getJoystick(), i);
            }
        }
        
        // Update analog states
        // Always use joystick API for generic joysticks, even if detected as game controller
        if (device.getJoystick()) {
            // Regular joystick axes - this works for both generic joysticks and game controllers
            leftStickX = SDL_JoystickGetAxis(device.getJoystick(), 0) / 32767.0f;
            leftStickY = SDL_JoystickGetAxis(device.getJoystick(), 1) / 32767.0f;
            rightStickX = SDL_JoystickGetAxis(device.getJoystick(), 2) / 32767.0f;
            rightStickY = SDL_JoystickGetAxis(device.getJoystick(), 3) / 32767.0f;
            
            // Apply sensitivity and invert Y if needed
            leftStickX *= config.analogSensitivity;
            leftStickY *= config.analogSensitivity;
            rightStickX *= config.analogSensitivity;
            rightStickY *= config.analogSensitivity;
            
            if (config.invertYAxis) {
                leftStickY = -leftStickY;
                rightStickY = -rightStickY;
            }
        }
    }
    
    bool isButtonPressed(int button) const {
        if (button < 0 || button >= 32) return false;
        return buttonStates[button] && !lastButtonStates[button];
    }
    
    void resetTimers() {
        lastMoveTime = SDL_GetTicks();
    }
};

/**
 * @brief Joystick input processor
 * 
 * Processes joystick input and converts to game actions
 */
class JoystickInputProcessor {
private:
    const JoystickConfig& config_;
    const JoystickState& state_;
    const JoystickDevice& device_;
    
public:
    JoystickInputProcessor(const JoystickConfig& config, const JoystickState& state, const JoystickDevice& device)
        : config_(config), state_(state), device_(device) {}
    
    bool shouldMoveLeft() const {
        return state_.isButtonPressed(config_.buttonLeft) || 
               (state_.leftStickX < -config_.analogDeadzone && SDL_GetTicks() - state_.lastMoveTime > config_.moveRepeatDelay) ||
               (device_.getController() && SDL_GameControllerGetButton(device_.getController(), SDL_CONTROLLER_BUTTON_DPAD_LEFT));
    }
    
    bool shouldMoveRight() const {
        return state_.isButtonPressed(config_.buttonRight) || 
               (state_.leftStickX > config_.analogDeadzone && SDL_GetTicks() - state_.lastMoveTime > config_.moveRepeatDelay) ||
               (device_.getController() && SDL_GameControllerGetButton(device_.getController(), SDL_CONTROLLER_BUTTON_DPAD_RIGHT));
    }
    
    bool shouldSoftDrop() const {
        return state_.isButtonPressed(config_.buttonSoftDrop) || 
               (state_.leftStickY > config_.analogDeadzone && SDL_GetTicks() - state_.lastSoftDropTime > config_.softDropRepeatDelay) ||
               (device_.getController() && SDL_GameControllerGetButton(device_.getController(), SDL_CONTROLLER_BUTTON_DPAD_DOWN));
    }
    
    bool shouldHardDrop() const {
        return state_.isButtonPressed(config_.buttonHardDrop);
    }
    
    bool shouldRotateCCW() const {
        return state_.isButtonPressed(config_.buttonRotateCCW) || 
               (state_.leftStickY < -config_.analogDeadzone && SDL_GetTicks() - state_.lastMoveTime > config_.moveRepeatDelay) ||
               (device_.getController() && SDL_GameControllerGetButton(device_.getController(), SDL_CONTROLLER_BUTTON_DPAD_UP));
    }
    
    bool shouldRotateCW() const {
        return state_.isButtonPressed(config_.buttonRotateCW) || 
               (state_.rightStickX > config_.analogDeadzone);
    }
    
    bool shouldPause() const {
        return state_.isButtonPressed(config_.buttonPause);
    }
    
    bool shouldRestart() const {
        return state_.isButtonPressed(config_.buttonStart);
    }
    
    bool shouldQuit() const {
        return state_.isButtonPressed(config_.buttonQuit);
    }
    
    bool shouldScreenshot() const {
        return false; // Screenshot not supported on joystick
    }
};

/**
 * @brief Main joystick system coordinator
 * 
 * Coordinates joystick device, configuration, state, and input processing
 */
class JoystickSystem {
private:
    JoystickDevice device_;
    JoystickConfig config_;
    JoystickState state_;
    std::unique_ptr<JoystickInputProcessor> processor_;
    
public:
    JoystickSystem() {
        processor_ = std::make_unique<JoystickInputProcessor>(config_, state_, device_);
    }
    
    // System management
    bool initialize() {
        return device_.initialize();
    }
    
    void cleanup() {
        device_.cleanup();
    }
    
    void update() {
        if (device_.isConnected()) {
            state_.updateButtonStates(device_, config_);
            
        }
    }
    
    // Configuration
    JoystickConfig& getConfig() { return config_; }
    const JoystickConfig& getConfig() const { return config_; }
    
    // State access
    const JoystickState& getState() const { return state_; }
    const JoystickDevice& getDevice() const { return device_; }
    
    // Input processing
    bool shouldMoveLeft() const { return processor_->shouldMoveLeft(); }
    bool shouldMoveRight() const { return processor_->shouldMoveRight(); }
    bool shouldSoftDrop() const { return processor_->shouldSoftDrop(); }
    bool shouldHardDrop() const { return processor_->shouldHardDrop(); }
    bool shouldRotateCCW() const { return processor_->shouldRotateCCW(); }
    bool shouldRotateCW() const { return processor_->shouldRotateCW(); }
    bool shouldPause() const { return processor_->shouldPause(); }
    bool shouldRestart() const { return processor_->shouldRestart(); }
    bool shouldQuit() const { return processor_->shouldQuit(); }
    bool shouldScreenshot() const { return processor_->shouldScreenshot(); }
    
    // Connection status
    bool isConnected() const { return device_.isConnected(); }
    
    // Timer management
    void resetTimers() { state_.resetTimers(); }
    
};

/**
 * @brief Joystick input handler
 * 
 * Handles joystick/controller input with analog and digital support
 * Now uses the modular JoystickSystem internally
 */
class JoystickInput : public InputHandler {
private:
    std::unique_ptr<JoystickSystem> joystickSystem_;
    
public:
    JoystickInput() : joystickSystem_(std::make_unique<JoystickSystem>()) {}
    
    bool shouldMoveLeft() override {
        return joystickSystem_ ? joystickSystem_->shouldMoveLeft() : false;
    }
    
    bool shouldMoveRight() override {
        return joystickSystem_ ? joystickSystem_->shouldMoveRight() : false;
    }
    
    bool shouldSoftDrop() override {
        return joystickSystem_ ? joystickSystem_->shouldSoftDrop() : false;
    }
    
    bool shouldHardDrop() override {
        return joystickSystem_ ? joystickSystem_->shouldHardDrop() : false;
    }
    
    bool shouldRotateCCW() override {
        return joystickSystem_ ? joystickSystem_->shouldRotateCCW() : false;
    }
    
    bool shouldRotateCW() override {
        return joystickSystem_ ? joystickSystem_->shouldRotateCW() : false;
    }
    
    bool shouldPause() override {
        return joystickSystem_ ? joystickSystem_->shouldPause() : false;
    }
    
    bool shouldRestart() override {
        return joystickSystem_ ? joystickSystem_->shouldRestart() : false;
    }
    
    bool shouldQuit() override {
        return joystickSystem_ ? joystickSystem_->shouldQuit() : false;
    }
    
    bool shouldScreenshot() override {
        return false; // Screenshot not supported on joystick
    }
    
    void update() override {
        if (joystickSystem_) {
            joystickSystem_->update();
        }
    }
    
    bool isConnected() override {
        return joystickSystem_ ? joystickSystem_->isConnected() : false;
    }
    
    void resetTimers() override {
        if (joystickSystem_) {
            joystickSystem_->resetTimers();
        }
    }
    
    bool initialize() {
        return joystickSystem_ ? joystickSystem_->initialize() : false;
    }
    
    void cleanup() {
        if (joystickSystem_) {
            joystickSystem_->cleanup();
        }
    }
    
    // Configuration access
    JoystickConfig& getConfig() {
        return joystickSystem_ ? joystickSystem_->getConfig() : throw std::runtime_error("JoystickSystem not initialized");
    }
    
    // Check if joystick has active input (for fallback to keyboard)
    bool hasActiveInput() {
        if (!joystickSystem_) return false;
        
        const auto& state = joystickSystem_->getState();
        const auto& config = joystickSystem_->getConfig();
        
        // Verificar se há botões pressionados ou movimento analógico
        for (int i = 0; i < 32; i++) {
            if (state.buttonStates[i]) return true;
        }
        
        // Verificar movimento analógico
        if (std::abs(state.leftStickX) > config.analogDeadzone || 
            std::abs(state.leftStickY) > config.analogDeadzone) {
            return true;
        }
        
        return false;
    }
};

// Implementação completa do cleanup do InputManager (após definição de JoystickInput)
void InputManager::cleanup() {
    for (auto& handler : handlers) {
        if (auto* joystick = dynamic_cast<JoystickInput*>(handler.get())) {
            joystick->cleanup();
        }
    }
    handlers.clear();
    primaryHandler = nullptr;
}

// processJoystickConfigs moved after pieceManager definition

// ===========================
//   SISTEMA DE GAME STATE MODULAR
// ===========================

/**
 * @brief Game board management system
 * 
 * Handles the game grid, piece placement, collision detection, and line clearing
 */
class GameBoard {
private:
    std::vector<std::vector<Cell>> grid_;
    
public:
    GameBoard() : grid_(ROWS, std::vector<Cell>(COLS)) {}
    
    // Grid access
    const std::vector<std::vector<Cell>>& getGrid() const { return grid_; }
    std::vector<std::vector<Cell>>& getGrid() { return grid_; }
    
    // Piece placement and collision
    bool canPlacePiece(const Active& piece, int dx, int dy, int drot) const {
        return !collides(piece, grid_, dx, dy, drot);
    }
    
    void placePiece(const Active& piece) {
        lockPiece(piece, grid_);
    }
    
    // Line clearing
    int clearLines() {
        int linesCleared = 0;
        for (int y = ROWS - 1; y >= 0; y--) {
            bool fullLine = true;
            for (int x = 0; x < COLS; x++) {
                if (!grid_[y][x].occ) {
                    fullLine = false;
                    break;
                }
            }
            
            if (fullLine) {
                grid_.erase(grid_.begin() + y);
                grid_.insert(grid_.begin(), std::vector<Cell>(COLS));
                linesCleared++;
                y++; // Check the same line again
            }
        }
        return linesCleared;
    }
    
    // Game over detection
    bool isGameOver(const Active& piece) const {
        return collides(piece, grid_, 0, 0, 0);
    }
    
    // Board state
    void reset() {
        for (auto& row : grid_) {
            for (auto& cell : row) {
                cell.occ = false;
            }
        }
    }
    
    // Tension detection for audio
    int getTensionLevel() const {
        int filledRows = 0;
        for (int y = ROWS - 5; y < ROWS; y++) {  // Últimas 5 linhas
            bool hasBlocks = false;
            for (int x = 0; x < COLS; x++) {
                if (grid_[y][x].occ) { 
                    hasBlocks = true; 
                    break; 
                }
            }
            if (hasBlocks) filledRows++;
        }
        return filledRows;
    }
    
    // Check tension and play audio
    void checkTension(AudioSystem& audio) const {
        int tensionLevel = getTensionLevel();
        audio.playTensionSound(tensionLevel);
    }
};

/**
 * @brief Score and level management system
 * 
 * Handles scoring, level progression, and game speed
 */
class ScoreSystem {
private:
    int score_ = 0;
    int lines_ = 0;
    int level_ = 0;
    int tickMs_ = gameConfig.tickMsStart;
    
public:
    ScoreSystem() = default;
    
    // Getters
    int getScore() const { return score_; }
    int getLines() const { return lines_; }
    int getLevel() const { return level_; }
    int getTickMs() const { return tickMs_; }
    
    // Score management
    void addScore(int points) {
        score_ += points;
    }
    
    void addLines(int lines) {
        lines_ += lines;
        level_ = lines_ / 10; // Level up every 10 lines
        
        // Update game speed based on level
        tickMs_ = gameConfig.tickMsStart - (level_ * SPEED_ACCELERATION);
        if (tickMs_ < gameConfig.tickMsMin) {
            tickMs_ = gameConfig.tickMsMin;
        }
    }
    
    void reset() {
        score_ = 0;
        lines_ = 0;
        level_ = 0;
        tickMs_ = gameConfig.tickMsStart;
    }
    
    // Configuration
    void setTickMs(int ms) {
        tickMs_ = ms;
    }
};

/**
 * @brief Piece management and randomization system
 * 
 * Handles piece bag, randomization, and next piece generation
 */
class PieceManager : public IPieceManager {
private:
    std::vector<int> bag_;
    size_t bagPos_ = 0;
    std::mt19937 rng_;
    int nextIdx_ = 0;
    
    // Configuration fields (replacing temporary global variables)
    int previewGrid_ = 6;
    RandType randomizerType_ = RandType::SIMPLE;
    int randBagSize_ = 0;
    
public:
    PieceManager() : rng_((unsigned)time(nullptr)) {
        // Don't initialize automatically - wait for explicit initialize() call
        // after PIECES is loaded
    }
    
    // Piece generation
    int getNextPiece() {
        if (bagPos_ >= bag_.size()) {
            refillBag();
        }
        
        int piece = bag_[bagPos_];
        bagPos_++;
        return piece;
    }
    
    int getCurrentNextPiece() const {
        return nextIdx_;
    }
    
    void setNextPiece(int pieceIdx) {
        nextIdx_ = pieceIdx;
    }
    
    // Bag management
    void refillBag() {
        bag_.clear();
        int n = (randBagSize_ > 0 ? randBagSize_ : (int)PIECES.size());
        n = std::min(n, (int)PIECES.size());
        
        for (int i = 0; i < n; i++) {
            bag_.push_back(i);
        }
        
        std::shuffle(bag_.begin(), bag_.end(), rng_);
        bagPos_ = 0;
    }
    
    void initialize() {
        refillBag();
        nextIdx_ = getNextPiece();
    }
    
    void reset() {
        bagPos_ = 0;
        initialize();
    }
    
    // Random number generation
    std::mt19937& getRng() { return rng_; }
    
    // Configuration methods
    int getPreviewGrid() const { return previewGrid_; }
    RandType getRandomizerType() const { return randomizerType_; }
    int getRandBagSize() const { return randBagSize_; }
    void setPreviewGrid(int value) { previewGrid_ = value; }
    void setRandomizerType(RandType type) { randomizerType_ = type; }
    void setRandBagSize(int size) { randBagSize_ = size; }
    
    // Piece loading methods
    bool loadPiecesFile() {
        if(const char* env = std::getenv("DROPBLOCKS_PIECES")) {
            if(loadPiecesPath(env)) return true;
        }
        if(!PIECES_FILE_PATH.empty()) {
            if(loadPiecesPath(PIECES_FILE_PATH)) return true;
        }
        if(loadPiecesPath("default.pieces")) return true;
        if(const char* home = std::getenv("HOME")){
            std::string p = std::string(home) + "/.config/default.pieces";
            if(loadPiecesPath(p)) return true;
        }
        return false;
    }
    
    void seedFallback() {
        SDL_Log("Usando fallback interno de peças.");
        PIECES.clear();
        auto mk = [](std::initializer_list<std::pair<int,int>> c, Uint8 r, Uint8 g, Uint8 b){
            Piece p; p.r=r; p.g=g; p.b=b;
            for(auto& coord : c) p.rot[0].push_back(coord);
            return p;
        };
        
        // Peças básicas do Tetris
        PIECES.push_back(mk({{0,0},{1,0},{2,0},{3,0}}, 80,120,220));  // I
        PIECES.push_back(mk({{0,0},{1,0},{0,1},{1,1}}, 220,180,80));  // O
        PIECES.push_back(mk({{0,0},{1,0},{2,0},{1,1}}, 160,80,220));  // T
        PIECES.push_back(mk({{0,0},{1,0},{1,1},{2,1}}, 80,220,80));   // S
        PIECES.push_back(mk({{1,0},{2,0},{0,1},{1,1}}, 220,80,80));   // Z
        PIECES.push_back(mk({{0,0},{0,1},{0,2},{1,2}}, 220,160,80));  // L
        PIECES.push_back(mk({{1,0},{1,1},{1,2},{0,2}}, 80,180,220));  // J
        
        // Peças extras
        PIECES.push_back(mk({{0,0},{1,0},{2,0},{0,1}}, 220,80,160));
        PIECES.push_back(mk({{0,0},{1,0},{2,0},{1,1}}, 160,220,80));
        PIECES.push_back(mk({{0,0},{1,0},{2,0},{2,1}}, 80,160,220));
        PIECES.push_back(mk({{0,0},{1,0},{2,0},{3,0}}, 220,160,80));
        PIECES.push_back(mk({{0,0},{1,0},{2,0},{0,1},{1,1}}, 220,160,80));
        PIECES.push_back(mk({{0,0},{1,0},{-1,0},{0,1},{1,1}}, 160,80,220));
    }
    
    void initializeRandomizer() {
        randomizerType_ = RandType::SIMPLE; 
        randBagSize_ = 0;
    }
};

// Global piece manager instance
PieceManager pieceManager;

// Funções que usam pieceManager (definidas após sua declaração)
static bool loadPiecesFile(){
    return pieceManager.loadPiecesFile();
}

static void seedPiecesFallback(){
    pieceManager.seedFallback();
}

static bool loadPiecesFromStream(std::istream& in) {
    PIECES.clear(); 
    pieceManager.setRandomizerType(RandType::SIMPLE); 
    pieceManager.setRandBagSize(0);

    std::string line, section;
    Piece cur; 
    bool inPiece = false; 
    bool rotExplicit = false;
    std::vector<std::pair<int,int>> rot0, rot1, rot2, rot3, base;

    auto flushPiece = [&]() {
        if (!inPiece) return;
        
        buildPieceRotations(cur, base, rot0, rot1, rot2, rot3, rotExplicit);
        
        if (!cur.rot.empty()) {
            PIECES.push_back(cur);
        }
        
        cur = Piece{}; 
        rotExplicit = false; 
        rot0.clear(); rot1.clear(); rot2.clear(); rot3.clear(); base.clear(); 
        inPiece = false;
    };

    while (std::getline(in, line)) {
        line = parsePiecesLine(line);
        trim(line); 
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            std::string sec = line.substr(1, line.size() - 2);
            std::string SEC = sec; 
            for (char& c : SEC) c = (char)std::toupper((unsigned char)c);
            
            if (SEC.rfind("PIECE.", 0) == 0) {
                flushPiece(); 
                inPiece = true; 
                cur = Piece{}; 
                rotExplicit = false;
                rot0.clear(); rot1.clear(); rot2.clear(); rot3.clear(); base.clear();
                cur.name = sec.substr(6);
            } else {
                flushPiece(); 
                inPiece = false; 
                section = SEC;
            }
            continue;
        }

        size_t eq = line.find('='); 
        if (eq == std::string::npos) continue;
        
        std::string k = line.substr(0, eq), v = line.substr(eq + 1); 
        trim(k); trim(v);
        std::string K = k; 
        for (char& c : K) c = (char)std::toupper((unsigned char)c);

        if (inPiece) {
            if (processPieceProperty(cur, K, v, base, rot0, rot1, rot2, rot3, rotExplicit)) {
                continue;
            }
        } else {
            if (section == "SET") {
                if (K == "NAME") { /* opcional */ continue; }
                if (K == "PREVIEWGRID" || K == "PREVIEW_GRID") { 
                    int n; 
                    if (parseInt(v, n) && n > 0 && n <= 10) pieceManager.setPreviewGrid(n); 
                    continue; 
                }
            }
            if (section == "RANDOMIZER") {
                if (K == "TYPE") { 
                    std::string vv = v; 
                    for (char& c : vv) c = (char)std::tolower((unsigned char)c);
                    pieceManager.setRandomizerType(vv == "bag" ? RandType::BAG : RandType::SIMPLE); 
                    continue; 
                }
                if (K == "BAGSIZE") { 
                    int n; 
                    if (parseInt(v, n) && n >= 0) pieceManager.setRandBagSize(n); 
                    continue; 
                }
            }
        }
    }
    flushPiece();
    return !PIECES.empty();
}

static bool processJoystickConfigs(const std::string& key, const std::string& val, int& processedLines, JoystickSystem& joystick) {
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
    
    // Configurações de timing
    if (key == "JOYSTICK_MOVE_REPEAT_DELAY") {
        int v = std::atoi(val.c_str());
        if (v >= 50 && v <= 1000) {
            config.moveRepeatDelay = v;
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
    if (seti("GAME_SPEED_START_MS", gameConfig.tickMsStart)) { processedLines++; return true; }
    if (seti("GAME_SPEED_MIN_MS", gameConfig.tickMsMin)) { processedLines++; return true; }
    if (seti("GAME_SPEED_ACCELERATION", SPEED_ACCELERATION)) { processedLines++; return true; }
    
    // Configurações de renderização
    if (setf("ASPECT_CORRECTION_FACTOR", ASPECT_CORRECTION_FACTOR)) { processedLines++; return true; }
    if (key == "PREVIEW_GRID") {
        int v = std::atoi(val.c_str());
        if (v > 0 && v <= 10) {
            pieceManager.setPreviewGrid(v);
            processedLines++;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Main game state coordinator
 * 
 * Coordinates all game systems and provides unified interface
 */
class GameState {
private:
    GameBoard board_;
    ScoreSystem score_;
    ComboSystem combo_;
    Active activePiece_;
    bool running_ = true;
    bool paused_ = false;
    bool gameover_ = false;
    Uint32 lastTick_ = 0;
    
    // Injected dependencies
    std::unique_ptr<IAudioSystem> audio_;
    std::unique_ptr<IThemeManager> theme_;
    std::unique_ptr<IPieceManager> pieces_;
    std::unique_ptr<IInputManager> input_;
    std::unique_ptr<IGameConfig> config_;
    
public:
    // Constructor with dependency injection
    GameState(DependencyContainer& container) 
        : audio_(container.resolve<IAudioSystem>("audio"))
        , theme_(container.resolve<IThemeManager>("theme"))
        , pieces_(container.resolve<IPieceManager>("pieces"))
        , input_(container.resolve<IInputManager>("input"))
        , config_(container.resolve<IGameConfig>("config"))
    {
        lastTick_ = SDL_GetTicks();
    }
    
    // Legacy constructor for backward compatibility
    GameState() {
        lastTick_ = SDL_GetTicks();
        // Initialize with default implementations (fallback)
        // Note: These will be replaced by external instances in initializeGame()
        audio_ = nullptr;
        theme_ = nullptr;
        pieces_ = nullptr;
        input_ = nullptr;
        config_ = nullptr;
    }
    
    // Method to set dependencies after construction (for legacy compatibility)
    void setDependencies(AudioSystem* audio, ThemeManager* theme, PieceManager* pieces, 
                        InputManager* input, ConfigManager* config) {
        DebugLogger::info("Setting dependencies for GameState");
        audio_ = std::unique_ptr<IAudioSystem>(audio);
        theme_ = std::unique_ptr<IThemeManager>(theme);
        pieces_ = std::unique_ptr<IPieceManager>(pieces);
        input_ = std::unique_ptr<IInputManager>(input);
        config_ = std::unique_ptr<IGameConfig>(config);
        
        // Verify all dependencies are set
        if (!audio_) DebugLogger::error("Audio dependency not set");
        if (!theme_) DebugLogger::error("Theme dependency not set");
        if (!pieces_) DebugLogger::error("Pieces dependency not set");
        if (!input_) DebugLogger::error("Input dependency not set");
        if (!config_) DebugLogger::error("Config dependency not set");
        
        DebugLogger::info("Dependencies set successfully");
    }
    
    // System access
    GameBoard& getBoard() { return board_; }
    const GameBoard& getBoard() const { return board_; }
    ScoreSystem& getScore() { return score_; }
    const ScoreSystem& getScore() const { return score_; }
    IPieceManager& getPieces() { return *pieces_; }
    const IPieceManager& getPieces() const { return *pieces_; }
    ComboSystem& getCombo() { return combo_; }
    const ComboSystem& getCombo() const { return combo_; }
    
    // Active piece management
    Active& getActivePiece() { return activePiece_; }
    const Active& getActivePiece() const { return activePiece_; }
    
    void setActivePiece(const Active& piece) { activePiece_ = piece; }
    
    // Game state
    bool isRunning() const { return running_; }
    bool isPaused() const { return paused_; }
    bool isGameOver() const { return gameover_; }
    
    void setRunning(bool running) { running_ = running; }
    void setPaused(bool paused) { paused_ = paused; }
    void setGameOver(bool gameover) { gameover_ = gameover; }
    
    // Timing
    Uint32 getLastTick() const { return lastTick_; }
    void setLastTick(Uint32 tick) { lastTick_ = tick; }
    
    // Game control
    void reset() {
        board_.reset();
        score_.reset();
        pieces_.reset();
        combo_.reset();
        gameover_ = false;
        paused_ = false;
        lastTick_ = SDL_GetTicks();
    }
    
    // Piece update logic
    void updatePiece() {
        if (!audio_) {
            DebugLogger::error("Audio system not initialized in updatePiece()");
            return;
        }
        
        auto coll = [&](int dx, int dy, int drot) { return !board_.canPlacePiece(activePiece_, dx, dy, drot); };
    if (!coll(0, 1, 0)) {
            activePiece_.y++;
    } else {
            board_.placePiece(activePiece_);
        audio_->playBeep(220.0, 25, 0.12f, true);  // Som de peça travando - mais sutil
        
            int c = board_.clearLines();
        if (c > 0) {
                score_.addLines(c);
            
            // Sistema de combos
                combo_.onLineClear(static_cast<AudioSystem&>(*audio_));
            
            // Som especial para Tetris (4 linhas)
            if (c == 4) {
                audio_->playTetrisSound();
            } else {
                // Som normal de linha limpa - mais responsivo
                double freq = 440.0 + (c * 110.0);  // 440, 550, 660 Hz para 1, 2, 3 linhas
                audio_->playBeep(freq, 30 + c * 10, 0.18f, false);  // Som mais curto e suave
            }
            
                int points = (c == 1 ? 100 : c == 2 ? 300 : c == 3 ? 500 : 800) * (score_.getLevel() + 1);
                score_.addScore(points);
                
                // Level progression is now handled automatically in ScoreSystem::addLines()
                // Note: Level up sound should be triggered when level actually changes
        } else {
            // Se não limpou linhas, reseta combo
                combo_.reset();
            }
            
            newActive(activePiece_, pieces_->getCurrentNextPiece()); 
            pieces_->setNextPiece(pieces_->getNextPiece());
            if (board_.isGameOver(activePiece_)) { 
                gameover_ = true; 
                paused_ = false; 
                combo_.reset();  // Reseta combo no game over
            audio_->playGameOverSound();  // Som icônico de game over
        }
    }
}

    // Convenience methods for backward compatibility
    std::vector<std::vector<Cell>>& grid = board_.getGrid();
    Active& act = activePiece_;
    bool& running = running_;
    bool& paused = paused_;
    bool& gameover = gameover_;
    Uint32& lastTick = lastTick_;
    ComboSystem& combo = combo_;
    
    // Score system access (using methods instead of direct references)
    int getScoreValue() const { return score_.getScore(); }
    int getLinesValue() const { return score_.getLines(); }
    int getLevelValue() const { return score_.getLevel(); }
    int getTickMsValue() const { return score_.getTickMs(); }
    
    void setScore(int score) { score_.addScore(score - score_.getScore()); }
    void setLines(int lines) { score_.addLines(lines - score_.getLines()); }
    void setLevel(int level) { 
        int currentLines = score_.getLines();
        int targetLines = level * 10;
        score_.addLines(targetLines - currentLines);
    }
    void setTickMs(int tickMs) { score_.setTickMs(tickMs); }
    
    // Access to next piece index
    int getNextIdx() const { return pieces_->getCurrentNextPiece(); }
    
    // ===========================
    //   INTERFACE LIMPA (FASE 3)
    // ===========================
    
    // Main game loop methods
    void update(SDL_Renderer* renderer) {
        if (!input_ || !audio_) {
            DebugLogger::error("Dependencies not initialized in update()");
            return;
        }
        
        handleInput(renderer);
        
        if (!isPaused() && !isGameOver()) {
            Uint32 now = SDL_GetTicks();
            if (now - getLastTick() >= (Uint32)getScore().getTickMs()) {
                updatePiece();
                setLastTick(now);
            }
            
            // Verificar tensão do tabuleiro
            getBoard().checkTension(static_cast<AudioSystem&>(*audio_));
            
            // Música de fundo
            audio_->playBackgroundMelody(getScore().getLevel());
        }
    }
    
    // Forward declaration - implementation moved after class definitions
    void render(class RenderManager& renderManager, const class LayoutCache& layout);
    
    // Input handling
    void handleInput(SDL_Renderer* renderer) {
        if (!input_ || !audio_) {
            DebugLogger::error("Dependencies not initialized in handleInput()");
            return;
        }
        
    // Atualizar todos os handlers de input
    input_->update();
        
        // Debug: verificar qual handler está ativo (apenas uma vez)
        static bool debugHandlerLogged = false;
        if (!debugHandlerLogged) {
            // Note: debugActiveHandler() is not available in interface, skipping for now
            debugHandlerLogged = true;
        }
    
    // Processar eventos SDL (apenas para quit e screenshot)
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) setRunning(false);
    }
    
    // Screenshot (apenas teclado)
    if (input_->shouldScreenshot()) {
        time_t now = time(0);
        struct tm* timeinfo = localtime(&now);
        char filename[64];
        strftime(filename, sizeof(filename), "dropblocks-screenshot_%Y-%m-%d_%H-%M-%S.bmp", timeinfo);
        
            if (saveScreenshot(renderer, filename)) audio_->playBeep(880.0, 80, 0.18f, false); 
    }
    
    // Quit
    if (input_->shouldQuit()) {
            setRunning(false);
    }
    
    // Pause
    if (input_->shouldPause()) {
            setPaused(!isPaused());
            audio_->playBeep(isPaused() ? 440.0 : 520.0, 30, 0.12f, false);
    }
    
    // Game Over - Restart
        if (isGameOver() && input_->shouldRestart()) {
            reset();
            pieces_->initialize();
        audio_->playBeep(520.0, 40, 0.15f, false);
        return;
    }
    
    // Controles de jogo (só funcionam quando não pausado e não em game over)
        if (!isPaused() && !isGameOver()) {
            auto coll = [&](int dx, int dy, int drot) { return !board_.canPlacePiece(activePiece_, dx, dy, drot); };
        
        // Movimento esquerda
        if (input_->shouldMoveLeft() && !coll(-1, 0, 0)) {
                activePiece_.x--;
            audio_->playMovementSound();
            input_->resetTimers();
        }
        
        // Movimento direita
        if (input_->shouldMoveRight() && !coll(1, 0, 0)) {
                activePiece_.x++;
            audio_->playMovementSound();
            input_->resetTimers();
        }
        
        // Soft drop
        if (input_->shouldSoftDrop()) {
            audio_->playSoftDropSound();
                updatePiece();
        }
        
        // Hard drop
        if (input_->shouldHardDrop()) {
                while (!coll(0, 1, 0)) activePiece_.y++;
                score_.addScore(2); // Bonus por hard drop
            audio_->playHardDropSound();
                updatePiece();
        }
        
        // Rotação CCW
        if (input_->shouldRotateCCW()) {
                rotateWithKicks(activePiece_, board_.getGrid(), -1, static_cast<AudioSystem&>(*audio_));
            audio_->playRotationSound(false);  // CCW
            input_->resetTimers();
        }
        
        // Rotação CW
        if (input_->shouldRotateCW()) {
                rotateWithKicks(activePiece_, board_.getGrid(), +1, static_cast<AudioSystem&>(*audio_));
            audio_->playRotationSound(true);   // CW
            input_->resetTimers();
        }
    }
}
};

/**
 * @brief Apply game configuration to GameState
 */
static void applyConfigToGame(GameState& state, const GameConfig& config) {
    gameConfig.tickMsStart = config.tickMsStart;
    gameConfig.tickMsMin = config.tickMsMin;
    SPEED_ACCELERATION = config.speedAcceleration;
    LEVEL_STEP = config.levelStep;
    ASPECT_CORRECTION_FACTOR = config.aspectCorrectionFactor;
    
    // Apply configuration to the new modular systems
    state.getScore().setTickMs(config.tickMsStart);
}

/**
 * @brief Apply pieces configuration to global theme
 */
static void applyConfigToPieces(const PiecesConfig& config) {
    
    // Apply piece colors
    if (!config.pieceColors.empty()) {
        themeManager.getTheme().piece_colors.clear();
        for (const auto& color : config.pieceColors) {
            themeManager.getTheme().piece_colors.push_back({color.r, color.g, color.b});
        }
    }
}

// Implementação da função initializeRandomizer
static void initializeRandomizer(GameState& state) {
    pieceManager.initializeRandomizer();
    
    // Use the new PieceManager system
    state.getPieces().reset();
    
    // Get the first piece and set it as active
    int firstPiece = state.getPieces().getNextPiece();
    newActive(state.getActivePiece(), firstPiece);
    
    // Set the next piece
    state.getPieces().setNextPiece(state.getPieces().getNextPiece());
    
    state.setLastTick(SDL_GetTicks());
    state.getCombo().reset();  // Reset combo no início
}

// Estrutura para cache de layout
struct LayoutCache {
    int SWr, SHr, CW, CH, CX, CY;
    int scale, GAP1, GAP2;
    int bannerW, panelTarget, usableLeftW;
    int cellBoard, GW, GH;
    int BX, BY, BW, BH, GX, GY;
    int panelX, panelW, panelY, panelH;
    bool dirty = true;
    
    void calculate() {
        SDL_DisplayMode dmNow; 
        SDL_GetCurrentDisplayMode(0, &dmNow);
        SWr = dmNow.w; 
        SHr = dmNow.h;
        
        // Compensação para tela LED 1920x1080 comprimida 3:2
        // Ajustar para usar melhor a tela 16:9
        if (SWr * 9 >= SHr * 16) { 
            // Tela mais larga que 16:9 - usar altura completa
            CH = SHr; 
            CW = (CH * 16) / 9;  // Proporção 16:9
            CX = (SWr - CW) / 2; 
            CY = 0; 
        } else { 
            // Tela mais alta que 16:9 - usar largura completa
            CW = SWr; 
            CH = (CW * 9) / 16;  // Proporção 16:9
            CX = 0; 
            CY = (SHr - CH) / 2; 
        }
        
        scale = HUD_FIXED_SCALE;
        GAP1 = BORDER + GAP1_SCALE * scale;
        GAP2 = BORDER + GAP2_SCALE * scale;
        
        bannerW = 8 * scale + 24;
        panelTarget = (int)(CW * 0.28);
        usableLeftW = CW - (BORDER + bannerW + GAP1) - panelTarget - GAP2;
        // Compensação para distorção 3:2 da tela LED (comprime 16:9 para 3:2)
        int cellW = usableLeftW / COLS;
        int cellH = (CH - 2 * BORDER) / ROWS;
        // Aplicar fator de correção para compensar compressão vertical
        cellH = (int)(cellH * ASPECT_CORRECTION_FACTOR);
        cellBoard = std::min(std::max(8, cellW), cellH);
        GW = cellBoard * COLS; 
        GH = cellBoard * ROWS;
        
        BX = CX + BORDER; 
        BY = CY + (CH - GH) / 2; 
        BW = bannerW; 
        BH = GH;
        GX = BX + BW + GAP1; 
        GY = BY;
        panelX = GX + GW + GAP2; 
        panelW = CX + CW - panelX - BORDER; 
        panelY = BY; 
        panelH = GH;
        
        dirty = false;
    }
};

// Função comum para eliminar duplicação
// DEPRECATED: Use GameState::updatePiece() instead
static void processPieceFall(GameState& state, AudioSystem& audio) {
    state.updatePiece();
}

// DEPRECATED: Use GameState::handleInput() instead
static void handleInput(GameState& state, AudioSystem& audio, SDL_Renderer* ren, InputManager& inputManager) {
    state.handleInput(ren);
}

// DEPRECATED: Use GameBoard::checkTension() instead
static void checkTensionSound(const GameState& state, AudioSystem& audio) {
    state.getBoard().checkTension(audio);
}

// DEPRECATED: Use GameState::update() instead
static void updateGame(GameState& state, AudioSystem& audio) {
    // This function is now handled by GameState::update()
    // Keeping for backward compatibility but functionality moved to GameState
}

// Funções especializadas extraídas da main
static bool initializeSDL() {
    // Inicializar cada subsistema separadamente para maior compatibilidade
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        DebugLogger::error("SDL_INIT_VIDEO error: " + std::string(SDL_GetError()));
        return false;
    }
    
    if (SDL_Init(SDL_INIT_TIMER) != 0) {
        DebugLogger::error("SDL_INIT_TIMER error: " + std::string(SDL_GetError()));
        return false;
    }
    
    if (SDL_Init(SDL_INIT_EVENTS) != 0) {
        DebugLogger::error("SDL_INIT_EVENTS error: " + std::string(SDL_GetError()));
        return false;
    }
    
    // Áudio e gamepad são opcionais
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        SDL_Log("Warning: SDL_INIT_AUDIO failed: %s", SDL_GetError());
    }
    
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("Warning: SDL_INIT_GAMECONTROLLER failed: %s", SDL_GetError());
    }
    
    if (SDL_Init(SDL_INIT_JOYSTICK) != 0) {
        SDL_Log("Warning: SDL_INIT_JOYSTICK failed: %s", SDL_GetError());
    }
    
    return true;
}

static void applyConfigToJoystick(InputManager& inputManager, const InputConfig& config) {
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
            
            return;
        }
    }
    
    DebugLogger::warning("No JoystickInput handler found for configuration");
}

static bool initializeGame(GameState& state, AudioSystem& audio, ConfigManager& configManager, InputManager& inputManager) {
    // Load configuration using new system
    if (!configManager.loadAll()) {
        DebugLogger::error("Failed to load configuration");
        return false;
    }
    
    // Set dependencies for GameState (legacy compatibility)
    state.setDependencies(&audio, &themeManager, &pieceManager, &inputManager, &configManager);
    
    // Apply configuration to existing systems
    applyConfigToAudio(audio, configManager.getAudio());
    applyConfigToTheme(configManager.getVisual());
    applyConfigToGame(state, configManager.getGame());
    applyConfigToPieces(configManager.getPieces());
    
    // Apply joystick configuration to InputManager
    applyConfigToJoystick(inputManager, configManager.getInput());
    
    // Carregar peças
    bool piecesOk = loadPiecesFile();
    if (!piecesOk) {
        seedPiecesFallback();
    }
    
    // Aplicar tema
    applyThemePieceColors();
    
    // Initialize PieceManager after PIECES is loaded
    state.getPieces().initialize();
    
    printf("Pieces: %zu, PreviewGrid=%d, Randomizer=%s, BagSize=%d\n",
           PIECES.size(), pieceManager.getPreviewGrid(), (pieceManager.getRandomizerType() == RandType::BAG ? "bag" : "simple"), pieceManager.getRandBagSize());
    printf("Audio: Master=%.1f, SFX=%.1f, Ambient=%.1f\n", 
           audio.masterVolume, audio.sfxVolume, audio.ambientVolume);
    fflush(stdout);
    
    return true;
}

static bool initializeWindow(SDL_Window*& win, SDL_Renderer*& ren) {
    SDL_DisplayMode dm; 
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
        DebugLogger::error("Failed to get display mode: " + std::string(SDL_GetError()));
        return false;
    }
    int SW = dm.w, SH = dm.h;
    
    win = SDL_CreateWindow("DropBlocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SW, SH,
                          SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) { 
        DebugLogger::error("Failed to create window: " + std::string(SDL_GetError()));
        return false; 
    }
    
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { 
        DebugLogger::error("Failed to create renderer: " + std::string(SDL_GetError()));
        return false; 
    }
    
    // Esconder cursor do mouse
    SDL_ShowCursor(SDL_DISABLE);
    
    return true;
}

// Declaração forward para initializeRandomizer
static void initializeRandomizer(GameState& state);

// ===========================
//   SISTEMA DE RENDERIZAÇÃO MODULAR
// ===========================

/**
 * @brief Abstract render layer interface
 * 
 * Provides a unified interface for all render layers with Z-order support
 */
class RenderLayer {
public:
    virtual ~RenderLayer() = default;
    
    // Core rendering method
    virtual void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) = 0;
    
    // Layer ordering and visibility
    virtual int getZOrder() const = 0;
    virtual bool isEnabled() const { return true; }
    virtual void setEnabled(bool enabled) { enabled_ = enabled; }
    
    // Layer identification
    virtual std::string getName() const = 0;
    
protected:
    bool enabled_ = true;
};

/**
 * @brief Background render layer
 * 
 * Renders the game background
 */
class BackgroundLayer : public RenderLayer {
public:
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override {
        SDL_SetRenderDrawColor(renderer, themeManager.getTheme().bg_r, themeManager.getTheme().bg_g, themeManager.getTheme().bg_b, 255);
        SDL_RenderClear(renderer);
    }
    
    int getZOrder() const override { return 0; }
    std::string getName() const override { return "Background"; }
};

/**
 * @brief Banner render layer
 * 
 * Renders the game banner with title and effects
 */
class BannerLayer : public RenderLayer {
private:
    AudioSystem* audio_ = nullptr;
    
public:
    BannerLayer(AudioSystem* audio) : audio_(audio) {}
    
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override {
        // Banner
        drawRoundedFilled(renderer, layout.BX, layout.BY, layout.BW, layout.BH, 10, 
                         themeManager.getTheme().banner_bg_r, themeManager.getTheme().banner_bg_g, themeManager.getTheme().banner_bg_b, 255);
        drawRoundedOutline(renderer, layout.BX, layout.BY, layout.BW, layout.BH, 10, 2, 
                          themeManager.getTheme().banner_outline_r, themeManager.getTheme().banner_outline_g, themeManager.getTheme().banner_outline_b, themeManager.getTheme().banner_outline_a);

        // Título vertical
        int bty = layout.BY + 10, cxText = layout.BX + (layout.BW - 5 * layout.scale) / 2;
        for (size_t i = 0; i < TITLE_TEXT.size(); ++i) {
            char ch = TITLE_TEXT[i];
            if (ch == ' ') { bty += 6 * layout.scale; continue; }
            ch = (char)std::toupper((unsigned char)ch);
            if (ch < 'A' || ch > 'Z') ch = ' ';
            drawPixelText(renderer, cxText, bty, std::string(1, ch), layout.scale, 
                         themeManager.getTheme().banner_text_r, themeManager.getTheme().banner_text_g, themeManager.getTheme().banner_text_b);
            bty += 9 * layout.scale;
        }

        // Sweep local do banner
        if (ENABLE_BANNER_SWEEP) {
            SDL_Rect clip{layout.BX, layout.BY, layout.BW, layout.BH};
            SDL_RenderSetClipRect(renderer, &clip);
            
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
            
            int bandH = SWEEP_BAND_H_S * layout.scale;
            int total = layout.BH + bandH;
            float tsec = SDL_GetTicks() / 1000.0f;
            int sweepY = (int)std::fmod(tsec * SWEEP_SPEED_PXPS, (float)total) - bandH;
            for (int i = 0; i < bandH; ++i) {
                float normalizedPos = (float)i / (float)bandH;
                float center = 0.5f;
                float distance = (normalizedPos - center) * 2.0f;
                float sigma = 0.3f + (1.0f - SWEEP_SOFTNESS) * 0.4f;
                float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
                Uint8 a = (Uint8)std::round(SWEEP_ALPHA_MAX * softness);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, a);
                SDL_Rect line{layout.BX, layout.BY + sweepY + i, layout.BW, 1};
                SDL_RenderFillRect(renderer, &line);
            }
            
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_RenderSetClipRect(renderer, nullptr);
            
            if (audio_) audio_->playSweepEffect();
        }
    }
    
    int getZOrder() const override { return 1; }
    std::string getName() const override { return "Banner"; }
};

/**
 * @brief Board render layer
 * 
 * Renders the game board with pieces
 */
class BoardLayer : public RenderLayer {
public:
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override {
        // Tabuleiro vazio
        for (int y = 0; y < ROWS; ++y) {
            for (int x = 0; x < COLS; ++x) {
                SDL_Rect r{layout.GX + x * layout.cellBoard, layout.GY + y * layout.cellBoard, 
                          layout.cellBoard - 1, layout.cellBoard - 1};
                SDL_SetRenderDrawColor(renderer, themeManager.getTheme().board_empty_r, themeManager.getTheme().board_empty_g, themeManager.getTheme().board_empty_b, 255);
                SDL_RenderFillRect(renderer, &r);
            }
        }

        // Peças fixas
        for (int y = 0; y < ROWS; ++y) {
            for (int x = 0; x < COLS; ++x) {
                if (state.grid[y][x].occ) {
                    SDL_Rect r{layout.GX + x * layout.cellBoard, layout.GY + y * layout.cellBoard, 
                              layout.cellBoard - 1, layout.cellBoard - 1};
                    SDL_SetRenderDrawColor(renderer, state.grid[y][x].r, state.grid[y][x].g, state.grid[y][x].b, 255);
                    SDL_RenderFillRect(renderer, &r);
                }
            }
        }

        // Peça ativa
        auto& pc = PIECES[state.act.idx];
        for (auto [px, py] : pc.rot[state.act.rot]) {
            SDL_Rect r{layout.GX + (state.act.x + px) * layout.cellBoard, 
                      layout.GY + (state.act.y + py) * layout.cellBoard, 
                      layout.cellBoard - 1, layout.cellBoard - 1};
            SDL_SetRenderDrawColor(renderer, pc.r, pc.g, pc.b, 255); 
            SDL_RenderFillRect(renderer, &r);
        }
    }
    
    int getZOrder() const override { return 2; }
    std::string getName() const override { return "Board"; }
};

/**
 * @brief HUD render layer
 * 
 * Renders the game HUD with score, level, and next piece
 */
class HUDLayer : public RenderLayer {
public:
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override {
        // Painel (HUD)
        drawRoundedFilled(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH, 12, 
                         themeManager.getTheme().panel_fill_r, themeManager.getTheme().panel_fill_g, themeManager.getTheme().panel_fill_b, 255);
        drawRoundedOutline(renderer, layout.panelX, layout.panelY, layout.panelW, layout.panelH, 12, 2, 
                          themeManager.getTheme().panel_outline_r, themeManager.getTheme().panel_outline_g, themeManager.getTheme().panel_outline_b, themeManager.getTheme().panel_outline_a);

        // HUD textos
        int tx = layout.panelX + 14, ty = layout.panelY + 14;
        drawPixelText(renderer, tx, ty, "SCORE", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); ty += 10 * layout.scale;
        drawPixelText(renderer, tx, ty, fmtScore(state.getScoreValue()), layout.scale + 1, themeManager.getTheme().hud_score_r, themeManager.getTheme().hud_score_g, themeManager.getTheme().hud_score_b); ty += 12 * (layout.scale + 1);
        drawPixelText(renderer, tx, ty, "LINES", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); ty += 8 * layout.scale;
        drawPixelText(renderer, tx, ty, std::to_string(state.getLinesValue()), layout.scale, themeManager.getTheme().hud_lines_r, themeManager.getTheme().hud_lines_g, themeManager.getTheme().hud_lines_b); ty += 10 * layout.scale;
        drawPixelText(renderer, tx, ty, "LEVEL", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); ty += 8 * layout.scale;
        drawPixelText(renderer, tx, ty, std::to_string(state.getLevelValue()), layout.scale, themeManager.getTheme().hud_level_r, themeManager.getTheme().hud_level_g, themeManager.getTheme().hud_level_b); ty += 10 * layout.scale;

        // NEXT (quadro pieceManager.getPreviewGrid() × pieceManager.getPreviewGrid())
        int boxW = layout.panelW - 28;
        int boxH = std::min(layout.panelH - (ty - layout.panelY) - 14, boxW);
        int boxX = layout.panelX + 14;
        int boxY = ty;

        drawRoundedFilled(renderer, boxX, boxY, boxW, boxH, 10, themeManager.getTheme().next_fill_r, themeManager.getTheme().next_fill_g, themeManager.getTheme().next_fill_b, 255);
        drawRoundedOutline(renderer, boxX, boxY, boxW, boxH, 10, 2, themeManager.getTheme().next_outline_r, themeManager.getTheme().next_outline_g, themeManager.getTheme().next_outline_b, themeManager.getTheme().next_outline_a);

        drawPixelText(renderer, boxX + 10, boxY + 10, "NEXT", layout.scale, themeManager.getTheme().next_label_r, themeManager.getTheme().next_label_g, themeManager.getTheme().next_label_b);

        int padIn = 14 + 4 * layout.scale;
        int innerX = boxX + 10, innerY = boxY + padIn;
        int innerW = boxW - 20, innerH = boxH - padIn - 10;

        int gridCols = pieceManager.getPreviewGrid(), gridRows = pieceManager.getPreviewGrid();
        int cellMini = std::min(innerW / gridCols, innerH / gridRows);
        if (cellMini < 1) cellMini = 1;
        if (cellMini > layout.cellBoard) cellMini = layout.cellBoard;

        int gridW = cellMini * gridCols, gridH = cellMini * gridRows;
        int gridX = innerX + (innerW - gridW) / 2;
        int gridY = innerY + (innerH - gridH) / 2;

        // quadriculado
        for (int gy = 0; gy < gridRows; ++gy) {
            for (int gx = 0; gx < gridCols; ++gx) {
                SDL_Rect q{gridX + gx * cellMini, gridY + gy * cellMini, cellMini - 1, cellMini - 1};
                bool isLight = ((gx + gy) & 1) != 0;
                if (themeManager.getTheme().next_grid_use_rgb) {
                    if (isLight)
                        SDL_SetRenderDrawColor(renderer, themeManager.getTheme().next_grid_light_r, themeManager.getTheme().next_grid_light_g, themeManager.getTheme().next_grid_light_b, 255);
                    else
                        SDL_SetRenderDrawColor(renderer, themeManager.getTheme().next_grid_dark_r, themeManager.getTheme().next_grid_dark_g, themeManager.getTheme().next_grid_dark_b, 255);
                } else {
                    Uint8 v = isLight ? themeManager.getTheme().next_grid_light : themeManager.getTheme().next_grid_dark;
                    SDL_SetRenderDrawColor(renderer, v, v, v, 255);
                }
                SDL_RenderFillRect(renderer, &q);
            }
        }

        // peça próxima centrada
        {
            auto& np = PIECES[state.getNextIdx()];
            int minx = 999, maxx = -999, miny = 999, maxy = -999;
            for (auto [px, py] : np.rot[0]) { 
                minx = std::min(minx, px); maxx = std::max(maxx, px); 
                miny = std::min(miny, py); maxy = std::max(maxy, py); 
            }
            int pw = (maxx - minx + 1), ph = (maxy - miny + 1);
            int offX = (gridCols - pw) / 2 - minx;
            int offY = (gridRows - ph) / 2 - miny;
            for (auto [px, py] : np.rot[0]) {
                SDL_Rect r{gridX + (px + offX) * cellMini, gridY + (py + offY) * cellMini, cellMini - 1, cellMini - 1};
                SDL_SetRenderDrawColor(renderer, np.r, np.g, np.b, 255); 
                SDL_RenderFillRect(renderer, &r);
            }
        }
    }
    
    int getZOrder() const override { return 3; }
    std::string getName() const override { return "HUD"; }
};

/**
 * @brief Overlay render layer
 * 
 * Renders game overlays (pause, game over)
 */
class OverlayLayer : public RenderLayer {
public:
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override {
        if (state.gameover || state.paused) {
            const std::string topText = state.paused ? "PAUSE" : "GAME OVER";
            const std::string subText = state.paused ? "" : "PRESS START";
            int topW = textWidthPx(topText, layout.scale + 2);
            int subW = subText.empty() ? 0 : textWidthPx(subText, layout.scale);
            int textW = std::max(topW, subW);
            int padX = 24, padY = 20;
            int textH = 7 * (layout.scale + 2) + (subText.empty() ? 0 : (8 * layout.scale + 7 * layout.scale));
            int ow = textW + padX * 2, oh = textH + padY * 2;
            int ox = layout.GX + (layout.GW - ow) / 2, oy = layout.GY + (layout.GH - oh) / 2;
            drawRoundedFilled(renderer, ox, oy, ow, oh, 14, themeManager.getTheme().overlay_fill_r, themeManager.getTheme().overlay_fill_g, themeManager.getTheme().overlay_fill_b, themeManager.getTheme().overlay_fill_a);
            drawRoundedOutline(renderer, ox, oy, ow, oh, 14, 2, themeManager.getTheme().overlay_outline_r, themeManager.getTheme().overlay_outline_g, themeManager.getTheme().overlay_outline_b, themeManager.getTheme().overlay_outline_a);
            int txc = ox + (ow - topW) / 2, tyc = oy + padY;
            drawPixelTextOutlined(renderer, txc, tyc, topText, layout.scale + 2, themeManager.getTheme().overlay_top_r, themeManager.getTheme().overlay_top_g, themeManager.getTheme().overlay_top_b, 0, 0, 0);
            if (!subText.empty()) {
                int sx = ox + (ow - subW) / 2, sy = tyc + 7 * (layout.scale + 2) + 8 * layout.scale;
                drawPixelTextOutlined(renderer, sx, sy, subText, layout.scale, themeManager.getTheme().overlay_sub_r, themeManager.getTheme().overlay_sub_g, themeManager.getTheme().overlay_sub_b, 0, 0, 0);
            }
        }
    }
    
    int getZOrder() const override { return 4; }
    std::string getName() const override { return "Overlay"; }
};

/**
 * @brief Post-effects render layer
 * 
 * Renders screen effects like scanlines and global sweep
 */
class PostEffectsLayer : public RenderLayer {
private:
    AudioSystem* audio_ = nullptr;
    
public:
    PostEffectsLayer(AudioSystem* audio) : audio_(audio) {}
    
    void render(SDL_Renderer* renderer, const GameState& state, const LayoutCache& layout) override {
        // Scanlines
        if (SCANLINE_ALPHA > 0) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, (Uint8)SCANLINE_ALPHA);
            for (int y = 0; y < layout.SHr; y += 2) { 
                SDL_Rect sl{0, y, layout.SWr, 1}; 
                SDL_RenderFillRect(renderer, &sl); 
            }
            
            if (audio_) audio_->playScanlineEffect();
        }

        // SWEEP GLOBAL (clareia)
        if (ENABLE_GLOBAL_SWEEP) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
            float tsec = SDL_GetTicks() / 1000.0f;
            int bandH = SWEEP_G_BAND_H_PX;
            int total = layout.SHr + bandH;
            int sweepY = (int)std::fmod(tsec * SWEEP_G_SPEED_PXPS, (float)total) - bandH;
            for (int i = 0; i < bandH; ++i) {
                float normalizedPos = (float)i / (float)bandH;
                float center = 0.5f;
                float distance = (normalizedPos - center) * 2.0f;
                float sigma = 0.3f + (1.0f - SWEEP_G_SOFTNESS) * 0.4f;
                float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
                Uint8 a = (Uint8)std::round(SWEEP_G_ALPHA_MAX * softness);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, a);
                SDL_Rect line{0, sweepY + i, layout.SWr, 1};
                SDL_RenderFillRect(renderer, &line);
            }
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }
    
    int getZOrder() const override { return 5; }
    std::string getName() const override { return "PostEffects"; }
};

/**
 * @brief Render manager for handling multiple render layers
 * 
 * Manages render layers with Z-order and provides unified rendering
 */
class RenderManager {
private:
    std::vector<std::unique_ptr<RenderLayer>> layers_;
    SDL_Renderer* renderer_ = nullptr;
    
public:
    RenderManager(SDL_Renderer* renderer) : renderer_(renderer) {}
    
    void addLayer(std::unique_ptr<RenderLayer> layer) {
        layers_.push_back(std::move(layer));
        // Sort by Z-order
        std::sort(layers_.begin(), layers_.end(), 
                 [](const std::unique_ptr<RenderLayer>& a, const std::unique_ptr<RenderLayer>& b) {
                     return a->getZOrder() < b->getZOrder();
                 });
    }
    
    void render(const GameState& state, const LayoutCache& layout) {
        for (auto& layer : layers_) {
            if (layer->isEnabled()) {
                layer->render(renderer_, state, layout);
            }
        }
    }
    
    void setLayerEnabled(const std::string& name, bool enabled) {
        for (auto& layer : layers_) {
            if (layer->getName() == name) {
                layer->setEnabled(enabled);
                break;
            }
        }
    }
    
    void cleanup() {
        layers_.clear();
    }
    
    // Getters for specific layers
    RenderLayer* getLayer(const std::string& name) {
        for (auto& layer : layers_) {
            if (layer->getName() == name) {
                return layer.get();
            }
        }
        return nullptr;
    }
    
    std::vector<std::string> getLayerNames() const {
        std::vector<std::string> names;
        for (const auto& layer : layers_) {
            names.push_back(layer->getName());
        }
        return names;
    }
};

// ===========================
//   CLASSES DE GERENCIAMENTO DO JOGO
// ===========================

/**
 * @brief Classe responsável por toda a inicialização do jogo
 *
 * Encapsula toda a lógica de inicialização em métodos especializados,
 * separando as responsabilidades do main() e facilitando testes.
 */
class GameInitializer {
private:
    bool sdlInitialized_ = false;
    bool audioInitialized_ = false;
    bool inputInitialized_ = false;
    bool configInitialized_ = false;
    bool windowInitialized_ = false;
    bool gameStateInitialized_ = false;

public:
    /**
     * @brief Inicializa o SDL2
     * @return true se inicialização bem-sucedida
     */
    bool initializeSDL() {
        if (sdlInitialized_) return true;

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0) {
            DebugLogger::error("SDL could not initialize: " + std::string(SDL_GetError()));
            return false;
        }

        sdlInitialized_ = true;
        DebugLogger::info("SDL2 initialized successfully");
        return true;
    }

    /**
     * @brief Inicialização básica (apenas SDL por enquanto)
     * @return true se inicialização bem-sucedida
     */
    bool initializeBasic() {
        printf("=== DROPBLOCKS STARTING ===\n");
        printf("VERSION: %s - %s\n", DROPBLOCKS_VERSION, DROPBLOCKS_BUILD_INFO);
        printf("BUILD: %s %s\n", __DATE__, __TIME__);
        printf("FEATURES: %s\n", DROPBLOCKS_FEATURES);
        printf("PHASE: GameInitializer Basic Implementation (v6.13)\n");
        fflush(stdout);

        return initializeSDL();
    }

    /**
     * @brief Inicializa o sistema de áudio
     * @param audio Referência para o AudioSystem
     * @return true se inicialização bem-sucedida
     */
    bool initializeAudio(AudioSystem& audio) {
        if (audioInitialized_) return true;

        if (!audio.initialize()) {
            DebugLogger::warning("Audio initialization failed, continuing without sound");
            // Não falha o jogo, apenas continua sem som
        }

        audioInitialized_ = true;
        DebugLogger::info("Audio system initialized successfully");
        return true;
    }

    /**
     * @brief Inicializa o sistema de input
     * @param inputManager Referência para o InputManager
     * @return true se inicialização bem-sucedida
     */
    bool initializeInput(InputManager& inputManager) {
        if (inputInitialized_) return true;

        // Adicionar keyboard input (sempre disponível)
        auto keyboardInput = std::make_unique<KeyboardInput>();
        inputManager.addHandler(std::move(keyboardInput));

        // Tentar adicionar joystick input
        auto joystickInput = std::make_unique<JoystickInput>();
        if (joystickInput->initialize()) {
            // Armazenar ponteiro antes de mover
            InputHandler* joystickPtr = joystickInput.get();
            inputManager.addHandler(std::move(joystickInput));
            // Definir JoystickInput como handler primário (prioridade sobre teclado)
            inputManager.setPrimaryHandler(joystickPtr);
            DebugLogger::info("Joystick/controller input enabled and set as primary");
        } else {
            DebugLogger::warning("No joystick/controller found, continuing with keyboard only");
        }

        inputInitialized_ = true;
        DebugLogger::info("Input system initialized successfully");
        return true;
    }

    /**
     * @brief Inicializa o sistema de configuração
     * @param configManager Referência para o ConfigManager
     * @return true se inicialização bem-sucedida
     */
    bool initializeConfig(ConfigManager& configManager) {
        if (configInitialized_) return true;

        // O ConfigManager já é inicializado no construtor
        // Aqui podemos adicionar validações adicionais se necessário

        configInitialized_ = true;
        DebugLogger::info("Config system initialized successfully");
        return true;
    }

    /**
     * @brief Inicializa a janela e renderer do SDL
     * @param win Ponteiro para SDL_Window (será preenchido)
     * @param ren Ponteiro para SDL_Renderer (será preenchido)
     * @return true se inicialização bem-sucedida
     */
    bool initializeWindow(SDL_Window*& win, SDL_Renderer*& ren) {
        if (windowInitialized_) return true;

        // Usar a mesma lógica da função original para tela cheia
        SDL_DisplayMode dm;
        if (SDL_GetCurrentDisplayMode(0, &dm) != 0) {
            DebugLogger::error("Failed to get display mode: " + std::string(SDL_GetError()));
            return false;
        }
        int SW = dm.w, SH = dm.h;

        win = SDL_CreateWindow("DropBlocks", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SW, SH,
                              SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
        if (!win) {
            DebugLogger::error("Window could not be created: " + std::string(SDL_GetError()));
            return false;
        }

        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!ren) {
            DebugLogger::error("Renderer could not be created: " + std::string(SDL_GetError()));
            SDL_DestroyWindow(win);
            return false;
        }

        windowInitialized_ = true;
        DebugLogger::info("Fullscreen window and renderer initialized successfully");
        return true;
    }

    /**
     * @brief Inicializa o estado do jogo
     * @param state Referência para o GameState
     * @param audio Referência para o AudioSystem
     * @param configManager Referência para o ConfigManager
     * @param inputManager Referência para o InputManager
     * @return true se inicialização bem-sucedida
     */
    bool initializeGameState(GameState& state, AudioSystem& audio,
                           ConfigManager& configManager, InputManager& inputManager) {
        if (gameStateInitialized_) return true;

        if (!initializeGame(state, audio, configManager, inputManager)) {
            DebugLogger::error("Game state initialization failed");
            return false;
        }

        gameStateInitialized_ = true;
        DebugLogger::info("Game state initialized successfully");
        return true;
    }

    /**
     * @brief Inicialização completa (SDL + Áudio + Input + Config + Janela + Estado do Jogo)
     * @param audio Referência para o AudioSystem
     * @param inputManager Referência para o InputManager
     * @param configManager Referência para o ConfigManager
     * @param state Referência para o GameState
     * @param win Ponteiro para SDL_Window
     * @param ren Ponteiro para SDL_Renderer
     * @return true se inicialização bem-sucedida
     */
    bool initializeComplete(AudioSystem& audio, InputManager& inputManager,
                          ConfigManager& configManager, GameState& state,
                          SDL_Window*& win, SDL_Renderer*& ren) {
        printf("🚀 Testing Complete GameInitializer...\n");
        printf("VERSION: %s - %s\n", DROPBLOCKS_VERSION, DROPBLOCKS_BUILD_INFO);
        printf("BUILD: %s %s\n", __DATE__, __TIME__);
        printf("FEATURES: %s\n", DROPBLOCKS_FEATURES);
        printf("PHASE: Complete GameInitializer Test (v6.13)\n");
        fflush(stdout);

        // Inicialização sequencial
        if (!initializeSDL()) return false;
        if (!initializeAudio(audio)) return false; // Continua mesmo se falhar
        if (!initializeInput(inputManager)) return false;
        if (!initializeConfig(configManager)) return false;
        if (!initializeGameState(state, audio, configManager, inputManager)) return false;
        if (!initializeWindow(win, ren)) return false;

        // Debug destacado ao final da inicialização
        printf("\n");
        printf("========================================\n");
        printf("🎮 DROPBLOCKS %s INICIALIZADO COM SUCESSO! 🎮\n", DROPBLOCKS_VERSION);
        printf("========================================\n");
        printf("✅ SDL2: OK\n");
        printf("✅ Audio: OK\n");
        printf("✅ Input: OK\n");
        printf("✅ Config: OK\n");
        printf("✅ GameState: OK\n");
        printf("✅ Fullscreen Window: OK\n");
        printf("========================================\n");
        printf("🎯 CONTROLES:\n");
        printf("   Teclado: ← → ↓ Z X SPACE P ENTER ESC\n");
        printf("   Joystick: D-pad + B0,B1,B8,B9\n");
        printf("========================================\n");
        printf("🚀 INICIANDO JOGO...\n");
        printf("\n");
        fflush(stdout);

        return true;
    }

    // Getters para status
    bool isSDLInitialized() const { return sdlInitialized_; }
    bool isAudioInitialized() const { return audioInitialized_; }
    bool isInputInitialized() const { return inputInitialized_; }
    bool isConfigInitialized() const { return configInitialized_; }
    bool isWindowInitialized() const { return windowInitialized_; }
    bool isGameStateInitialized() const { return gameStateInitialized_; }
};

/**
 * @brief Classe responsável pelo loop principal do jogo
 *
 * Encapsula toda a lógica do loop principal, separando as responsabilidades
 * de atualização e renderização do main().
 */
class GameLoop {
private:
    bool running_ = false;
    LayoutCache layoutCache_;

public:
    /**
     * @brief Executa o loop principal do jogo
     * @param state Referência para o GameState
     * @param renderManager Referência para o RenderManager
     * @param ren Ponteiro para SDL_Renderer
     */
    void run(GameState& state, RenderManager& renderManager, SDL_Renderer* ren) {
        if (running_) {
            DebugLogger::warning("Game loop is already running");
            return;
        }

        running_ = true;
        DebugLogger::info("Starting main game loop");

        while (state.isRunning() && running_) {
            // Cache de layout
            if (layoutCache_.dirty) {
                layoutCache_.calculate();
            }

            // Update game state (input, logic, audio)
            state.update(ren);

            // Render game
            state.render(renderManager, layoutCache_);

            SDL_RenderPresent(ren);
            SDL_Delay(1);
        }

        running_ = false;
        DebugLogger::info("Main game loop ended");
    }

    /**
     * @brief Para o loop principal
     */
    void stop() {
        running_ = false;
        DebugLogger::info("Game loop stop requested");
    }

    /**
     * @brief Verifica se o loop está rodando
     * @return true se o loop está ativo
     */
    bool isRunning() const {
        return running_;
    }

    /**
     * @brief Força recálculo do layout cache
     */
    void invalidateLayout() {
        layoutCache_.dirty = true;
    }

    /**
     * @brief Obtém referência para o layout cache
     * @return Referência para LayoutCache
     */
    LayoutCache& getLayoutCache() {
        return layoutCache_;
    }
};

/**
 * @brief Classe responsável pela limpeza do jogo
 *
 * Encapsula toda a lógica de limpeza em métodos especializados,
 * garantindo que todos os recursos sejam liberados corretamente.
 */
class GameCleanup {
private:
    bool cleaned_ = false;

public:
    /**
     * @brief Limpa o sistema de áudio
     * @param audio Referência para o AudioSystem
     */
    void cleanupAudio(AudioSystem& audio) {
        audio.cleanup();
        DebugLogger::info("Audio system cleaned up");
    }

    /**
     * @brief Limpa o sistema de input
     * @param inputManager Referência para o InputManager
     */
    void cleanupInput(InputManager& inputManager) {
        inputManager.cleanup();
        DebugLogger::info("Input system cleaned up");
    }

    /**
     * @brief Limpa a janela e renderer do SDL
     * @param win Ponteiro para SDL_Window
     * @param ren Ponteiro para SDL_Renderer
     */
    void cleanupWindow(SDL_Window* win, SDL_Renderer* ren) {
        if (ren) {
            SDL_DestroyRenderer(ren);
            DebugLogger::info("Renderer destroyed");
        }

        if (win) {
            SDL_DestroyWindow(win);
            DebugLogger::info("Window destroyed");
        }
    }

    /**
     * @brief Limpa o sistema de renderização
     * @param renderManager Referência para o RenderManager
     */
    void cleanupRender(RenderManager& renderManager) {
        renderManager.cleanup();
        DebugLogger::info("Render system cleaned up");
    }

    /**
     * @brief Limpa o SDL2
     */
    void cleanupSDL() {
        SDL_Quit();
        DebugLogger::info("SDL2 cleaned up");
    }

    /**
     * @brief Limpeza completa do jogo
     * @param audio Referência para o AudioSystem
     * @param inputManager Referência para o InputManager
     * @param renderManager Referência para o RenderManager
     * @param win Ponteiro para SDL_Window
     * @param ren Ponteiro para SDL_Renderer
     */
    void cleanupAll(AudioSystem& audio, InputManager& inputManager,
                   RenderManager& renderManager, SDL_Window* win, SDL_Renderer* ren) {
        if (cleaned_) return;

        DebugLogger::info("Starting game cleanup");

        // Ordem de limpeza (inversa da inicialização)
        cleanupRender(renderManager);
        cleanupInput(inputManager);
        cleanupAudio(audio);
        cleanupWindow(win, ren);
        cleanupSDL();

        cleaned_ = true;
        DebugLogger::info("Game cleanup completed");
    }

    /**
     * @brief Verifica se a limpeza foi realizada
     * @return true se já foi limpo
     */
    bool isCleaned() const {
        return cleaned_;
    }
};

// ===========================
//   IMPLEMENTAÇÃO DE MÉTODOS DO GAMESTATE
// ===========================

// Implementação do método render do GameState
void GameState::render(RenderManager& renderManager, const LayoutCache& layout) {
    renderManager.render(*this, layout);
}

// ===========================
//   FUNÇÕES DE RENDERIZAÇÃO (LEGADO - MANTIDAS PARA COMPATIBILIDADE)
// ===========================

static void renderBackground(SDL_Renderer* ren, const LayoutCache& layout) {
    SDL_SetRenderDrawColor(ren, themeManager.getTheme().bg_r, themeManager.getTheme().bg_g, themeManager.getTheme().bg_b, 255);
    SDL_RenderClear(ren);
}

static void renderBanner(SDL_Renderer* ren, const LayoutCache& layout, AudioSystem& audio) {
    // Banner
    drawRoundedFilled(ren, layout.BX, layout.BY, layout.BW, layout.BH, 10, 
                     themeManager.getTheme().banner_bg_r, themeManager.getTheme().banner_bg_g, themeManager.getTheme().banner_bg_b, 255);
    drawRoundedOutline(ren, layout.BX, layout.BY, layout.BW, layout.BH, 10, 2, 
                      themeManager.getTheme().banner_outline_r, themeManager.getTheme().banner_outline_g, themeManager.getTheme().banner_outline_b, themeManager.getTheme().banner_outline_a);

    // Título vertical
    int bty = layout.BY + 10, cxText = layout.BX + (layout.BW - 5 * layout.scale) / 2;
    for (size_t i = 0; i < TITLE_TEXT.size(); ++i) {
        char ch = TITLE_TEXT[i];
        if (ch == ' ') { bty += 6 * layout.scale; continue; }
        ch = (char)std::toupper((unsigned char)ch);
        if (ch < 'A' || ch > 'Z') ch = ' ';
        drawPixelText(ren, cxText, bty, std::string(1, ch), layout.scale, 
                     themeManager.getTheme().banner_text_r, themeManager.getTheme().banner_text_g, themeManager.getTheme().banner_text_b);
        bty += 9 * layout.scale;
    }

    // Sweep local do banner
    if (ENABLE_BANNER_SWEEP) {
        SDL_Rect clip{layout.BX, layout.BY, layout.BW, layout.BH};
        SDL_RenderSetClipRect(ren, &clip);
        
        // Usar modo ADD para clarear o banner
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
        
        int bandH = SWEEP_BAND_H_S * layout.scale;
        int total = layout.BH + bandH;
        float tsec = SDL_GetTicks() / 1000.0f;
        int sweepY = (int)std::fmod(tsec * SWEEP_SPEED_PXPS, (float)total) - bandH;
        for (int i = 0; i < bandH; ++i) {
            // Usar função gaussiana para bordas muito suaves
            float normalizedPos = (float)i / (float)bandH; // 0.0 a 1.0
            float center = 0.5f;
            float distance = (normalizedPos - center) * 2.0f; // -1.0 a 1.0
            float sigma = 0.3f + (1.0f - SWEEP_SOFTNESS) * 0.4f; // 0.3 a 0.7
            float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
            Uint8 a = (Uint8)std::round(SWEEP_ALPHA_MAX * softness);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, a);
            SDL_Rect line{layout.BX, layout.BY + sweepY + i, layout.BW, 1};
            SDL_RenderFillRect(ren, &line);
        }
        
        // Resetar modo de blend e clipping
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        SDL_RenderSetClipRect(ren, nullptr);
        
        // Som de sweep
        audio.playSweepEffect();
    }
}

static void renderBoard(SDL_Renderer* ren, const GameState& state, const LayoutCache& layout) {
    // Tabuleiro vazio
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            SDL_Rect r{layout.GX + x * layout.cellBoard, layout.GY + y * layout.cellBoard, 
                      layout.cellBoard - 1, layout.cellBoard - 1};
            SDL_SetRenderDrawColor(ren, themeManager.getTheme().board_empty_r, themeManager.getTheme().board_empty_g, themeManager.getTheme().board_empty_b, 255);
            SDL_RenderFillRect(ren, &r);
        }
    }

    // Peças fixas
    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            if (state.grid[y][x].occ) {
                SDL_Rect r{layout.GX + x * layout.cellBoard, layout.GY + y * layout.cellBoard, 
                          layout.cellBoard - 1, layout.cellBoard - 1};
                SDL_SetRenderDrawColor(ren, state.grid[y][x].r, state.grid[y][x].g, state.grid[y][x].b, 255);
                SDL_RenderFillRect(ren, &r);
            }
        }
    }

    // Peça ativa
    auto& pc = PIECES[state.act.idx];
    for (auto [px, py] : pc.rot[state.act.rot]) {
        SDL_Rect r{layout.GX + (state.act.x + px) * layout.cellBoard, 
                  layout.GY + (state.act.y + py) * layout.cellBoard, 
                  layout.cellBoard - 1, layout.cellBoard - 1};
        SDL_SetRenderDrawColor(ren, pc.r, pc.g, pc.b, 255); 
        SDL_RenderFillRect(ren, &r);
    }
}

static void renderHUD(SDL_Renderer* ren, const GameState& state, const LayoutCache& layout) {
    // Painel (HUD)
    drawRoundedFilled(ren, layout.panelX, layout.panelY, layout.panelW, layout.panelH, 12, 
                     themeManager.getTheme().panel_fill_r, themeManager.getTheme().panel_fill_g, themeManager.getTheme().panel_fill_b, 255);
    drawRoundedOutline(ren, layout.panelX, layout.panelY, layout.panelW, layout.panelH, 12, 2, 
                      themeManager.getTheme().panel_outline_r, themeManager.getTheme().panel_outline_g, themeManager.getTheme().panel_outline_b, themeManager.getTheme().panel_outline_a);

    // HUD textos
    int tx = layout.panelX + 14, ty = layout.panelY + 14;
    drawPixelText(ren, tx, ty, "SCORE", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); ty += 10 * layout.scale;
    drawPixelText(ren, tx, ty, fmtScore(state.getScoreValue()), layout.scale + 1, themeManager.getTheme().hud_score_r, themeManager.getTheme().hud_score_g, themeManager.getTheme().hud_score_b); ty += 12 * (layout.scale + 1);
    drawPixelText(ren, tx, ty, "LINES", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); ty += 8 * layout.scale;
    drawPixelText(ren, tx, ty, std::to_string(state.getLinesValue()), layout.scale, themeManager.getTheme().hud_lines_r, themeManager.getTheme().hud_lines_g, themeManager.getTheme().hud_lines_b); ty += 10 * layout.scale;
    drawPixelText(ren, tx, ty, "LEVEL", layout.scale, themeManager.getTheme().hud_label_r, themeManager.getTheme().hud_label_g, themeManager.getTheme().hud_label_b); ty += 8 * layout.scale;
    drawPixelText(ren, tx, ty, std::to_string(state.getLevelValue()), layout.scale, themeManager.getTheme().hud_level_r, themeManager.getTheme().hud_level_g, themeManager.getTheme().hud_level_b); ty += 10 * layout.scale;

    // NEXT (quadro pieceManager.getPreviewGrid() × pieceManager.getPreviewGrid())
    int boxW = layout.panelW - 28;
    int boxH = std::min(layout.panelH - (ty - layout.panelY) - 14, boxW);
    int boxX = layout.panelX + 14;
    int boxY = ty;

    drawRoundedFilled(ren, boxX, boxY, boxW, boxH, 10, themeManager.getTheme().next_fill_r, themeManager.getTheme().next_fill_g, themeManager.getTheme().next_fill_b, 255);
    drawRoundedOutline(ren, boxX, boxY, boxW, boxH, 10, 2, themeManager.getTheme().next_outline_r, themeManager.getTheme().next_outline_g, themeManager.getTheme().next_outline_b, themeManager.getTheme().next_outline_a);

    drawPixelText(ren, boxX + 10, boxY + 10, "NEXT", layout.scale, themeManager.getTheme().next_label_r, themeManager.getTheme().next_label_g, themeManager.getTheme().next_label_b);

    int padIn = 14 + 4 * layout.scale;
    int innerX = boxX + 10, innerY = boxY + padIn;
    int innerW = boxW - 20, innerH = boxH - padIn - 10;

    int gridCols = pieceManager.getPreviewGrid(), gridRows = pieceManager.getPreviewGrid();
    int cellMini = std::min(innerW / gridCols, innerH / gridRows);
    if (cellMini < 1) cellMini = 1;
    if (cellMini > layout.cellBoard) cellMini = layout.cellBoard;

    int gridW = cellMini * gridCols, gridH = cellMini * gridRows;
    int gridX = innerX + (innerW - gridW) / 2;
    int gridY = innerY + (innerH - gridH) / 2;

    // quadriculado
    for (int gy = 0; gy < gridRows; ++gy) {
        for (int gx = 0; gx < gridCols; ++gx) {
            SDL_Rect q{gridX + gx * cellMini, gridY + gy * cellMini, cellMini - 1, cellMini - 1};
            bool isLight = ((gx + gy) & 1) != 0;
            if (themeManager.getTheme().next_grid_use_rgb) {
                if (isLight)
                    SDL_SetRenderDrawColor(ren, themeManager.getTheme().next_grid_light_r, themeManager.getTheme().next_grid_light_g, themeManager.getTheme().next_grid_light_b, 255);
                else
                    SDL_SetRenderDrawColor(ren, themeManager.getTheme().next_grid_dark_r, themeManager.getTheme().next_grid_dark_g, themeManager.getTheme().next_grid_dark_b, 255);
            } else {
                Uint8 v = isLight ? themeManager.getTheme().next_grid_light : themeManager.getTheme().next_grid_dark;
                SDL_SetRenderDrawColor(ren, v, v, v, 255);
            }
            SDL_RenderFillRect(ren, &q);
        }
    }

    // peça próxima centrada
    {
        auto& np = PIECES[state.getNextIdx()];
        int minx = 999, maxx = -999, miny = 999, maxy = -999;
        for (auto [px, py] : np.rot[0]) { 
            minx = std::min(minx, px); maxx = std::max(maxx, px); 
            miny = std::min(miny, py); maxy = std::max(maxy, py); 
        }
        int pw = (maxx - minx + 1), ph = (maxy - miny + 1);
        int offX = (gridCols - pw) / 2 - minx;
        int offY = (gridRows - ph) / 2 - miny;
        for (auto [px, py] : np.rot[0]) {
            SDL_Rect r{gridX + (px + offX) * cellMini, gridY + (py + offY) * cellMini, cellMini - 1, cellMini - 1};
            SDL_SetRenderDrawColor(ren, np.r, np.g, np.b, 255); 
            SDL_RenderFillRect(ren, &r);
        }
    }
}

static void renderOverlay(SDL_Renderer* ren, const GameState& state, const LayoutCache& layout) {
    if (state.gameover || state.paused) {
        const std::string topText = state.paused ? "PAUSE" : "GAME OVER";
        const std::string subText = state.paused ? "" : "PRESS START";
        int topW = textWidthPx(topText, layout.scale + 2);
        int subW = subText.empty() ? 0 : textWidthPx(subText, layout.scale);
        int textW = std::max(topW, subW);
        int padX = 24, padY = 20;
        int textH = 7 * (layout.scale + 2) + (subText.empty() ? 0 : (8 * layout.scale + 7 * layout.scale));
        int ow = textW + padX * 2, oh = textH + padY * 2;
        int ox = layout.GX + (layout.GW - ow) / 2, oy = layout.GY + (layout.GH - oh) / 2;
        drawRoundedFilled(ren, ox, oy, ow, oh, 14, themeManager.getTheme().overlay_fill_r, themeManager.getTheme().overlay_fill_g, themeManager.getTheme().overlay_fill_b, themeManager.getTheme().overlay_fill_a);
        drawRoundedOutline(ren, ox, oy, ow, oh, 14, 2, themeManager.getTheme().overlay_outline_r, themeManager.getTheme().overlay_outline_g, themeManager.getTheme().overlay_outline_b, themeManager.getTheme().overlay_outline_a);
        int txc = ox + (ow - topW) / 2, tyc = oy + padY;
        drawPixelTextOutlined(ren, txc, tyc, topText, layout.scale + 2, themeManager.getTheme().overlay_top_r, themeManager.getTheme().overlay_top_g, themeManager.getTheme().overlay_top_b, 0, 0, 0);
        if (!subText.empty()) {
            int sx = ox + (ow - subW) / 2, sy = tyc + 7 * (layout.scale + 2) + 8 * layout.scale;
            drawPixelTextOutlined(ren, sx, sy, subText, layout.scale, themeManager.getTheme().overlay_sub_r, themeManager.getTheme().overlay_sub_g, themeManager.getTheme().overlay_sub_b, 0, 0, 0);
        }
    }
}

static void renderPostEffects(SDL_Renderer* ren, const LayoutCache& layout, AudioSystem& audio) {
    // Scanlines
    if (SCANLINE_ALPHA > 0) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, (Uint8)SCANLINE_ALPHA);
        for (int y = 0; y < layout.SHr; y += 2) { 
            SDL_Rect sl{0, y, layout.SWr, 1}; 
            SDL_RenderFillRect(ren, &sl); 
        }
        
        // Som de scanline
        audio.playScanlineEffect();
    }

    // SWEEP GLOBAL (clareia)
    if (ENABLE_GLOBAL_SWEEP) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
        float tsec = SDL_GetTicks() / 1000.0f;
        int bandH = SWEEP_G_BAND_H_PX;
        int total = layout.SHr + bandH;
        int sweepY = (int)std::fmod(tsec * SWEEP_G_SPEED_PXPS, (float)total) - bandH;
        for (int i = 0; i < bandH; ++i) {
            // Usar função gaussiana para bordas muito suaves
            float normalizedPos = (float)i / (float)bandH; // 0.0 a 1.0
            float center = 0.5f;
            float distance = (normalizedPos - center) * 2.0f; // -1.0 a 1.0
            float sigma = 0.3f + (1.0f - SWEEP_G_SOFTNESS) * 0.4f; // 0.3 a 0.7
            float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
            Uint8 a = (Uint8)std::round(SWEEP_G_ALPHA_MAX * softness);
            SDL_SetRenderDrawColor(ren, 255, 255, 255, a);
            SDL_Rect line{0, sweepY + i, layout.SWr, 1};
            SDL_RenderFillRect(ren, &line);
        }
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
}

// ===========================
//   MAIN E CONTROLES
// ===========================
/**
 * @brief Main game entry point
 * 
 * Initializes SDL2, loads configuration and piece sets, sets up audio,
 * and runs the main game loop until the user quits.
 * 
 * @param argc Command line argument count (unused)
 * @param argv Command line arguments (unused)
 * @return Exit status (0 for success)
 */
int main(int, char**) {
    // Instanciar classes de gerenciamento
    GameInitializer initializer;
    GameLoop gameLoop;
    GameCleanup cleanup;

    // Objetos do jogo
    AudioSystem audio;
    InputManager inputManager;
    ConfigManager configManager;
    GameState state;
    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;

    // Inicialização completa usando GameInitializer
    printf("🚀 Testing Complete GameInitializer...\n");
    if (!initializer.initializeComplete(audio, inputManager, configManager, state, win, ren)) {
        printf("❌ GameInitializer complete test failed!\n");
        return 1;
    }
    printf("✅ GameInitializer complete test passed!\n");

    // Verificar status de inicialização
    printf("\n🔍 INITIALIZATION STATUS:\n");
    printf("✅ SDL2: %s\n", initializer.isSDLInitialized() ? "OK" : "FAILED");
    printf("✅ Audio: %s\n", initializer.isAudioInitialized() ? "OK" : "FAILED");
    printf("✅ Input: %s\n", initializer.isInputInitialized() ? "OK" : "FAILED");
    printf("✅ Config: %s\n", initializer.isConfigInitialized() ? "OK" : "FAILED");
    printf("✅ Fullscreen Window: %s\n", initializer.isWindowInitialized() ? "OK" : "FAILED");
    printf("✅ GameState: %s\n", initializer.isGameStateInitialized() ? "OK" : "FAILED");

    // Configurar render manager
    RenderManager renderManager(ren);

    // Adicionar camadas de renderização
    renderManager.addLayer(std::make_unique<BackgroundLayer>());
    renderManager.addLayer(std::make_unique<BannerLayer>(&audio));
    renderManager.addLayer(std::make_unique<BoardLayer>());
    renderManager.addLayer(std::make_unique<HUDLayer>());
    renderManager.addLayer(std::make_unique<OverlayLayer>());
    renderManager.addLayer(std::make_unique<PostEffectsLayer>(&audio));

    // Inicializar randomizador
    initializeRandomizer(state);

    printf("\n🎉 Complete GameInitializer v6.13 test completed successfully!\n");
    printf("📝 Next step: Implement GameLoop and GameCleanup classes\n");

    // Loop principal usando GameLoop
    gameLoop.run(state, renderManager, ren);

    // Limpeza usando GameCleanup
    cleanup.cleanupAll(audio, inputManager, renderManager, win, ren);

    return 0;
}
