#pragma once

#include <string>

namespace ks {

/**
 * Lightweight engine module interface — Qt-free.
 * Modules are ticked by Engine::tick() with fixed dt.
 */
class EngineModule {
public:
    virtual ~EngineModule() = default;

    virtual std::string moduleName() const = 0;
    virtual std::string moduleId() const { return moduleName(); }

    virtual bool initialize() {
        m_initialized = true;
        return true;
    }
    virtual void shutdown() { m_initialized = false; }
    virtual bool isInitialized() const { return m_initialized; }

    /** Called each fixed physics/engine step (seconds). */
    virtual void update(double /*dt*/) {}

    virtual int priority() const { return 0; }

protected:
    bool m_initialized = false;
};

// Transitional alias for older includes under ks::engine
namespace engine {
using EngineModule = ::ks::EngineModule;
}

} // namespace ks
