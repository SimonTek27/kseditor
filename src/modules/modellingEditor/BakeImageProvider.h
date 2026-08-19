#pragma once

#include <QQuickImageProvider>
#include "3DModelingQmlBridge.h"

namespace ks {

// Serves the latest baked texture map to QML via "image://bake/result" URLs.
// The revision parameter (image://bake/result?rev=N) changes on every bake so
// the Image element re-fetches the cached image.
class BakeImageProvider : public QQuickImageProvider {
public:
    BakeImageProvider(KSModelerQml* bridge)
        : QQuickImageProvider(QQuickImageProvider::Image), m_bridge(bridge) {}

    QImage requestImage(const QString& id, QSize* size, const QSize&) override {
        if (size) *size = m_bridge->bakeResultImage().size();
        return m_bridge->bakeResultImage();
    }

private:
    KSModelerQml* m_bridge;
};

} // namespace ks
