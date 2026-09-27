#pragma once

#include <string>

namespace ks {
namespace engine {

/**
 * Lightweight engine module interface - no Qt dependency.
 */
class EngineModule {
public:
    virtual ~EngineModule() = default;

    virtual std::string moduleName() const = 0;
    virtual std::string moduleId() const = 0;

    virtual bool initialize() { return true; }
    virtual void shutdown() {}
    virtual bool isInitialized() const { return m_initialized; }

    virtual int priority() const { return 0; }

protected:
    bool m_initialized = false;
};

} // namespace engine
} // namespace ks
