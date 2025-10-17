#include "../include/DebugLogger.hpp"
#include <cstdio>

static bool g_debug_enabled = true;
static int g_debug_level = DebugLogger::DEBUG;

void DebugLogger::setEnabled(bool enabled) { g_debug_enabled = enabled; }
void DebugLogger::setLevel(int level) { g_debug_level = level; }

static void logImpl(DebugLogger::Level level, const std::string& message) {
    if (!g_debug_enabled || level > g_debug_level) return;
    const char* prefix = "";
    switch (level) {
        case DebugLogger::ERROR:   prefix = "ERROR"; break;
        case DebugLogger::WARNING: prefix = "WARNING"; break;
        case DebugLogger::INFO:    prefix = "INFO"; break;
        case DebugLogger::DEBUG:   prefix = "DEBUG"; break;
    }
    std::printf("[%s] %s\n", prefix, message.c_str());
    std::fflush(stdout);
}

void DebugLogger::error(const std::string& message)   { logImpl(ERROR,   message); }
void DebugLogger::warning(const std::string& message) { logImpl(WARNING, message); }
void DebugLogger::info(const std::string& message)    { logImpl(INFO,    message); }
void DebugLogger::debug(const std::string& message)   { logImpl(DEBUG,   message); }


