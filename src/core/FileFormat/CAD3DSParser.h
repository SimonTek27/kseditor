#pragma once

#include <QString>
#include <QVector>
#include "Math/MathCore.h"

namespace ks {

struct TDSVertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;
    QVector<Vec2> extraUVs;
};

struct TDSMesh {
    QString name;
    QVector<TDSVertex> vertices;
    QVector<uint32_t> indices;
    QString materialName;
    Vec3 color;
    float opacity = 1.0f;
};

struct TDSMaterial {
    QString name;
    Vec3 ambient = {0.2f, 0.2f, 0.2f};
    Vec3 diffuse = {0.8f, 0.8f, 0.8f};
    Vec3 specular = {0, 0, 0};
    float shininess = 0.0f;
    float transparency = 0.0f;
    QString textureMap1;
    QString textureMap2;
};

struct TDSKeyframe {
    int frame;
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
};

struct TDSObject {
    QString name;
    TDSMesh mesh;
    QVector<TDSKeyframe> keyframes;
};

struct TDSScene {
    QString name;
    QVector<TDSObject> objects;
    QVector<TDSMaterial> materials;
    int materialCount = 0;
};

class CAD3DSParser {
public:
    CAD3DSParser() = default;

    bool loadFromFile(const QString& filePath);
    const TDSScene& scene() const { return m_scene; }
    QString lastError() const { return m_lastError; }

private:
    TDSScene m_scene;
    QString m_lastError;

    bool parseChunks(const QByteArray& data, int& pos, int endPos);
    bool parseNamedObject(const QByteArray& data, int& pos, int endPos);
    bool parseTriMesh(const QByteArray& data, int& pos, int endPos, TDSMesh& mesh);
    bool parseMatEntry(const QByteArray& data, int& pos, int endPos);
};

} // namespace ks