#pragma once

/**
 * @file EngineModule.h
 * @brief Base interface for engine modules — Qt-free (no QObject)
 */

#include <string>

namespace ks {
namespace engine {

class EngineModule {
public:
    virtual ~EngineModule() = default;

    virtual std::string moduleName() const = 0;
    virtual std::string moduleId() const = 0;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual bool isInitialized() const { return m_initialized; }

    // Module priority (higher = initialized first)
    virtual int priority() const { return 0; }

protected:
    bool m_initialized = false;
};

} // namespace engine
} // namespace ks
