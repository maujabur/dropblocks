#include "app/ScoreSystem.hpp"
#include "ConfigTypes.hpp"

extern GameConfig gameConfig;
extern const int SPEED_ACCELERATION;

ScoreSystem::ScoreSystem() { tickMs_ = gameConfig.tickMsStart; }
int ScoreSystem::getScore() const { return score_; }
int ScoreSystem::getLines() const { return lines_; }
int ScoreSystem::getLevel() const { return level_; }
int ScoreSystem::getTickMs() const { return tickMs_; }
void ScoreSystem::addScore(int points) { score_ += points; }
void ScoreSystem::addLines(int lines) {
    lines_ += lines; level_ = lines_ / 10;
    tickMs_ = gameConfig.tickMsStart - (level_ * SPEED_ACCELERATION);
    if (tickMs_ < gameConfig.tickMsMin) tickMs_ = gameConfig.tickMsMin;
}
void ScoreSystem::reset() { score_ = 0; lines_ = 0; level_ = 0; tickMs_ = gameConfig.tickMsStart; }
void ScoreSystem::setTickMs(int ms) { tickMs_ = ms; }


