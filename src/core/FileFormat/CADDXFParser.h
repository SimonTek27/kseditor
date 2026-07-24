#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include "Math/MathCore.h"

namespace ks {

struct DXFEntity {
    QString type;
    int handle;
    QMap<int, QString> values; // group code -> value
};

struct DXFLayer {
    QString name;
    int colorNumber = 7;
    bool isFrozen = false;
    bool isLocked = false;
};

struct DXFMesh {
    QString name;
    QVector<Vec3> vertices;
    QVector<uint32_t> indices;
    QString layerName;
};

struct DXFScene {
    QString name;
    QString version;
    QVector<DXFLayer> layers;
    QVector<DXFMesh> meshes;
    QVector<DXFEntity> entities;
};

class CADDXFParser {
public:
    CADDXFParser() = default;

    bool loadFromFile(const QString& filePath);
    const DXFScene& scene() const { return m_scene; }
    QString lastError() const { return m_lastError; }

private:
    DXFScene m_scene;
    QString m_lastError;

    bool parseContent(const QString& content);
    void parseEntities(const QVector<QPair<int, QString>>& groups, int& pos);
    void parsePolyline(const QVector<QPair<int, QString>>& groups, int& pos, DXFMesh& mesh);
    void parseInsert(const QVector<QPair<int, QString>>& groups, int& pos);
};

} // namespace ks