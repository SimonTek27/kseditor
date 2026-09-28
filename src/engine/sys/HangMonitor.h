#pragma once
#include <string>
namespace ks { namespace engine { namespace sys {
class HangMonitor {
public:
    static HangMonitor& instance() { static HangMonitor s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    void heartbeat() {}
};
}}} // namespace
