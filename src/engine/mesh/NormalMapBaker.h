#pragma once

#include <QVector>
#include <QVector2D>
#include <QVector3D>

namespace ks {

struct BakerVertex {
    QVector3D position;
    QVector3D normal;
    QVector2D texCoord;
    QVector3D tangent;
    QVector3D bitangent;
};

struct BakerMesh {
    QVector<BakerVertex> vertices;
    QVector<QVector<int>> faces;
};

class NormalMapBaker {
public:
    static bool computeTangents(BakerMesh& mesh);
    static bool bakeNormalMap(const BakerMesh& highPoly, const BakerMesh& lowPoly,
                              const QVector2D& textureSize, QVector<QVector3D>& normalMap);
};

}