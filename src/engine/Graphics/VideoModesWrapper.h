#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include "VideoModesFunctions.h"

namespace ks::engine::graphics {

// ============================================================================
// VideoModesWrapper - High-level wrapper for ksengineVideoModes.dll
// Functions are linked statically via ksengineVideoModes.lib
// ============================================================================

class VideoModesWrapper : public QObject {
    Q_OBJECT

public:
    static VideoModesWrapper* instance();

    explicit VideoModesWrapper(QObject* parent = nullptr);
    ~VideoModesWrapper();

    // --- Initialization ---
    bool initialize();
    void shutdown();
    bool isInitialized() const { return m_initialized; }

    // --- Video mode query ---
    bool getMode(int modeIndex, VideoModeInfo& info) const;

    // --- Raw access ---
    bool isAvailable() const { return m_initialized; }

signals:
    void initialized();
    void error(const QString& message);

private:
    bool m_initialized = false;
};

} // namespace ks::engine::graphics
