#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include "Math/MathCore.h"

namespace ks {

struct VRMLVertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;
    Vec3 color;
    bool hasColor = false;
};

struct VRMLMesh {
    QString name;
    QVector<VRMLVertex> vertices;
    QVector<uint32_t> indices;
    QString materialId;
};

struct VRMLMaterial {
    QString name;
    Vec3 diffuseColor = {0.8f, 0.8f, 0.8f};
    Vec3 specularColor = {0, 0, 0};
    Vec3 emissiveColor = {0, 0, 0};
    float shininess = 0.2f;
    float transparency = 0.0f;
    QString textureUrl;
};

struct VRMLScene {
    QString name;
    QString version = "2.0";
    QVector<VRMLMesh> meshes;
    QMap<QString, VRMLMaterial> materials;
};

class CADVRMLParser {
public:
    CADVRMLParser() = default;

    bool loadFromFile(const QString& filePath);
    const VRMLScene& scene() const { return m_scene; }
    QString lastError() const { return m_lastError; }

private:
    VRMLScene m_scene;
    QString m_lastError;

    bool parseContent(const QString& content);
    bool parseIndexedFaceSet(const QString& defName, const QString& content, VRMLMesh& mesh);
};

} // namespace ks