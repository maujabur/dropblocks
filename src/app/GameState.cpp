#include "app/GameState.hpp"
#include "app/GameHelpers.hpp"
#include "audio/AudioSystem.hpp"
#include "ThemeManager.hpp"
#include "pieces/PieceManager.hpp"
#include "input/InputManager.hpp"
#include "ConfigManager.hpp"
#include "render/RenderManager.hpp"
#include "render/LayoutCache.hpp"
#include "game/Mechanics.hpp"
#include "util/UiUtil.hpp"
#include "DebugLogger.hpp"
#include "pieces/Piece.hpp"
#include <ctime>

extern std::vector<Piece> PIECES;

// DependencyContainer forward (defined in dropblocks.cpp)
class DependencyContainer;

GameState::GameState(DependencyContainer& container)
    : grid(board_.getGrid()), act(activePiece_), running(running_), paused(paused_),
      gameover(gameover_), lastTick(lastTick_), combo(combo_)
{
    // DI constructor stub (container.resolve not implemented here yet)
    lastTick_ = SDL_GetTicks();
}

GameState::GameState()
    : grid(board_.getGrid()), act(activePiece_), running(running_), paused(paused_),
      gameover(gameover_), lastTick(lastTick_), combo(combo_)
{
    lastTick_ = SDL_GetTicks();
    audio_ = nullptr;
    theme_ = nullptr;
    pieces_ = nullptr;
    input_ = nullptr;
    config_ = nullptr;
}

void GameState::setDependencies(AudioSystem* audio, ThemeManager* theme, PieceManager* pieces, InputManager* input, ConfigManager* config) {
    DebugLogger::info("Setting dependencies for GameState");
    audio_ = std::unique_ptr<IAudioSystem>(audio);
    theme_ = std::unique_ptr<IThemeManager>(theme);
    pieces_ = std::unique_ptr<IPieceManager>(pieces);
    input_ = std::unique_ptr<IInputManager>(input);
    config_ = std::unique_ptr<IGameConfig>(config);
    
    if (!audio_) DebugLogger::error("Audio dependency not set");
    if (!theme_) DebugLogger::error("Theme dependency not set");
    if (!pieces_) DebugLogger::error("Pieces dependency not set");
    if (!input_) DebugLogger::error("Input dependency not set");
    if (!config_) DebugLogger::error("Config dependency not set");
    
    DebugLogger::info("Dependencies set successfully");
}

GameBoard& GameState::getBoard() { return board_; }
const GameBoard& GameState::getBoard() const { return board_; }
ScoreSystem& GameState::getScore() { return score_; }
const ScoreSystem& GameState::getScore() const { return score_; }
ComboSystem& GameState::getCombo() { return combo_; }
const ComboSystem& GameState::getCombo() const { return combo_; }
IPieceManager& GameState::getPieces() { return *pieces_; }
const IPieceManager& GameState::getPieces() const { return *pieces_; }

Active& GameState::getActivePiece() { return activePiece_; }
const Active& GameState::getActivePiece() const { return activePiece_; }
void GameState::setActivePiece(const Active& piece) { activePiece_ = piece; }

bool GameState::isRunning() const { return running_; }
bool GameState::isPaused() const { return paused_; }
bool GameState::isGameOver() const { return gameover_; }
void GameState::setRunning(bool v) { running_ = v; }
void GameState::setPaused(bool v) { paused_ = v; }
void GameState::setGameOver(bool v) { gameover_ = v; }

Uint32 GameState::getLastTick() const { return lastTick_; }
void GameState::setLastTick(Uint32 t) { lastTick_ = t; }

void GameState::reset() {
    board_.reset();
    score_.reset();
    combo_.reset();
    gameover_ = false;
    paused_ = false;
    lastTick_ = SDL_GetTicks();
    resetPieceStats();
}

void GameState::restartRound() {
    reset();
    if (pieces_) {
        pieces_->initializeRandomizer();
        pieces_->reset();
    }
    int first = pieces_ ? pieces_->getNextPiece() : 0;
    newActive(activePiece_, first);
    incrementPieceStat(first);
    if (pieces_) pieces_->setNextPiece(pieces_->getNextPiece());
    setLastTick(SDL_GetTicks());
    if (input_) input_->resetTimers();
    if (audio_) static_cast<AudioSystem&>(*audio_).playBeep(520.0, 40, 0.15f, false);
}

void GameState::updatePiece() {
    if (!audio_) { DebugLogger::error("Audio system not initialized in updatePiece()"); return; }
    
    auto coll = [&](int dx, int dy, int drot) { return !board_.canPlacePiece(activePiece_, dx, dy, drot); };
    if (!coll(0, 1, 0)) {
        activePiece_.y++;
    } else {
        board_.placePiece(activePiece_);
        audio_->playBeep(220.0, 25, 0.12f, true);
        
        int c = board_.clearLines();
        if (c > 0) {
            score_.addLines(c);
            combo_.onLineClear(static_cast<AudioSystem&>(*audio_));
            
            if (c == 4) {
                audio_->playTetrisSound();
            } else {
                double freq = 440.0 + (c * 110.0);
                audio_->playBeep(freq, 30 + c * 10, 0.18f, false);
            }
            
            int points = (c == 1 ? 100 : c == 2 ? 300 : c == 3 ? 500 : 800) * (score_.getLevel() + 1);
            score_.addScore(points);
        } else {
            combo_.reset();
        }
        
        int nextPiece = pieces_->getCurrentNextPiece();
        newActive(activePiece_, nextPiece);
        incrementPieceStat(nextPiece);
        pieces_->setNextPiece(pieces_->getNextPiece());
        if (board_.isGameOver(activePiece_)) {
            gameover_ = true;
            paused_ = false;
            combo_.reset();
            audio_->playGameOverSound();
        }
    }
}

int GameState::getScoreValue() const { return score_.getScore(); }
int GameState::getLinesValue() const { return score_.getLines(); }
int GameState::getLevelValue() const { return score_.getLevel(); }
int GameState::getTickMsValue() const { return score_.getTickMs(); }
void GameState::setScore(int score) { score_.addScore(score - score_.getScore()); }
void GameState::setLines(int lines) { score_.addLines(lines - score_.getLines()); }
void GameState::setLevel(int level) {
    int currentLines = score_.getLines();
    int targetLines = level * 10;
    score_.addLines(targetLines - currentLines);
}
void GameState::setTickMs(int tickMs) { score_.setTickMs(tickMs); }
int GameState::getNextIdx() const { return pieces_->getCurrentNextPiece(); }

void GameState::update(SDL_Renderer* renderer) {
    if (!input_ || !audio_) { DebugLogger::error("Dependencies not initialized in update()"); return; }
    
    handleInput(renderer);
    
    if (!isPaused() && !isGameOver()) {
        Uint32 now = SDL_GetTicks();
        if (now - getLastTick() >= (Uint32)getScore().getTickMs()) {
            updatePiece();
            setLastTick(now);
        }
        
        getBoard().checkTension(static_cast<AudioSystem&>(*audio_));
        audio_->playBackgroundMelody(getScore().getLevel());
    }
}

void GameState::render(RenderManager& renderManager, const LayoutCache& layout) {
    renderManager.render(*this, layout);
}

void GameState::handleInput(SDL_Renderer* renderer) {
    if (!input_ || !audio_) { DebugLogger::error("Dependencies not initialized in handleInput()"); return; }
    
    input_->update();
    
    if (input_->shouldScreenshot()) {
        time_t now = time(0);
        struct tm* timeinfo = localtime(&now);
        char filename[64];
        strftime(filename, sizeof(filename), "dropblocks-screenshot_%Y-%m-%d_%H-%M-%S.bmp", timeinfo);
        if (saveScreenshot(renderer, filename)) audio_->playBeep(880.0, 80, 0.18f, false);
    }
    
    if (input_->shouldQuit()) {
        setRunning(false);
    }
    
    if (input_->shouldPause()) {
        setPaused(!isPaused());
        audio_->playBeep(isPaused() ? 440.0 : 520.0, 30, 0.12f, false);
    }
    
    // Force restart (R key) - works anytime
    if (input_->shouldForceRestart()) {
        restartRound();
        return;
    }
    
    // Normal restart (RETURN) - only on game over
    if (isGameOver() && input_->shouldRestart()) {
        restartRound();
        return;
    }
    
    if (!isPaused() && !isGameOver()) {
        auto coll = [&](int dx, int dy, int drot) { return !board_.canPlacePiece(activePiece_, dx, dy, drot); };
        
        if (input_->shouldMoveLeft() && !coll(-1, 0, 0)) {
            activePiece_.x--;
            audio_->playMovementSound();
        }
        
        if (input_->shouldMoveRight() && !coll(1, 0, 0)) {
            activePiece_.x++;
            audio_->playMovementSound();
        }
        
        if (input_->shouldSoftDrop()) {
            audio_->playSoftDropSound();
            updatePiece();
        }
        
        if (input_->shouldHardDrop()) {
            int maxSteps = (int)board_.getGrid().size() + 10;
            for (int i = 0; i < maxSteps && !coll(0, 1, 0); i++) { activePiece_.y++; }
            audio_->playHardDropSound();
            updatePiece();
        }
        
        if (input_->shouldRotateCCW()) {
            rotateWithKicks(activePiece_, board_.getGrid(), -1, static_cast<AudioSystem&>(*audio_));
            audio_->playRotationSound(false);
        }
        
        if (input_->shouldRotateCW()) {
            rotateWithKicks(activePiece_, board_.getGrid(), +1, static_cast<AudioSystem&>(*audio_));
            audio_->playRotationSound(true);
        }
    }
}

const std::vector<int>& GameState::getPieceStats() const {
    return pieceStats_;
}

void GameState::incrementPieceStat(int pieceIdx) {
    if (pieceIdx < 0) return;
    // Expandir o vetor se necessário
    if (pieceIdx >= (int)pieceStats_.size()) {
        pieceStats_.resize(pieceIdx + 1, 0);
    }
    pieceStats_[pieceIdx]++;
}

void GameState::resetPieceStats() {
    pieceStats_.clear();
    // Inicializar com zeros para todas as peças conhecidas
    if (!PIECES.empty()) {
        pieceStats_.resize(PIECES.size(), 0);
    }
}
