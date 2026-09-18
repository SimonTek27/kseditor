#include "SystemDllInitializer.h"
#include "SystemDllManager.h"
#include "../Audio/FmodRuntimeWrapper.h"
#include "../Graphics/VideoModesWrapper.h"
#include "../assets/RaceFlagAssetLoader.h"
#include <QDebug>

namespace ks::engine::sys {

// ============================================================================
// Singleton
// ============================================================================

SystemDllInitializer* SystemDllInitializer::instance() {
    static SystemDllInitializer inst;
    return &inst;
}

// ============================================================================
// Construction / Destruction
// ============================================================================

SystemDllInitializer::SystemDllInitializer(QObject* parent)
    : QObject(parent) {
}

SystemDllInitializer::~SystemDllInitializer() {
    shutdownAll();
}

// ============================================================================
// Initialization
// ============================================================================

SystemDllInitializer::InitResult SystemDllInitializer::initializeAll() {
    if (m_initialized) return m_lastResult;

    InitResult result;
    int totalLoaded = 0;
    int totalFailed = 0;

    qDebug() << "SystemDllInitializer: Starting initialization...";

    // Check DLL availability via SystemDllManager
    auto* mgr = SystemDllManager::instance();

    // Initialize FMOD Core
    if (m_fmodEnabled) {
        if (initializeFmodCore()) {
            result.fmodCore = true;
            totalLoaded++;
            emit componentLoaded("FMOD Core");
        } else {
            result.errors.append("FMOD Core initialization failed");
            totalFailed++;
            emit componentFailed("FMOD Core", "FMOD Core initialization failed");
        }
    }

    // Initialize FMOD Studio (optional)
    if (m_fmodStudioEnabled && result.fmodCore) {
        if (initializeFmodStudio()) {
            result.fmodStudio = true;
            totalLoaded++;
            emit componentLoaded("FMOD Studio");
        } else {
            result.errors.append("FMOD Studio initialization failed");
            totalFailed++;
            emit componentFailed("FMOD Studio", "FMOD Studio initialization failed");
        }
    }

    // Initialize Video Modes
    if (m_videoModesEnabled) {
        if (initializeVideoModes()) {
            result.videoModes = true;
            totalLoaded++;
            emit componentLoaded("Video Modes");
        } else {
            result.errors.append("Video Modes initialization failed");
            totalFailed++;
            emit componentFailed("Video Modes", "Video Modes initialization failed");
        }
    }

    // Initialize Streamline
    if (m_streamlineEnabled) {
        if (initializeStreamline()) {
            result.streamline = true;
            totalLoaded++;
            emit componentLoaded("Streamline");
        } else {
            result.errors.append("Streamline initialization failed");
            totalFailed++;
            emit componentFailed("Streamline", "Streamline initialization failed");
        }
    }

    // Initialize Race Flags
    if (m_raceFlagsEnabled) {
        if (initializeRaceFlags()) {
            result.raceFlags = true;
            totalLoaded++;
            emit componentLoaded("Race Flags");
        } else {
            result.errors.append("Race Flags failed to load");
            totalFailed++;
            emit componentFailed("Race Flags", "Raceflags directory not found");
        }
    }

    result.totalLoaded = totalLoaded;
    result.totalFailed = totalFailed;
    m_lastResult = result;
    m_initialized = true;

    qDebug() << "SystemDllInitializer: Initialization complete -"
             << totalLoaded << "loaded," << totalFailed << "failed";

    emit initialized(result);
    return result;
}

void SystemDllInitializer::shutdownAll() {
    if (!m_initialized) return;

    qDebug() << "SystemDllInitializer: Shutting down...";

    // Shutdown in reverse order
    if (m_raceFlagsEnabled) {
        // RaceFlagAssetLoader is a singleton, no explicit shutdown needed
    }

    if (m_streamlineEnabled) {
        // StreamlineIntegration handles its own shutdown
    }

    if (m_videoModesEnabled) {
        auto* videoModes = graphics::VideoModesWrapper::instance();
        if (videoModes->isInitialized()) {
            videoModes->shutdown();
        }
    }

    if (m_fmodStudioEnabled) {
        auto* fmod = audio::FmodRuntimeWrapper::instance();
        if (fmod->isStudioInitialized()) {
            fmod->shutdownStudio();
        }
    }

    if (m_fmodEnabled) {
        auto* fmod = audio::FmodRuntimeWrapper::instance();
        if (fmod->isInitialized()) {
            fmod->shutdown();
        }
    }

    m_initialized = false;
    qDebug() << "SystemDllInitializer: Shutdown complete";
}

// ============================================================================
// Individual component initialization
// ============================================================================

bool SystemDllInitializer::initializeFmodCore() {
    auto* fmod = audio::FmodRuntimeWrapper::instance();

    // FMOD functions are linked statically - just create and init the system
    if (!fmod->createSystem()) {
        return false;
    }

    if (!fmod->initSystem()) {
        return false;
    }

    qDebug() << "SystemDllInitializer: FMOD Core initialized";
    return true;
}

bool SystemDllInitializer::initializeFmodStudio() {
    auto* fmod = audio::FmodRuntimeWrapper::instance();
    if (!fmod->isInitialized()) {
        qWarning() << "SystemDllInitializer: Cannot initialize FMOD Studio without Core";
        return false;
    }

    // FMOD Studio functions are linked statically
    if (!fmod->createStudioSystem()) {
        return false;
    }

    if (!fmod->initStudioSystem()) {
        return false;
    }

    qDebug() << "SystemDllInitializer: FMOD Studio initialized";
    return true;
}

bool SystemDllInitializer::initializeVideoModes() {
    auto* videoModes = graphics::VideoModesWrapper::instance();

    // VideoModes functions are linked statically
    if (!videoModes->initialize()) {
        return false;
    }

    qDebug() << "SystemDllInitializer: Video Modes initialized";
    return true;
}

bool SystemDllInitializer::initializeStreamline() {
    // Streamline is initialized by StreamlineIntegration when needed
    // sl.common.dll availability is checked but full init is deferred
    auto* mgr = SystemDllManager::instance();
    bool available = mgr->isAvailable(DllId::StreamlineCommon);
    if (!available) {
        qDebug() << "SystemDllInitializer: sl.common.dll not found, Streamline will be unavailable";
    }
    return available;
}

bool SystemDllInitializer::initializeRaceFlags() {
    auto* raceFlags = assets::RaceFlagAssetLoader::instance();
    if (!raceFlags->loadDefault()) {
        return false;
    }

    qDebug() << "SystemDllInitializer: Race Flags loaded -"
             << raceFlags->getFlagCount() << "flags";
    return true;
}

} // namespace ks::engine::sys
