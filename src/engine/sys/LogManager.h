#pragma once
#include <string>
#include <cstdio>
#include <mutex>
#include <vector>

namespace ks {
namespace engine {
namespace sys {

enum class LogLevel { Debug, Info, Warning, Error };

class LogManager {
public:
    static LogManager& instance() { static LogManager s; return s; }
    void log(LogLevel level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(m_mutex);
        const char* tag = "I";
        if (level == LogLevel::Debug) tag = "D";
        else if (level == LogLevel::Warning) tag = "W";
        else if (level == LogLevel::Error) tag = "E";
        std::fprintf(stderr, "[%s] %s\n", tag, msg.c_str());
        m_lines.push_back(msg);
        if (m_lines.size() > 2000) m_lines.erase(m_lines.begin(), m_lines.begin() + 500);
    }
    void info(const std::string& m) { log(LogLevel::Info, m); }
    void warn(const std::string& m) { log(LogLevel::Warning, m); }
    void error(const std::string& m) { log(LogLevel::Error, m); }
private:
    std::mutex m_mutex;
    std::vector<std::string> m_lines;
};

} // namespace sys
} // namespace engine
} // namespace ks
