#pragma once

#include "BISBinaryReader.h"
#include <QString>
#include <QVector>
#include <QVector3D>
#include <QVector2D>
#include <QMatrix4x4>
#include <QJsonObject>

namespace ks::fileformat {

// ============================================================================
// P3D Model - Unified model structure for P3D (ODOL/MLOD) files
// Rewritten from CWR/Poseidon ODOLLoader.hpp and MLODLoader.hpp
// ============================================================================

struct P3DMaterial {
    QString name;
    QString texturePath;
    bool isShadowVolume = false;
};

struct P3DVertex {
    QVector3D position;
    QVector3D normal;
    QVector2D uv;
};

struct P3DFace {
    QVector<int> indices;
    int materialIndex = 0;
    bool isQuad = false;
};

struct P3DNamedSelection {
    QString name;
    QVector<int> vertexIndices;
    QVector<int> faceIndices;
};

struct P3DNamedProperty {
    QString key;
    QString value;
};

struct P3DProxy {
    QString path;
    QMatrix4x4 transform;
};

struct P3DEdge {
    int v0 = 0, v1 = 0;
};

struct P3DFrame {
    float time = 0;
    QVector<QVector3D> bonePositions;
};

struct P3DBounds {
    QVector3D center;
    float radius = 0;
};

struct P3DLOD {
    float resolution = 0;
    QVector<P3DVertex> vertices;
    QVector<P3DFace> faces;
    QVector<P3DMaterial> materials;
    QVector<P3DNamedSelection> namedSelections;
    QVector<P3DNamedProperty> namedProperties;
    QVector<P3DProxy> proxies;
    QVector<P3DEdge> edges;
    P3DBounds bounds;
};

struct P3DModel {
    QVector<P3DLOD> lods;
    float mass = 0;
    float armor = 0;
    P3DBounds boundingSphere;

    // LOD index mappings (which LOD index to use for each purpose)
    int geometryLOD = -1;
    int memoryLOD = -1;
    int fireLOD = -1;
    int viewLOD = -1;

    bool isValid() const { return !lods.isEmpty(); }
};

// ============================================================================
// ODOL (P3D v7) Loader
// ============================================================================

class P3DODOLLoader {
public:
    P3DModel load(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return loadFromDevice(file);
    }

    P3DModel loadFromDevice(QIODevice& device) {
        BinaryReader reader(device);
        P3DModel model;

        // Read header: "ODOL" + version(int32) + LOD count(int32)
        char sig[4];
        reader.readBytes(sig, 4);
        if (memcmp(sig, "ODOL", 4) != 0)
            return {};

        int32_t version;
        reader.read(version);
        if (version != 7) return {};

        int32_t lodCount;
        reader.read(lodCount);

        // Read each LOD
        for (int i = 0; i < lodCount; ++i) {
            P3DLOD lod = readODOLLod(reader);
            if (i == 0) model.lods.append(lod);
            else model.lods.append(lod);
        }

        // Read LOD resolutions
        for (int i = 0; i < lodCount; ++i) {
            float res = reader.read<float>();
            if (i < model.lods.size())
                model.lods[i].resolution = res;
        }

        // Read model-wide properties
        // Read special LOD indices
        reader.read(model.geometryLOD);
        reader.read(model.memoryLOD);
        reader.read(model.fireLOD);
        reader.read(model.viewLOD);
        // Skip remaining LOD indices (gunner, commander, cargo, etc.)
        for (int i = 0; i < 8; ++i) {
            int32_t dummy;
            reader.read(dummy);
        }

        // Read bounding sphere
        read(reader, model.boundingSphere);

        // Read mass and armor
        reader.read(model.mass);
        reader.read(model.armor);

        return model;
    }

private:
    P3DLOD readODOLLod(BinaryReader& reader) {
        P3DLOD lod;

        // Read bounding box
        BisBoundingBox bbox;
        read(reader, bbox);
        lod.bounds.center = QVector3D(
            (bbox.min.x + bbox.max.x) * 0.5f,
            (bbox.min.y + bbox.max.y) * 0.5f,
            (bbox.min.z + bbox.max.z) * 0.5f);
        lod.bounds.radius = 0;

        // Read textures
        uint32_t textureCount = reader.read<uint32_t>();
        QVector<QString> textures;
        for (uint32_t i = 0; i < textureCount; ++i) {
            textures.append(reader.readString());
        }

        // Read vertices
        uint32_t vertexCount = reader.read<uint32_t>();
        lod.vertices.resize(vertexCount);

        // Read clip flags (skip)
        for (uint32_t i = 0; i < vertexCount; ++i) {
            reader.read<uint32_t>(); // clipFlags
        }

        // Read UVs
        for (uint32_t i = 0; i < vertexCount; ++i) {
            reader.read(lod.vertices[i].uv);
        }

        // Read positions
        for (uint32_t i = 0; i < vertexCount; ++i) {
            read(reader, lod.vertices[i].position);
        }

        // Read normals
        for (uint32_t i = 0; i < vertexCount; ++i) {
            read(reader, lod.vertices[i].normal);
        }

        // Read faces
        uint32_t faceCount = reader.read<uint32_t>();
        lod.faces.resize(faceCount);

        for (uint32_t i = 0; i < faceCount; ++i) {
            uint32_t flags = reader.read<uint32_t>();
            uint32_t texIdx = reader.read<uint32_t>();
            lod.faces[i].materialIndex = texIdx;

            uint32_t vertCount = reader.read<uint32_t>();
            lod.faces[i].isQuad = (vertCount == 4);
            lod.faces[i].indices.resize(vertCount);
            for (uint32_t j = 0; j < vertCount; ++j) {
                reader.read(lod.faces[i].indices[j]);
            }
        }

        // Read sections
        uint32_t sectionCount = reader.read<uint32_t>();
        for (uint32_t i = 0; i < sectionCount; ++i) {
            reader.read<uint32_t>(); // faceIndexLowerBound
            reader.read<uint32_t>(); // faceIndexUpperBound
            reader.read<uint32_t>(); // materialIndex
            reader.read<uint32_t>(); // textureIndex
            reader.read<uint32_t>(); // special
        }

        // Read named selections
        uint32_t nsCount = reader.read<uint32_t>();
        for (uint32_t i = 0; i < nsCount; ++i) {
            P3DNamedSelection ns;
            ns.name = reader.readString();
            // Read vertex indices
            uint32_t vertCount = reader.read<uint32_t>();
            ns.vertexIndices.resize(vertCount);
            for (uint32_t j = 0; j < vertCount; ++j) {
                reader.read(ns.vertexIndices[j]);
            }
            // Read face indices
            uint32_t faceIdxCount = reader.read<uint32_t>();
            ns.faceIndices.resize(faceIdxCount);
            for (uint32_t j = 0; j < faceIdxCount; ++j) {
                reader.read(ns.faceIndices[j]);
            }
            lod.namedSelections.append(ns);
        }

        // Read named properties
        uint32_t npCount = reader.read<uint32_t>();
        for (uint32_t i = 0; i < npCount; ++i) {
            P3DNamedProperty np;
            np.key = reader.readString();
            np.value = reader.readString();
            lod.namedProperties.append(np);
        }

        // Read edges
        uint32_t edgeCount = reader.read<uint32_t>();
        lod.edges.resize(edgeCount);
        for (uint32_t i = 0; i < edgeCount; ++i) {
            reader.read(lod.edges[i].v0);
            reader.read(lod.edges[i].v1);
        }

        // Read proxies
        uint32_t proxyCount = reader.read<uint32_t>();
        for (uint32_t i = 0; i < proxyCount; ++i) {
            P3DProxy proxy;
            proxy.path = reader.readString();
            for (int r = 0; r < 4; ++r) {
                BisVector3 row;
                read(reader, row);
                proxy.transform(r, 0) = row.x;
                proxy.transform(r, 1) = row.y;
                proxy.transform(r, 2) = row.z;
            }
            lod.proxies.append(proxy);
        }

        // Read material table
        uint32_t matCount = reader.read<uint32_t>();
        lod.materials.resize(matCount);
        for (uint32_t i = 0; i < matCount; ++i) {
            lod.materials[i].name = reader.readString();
            lod.materials[i].texturePath = reader.readString();
        }

        // Read frames (animation)
        uint32_t frameCount = reader.read<uint32_t>();
        if (frameCount > 0) {
            // Read frame timestamps
            QVector<float> frameTimes(frameCount);
            for (uint32_t i = 0; i < frameCount; ++i) {
                reader.read(frameTimes[i]);
            }

            // Read bone positions per frame
            uint32_t boneCount = reader.read<uint32_t>();
            for (uint32_t i = 0; i < frameCount; ++i) {
                P3DFrame frame;
                frame.time = frameTimes[i];
                frame.bonePositions.resize(boneCount);
                for (uint32_t j = 0; j < boneCount; ++j) {
                    read(reader, frame.bonePositions[j]);
                }
                lod.frames.append(frame);
            }
        }

        return lod;
    }
};

} // namespace ks::fileformat
