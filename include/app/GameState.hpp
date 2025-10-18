#pragma once
#include <memory>
#include <vector>
#include <SDL2/SDL.h>
#include "app/GameTypes.hpp"
#include "app/GameBoard.hpp"
#include "app/ScoreSystem.hpp"
#include "app/ComboSystem.hpp"
#include "Interfaces.hpp"

class RenderManager;
struct LayoutCache;
class DependencyContainer;

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
    std::vector<int> pieceStats_; // contador de cada tipo de peça sorteada
    
    std::unique_ptr<IAudioSystem> audio_;
    std::unique_ptr<IThemeManager> theme_;
    std::unique_ptr<IPieceManager> pieces_;
    std::unique_ptr<IInputManager> input_;
    std::unique_ptr<IGameConfig> config_;
    
    void restartRound();

public:
    GameState(DependencyContainer& container);
    GameState();
    
    void setDependencies(class AudioSystem* audio, class ThemeManager* theme, 
                        class PieceManager* pieces, class InputManager* input, 
                        class ConfigManager* config);
    
    GameBoard& getBoard();
    const GameBoard& getBoard() const;
    ScoreSystem& getScore();
    const ScoreSystem& getScore() const;
    ComboSystem& getCombo();
    const ComboSystem& getCombo() const;
    IPieceManager& getPieces();
    const IPieceManager& getPieces() const;
    
    Active& getActivePiece();
    const Active& getActivePiece() const;
    void setActivePiece(const Active& piece);
    
    bool isRunning() const;
    bool isPaused() const;
    bool isGameOver() const;
    void setRunning(bool v);
    void setPaused(bool v);
    void setGameOver(bool v);
    
    Uint32 getLastTick() const;
    void setLastTick(Uint32 t);
    
    void reset();
    void update(SDL_Renderer* renderer);
    void render(RenderManager& renderManager, const LayoutCache& layout);
    void handleInput(SDL_Renderer* renderer);
    void updatePiece();
    
    // Backward compat
    std::vector<std::vector<Cell>>& grid;
    Active& act;
    bool& running;
    bool& paused;
    bool& gameover;
    Uint32& lastTick;
    ComboSystem& combo;
    
    int getScoreValue() const;
    int getLinesValue() const;
    int getLevelValue() const;
    int getTickMsValue() const;
    void setScore(int score);
    void setLines(int lines);
    void setLevel(int level);
    void setTickMs(int tickMs);
    int getNextIdx() const;
    
    // Piece statistics
    const std::vector<int>& getPieceStats() const;
    void incrementPieceStat(int pieceIdx);
    void resetPieceStats();
};
