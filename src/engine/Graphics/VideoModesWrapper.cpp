#include "VideoModesWrapper.h"
#include <QDebug>

namespace ks::engine::graphics {

// ============================================================================
// Singleton
// ============================================================================

VideoModesWrapper* VideoModesWrapper::instance() {
    static VideoModesWrapper inst;
    return &inst;
}

// ============================================================================
// Construction / Destruction
// ============================================================================

VideoModesWrapper::VideoModesWrapper(QObject* parent)
    : QObject(parent) {
}

VideoModesWrapper::~VideoModesWrapper() {
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

bool VideoModesWrapper::initialize() {
    if (m_initialized) return true;

    int result = acInitVideoModes();
    if (result != 0) {
        emit error(QString("acInitVideoModes failed: %1").arg(result));
        qWarning() << "VideoModesWrapper: acInitVideoModes failed:" << result;
        return false;
    }

    m_initialized = true;
    qDebug() << "VideoModesWrapper: Initialized";
    emit initialized();
    return true;
}

void VideoModesWrapper::shutdown() {
    if (!m_initialized) return;

    m_initialized = false;
    qDebug() << "VideoModesWrapper: Shutdown";
}

// ============================================================================
// Video mode query
// ============================================================================

bool VideoModesWrapper::getMode(int modeIndex, VideoModeInfo& info) const {
    if (!m_initialized) return false;
    return acGetVideoMode(modeIndex, &info) == 0;
}

} // namespace ks::engine::graphics
