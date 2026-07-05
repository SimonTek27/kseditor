#include "TrackTerrainSystem.h"
#include <QFile>
#include <QTextStream>
#include <QImage>
#include <QDataStream>
#include <QDebug>
#include <QtMath>
#include <QMap>
#include <QPair>
#include <cfloat>
#include <random>

// ─── TrackTerrainSystem static methods ──────────────────────────────────

QVector<QVector<float>> TrackTerrainSystem::loadHeightMap(const QString& path)
{
    QVector<QVector<float>> map;
    QImage img(path);
    if (img.isNull()) return map;
    map.resize(img.height());
    for (int y = 0; y < img.height(); ++y) {
        map[y].resize(img.width());
        for (int x = 0; x < img.width(); ++x) {
            map[y][x] = qGray(img.pixel(x, y)) / 255.0f;
        }
    }
    return map;
}

bool TrackTerrainSystem::saveHeightMap(const QVector<QVector<float>>& heightMap, const QString& path)
{
    if (heightMap.isEmpty() || heightMap[0].isEmpty()) return false;
    int w = heightMap[0].size(), h = heightMap.size();
    QImage img(w, h, QImage::Format_Grayscale8);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            img.setPixel(x, y, qBound(0, static_cast<int>(heightMap[y][x] * 255), 255));
    return img.save(path);
}

QVector<QVector<float>> TrackTerrainSystem::generateHeightMap(int width, int height, float scale)
{
    QVector<QVector<float>> map(height, QVector<float>(width, 0.0f));
    std::mt19937 rng(42);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            float nx = x / (float)width * scale;
            float ny = y / (float)height * scale;
            map[y][x] = sin(nx) * cos(ny) * 0.5f + 0.5f;
        }
    return map;
}

TrackTerrainSystem::TerrainMesh TrackTerrainSystem::generateMesh(const QVector<QVector<float>>& hm, const TerrainConfig& cfg)
{
    TerrainMesh mesh;
    int w = hm.isEmpty() ? 0 : hm[0].size(), h = hm.size();
    if (w < 2 || h < 2) return mesh;
    mesh.width = w; mesh.height = h;
    float step = cfg.size / (float)qMax(w, h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            TerrainVertex v;
            v.x = x * step - cfg.size * 0.5f;
            v.z = y * step - cfg.size * 0.5f;
            v.y = hm[y][x] * cfg.heightScale + cfg.heightOffset;
            v.u = x / (float)(w - 1);
            v.v = y / (float)(h - 1);
            v.normal[0] = 0; v.normal[1] = 1; v.normal[2] = 0;
            mesh.vertices.append(v);
        }
    for (int y = 0; y < h - 1; ++y)
        for (int x = 0; x < w - 1; ++x) {
            int i = y * w + x;
            mesh.triangles.append({i, i + w, i + 1, 0});
            mesh.triangles.append({i + 1, i + w, i + w + 1, 0});
        }
    return mesh;
}

TrackTerrainSystem::TerrainMesh TrackTerrainSystem::smoothMesh(const TerrainMesh& mesh, int iter)
{
    TerrainMesh out = mesh;
    for (int n = 0; n < iter; ++n) {
        QVector<float> avg(out.vertices.size(), 0.0f);
        QVector<int> count(out.vertices.size(), 0);
        for (const auto& tri : out.triangles) {
            for (int k = 0; k < 3; ++k) {
                int a = tri.indices[k], b = tri.indices[(k + 1) % 3];
                avg[a] += out.vertices[b].y; count[a]++;
            }
        }
        for (int i = 0; i < out.vertices.size(); ++i)
            if (count[i] > 0) out.vertices[i].y = avg[i] / count[i];
    }
    return out;
}

TrackTerrainSystem::TerrainMesh TrackTerrainSystem::decimateMesh(const TerrainMesh& mesh, float ratio)
{
    if (ratio >= 1.0f || mesh.triangles.size() < 4) return mesh;

    TerrainMesh out = mesh;
    int targetTris = static_cast<int>(mesh.triangles.size() * qBound(0.01f, ratio, 1.0f));

    // Simple vertex clustering decimation: merge vertices within a grid cell
    float gridSize = 0.0f;
    for (const auto& v : mesh.vertices) {
        gridSize = qMax(gridSize, qAbs(v.x));
        gridSize = qMax(gridSize, qAbs(v.y));
        gridSize = qMax(gridSize, qAbs(v.z));
    }
    gridSize *= 2.0f / qMax(1, static_cast<int>(1.0f / ratio));

    QMap<QPair<int,int>, int> vertexMap; // grid key -> output vertex index
    TerrainMesh result;
    result.width = mesh.width;
    result.height = mesh.height;

    auto gridKey = [&](const TerrainVertex& v) -> QPair<int,int> {
        int gx = static_cast<int>(qFloor(v.x / gridSize));
        int gy = static_cast<int>(qFloor(v.z / gridSize));
        return qMakePair(gx, gy);
    };

    for (const auto& tri : mesh.triangles) {
        TerrainTriangle outTri;
        bool valid = true;
        for (int k = 0; k < 3; ++k) {
            const auto& srcVert = mesh.vertices[tri.indices[k]];
            auto key = gridKey(srcVert);
            if (!vertexMap.contains(key)) {
                vertexMap[key] = result.vertices.size();
                result.vertices.append(srcVert);
            }
            outTri.indices[k] = vertexMap[key];
        }
        if (valid) {
            outTri.surfaceType = tri.surfaceType;
            result.triangles.append(outTri);
        }
    }

    return result;
}

TrackTerrainSystem::TerrainMesh TrackTerrainSystem::subdivideMesh(const TerrainMesh& mesh, int sub)
{
    TerrainMesh out = mesh;
    for (int s = 0; s < sub; ++s) {
        TerrainMesh next;
        next.width = out.width; next.height = out.height;
        QMap<QPair<int,int>,int> midCache;
        auto mid = [&](int a, int b) {
            if (a > b) qSwap(a, b);
            auto key = qMakePair(a, b);
            if (midCache.contains(key)) return midCache[key];
            TerrainVertex m;
            m.x = (out.vertices[a].x + out.vertices[b].x) * 0.5f;
            m.y = (out.vertices[a].y + out.vertices[b].y) * 0.5f;
            m.z = (out.vertices[a].z + out.vertices[b].z) * 0.5f;
            m.u = (out.vertices[a].u + out.vertices[b].u) * 0.5f;
            m.v = (out.vertices[a].v + out.vertices[b].v) * 0.5f;
            next.vertices.append(m);
            return (midCache[key] = next.vertices.size() - 1);
        };
        for (const auto& tri : out.triangles) {
            int a = tri.indices[0], b = tri.indices[1], c = tri.indices[2];
            int ab = mid(a, b), bc = mid(b, c), ca = mid(c, a);
            next.triangles.append({a, ab, ca, 0});
            next.triangles.append({b, bc, ab, 0});
            next.triangles.append({c, ca, bc, 0});
            next.triangles.append({ab, bc, ca, 0});
        }
        out = next;
    }
    return out;
}

QVector<int> TrackTerrainSystem::assignSurfaces(const TerrainMesh& mesh, const QVector<TerrainLayer>& layers)
{
    QVector<int> surfaces(mesh.triangles.size(), 0);
    for (int i = 0; i < mesh.triangles.size(); ++i) {
        const auto& tri = mesh.triangles[i];
        float h = (mesh.vertices[tri.indices[0]].y + mesh.vertices[tri.indices[1]].y + mesh.vertices[tri.indices[2]].y) / 3.0f;
        surfaces[i] = getSurfaceAtHeight(h, layers);
    }
    return surfaces;
}

int TrackTerrainSystem::getSurfaceAtHeight(float height, const QVector<TerrainLayer>& layers)
{
    for (int i = 0; i < layers.size(); ++i)
        if (height >= layers[i].minHeight && height <= layers[i].maxHeight) return i;
    return 0;
}

int TrackTerrainSystem::getSurfaceAtSlope(float slope, const QVector<TerrainLayer>& layers)
{
    for (int i = 0; i < layers.size(); ++i)
        if (slope >= layers[i].minSlope && slope <= layers[i].maxSlope) return i;
    return 0;
}

float TrackTerrainSystem::calculateSlope(const TerrainMesh& mesh, int triangleIndex)
{
    if (triangleIndex < 0 || triangleIndex >= mesh.triangles.size()) return 0.0f;
    const auto& tri = mesh.triangles[triangleIndex];
    const auto& v0 = mesh.vertices[tri.indices[0]];
    const auto& v1 = mesh.vertices[tri.indices[1]];
    const auto& v2 = mesh.vertices[tri.indices[2]];
    float n[3]; calculateTriangleNormal(v0, v1, v2, n);
    float angle = acosf(n[1]);
    return angle * 180.0f / M_PI;
}

float TrackTerrainSystem::calculateArea(const TerrainMesh& mesh)
{
    float total = 0.0f;
    for (const auto& tri : mesh.triangles) {
        const auto& a = mesh.vertices[tri.indices[0]];
        const auto& b = mesh.vertices[tri.indices[1]];
        const auto& c = mesh.vertices[tri.indices[2]];
        float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
        float vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
        float cx = uy * vz - uz * vy;
        float cy = uz * vx - ux * vz;
        float cz = ux * vy - uy * vx;
        total += sqrtf(cx * cx + cy * cy + cz * cz) * 0.5f;
    }
    return total;
}

float TrackTerrainSystem::calculateMinHeight(const QVector<QVector<float>>& hm)
{
    float min = FLT_MAX;
    for (const auto& row : hm)
        for (float v : row) if (v < min) min = v;
    return min;
}

float TrackTerrainSystem::calculateMaxHeight(const QVector<QVector<float>>& hm)
{
    float max = -FLT_MAX;
    for (const auto& row : hm)
        for (float v : row) if (v > max) max = v;
    return max;
}

bool TrackTerrainSystem::exportToObj(const TerrainMesh& mesh, const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QTextStream out(&file);
    for (const auto& v : mesh.vertices)
        out << "v " << v.x << " " << v.y << " " << v.z << "\n";
    for (const auto& v : mesh.vertices)
        out << "vt " << v.u << " " << v.v << "\n";
    for (const auto& tri : mesh.triangles)
        out << "f " << (tri.indices[0] + 1) << "/" << (tri.indices[0] + 1)
            << " " << (tri.indices[1] + 1) << "/" << (tri.indices[1] + 1)
            << " " << (tri.indices[2] + 1) << "/" << (tri.indices[2] + 1) << "\n";
    return true;
}

bool TrackTerrainSystem::exportToKn5(const TerrainMesh& mesh, const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    // KN5 magic
    out << static_cast<uint32_t>(0x00534D4B); // "KSMS"

    // Version
    out << static_cast<uint32_t>(200);

    // Root node
    out << static_cast<uint32_t>(1); // root node count
    out << static_cast<uint32_t>(0); // node name length (empty)
    out << static_cast<uint32_t>(0); // child count = 0

    // Mesh data
    out << static_cast<uint32_t>(mesh.vertices.size());
    for (const auto& v : mesh.vertices) {
        out << v.x << v.y << v.z;
        out << v.normal[0] << v.normal[1] << v.normal[2];
        out << v.u << v.v;
    }

    out << static_cast<uint32_t>(mesh.triangles.size());
    for (const auto& tri : mesh.triangles) {
        out << static_cast<uint32_t>(tri.indices[0]);
        out << static_cast<uint32_t>(tri.indices[1]);
        out << static_cast<uint32_t>(tri.indices[2]);
    }

    file.close();
    qInfo() << "Exported terrain mesh to" << path;
    return true;
}

bool TrackTerrainSystem::exportToFBX(const TerrainMesh& mesh, const QString& path)
{
    if (mesh.vertices.isEmpty() || mesh.triangles.isEmpty()) {
        qWarning() << "Cannot export empty mesh to FBX";
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open" << path << "for writing:" << file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);

    out << "FBXHeaderExtension:  {\n"
        << "  FBXVersion: 7300\n"
        << "  FBXTimeSpan:  { TimeSpanStart: 0; TimeSpanStop: 0; }\n"
        << "  CreationTimeStamp:  { Version: 1000; Year: 2024; Month: 1; Day: 1; Hour: 0; Minute: 0; Second: 0; Millisecond: 0; }\n"
        << "}\n"
        << "GlobalSettings:  {\n"
        << "  Version: 1000\n"
        << "  Properties70:  { P:\"UnitScaleFactor\", \"double\", \"Number\", \"\", 1.0 }\n"
        << "}\n"
        << "Documents:  { Count: 1, Document: 1, \"\", \"Scene\" }\n"
        << "References:  { }\n";

    int vertexCount = mesh.vertices.size();
    int polyCount = mesh.triangles.size();

    out << "Objects:  {\n";

    // Model node
    out << "  Model: 1, \"Terrain\", \"Mesh\" {\n"
        << "    Version: 232\n"
        << "    Properties70:  { P:\"InheritType\", \"enum\", \"\", \"\", 1 }\n"
        << "    Shading: Y\n"
        << "    Culling: \"CullingOff\"\n"
        << "  }\n";

    // Geometry
    out << "  Geometry: 2, \"TerrainGeometry\", \"Mesh\" {\n"
        << "    Vertices: " << vertexCount << " {\n";

    for (int i = 0; i < vertexCount; ++i) {
        const auto& v = mesh.vertices[i];
        out << "      " << v.x << ", " << v.y << ", " << v.z << (i < vertexCount - 1 ? "," : "");
        if ((i % 10) == 9 || i == vertexCount - 1) out << "\n";
        else out << " ";
    }
    out << "    }\n";

    // Polygon (face) indices
    out << "    PolygonVertexIndex: " << (polyCount * 3) << " {\n";
    for (int i = 0; i < polyCount; ++i) {
        const auto& tri = mesh.triangles[i];
        out << "      " << tri.indices[0] << ", " << tri.indices[1] << ", " << (-tri.indices[2] - 1);
        if (i < polyCount - 1) out << ",";
        out << "\n";
    }
    out << "    }\n";

    // Normals per vertex per polygon
    out << "    LayerElementNormal: 0 {\n"
        << "      Version: 101\n"
        << "      Name: \"\"\n"
        << "      MappingInformationType: \"ByPolygonVertex\"\n"
        << "      ReferenceInformationType: \"Direct\"\n"
        << "      Normals: " << (polyCount * 3) << " {\n";

    for (int i = 0; i < polyCount; ++i) {
        const auto& tri = mesh.triangles[i];
        auto n0 = mesh.vertices[tri.indices[0]].normal;
        auto n1 = mesh.vertices[tri.indices[1]].normal;
        auto n2 = mesh.vertices[tri.indices[2]].normal;
        out << "        " << n0[0] << ", " << n0[1] << ", " << n0[2] << ",";
        out << n1[0] << ", " << n1[1] << ", " << n1[2] << ",";
        out << n2[0] << ", " << n2[1] << ", " << n2[2];
        if (i < polyCount - 1) out << ",";
        out << "\n";
    }
    out << "    }\n";

    // UVs
    if (!mesh.vertices.isEmpty()) {
        bool hasUV = !qIsNaN(mesh.vertices[0].u);
        if (hasUV) {
            out << "    LayerElementUV: 0 {\n"
                << "      Version: 101\n"
                << "      Name: \"UVMap\"\n"
                << "      MappingInformationType: \"ByPolygonVertex\"\n"
                << "      ReferenceInformationType: \"IndexToDirect\"\n"
                << "      UV: " << vertexCount << " {\n";

            for (int i = 0; i < vertexCount; ++i) {
                const auto& v = mesh.vertices[i];
                out << "        " << v.u << ", " << v.v;
                if (i < vertexCount - 1) out << ",";
                if ((i % 10) == 9 || i == vertexCount - 1) out << "\n";
                else out << " ";
            }
            out << "      }\n";

            out << "      UVIndex: " << (polyCount * 3) << " {\n";
            for (int i = 0; i < polyCount; ++i) {
                const auto& tri = mesh.triangles[i];
                out << "        " << tri.indices[0] << ", " << tri.indices[1] << ", " << tri.indices[2];
                if (i < polyCount - 1) out << ",";
                out << "\n";
            }
            out << "      }\n"
                << "    }\n";
        }
    }

    // Layer element material
    out << "    LayerElementMaterial: 0 {\n"
        << "      Version: 101\n"
        << "      Name: \"\"\n"
        << "      MappingInformationType: \"AllSame\"\n"
        << "      ReferenceInformationType: \"IndexToDirect\"\n"
        << "    }\n";

    // Layers
    out << "    Layer: 0 {\n"
        << "      Version: 100\n"
        << "      LayerElement:  { Type: \"LayerElementNormal\", TypedIndex: 0 }\n"
        << "      LayerElement:  { Type: \"LayerElementUV\", TypedIndex: 0 }\n"
        << "      LayerElement:  { Type: \"LayerElementMaterial\", TypedIndex: 0 }\n"
        << "    }\n"
        << "  }\n";

    out << "}\n"; // Objects

    // Connections
    out << "Connections:  {\n"
        << "  Connect: \"OO\", 2, 1\n"
        << "}\n";

    file.close();
    return true;
}

TrackTerrainSystem::TerrainConfig TrackTerrainSystem::getFlatConfig()
{
    TerrainConfig c;
    c.resolution = 128; c.size = 500.0f; c.heightScale = 5.0f;
    return c;
}

TrackTerrainSystem::TerrainConfig TrackTerrainSystem::getHillConfig()
{
    TerrainConfig c;
    c.resolution = 256; c.size = 1000.0f; c.heightScale = 30.0f;
    return c;
}

TrackTerrainSystem::TerrainConfig TrackTerrainSystem::getMountainConfig()
{
    TerrainConfig c;
    c.resolution = 512; c.size = 2000.0f; c.heightScale = 100.0f;
    return c;
}

QVector<TrackTerrainSystem::TerrainLayer> TrackTerrainSystem::getDefaultLayers()
{
    return {
        {"Grass",  "textures/grass.png",  0.0f,  10.0f, 0.0f, 30.0f, 1.0f},
        {"Dirt",   "textures/dirt.png",   5.0f,  30.0f, 0.0f, 45.0f, 1.0f},
        {"Rock",   "textures/rock.png",   20.0f, 100.0f, 30.0f, 90.0f, 1.0f},
    };
}

bool TrackTerrainSystem::validateConfig(const TerrainConfig& config, QString* error)
{
    if (config.resolution < 2) { if (error) *error = "Resolution too low"; return false; }
    if (config.size <= 0) { if (error) *error = "Size must be positive"; return false; }
    return true;
}

bool TrackTerrainSystem::validateMesh(const TerrainMesh& mesh, QString* error)
{
    if (mesh.vertices.size() < 3) { if (error) *error = "Too few vertices"; return false; }
    if (mesh.triangles.size() < 1) { if (error) *error = "No triangles"; return false; }
    for (const auto& tri : mesh.triangles)
        for (int i = 0; i < 3; ++i)
            if (tri.indices[i] < 0 || tri.indices[i] >= mesh.vertices.size()) {
                if (error) *error = "Invalid vertex index"; return false;
            }
    return true;
}

float TrackTerrainSystem::calculateTriangleNormal(const TerrainVertex& v0, const TerrainVertex& v1, const TerrainVertex& v2, float* normal)
{
    float ux = v1.x - v0.x, uy = v1.y - v0.y, uz = v1.z - v0.z;
    float vx = v2.x - v0.x, vy = v2.y - v0.y, vz = v2.z - v0.z;
    normal[0] = uy * vz - uz * vy;
    normal[1] = uz * vx - ux * vz;
    normal[2] = ux * vy - uy * vx;
    float len = sqrtf(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
    if (len > 0.0001f) { normal[0] /= len; normal[1] /= len; normal[2] /= len; }
    return len;
}

// ─── TrackTerrainManager ────────────────────────────────────────────────

TrackTerrainManager::TrackTerrainManager(const QString& trackPath)
    : m_trackPath(trackPath) {}

void TrackTerrainManager::setConfig(const TrackTerrainSystem::TerrainConfig& config)
{
    m_config = config;
}

bool TrackTerrainManager::loadHeightMap()
{
    if (m_config.heightmapPath.isEmpty()) return false;
    QString path = m_trackPath + "/" + m_config.heightmapPath;
    m_heightMap = TrackTerrainSystem::loadHeightMap(path);
    return !m_heightMap.isEmpty();
}

bool TrackTerrainManager::saveHeightMap()
{
    if (m_config.heightmapPath.isEmpty()) return false;
    QString path = m_trackPath + "/" + m_config.heightmapPath;
    return TrackTerrainSystem::saveHeightMap(m_heightMap, path);
}

bool TrackTerrainManager::generateMesh()
{
    if (m_heightMap.isEmpty()) return false;
    m_mesh = TrackTerrainSystem::generateMesh(m_heightMap, m_config);
    return !m_mesh.vertices.isEmpty();
}

bool TrackTerrainManager::assignSurfaces()
{
    if (m_mesh.vertices.isEmpty()) return false;
    if (m_layers.isEmpty()) {
        m_layers = TrackTerrainSystem::getDefaultLayers();
    }
    QVector<int> surfaces = TrackTerrainSystem::assignSurfaces(m_mesh, m_layers);
    for (int i = 0; i < m_mesh.triangles.size() && i < surfaces.size(); ++i) {
        m_mesh.triangles[i].surfaceType = surfaces[i];
    }
    return true;
}

bool TrackTerrainManager::exportMesh(const QString& format)
{
    if (m_mesh.vertices.isEmpty()) return false;
    if (format == "obj") return TrackTerrainSystem::exportToObj(m_mesh, m_trackPath + "/terrain.obj");
    if (format == "kn5") return TrackTerrainSystem::exportToKn5(m_mesh, m_trackPath + "/terrain.kn5");
    if (format == "fbx") return TrackTerrainSystem::exportToFBX(m_mesh, m_trackPath + "/terrain.fbx");
    return false;
}

float TrackTerrainManager::getTerrainArea() const
{
    return TrackTerrainSystem::calculateArea(m_mesh);
}

float TrackTerrainManager::getMinHeight() const
{
    return TrackTerrainSystem::calculateMinHeight(m_heightMap);
}

float TrackTerrainManager::getMaxHeight() const
{
    return TrackTerrainSystem::calculateMaxHeight(m_heightMap);
}
