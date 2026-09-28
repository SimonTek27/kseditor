#pragma once
#include <string>

namespace ks {
namespace engine {
namespace sys {

class SystemDllInitializer {
public:
    static SystemDllInitializer& instance() { static SystemDllInitializer s; return s; }
    bool initialize() { m_ok = true; return true; }
    void shutdown() { m_ok = false; }
    bool isReady() const { return m_ok; }

private:
    bool m_ok = false;
};

} // namespace sys
} // namespace engine
} // namespace ks
