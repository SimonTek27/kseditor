#include "LODExporter.h"
#include "../KN5Types.h"
#include "../KN5Parser.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QSet>

namespace ks {

bool LODExporter::exportLODs(const QString& basePath, const MeshData& highPoly, const LODSettings& settings) {
    QFileInfo fileInfo(basePath);
    QString baseName = fileInfo.baseName();
    QString outputDir = fileInfo.absolutePath();

    bool allOk = true;

    QVector<MeshData> lodMeshes;
    lodMeshes.reserve(settings.lodCount);
    for (int i = 0; i < settings.lodCount; i++) {
        float ratio = settings.decimateRatios.size() > i ? settings.decimateRatios[i] : 0.5f;
        lodMeshes.append(generateLOD(highPoly, ratio));
    }

    if (settings.separateFiles) {
        for (int i = 0; i < settings.lodCount; i++) {
            const auto& lodMesh = lodMeshes[i];
            QString lodPath = outputDir + "/" + baseName + "_lod" + QString::number(i) + ".kn5";
            if (writeMeshToKN5(lodPath, lodMesh, QString("LOD_%1").arg(i))) {
                qInfo() << "LODExporter: Exported" << lodPath << "with" << lodMesh.faces.size() << "tris";
            } else {
                qWarning() << "LODExporter: Failed to export" << lodPath;
                allOk = false;
            }
        }
    } else {
        QString combinedPath = outputDir + "/" + baseName + "_lods.kn5";
        KN5Parser::KN5File kn5;
        kn5.header.magic = KN5_MAGIC;
        kn5.header.version = KN5_VERSION;

        KN5Parser::KN5File::Material mat;
        mat.id = 0;
        mat.name = "LODMaterial";
        mat.shaderName = "ksPerPixel";
        mat.type = KN5Parser::KN5File::Material::Type::Normal;
        kn5.materials.append(mat);

        for (int i = 0; i < lodMeshes.size(); i++) {
            const auto& mesh = lodMeshes[i];
            QString meshName = QString("LOD_%1").arg(i);

            KN5Parser::KN5File::Mesh kn5Mesh;
            kn5Mesh.name = meshName;

            for (const auto& v : mesh.vertices)
                kn5Mesh.positions.append(v.position);
            kn5Mesh.normals = mesh.normals;
            kn5Mesh.uv0 = mesh.uvs;
            kn5Mesh.uv1 = mesh.uv2s;
            kn5Mesh.tangents = mesh.tangents;

            QByteArray idxData;
            QDataStream idxStream(&idxData, QIODevice::WriteOnly);
            idxStream.setByteOrder(QDataStream::LittleEndian);
            for (const auto& face : mesh.faces)
                for (int idx : face.indices)
                    idxStream << static_cast<quint16>(idx);
            kn5Mesh.indexData = idxData;

            if (!mesh.vertices.isEmpty()) {
                QVector3D minV(1e9, 1e9, 1e9), maxV(-1e9, -1e9, -1e9);
                for (const auto& v : mesh.vertices) {
                    minV.setX(qMin(minV.x(), v.position.x()));
                    minV.setY(qMin(minV.y(), v.position.y()));
                    minV.setZ(qMin(minV.z(), v.position.z()));
                    maxV.setX(qMax(maxV.x(), v.position.x()));
                    maxV.setY(qMax(maxV.y(), v.position.y()));
                    maxV.setZ(qMax(maxV.z(), v.position.z()));
                }
                kn5Mesh.boundingMin = {minV.x(), minV.y(), minV.z()};
                kn5Mesh.boundingMax = {maxV.x(), maxV.y(), maxV.z()};
                kn5Mesh.boundingRadius = (maxV - minV).length() * 0.5f;

                KN5Parser::KN5File::SubMesh subMesh;
                subMesh.materialIndex = 0;
                subMesh.vertexOffset = 0;
                subMesh.vertexCount = static_cast<quint32>(kn5Mesh.positions.size());
                subMesh.indexOffset = 0;
                subMesh.indexCount = static_cast<quint32>(mesh.faces.size() * 3);
                subMesh.boundingMin = kn5Mesh.boundingMin;
                subMesh.boundingMax = kn5Mesh.boundingMax;
                kn5Mesh.subMeshes.append(subMesh);
            }

            kn5.meshes.append(kn5Mesh);
        }

        if (!lodMeshes.isEmpty()) {
            if (KN5Parser::KN5ParserImpl::write(combinedPath, kn5)) {
                qInfo() << "LODExporter: Exported combined LODs to" << combinedPath
                        << "with" << lodMeshes.size() << "LOD levels";
            } else {
                qWarning() << "LODExporter: Failed to export combined LODs to" << combinedPath;
                allOk = false;
            }
        }
    }

    return allOk;
}

static bool writeMeshToKN5(const QString& path, const MeshData& mesh, const QString& meshName) {
    using namespace KN5Parser;

    KN5File kn5;
    kn5.header.magic = KN5_MAGIC;
    kn5.header.version = KN5_VERSION;

    KN5Parser::KN5File::Mesh kn5Mesh;
    kn5Mesh.name = meshName;

    for (const auto& v : mesh.vertices)
        kn5Mesh.positions.append(v.position);
    kn5Mesh.normals = mesh.normals;
    kn5Mesh.uv0 = mesh.uvs;
    kn5Mesh.uv1 = mesh.uv2s;
    kn5Mesh.tangents = mesh.tangents;

    QByteArray idxData;
    QDataStream idxStream(&idxData, QIODevice::WriteOnly);
    idxStream.setByteOrder(QDataStream::LittleEndian);
    for (const auto& face : mesh.faces)
        for (int idx : face.indices)
            idxStream << static_cast<quint16>(idx);
    kn5Mesh.indexData = idxData;

    if (!mesh.vertices.isEmpty()) {
        QVector3D minV(1e9, 1e9, 1e9), maxV(-1e9, -1e9, -1e9);
        for (const auto& v : mesh.vertices) {
            minV.setX(qMin(minV.x(), v.position.x()));
            minV.setY(qMin(minV.y(), v.position.y()));
            minV.setZ(qMin(minV.z(), v.position.z()));
            maxV.setX(qMax(maxV.x(), v.position.x()));
            maxV.setY(qMax(maxV.y(), v.position.y()));
            maxV.setZ(qMax(maxV.z(), v.position.z()));
        }
        kn5Mesh.boundingMin = {minV.x(), minV.y(), minV.z()};
        kn5Mesh.boundingMax = {maxV.x(), maxV.y(), maxV.z()};
        kn5Mesh.boundingRadius = (maxV - minV).length() * 0.5f;

        KN5Parser::KN5File::SubMesh subMesh;
        subMesh.materialIndex = 0;
        subMesh.vertexOffset = 0;
        subMesh.vertexCount = static_cast<quint32>(kn5Mesh.positions.size());
        subMesh.indexOffset = 0;
        subMesh.indexCount = static_cast<quint32>(mesh.faces.size() * 3);
        subMesh.boundingMin = kn5Mesh.boundingMin;
        subMesh.boundingMax = kn5Mesh.boundingMax;
        kn5Mesh.subMeshes.append(subMesh);
    }

    kn5.meshes.append(kn5Mesh);

    KN5Parser::KN5File::Material mat;
    mat.id = 0;
    mat.name = "LODMaterial";
    mat.shaderName = "ksPerPixel";
    mat.type = KN5Parser::KN5File::Material::Type::Normal;
    kn5.materials.append(mat);

    return KN5Parser::KN5ParserImpl::write(path, kn5);
}

MeshData LODExporter::generateLOD(const MeshData& source, float decimateRatio, int targetTris) {
    MeshData lod = source;

    int targetFaces = targetTris > 0 ? targetTris : (int)(source.faces.size() * decimateRatio);
    targetFaces = qMax(4, targetFaces);

    if (targetFaces >= source.faces.size())
        return lod;

    lod.faces = LODSystem::decimateMesh(source, (float)targetFaces / source.faces.size()).faces;
    lod.computeBoundingBox();
    lod.boundingRadius = calculateBoundingSphere(lod).length();
    return lod;
}

MeshData LODExporter::reduceVertices(MeshData source, int targetCount) {
    return LODSystem::reduceVertices(source, targetCount);
}

MeshData LODExporter::simplifyMesh(const MeshData& source, float targetRatio) {
    return LODSystem::decimateMesh(source, targetRatio);
}

float LODExporter::calculateScreenSize(const MeshData& mesh, float distance, float fov) {
    return LODSystem::calculateScreenSize(mesh, distance, fov);
}

int LODExporter::estimateLODTriangles(int highPolyTris, int lodIndex, int totalLODs) {
    return LODSystem::estimateLODTriangles(highPolyTris, lodIndex, totalLODs);
}

bool LODExporter::validateLODChain(const QVector<LODLevel>& lods) {
    if (lods.isEmpty()) return false;
    for (int i = 1; i < lods.size(); i++) {
        if (lods[i].decimateRatio >= lods[i-1].decimateRatio) {
            qWarning() << "LODExporter: LOD" << i << "has higher or equal decimate ratio than LOD" << (i-1);
            return false;
        }
        if (lods[i].distance <= lods[i-1].distance) {
            qWarning() << "LODExporter: LOD" << i << "distance <= LOD" << (i-1);
            return false;
        }
    }
    return true;
}

QString LODExporter::getLODFileName(const QString& baseName, int lodIndex) {
    return LODSystem::getLODFileName(baseName, lodIndex);
}

QVector3D LODExporter::calculateBoundingSphere(const MeshData& mesh) {
    return LODSystem::calculateBoundingSphere(mesh);
}

}
