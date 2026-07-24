#pragma once

#include <QString>
#include <QVector>
#include "Math/MathCore.h"

namespace ks {

struct PLYVertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 color;
    bool hasColor = false;
};

struct PLYFace {
    QVector<int> vertexIndices;
};

struct PLYMesh {
    QString name;
    QVector<PLYVertex> vertices;
    QVector<PLYFace> faces;
};

struct PLYScene {
    QString name;
    PLYMesh mesh;
    bool isBinary = false;
    int vertexCount = 0;
    int faceCount = 0;
};

class CADPLYParser {
public:
    CADPLYParser() = default;

    bool loadFromFile(const QString& filePath);
    const PLYScene& scene() const { return m_scene; }
    QString lastError() const { return m_lastError; }

private:
    PLYScene m_scene;
    QString m_lastError;

    bool parseASCII(const QByteArray& data);
    bool parseBinaryLE(const QByteArray& data);
};

} // namespace ks