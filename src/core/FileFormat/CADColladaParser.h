#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include "Math/MathCore.h"

namespace ks {

struct ColladaVertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;
};

struct ColladaMesh {
    QString name;
    QString materialId;
    QVector<ColladaVertex> vertices;
    QVector<uint32_t> indices;
};

struct ColladaMaterial {
    QString id;
    QString name;
    Vec3 diffuse = {0.8f, 0.8f, 0.8f};
    Vec3 specular = {0.0f, 0.0f, 0.0f};
    Vec3 ambient = {0.2f, 0.2f, 0.2f};
    float shininess = 32.0f;
    float transparency = 1.0f;
    QString diffuseTexture;
};

struct ColladaScene {
    QString name;
    QVector<ColladaMesh> meshes;
    QMap<QString, ColladaMaterial> materials;
    QMap<QString, QString> metadata;
};

class CADColladaParser {
public:
    CADColladaParser() = default;

    bool loadFromFile(const QString& filePath);
    const ColladaScene& scene() const { return m_scene; }
    QString lastError() const { return m_lastError; }

private:
    ColladaScene m_scene;
    QString m_lastError;

    bool parseLibraryGeometries(class QXmlStreamReader& xml);
    bool parseLibraryMaterials(class QXmlStreamReader& xml);
    bool parseLibraryEffects(class QXmlStreamReader& xml);
    bool parseLibraryImages(class QXmlStreamReader& xml);
    bool parseMesh(class QXmlStreamReader& xml, const QString& meshId);
    QVector<float> parseFloatArray(class QXmlStreamReader& xml, int count);
    uint32_t parseAccessor(class QXmlStreamReader& xml, const QString& sourceId);
};

} // namespace ks