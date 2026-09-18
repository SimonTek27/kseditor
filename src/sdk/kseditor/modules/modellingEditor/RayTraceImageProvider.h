#pragma once

#include <QQuickImageProvider>
#include "3DModelingQmlBridge.h"

namespace ks {

// Serves the latest CPU-rendered raytrace frame to QML via
// "image://raytrace/frame" URLs. The URL changes whenever the frame
// revision increments, forcing a re-fetch from the cached image.
class RayTraceImageProvider : public QQuickImageProvider {
public:
    RayTraceImageProvider(KSModelerQml* bridge)
        : QQuickImageProvider(QQuickImageProvider::Image), m_bridge(bridge) {}

    QImage requestImage(const QString& id, QSize* size, const QSize&) override {
        if (size) *size = m_bridge->rayTraceFrameImage().size();
        return m_bridge->rayTraceFrameImage();
    }

private:
    KSModelerQml* m_bridge;
};

} // namespace ks
