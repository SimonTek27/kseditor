#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include "Math/MathCore.h"

namespace ks {

struct ThreeMFMesh {
    QString name;
    QVector<Vec3> vertices;
    QVector<uint32_t> indices;
    QString materialId;
};

struct ThreeMFMaterial {
    QString id;
    QString name;
    Vec3 color = {1, 1, 1};
    float opacity = 1.0f;
};

struct ThreeMFScene {
    QString name;
    QVector<ThreeMFMesh> meshes;
    QMap<QString, ThreeMFMaterial> materials;
};

class CAD3MFParser {
public:
    CAD3MFParser() = default;

    bool loadFromFile(const QString& filePath);
    const ThreeMFScene& scene() const { return m_scene; }
    QString lastError() const { return m_lastError; }

private:
    ThreeMFScene m_scene;
    QString m_lastError;

    bool parseZip(const QByteArray& data);
};

} // namespace ks