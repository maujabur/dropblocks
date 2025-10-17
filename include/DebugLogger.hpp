#pragma once

#include <string>

class DebugLogger {
public:
    enum Level { ERROR = 0, WARNING = 1, INFO = 2, DEBUG = 3 };

    static void setEnabled(bool enabled);
    static void setLevel(int level);

    static void error(const std::string& message);
    static void warning(const std::string& message);
    static void info(const std::string& message);
    static void debug(const std::string& message);
};


