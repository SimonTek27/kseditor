#pragma once

#include <QString>
#include <QVector>
#include <QByteArray>
#include "Math/MathCore.h"

namespace ks {

struct TerrainLayer {
    QString name;
    QString blendmapPath;
    QString texturePath;
    QString normalPath;
    float tileSize = 1.0f;
    float blendStrength = 1.0f;
    float heightOffset = 0.0f;
    float heightScale = 1.0f;
};

struct TerrainFile {
    int resolution = 256;
    float size = 1000.0f;
    float heightScale = 100.0f;
    float seaLevel = 0.0f;
    float position[3] = {0, 0, 0};

    QVector<float> heightData;
    QVector<QByteArray> splatMaps;
    QVector<TerrainLayer> layers;

    // Optional detailed info
    float minHeight = 0.0f;
    float maxHeight = 0.0f;
    bool hasHeightmap = false;

    // Derived
    int verticesPerSide = 0;
    int triangles = 0;
};

class TerrainFormatParser {
public:
    static bool loadHeightmap(const QString& filePath, TerrainFile& outTerrain);
    static bool loadRAW(const QString& filePath, TerrainFile& outTerrain);
    static bool loadPNG(const QString& filePath, TerrainFile& outTerrain);
    static bool loadTER(const QString& filePath, TerrainFile& outTerrain);

    static bool saveRAW(const QString& filePath, const TerrainFile& terrain);
    static bool savePNG(const QString& filePath, const TerrainFile& terrain);

    static QString lastError() { return s_lastError; }

    // Generate procedural terrain
    static bool generateProcedural(TerrainFile& outTerrain, int resolution, float size, float heightScale, int seed = 0);

private:
    static QString s_lastError;

    static void normalizeHeightData(TerrainFile& terrain);
};

} // namespace ks