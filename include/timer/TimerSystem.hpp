#pragma once

#include <SDL2/SDL.h>
#include <string>
#include "ConfigTypes.hpp"

/**
 * @brief Sistema de countdown timer para modo kiosk
 * 
 * Gerencia um timer configurável que pode limitar o tempo de jogo.
 * Suporte para pause/resume, toggle via teclado e configuração visual.
 */
class TimerSystem {
public:
    enum class State {
        STOPPED,    // Timer parado (inicial)
        RUNNING,    // Timer rodando
        PAUSED,     // Timer pausado
        EXPIRED     // Timer expirado
    };

private:
    TimerConfig config_;
    State state_;
    Uint32 startTime_;          // Tempo de início
    Uint32 pausedTime_;         // Tempo pausado acumulado
    Uint32 pauseStartTime_;     // Momento em que pausou (para calcular duração do pause)
    Uint32 lastUpdateTime_;     // Último tempo de atualização
    Uint32 gamePauseStartTime_; // Tempo quando o jogo foi pausado
    bool gameWasPaused_;        // Flag para rastrear se o jogo estava pausado
    int remainingSeconds_;      // Segundos restantes (cache)
    bool wasWarning_;           // Flag para detectar mudança de estado
    bool wasCritical_;          // Flag para detectar mudança de estado
    
    void updateRemainingTime();
    
public:
    TimerSystem();
    explicit TimerSystem(const TimerConfig& config);
    
    // Controle do timer
    void start();
    void pause();
    void resume();
    void reset();
    void stop();
    void toggle();
    
    // Controle externo do pause (quando o jogo pausa)
    void notifyGamePaused(bool isPaused);
    
    // Estado do timer
    State getState() const { return state_; }
    bool isEnabled() const { return config_.enabled; }
    bool isRunning() const { return state_ == State::RUNNING; }
    bool isPaused() const { return state_ == State::PAUSED; }
    bool isExpired() const { return state_ == State::EXPIRED; }
    bool isStopped() const { return state_ == State::STOPPED; }
    
    // Tempo
    int getRemainingSeconds() const { return remainingSeconds_; }
    int getTotalSeconds() const { return config_.durationSeconds; }
    float getProgress() const; // 0.0 (início) a 1.0 (fim)
    std::string getFormattedTime() const; // Format MM:SS
    
    // Estados visuais
    bool isWarning() const;     // ≤ 30 segundos
    bool isCritical() const;    // ≤ 10 segundos
    RGB getCurrentColor() const;
    
    // Configuração
    void setConfig(const TimerConfig& config);
    const TimerConfig& getConfig() const { return config_; }
    void setEnabled(bool enabled);
    void setDuration(int seconds);
    
    // Layout
    const ElementLayout& getLayout() const { return config_.layout; }
    void setLayout(const ElementLayout& layout) { config_.layout = layout; }
    
    // Update (deve ser chamado no game loop)
    void update();
};