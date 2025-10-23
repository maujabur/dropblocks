#include "timer/TimerSystem.hpp"
#include "DebugLogger.hpp"
#include <sstream>
#include <iomanip>

TimerSystem::TimerSystem() : state_(State::STOPPED), startTime_(0), pausedTime_(0), 
                            pauseStartTime_(0), lastUpdateTime_(0), gamePauseStartTime_(0), gameWasPaused_(false),
                            remainingSeconds_(0), wasWarning_(false), 
                            wasCritical_(false) {
    // Configuração padrão (usa valores default de ConfigTypes.hpp)
    // config_ já é inicializado com os valores padrão da struct TimerConfig
    
    remainingSeconds_ = config_.durationSeconds;
}

TimerSystem::TimerSystem(const TimerConfig& config) : TimerSystem() {
    setConfig(config);
}

void TimerSystem::start() {
    if (!config_.enabled) return;
    
    if (state_ == State::STOPPED || state_ == State::EXPIRED) {
        // Novo início
        startTime_ = SDL_GetTicks();
        pausedTime_ = 0;
        pauseStartTime_ = 0;
        state_ = State::RUNNING;
        remainingSeconds_ = config_.durationSeconds;
        wasWarning_ = false;
        wasCritical_ = false;
        lastUpdateTime_ = startTime_;
        DebugLogger::info("Timer started: " + std::to_string(config_.durationSeconds) + " seconds");
    } else if (state_ == State::PAUSED) {
        // Resume do pause
        resume();
    }
}

void TimerSystem::pause() {
    if (state_ == State::RUNNING) {
        // Atualizar tempo antes de pausar
        updateRemainingTime();
        state_ = State::PAUSED;
        Uint32 currentTime = SDL_GetTicks();
        // Marcar momento do pause (não acumular ainda)
        pauseStartTime_ = currentTime;
        DebugLogger::info("Timer paused at " + std::to_string(remainingSeconds_) + " seconds remaining");
    }
}

void TimerSystem::resume() {
    if (state_ == State::PAUSED) {
        state_ = State::RUNNING;
        Uint32 currentTime = SDL_GetTicks();
        // Acumular tempo pausado
        pausedTime_ += (currentTime - pauseStartTime_);
        lastUpdateTime_ = currentTime;
        DebugLogger::info("Timer resumed with " + std::to_string(remainingSeconds_) + " seconds remaining");
    }
}

void TimerSystem::reset() {
    state_ = State::STOPPED;
    startTime_ = 0;
    pausedTime_ = 0;
    pauseStartTime_ = 0;
    lastUpdateTime_ = 0;
    gamePauseStartTime_ = 0;
    gameWasPaused_ = false;
    remainingSeconds_ = config_.durationSeconds;
    wasWarning_ = false;
    wasCritical_ = false;
    DebugLogger::info("Timer reset to " + std::to_string(config_.durationSeconds) + " seconds");
}

void TimerSystem::stop() {
    if (state_ != State::STOPPED) {
        state_ = State::STOPPED;
        DebugLogger::info("Timer stopped");
    }
}

void TimerSystem::toggle() {
    if (!config_.enabled) {
        // Se desabilitado, habilita e inicia
        config_.enabled = true;
        start();
        DebugLogger::info("Timer enabled and started");
    } else {
        // Se habilitado, desabilita completamente
        config_.enabled = false;
        stop();
        DebugLogger::info("Timer disabled");
    }
}

void TimerSystem::notifyGamePaused(bool isPaused) {
    if (!config_.enabled) return;
    
    if (isPaused && state_ == State::RUNNING) {
        // Pausar timer quando jogo pausa
        pause();
    } else if (!isPaused && state_ == State::PAUSED) {
        // Retomar timer quando jogo despausa
        resume();
    }
}

void TimerSystem::updateRemainingTime() {
    if (state_ != State::RUNNING) return;
    
    Uint32 currentTime = SDL_GetTicks();
    Uint32 elapsedTime = (currentTime - startTime_) - pausedTime_;
    int elapsedSeconds = elapsedTime / 1000;
    
    remainingSeconds_ = config_.durationSeconds - elapsedSeconds;
    
    if (remainingSeconds_ <= 0) {
        remainingSeconds_ = 0;
        state_ = State::EXPIRED;
        DebugLogger::info("Timer expired!");
    }
}

float TimerSystem::getProgress() const {
    if (config_.durationSeconds <= 0) return 1.0f;
    
    float progress = 1.0f - (static_cast<float>(remainingSeconds_) / static_cast<float>(config_.durationSeconds));
    return std::max(0.0f, std::min(1.0f, progress));
}

std::string TimerSystem::getFormattedTime() const {
    int minutes = remainingSeconds_ / 60;
    int seconds = remainingSeconds_ % 60;
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << seconds;
    
    return ss.str();
}

bool TimerSystem::isWarning() const {
    return config_.showWarningAt30s && remainingSeconds_ <= 30 && remainingSeconds_ > 10;
}

bool TimerSystem::isCritical() const {
    return config_.showWarningAt10s && remainingSeconds_ <= 10;
}

RGB TimerSystem::getCurrentColor() const {
    if (isCritical()) {
        return config_.criticalColor;
    } else if (isWarning()) {
        return config_.warningColor;
    } else {
        return config_.normalColor;
    }
}

void TimerSystem::setConfig(const TimerConfig& config) {
    bool wasEnabled = config_.enabled;
    config_ = config;
    remainingSeconds_ = config_.durationSeconds;
    
    // Se estava desabilitado e agora está habilitado, não inicia automaticamente
    // Se estava habilitado e agora está desabilitado, para o timer
    if (wasEnabled && !config_.enabled) {
        stop();
    }
}

void TimerSystem::setEnabled(bool enabled) {
    bool wasEnabled = config_.enabled;
    config_.enabled = enabled;
    
    if (wasEnabled && !enabled) {
        stop();
    } else if (!wasEnabled && enabled) {
        remainingSeconds_ = config_.durationSeconds;
        // Não inicia automaticamente, apenas habilita
    }
}

void TimerSystem::setDuration(int seconds) {
    if (seconds > 0) {
        config_.durationSeconds = seconds;
        
        // Se o timer está parado, atualiza o tempo restante
        if (state_ == State::STOPPED) {
            remainingSeconds_ = seconds;
        }
    }
}

void TimerSystem::update() {
    if (!config_.enabled || state_ != State::RUNNING) return;
    
    Uint32 currentTime = SDL_GetTicks();
    
    // Atualiza a cada segundo ou se for a primeira atualização
    if (lastUpdateTime_ == 0 || (currentTime - lastUpdateTime_) >= 1000) {
        updateRemainingTime();
        lastUpdateTime_ = currentTime;
        
        // Log de transições de estado
        bool isWarn = isWarning();
        bool isCrit = isCritical();
        
        if (isCrit && !wasCritical_) {
            DebugLogger::info("Timer entering critical state: " + std::to_string(remainingSeconds_) + "s remaining");
            wasCritical_ = true;
        } else if (isWarn && !wasWarning_ && !isCrit) {
            DebugLogger::info("Timer entering warning state: " + std::to_string(remainingSeconds_) + "s remaining");
            wasWarning_ = true;
        }
    }
}