/**
 * DropBlocks (SDL2)
 * - Modular architecture: Config, Theme, Audio, Pieces, Render layers, App loop
 * - Visual effects via bridge (no global externs)
 * - Centralized input (quit/pause/restart) via InputManager
 * - Guideline-like rotations (SRS) with fallback pieces
 *
 * Controls
 * - Keyboard: Arrow keys, Z/X/Up, Space, P, Enter, ESC, F12
 * - Joystick: D-pad, A/B/X/Y, Start/Back, analog (deadzone)
 *
 * Build (example)
 * - g++ dropblocks.cpp -o dropblocks `sdl2-config --cflags --libs` -O2
 */

// TODO: countdown timer (exhibitions)
// TODO: region layout configurability (position/size)
// TODO: resolution/screen ratio tuning
// TODO: thousand separator formatting
// TODO: grayscale fallback removal



#include <SDL2/SDL.h>
#include "include/render/RenderLayer.hpp"
#include "include/render/RenderManager.hpp"
#include "include/render/LayoutCache.hpp"
#include "include/app/GameInitializer.hpp"
#include "include/app/GameLoop.hpp"
#include "include/app/GameCleanup.hpp"
#include "include/render/Primitives.hpp"
#include "include/render/Layers.hpp"
#include "include/ConfigManager.hpp"
#include "include/input/IInputManager.hpp"
#include "include/DebugLogger.hpp"
#include "include/input/InputHandler.hpp"
#include "include/input/KeyboardInput.hpp"
#include "include/input/InputManager.hpp"
#include "include/ThemeManager.hpp"
#include "include/pieces/Piece.hpp"
#include "include/audio/AudioSystem.hpp"
#include "include/pieces/PieceManager.hpp"
#include "include/render/GameStateBridge.hpp"
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <functional>
#include <map>
#include "include/ConfigTypes.hpp"
#include <stdexcept>
#include <algorithm>
#include <chrono>

// ===========================
//   DEFINIÇÕES DE VERSÃO
// ===========================
#define DROPBLOCKS_VERSION "6.19"
#define DROPBLOCKS_BUILD_INFO "Phase 10: Visual bridge + input centralization"
#define DROPBLOCKS_FEATURES "Visual effects via bridge; centralized quit/pause; pieces loader modular"

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

// Use canonical interfaces
#include "include/Interfaces.hpp"

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

// DebugLogger moved to include/DebugLogger.hpp and src/DebugLogger.cpp

// ===========================
//   SISTEMA DE CONFIGURAÇÃO MODULAR
// ===========================

/**
 * @brief Audio configuration and settings
 * 
 * Manages all audio-related configuration options
 */
// AudioConfig moved to include/ConfigTypes.hpp

/**
 * @brief Input configuration structure
 * 
 * Contains all input settings including joystick and keyboard
 */
// InputConfig moved to include/ConfigTypes.hpp

/**
 * @brief Pieces configuration structure
 * 
 * Contains all piece-related settings
 */
// PiecesConfig moved to include/ConfigTypes.hpp

/**
 * @brief Game configuration structure
 * 
 * Contains all game mechanics settings
 */
// GameConfig moved to include/ConfigTypes.hpp

/**
 * @brief Centralized configuration manager
 * 
 * Manages all configuration categories and provides unified access
 */
// ConfigManager moved to include/ConfigManager.hpp and src/ConfigManager.cpp

/**
 * @brief Abstract configuration parser interface
 * 
 * Provides a unified interface for parsing different configuration categories
 */
// (parsers and validation moved out)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// ===========================
//   PARÂMETROS & THEME
// ===========================

int   ROUNDED_PANELS = 1;           // 1 = arredondado; 0 = retângulo (synced from VisualConfig.layout)
static int   HUD_FIXED_SCALE   = 6;        // escala fixa do HUD
std::string TITLE_TEXT  = "---H A C K T R I S";// texto vertical (A–Z e espaço)
static int   GAP1_SCALE        = 10;       // banner ↔ tabuleiro (x scale)
static int   GAP2_SCALE        = 10;       // tabuleiro ↔ painel  (x scale)

// Deprecated globals (replaced by VisualEffectsView bridge); kept only to satisfy legacy code paths if any
static bool  ENABLE_BANNER_SWEEP = true;
static bool  ENABLE_GLOBAL_SWEEP = true;
static float SWEEP_SPEED_PXPS  = 15.0f;
static int   SWEEP_BAND_H_S    = 30;
static int   SWEEP_ALPHA_MAX   = 100;
static float SWEEP_SOFTNESS    = 0.7f;
static float SWEEP_G_SPEED_PXPS = 20.0f;
static int   SWEEP_G_BAND_H_PX  = 100;
static int   SWEEP_G_ALPHA_MAX  = 50;
static float SWEEP_G_SOFTNESS   = 0.9f;
static int   SCANLINE_ALPHA     = 20;

// Caminho opcional indicado no cfg para o arquivo de peças
std::string PIECES_FILE_PATH = "";

// Config do set de peças
static int PREVIEW_GRID = 6;               // NxN no NEXT (padrão 6)

// Visual effects bridge backing store (read by db_getVisualEffects)
static VisualEffectsView g_visualView{};

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
// Theme now in include/ThemeManager.hpp

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
// Piece moved to include/pieces/Piece.hpp

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
// ThemeManager class moved to include/ThemeManager.hpp

// Global manager instances (temporary during migration)
static GameConfig gameConfig;
ThemeManager themeManager;

// Forward declarations for classes
class PieceManager;
class JoystickSystem;

// Forward declarations for functions that use pieceManager
static bool loadPiecesFromStream(std::istream& in);
static bool processJoystickConfigs(const std::string& key, const std::string& val, int& processedLines, JoystickSystem& joystick);

// Temporary global variables eliminated - now using PieceManager private fields

std::vector<Piece> PIECES;

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

    // Visual effect keys now handled via ConfigManager VisualConfig -> applyConfigToTheme/db_getVisualEffects
    // Layout shortcuts kept for backward compatibility
    if (seti("ROUNDED_PANELS", ROUNDED_PANELS)) { processedLines++; return true; }
    if (seti("HUD_FIXED_SCALE", HUD_FIXED_SCALE)) { processedLines++; return true; }
    if (seti("GAP1_SCALE", GAP1_SCALE)) { processedLines++; return true; }
    if (seti("GAP2_SCALE", GAP2_SCALE)) { processedLines++; return true; }
    
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

bool db_loadPiecesPath(const std::string& p){
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
        // Allow y < 0 (above visible top) so rotations near spawn/top are possible
        if (y < 0) continue;
        if (x<0 || x>=COLS || y>=ROWS) return true;
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
/* moved to src/render/Primitives.cpp
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
void drawPixelText(SDL_Renderer* ren, int x, int y, const std::string& s, int scale, Uint8 r, Uint8 g, Uint8 b){
    SDL_Rect px; SDL_SetRenderDrawColor(ren, r,g,b,255);
    int cx=x;
    for(char c : s){
        if(c=='\n'){ y += (7*scale + scale*2); cx=x; continue; }
        for(int yy=0; yy<7; ++yy) for(int xx=0; xx<5; ++xx)
            if(glyph(c,xx,yy)){ px = { cx + xx*scale, y + yy*scale, scale, scale }; SDL_RenderFillRect(ren, &px); }
        cx += 6*scale;
    }
}
int textWidthPx(const std::string& s, int scale){
    if(s.empty()) return 0; return (int)s.size() * 6 * scale - scale;
}
void drawPixelTextOutlined(SDL_Renderer* ren, int x, int y, const std::string& s, int scale,
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
void drawRoundedFilled(SDL_Renderer* r, int x, int y, int w, int h, int rad, Uint8 R, Uint8 G, Uint8 B, Uint8 A){
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
void drawRoundedOutline(SDL_Renderer* r, int x, int y, int w, int h, int rad, int thick, Uint8 R, Uint8 G, Uint8 B, Uint8 A){
    for(int i=0;i<thick;i++){
        drawRoundedFilled(r, x+i, y+i, w-2*i, h-2*i, std::max(0,rad-i), R,G,B,A);
    }
}
*/

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
//   SISTEMA DE ÁUDIO (extraído)
// ===========================

/**
 * @brief Audio hardware management and basic synthesis
 * 
 * Handles SDL audio device initialization, cleanup, and basic sound generation
 */
/* moved to src/audio/AudioSystem.cpp
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
/* moved to src/audio/AudioSystem.cpp
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
/* struct AudioSystem : public IAudioSystem {
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
*/

// ===========================
//   IMPLEMENTAÇÕES DO SISTEMA DE CONFIGURAÇÃO
// ===========================

// Implementação do VisualConfigParser
/* moved to src/ConfigManager.cpp
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
*/

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
    g_visualView.bannerSweep = config.effects.bannerSweep;
    g_visualView.globalSweep = config.effects.globalSweep;
    g_visualView.sweepSpeedPxps = config.effects.sweepSpeedPxps;
    g_visualView.sweepBandHS = config.effects.sweepBandHS;
    g_visualView.sweepAlphaMax = config.effects.sweepAlphaMax;
    g_visualView.sweepSoftness = config.effects.sweepSoftness;
    g_visualView.sweepGSpeedPxps = config.effects.sweepGSpeedPxps;
    g_visualView.sweepGBandHPx = config.effects.sweepGBandHPx;
    g_visualView.sweepGAlphaMax = config.effects.sweepGAlphaMax;
    g_visualView.sweepGSoftness = config.effects.sweepGSoftness;
    g_visualView.scanlineAlpha = config.effects.scanlineAlpha;
    
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

    // 2.5) Ajuste mínimo para manter dentro do tabuleiro (ajuda parede direita/esquerda)
    {
        int minX = 999, maxX = -999;
        for (auto [px,py] : p.rot[to]) { int x = act.x + px; if (x < minX) minX = x; if (x > maxX) maxX = x; }
        int dx = 0;
        if (minX < 0) dx = -minX; else if (maxX >= COLS) dx = (COLS - 1) - maxX;
        if (dx != 0) {
            const std::pair<int,int> boundTests[] = { {dx,0}, {dx,-1} };
            for (auto [kx,ky] : boundTests) {
                if (!collides(act, grid, kx, ky, dir)) { act.x += kx; act.y += ky; act.rot = to; if(kx!=0||ky!=0) audio.playKickSound(); return; }
            }
        }
    }

    // 3) Fallback SRS simplificado (quando não há kicks definidos no arquivo)
    {
        // Include tests that help near walls and near the top (allow small downward kicks)
        const std::pair<int,int> tests[] = {
            {0,0}, {-1,0}, {1,0}, {0,-1}, {-1,-1}, {1,-1}, {0,-2}, {-2,0}, {2,0}, {0,1}
        };
        for (auto [kx,ky] : tests) {
            if (!collides(act, grid, kx, ky, dir)) { act.x += kx; act.y += ky; act.rot = to; if(kx!=0||ky!=0) audio.playKickSound(); return; }
        }
    }
    // 4) Fallback mínimo
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
// Moved to include/input/InputHandler.hpp

/**
 * @brief Keyboard input handler
 * 
 * Handles keyboard input using SDL events
 */
// Moved to include/input/KeyboardInput.hpp


/**
 * @brief Input manager for handling multiple input sources
 * 
 * Manages multiple input handlers and provides unified input processing
 */
// Moved to include/input/InputManager.hpp

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

// cleanup implementado no header InputManager.hpp

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
/* class PieceManager : public IPieceManager {
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
            if(db_loadPiecesPath(env)) return true;
        }
        if(!PIECES_FILE_PATH.empty()) {
            if(db_loadPiecesPath(PIECES_FILE_PATH)) return true;
        }
        if(db_loadPiecesPath("default.pieces")) return true;
        if(const char* home = std::getenv("HOME")){
            std::string p = std::string(home) + "/.config/default.pieces";
            if(db_loadPiecesPath(p)) return true;
        }
        return false;
    }
    
    void seedFallback() {
        SDL_Log("Usando fallback interno de peças.");
        PIECES.clear();
    auto rotate90_local = [](std::vector<std::pair<int,int>>& pts){
        for (auto& p : pts) { int x = p.first, y = p.second; p.first = -y; p.second = x; }
    };
    auto mk = [&](const char* name, std::initializer_list<std::pair<int,int>> c, Uint8 r, Uint8 g, Uint8 b){
        Piece p; p.name=name; p.r=r; p.g=g; p.b=b;
            for(auto& coord : c) p.rot[0].push_back(coord);
        // Build 4 rotations from base
        p.rot[1] = p.rot[0]; rotate90_local(p.rot[1]);
        p.rot[2] = p.rot[1]; rotate90_local(p.rot[2]);
        p.rot[3] = p.rot[2]; rotate90_local(p.rot[3]);
            return p;
        };
        
        // Peças básicas do Tetris
        PIECES.push_back(mk("I", {{0,0},{1,0},{2,0},{3,0}}, 80,120,220));  // I
        PIECES.push_back(mk("O", {{0,0},{1,0},{0,1},{1,1}}, 220,180,80));  // O
        PIECES.push_back(mk("T", {{0,0},{1,0},{2,0},{1,1}}, 160,80,220));  // T
        PIECES.push_back(mk("S", {{1,0},{2,0},{0,1},{1,1}}, 80,220,80));   // S
        PIECES.push_back(mk("Z", {{0,0},{1,0},{1,1},{2,1}}, 220,80,80));   // Z
        PIECES.push_back(mk("L", {{0,0},{0,1},{0,2},{1,2}}, 220,160,80));  // L
        PIECES.push_back(mk("J", {{1,0},{1,1},{1,2},{0,2}}, 80,180,220));  // J
        
        // Peças extras
        PIECES.push_back(mk("XS1", {{0,0},{1,0},{2,0},{0,1}}, 220,80,160));
        PIECES.push_back(mk("XT1", {{0,0},{1,0},{2,0},{1,1}}, 160,220,80));
        PIECES.push_back(mk("XZ1", {{0,0},{1,0},{2,0},{2,1}}, 80,160,220));
        PIECES.push_back(mk("XI1", {{0,0},{1,0},{2,0},{3,0}}, 220,160,80));
        PIECES.push_back(mk("XU1", {{0,0},{1,0},{2,0},{0,1},{1,1}}, 220,160,80));
        PIECES.push_back(mk("XW1", {{0,0},{1,0},{-1,0},{0,1},{1,1}}, 160,80,220));
    }
    
    void initializeRandomizer() {
        randomizerType_ = RandType::SIMPLE; 
        randBagSize_ = 0;
    }
};
*/

// Global piece manager instance
PieceManager pieceManager;

// Funções que usam pieceManager (definidas após sua declaração)
static bool loadPiecesFile(){ return pieceManager.loadPiecesFile(); }
static void seedPiecesFallback(){ pieceManager.seedFallback(); }

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
        // pieces_.reset();  // avoid resetting piece RNG/next during restart; handled explicitly elsewhere
        combo_.reset();
        gameover_ = false;
        paused_ = false;
        lastTick_ = SDL_GetTicks();
    }

private:
    void restartRound() {
        // Reset core game state
        reset();
        // Re-seed and reset pieces (fresh bag and next)
        if (pieces_) {
            pieces_->initializeRandomizer();
            pieces_->reset();
        }
        // Define active and next pieces
        int first = pieces_ ? pieces_->getNextPiece() : 0;
        newActive(activePiece_, first);
        if (pieces_) pieces_->setNextPiece(pieces_->getNextPiece());
        // Timing and input
        setLastTick(SDL_GetTicks());
        if (input_) input_->resetTimers();
        if (audio_) static_cast<AudioSystem&>(*audio_).playBeep(520.0, 40, 0.15f, false);
    }
public:
    
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
    
    // Eventos SDL agora são tratados via InputHandlers/Manager (quit/teclado/joystick)
    
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
            restartRound();
        return;
    }
        else if (!isGameOver() && input_->shouldRestart()) {
            
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
                int maxSteps = (int)board_.getGrid().size() + 10;
                while (!coll(0, 1, 0) && maxSteps-- > 0) activePiece_.y++;
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

// ============
// Bridge impl
// ============
bool db_getBoardSize(const GameState& state, int& rows, int& cols) {
    const auto& g = state.getBoard().getGrid();
    rows = (int)g.size();
    cols = rows ? (int)g[0].size() : 0;
    return rows > 0 && cols > 0;
}
bool db_getBoardCell(const GameState& state, int x, int y, Uint8& r, Uint8& g, Uint8& b, bool& occ) {
    const auto& gr = state.getBoard().getGrid();
    if (y < 0 || y >= (int)gr.size() || x < 0 || x >= (int)gr[y].size()) return false;
    const auto& c = gr[y][x];
    r = c.r; g = c.g; b = c.b; occ = c.occ; return true;
}
bool db_getActive(const GameState& state, int& idx, int& rot, int& x, int& y) {
    const Active& a = state.getActivePiece();
    idx = a.idx; rot = a.rot; x = a.x; y = a.y; return true;
}
bool db_getNextIdx(const GameState& state, int& nextIdx) {
    nextIdx = state.getNextIdx(); return true;
}
bool db_isPaused(const GameState& state) { return state.isPaused(); }
bool db_isGameOver(const GameState& state) { return state.isGameOver(); }
int db_getScore(const GameState& state) { return state.getScoreValue(); }
int db_getLines(const GameState& state) { return state.getLinesValue(); }
int db_getLevel(const GameState& state) { return state.getLevelValue(); }
bool db_isRunning(const GameState& state) { return state.isRunning(); }
void db_update(GameState& state, SDL_Renderer* renderer) { state.update(renderer); }
void db_render(GameState& state, RenderManager& renderManager, const LayoutCache& layout) { state.render(renderManager, layout); }
void db_layoutCalculate(LayoutCache& layout);
extern VisualEffectsView g_visualView; // already defined above
const VisualEffectsView& db_getVisualEffects() { return g_visualView; }

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

// Bridge layout calculation (replicates the original calculate with current globals)
void db_layoutCalculate(LayoutCache& layout) {
    SDL_DisplayMode dmNow; SDL_GetCurrentDisplayMode(0, &dmNow);
    layout.SWr = dmNow.w; layout.SHr = dmNow.h;
    if (layout.SWr * 9 >= layout.SHr * 16) {
        layout.CH = layout.SHr; layout.CW = (layout.CH * 16) / 9; layout.CX = (layout.SWr - layout.CW) / 2; layout.CY = 0;
        } else { 
        layout.CW = layout.SWr; layout.CH = (layout.CW * 9) / 16; layout.CX = 0; layout.CY = (layout.SHr - layout.CH) / 2;
    }
    layout.scale = HUD_FIXED_SCALE;
    layout.GAP1 = BORDER + GAP1_SCALE * layout.scale;
    layout.GAP2 = BORDER + GAP2_SCALE * layout.scale;
    layout.bannerW = 8 * layout.scale + 24;
    layout.panelTarget = (int)(layout.CW * 0.28);
    layout.usableLeftW = layout.CW - (BORDER + layout.bannerW + layout.GAP1) - layout.panelTarget - layout.GAP2;
    int cellW = layout.usableLeftW / COLS;
    int cellH = (layout.CH - 2 * BORDER) / ROWS;
        cellH = (int)(cellH * ASPECT_CORRECTION_FACTOR);
    layout.cellBoard = std::min(std::max(8, cellW), cellH);
    layout.GW = layout.cellBoard * COLS; layout.GH = layout.cellBoard * ROWS;
    layout.BX = layout.CX + BORDER; layout.BY = layout.CY + (layout.CH - layout.GH) / 2; layout.BW = layout.bannerW; layout.BH = layout.GH;
    layout.GX = layout.BX + layout.BW + layout.GAP1; layout.GY = layout.BY;
    layout.panelX = layout.GX + layout.GW + layout.GAP2; layout.panelW = layout.CX + layout.CW - layout.panelX - BORDER; layout.panelY = layout.BY; layout.panelH = layout.GH;
}

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

bool initializeGame(GameState& state, AudioSystem& audio, ConfigManager& configManager, InputManager& inputManager) {
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

// RenderLayer interface moved to include/render/RenderLayer.hpp

/**
 * @brief Background render layer
 * 
 * Renders the game background
 */
// Layers moved to include/render/Layers.hpp and src/render/Layers.cpp

/**
 * @brief Banner render layer
 * 
 * Renders the game banner with title and effects
 */

/**
 * @brief Board render layer
 * 
 * Renders the game board with pieces
 */

/**
 * @brief HUD render layer
 * 
 * Renders the game HUD with score, level, and next piece
 */

/**
 * @brief Overlay render layer
 * 
 * Renders game overlays (pause, game over)
 */

/**
 * @brief Post-effects render layer
 * 
 * Renders screen effects like scanlines and global sweep
 */

// RenderManager moved to include/render/RenderManager.hpp and src/render/RenderManager.cpp

// ===========================
//   CLASSES DE GERENCIAMENTO DO JOGO
// ===========================

/**
 * @brief Classe responsável por toda a inicialização do jogo
 *
 * Encapsula toda a lógica de inicialização em métodos especializados,
 * separando as responsabilidades do main() e facilitando testes.
 */
// GameInitializer moved to include/app/GameInitializer.hpp and src/app/GameInitializer.cpp

/**
 * @brief Classe responsável pelo loop principal do jogo
 * 
 * Encapsula toda a lógica do loop principal, separando as responsabilidades
 * de atualização e renderização do main().
 */
// GameLoop moved to include/app/GameLoop.hpp and src/app/GameLoop.cpp

/**
 * @brief Classe responsável pela limpeza do jogo
 * 
 * Encapsula toda a lógica de limpeza em métodos especializados,
 * garantindo que todos os recursos sejam liberados corretamente.
 */
// GameCleanup moved to include/app/GameCleanup.hpp and src/app/GameCleanup.cpp

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
    const auto& vis = db_getVisualEffects();
    if (vis.bannerSweep) {
        SDL_Rect clip{layout.BX, layout.BY, layout.BW, layout.BH};
        SDL_RenderSetClipRect(ren, &clip);
        
        // Usar modo ADD para clarear o banner
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
        
        int bandH = vis.sweepBandHS * layout.scale;
        int total = layout.BH + bandH;
        float tsec = SDL_GetTicks() / 1000.0f;
        int sweepY = (int)std::fmod(tsec * vis.sweepSpeedPxps, (float)total) - bandH;
        for (int i = 0; i < bandH; ++i) {
            // Usar função gaussiana para bordas muito suaves
            float normalizedPos = (float)i / (float)bandH; // 0.0 a 1.0
            float center = 0.5f;
            float distance = (normalizedPos - center) * 2.0f; // -1.0 a 1.0
            float sigma = 0.3f + (1.0f - vis.sweepSoftness) * 0.4f; // 0.3 a 0.7
            float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
            Uint8 a = (Uint8)std::round(vis.sweepAlphaMax * softness);
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
    const auto& vis = db_getVisualEffects();
    if (vis.scanlineAlpha > 0) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, (Uint8)vis.scanlineAlpha);
        for (int y = 0; y < layout.SHr; y += 2) { 
            SDL_Rect sl{0, y, layout.SWr, 1}; 
            SDL_RenderFillRect(ren, &sl); 
        }
        
        // Som de scanline
        audio.playScanlineEffect();
    }

    // SWEEP GLOBAL (clareia)
    if (vis.globalSweep) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
        float tsec = SDL_GetTicks() / 1000.0f;
        int bandH = vis.sweepGBandHPx;
        int total = layout.SHr + bandH;
        int sweepY = (int)std::fmod(tsec * vis.sweepGSpeedPxps, (float)total) - bandH;
        for (int i = 0; i < bandH; ++i) {
            // Usar função gaussiana para bordas muito suaves
            float normalizedPos = (float)i / (float)bandH; // 0.0 a 1.0
            float center = 0.5f;
            float distance = (normalizedPos - center) * 2.0f; // -1.0 a 1.0
            float sigma = 0.3f + (1.0f - vis.sweepGSoftness) * 0.4f; // 0.3 a 0.7
            float softness = std::exp(-(distance * distance) / (2.0f * sigma * sigma));
            Uint8 a = (Uint8)std::round(vis.sweepGAlphaMax * softness);
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
    printf("%s SDL2: %s\n", initializer.isSDLInitialized() ? "✅" : "❌", initializer.isSDLInitialized() ? "OK" : "FAILED");
    printf("%s Audio: %s\n", initializer.isAudioInitialized() ? "✅" : "❌", initializer.isAudioInitialized() ? "OK" : "FAILED");
    printf("%s Input: %s\n", initializer.isInputInitialized() ? "✅" : "❌", initializer.isInputInitialized() ? "OK" : "FAILED");
    printf("%s Config: %s\n", initializer.isConfigInitialized() ? "✅" : "❌", initializer.isConfigInitialized() ? "OK" : "FAILED");
    printf("%s Fullscreen Window: %s\n", initializer.isWindowInitialized() ? "✅" : "❌", initializer.isWindowInitialized() ? "OK" : "FAILED");
    printf("%s GameState: %s\n", initializer.isGameStateInitialized() ? "✅" : "❌", initializer.isGameStateInitialized() ? "OK" : "FAILED");

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

    printf("\n🎉 Initialization completed successfully!\n");

    // Loop principal usando GameLoop
    gameLoop.run(state, renderManager, ren);

    // Limpeza usando GameCleanup
    cleanup.cleanupAll(audio, inputManager, renderManager, win, ren);

    return 0;
}
