#pragma once

struct GameConfig;

class ScoreSystem {
private:
    int score_ = 0;
    int lines_ = 0;
    int level_ = 0;
    int tickMs_ = 0;
public:
    ScoreSystem();
    int getScore() const;
    int getLines() const;
    int getLevel() const;
    int getTickMs() const;
    void addScore(int points);
    void addLines(int lines);
    void reset();
    void setTickMs(int ms);
};


