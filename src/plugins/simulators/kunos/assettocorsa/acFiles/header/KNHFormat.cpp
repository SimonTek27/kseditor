#include "KNHFormat.h"
#include "FileFormat/CADOBJParser.h"
#include "FileFormat/GLBParser.h"
#include "FileFormat/FBXParser.h"
#include <QFile>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDebug>

namespace ks {

QStringList FileFormat::supportedReadFormats() {
    return QStringList() << ".knh" << ".kn5" << ".glb" << ".fbx" << ".obj";
}

QStringList FileFormat::supportedWriteFormats() {
    return QStringList() << ".knh" << ".json";
}

bool FileFormat::isValidFormat(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray header = file.read(4);
    if (header.size() < 4) return false;

    return detectFormat(header);
}

bool FileFormat::detectFormat(const QByteArray& header) {
    if (header.startsWith("KNH")) return true;
    if (header.startsWith("KN5")) return true;
    if (header.startsWith("glTF")) return true;
    return false;
}

void KNHScene::clear() {
    name.clear();
    nodes.clear();
    meshes.clear();
    materials.clear();
    textures.clear();
    lods.clear();
    animations.clear();
    collisions.clear();
    skeleton.reset();
}

bool KNHScene::isEmpty() const {
    return nodes.isEmpty() && meshes.isEmpty();
}

int KNHScene::totalVertexCount() const {
    int count = 0;
    for (const auto& mesh : meshes) {
        count += mesh.vertices.size();
    }
    return count;
}

int KNHScene::totalTriangleCount() const {
    int count = 0;
    for (const auto& mesh : meshes) {
        count += mesh.indices.size() / 3;
    }
    return count;
}

bool KNHReader::read(const QString& path, KNHScene& scene) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        m_error = QString("Cannot open file: %1").arg(path);
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    KNHHeader header;
    if (!readHeader(stream, header)) {
        return false;
    }

    if (!readNodes(stream, scene)) return false;
    if (!readMeshes(stream, scene)) return false;
    if (!readMaterials(stream, scene)) return false;
    if (!readTextures(stream, scene)) return false;

    return true;
}

bool KNHReader::readHeader(QDataStream& stream, KNHHeader& header) {
    char magic[4];
    stream.readRawData(magic, 4);

    if (QByteArray(magic, 3) != "KNH") {
        m_error = "Invalid KNH file format";
        return false;
    }

    stream >> header.version;
    stream >> header.flags;
    stream >> header.dataOffset;
    stream >> header.dataSize;

    return true;
}

bool KNHReader::readNodes(QDataStream& stream, KNHScene& scene) {
    quint32 count;
    stream >> count;

    scene.nodes.resize(count);
    for (quint32 i = 0; i < count; ++i) {
        stream >> scene.nodes[i].name;
        stream >> scene.nodes[i].parentName;

        float m00, m01, m02, m03;
        float m10, m11, m12, m13;
        float m20, m21, m22, m23;
        float m30, m31, m32, m33;
        stream >> m00 >> m01 >> m02 >> m03;
        stream >> m10 >> m11 >> m12 >> m13;
        stream >> m20 >> m21 >> m22 >> m23;
        stream >> m30 >> m31 >> m32 >> m33;

        QMatrix4x4 mat;
        mat.setRow(0, QVector4D(m00, m01, m02, m03));
        mat.setRow(1, QVector4D(m10, m11, m12, m13));
        mat.setRow(2, QVector4D(m20, m21, m22, m23));
        mat.setRow(3, QVector4D(m30, m31, m32, m33));
        scene.nodes[i].transform = mat;

        stream >> scene.nodes[i].position;
        stream >> scene.nodes[i].rotation;
        stream >> scene.nodes[i].scale;
        stream >> scene.nodes[i].visible;
    }

    return true;
}

bool KNHReader::readMeshes(QDataStream& stream, KNHScene& scene) {
    quint32 count;
    stream >> count;

    scene.meshes.resize(count);
    for (quint32 i = 0; i < count; ++i) {
        stream >> scene.meshes[i].name;
        stream >> scene.meshes[i].materialName;

        quint32 vertCount;
        stream >> vertCount;
        scene.meshes[i].vertices.resize(vertCount);
        scene.meshes[i].normals.resize(vertCount);
        scene.meshes[i].uvs.resize(vertCount);

        for (quint32 j = 0; j < vertCount; ++j) {
            stream >> scene.meshes[i].vertices[j];
            stream >> scene.meshes[i].normals[j];
            stream >> scene.meshes[i].uvs[j];
        }

        quint32 idxCount;
        stream >> idxCount;
        scene.meshes[i].indices.resize(idxCount);
        for (quint32 j = 0; j < idxCount; ++j) {
            stream >> scene.meshes[i].indices[j];
        }
    }

    return true;
}

bool KNHReader::readMaterials(QDataStream& stream, KNHScene& scene) {
    quint32 count;
    stream >> count;

    scene.materials.resize(count);
    for (quint32 i = 0; i < count; ++i) {
        stream >> scene.materials[i].name;
        stream >> scene.materials[i].diffuseMap;
        stream >> scene.materials[i].normalMap;
        stream >> scene.materials[i].specularMap;
        stream >> scene.materials[i].ambientColor;
        stream >> scene.materials[i].diffuseColor;
        stream >> scene.materials[i].specularColor;
        stream >> scene.materials[i].shininess;
    }

    return true;
}

bool KNHReader::readTextures(QDataStream& stream, KNHScene& scene) {
    quint32 count;
    stream >> count;

    scene.textures.resize(count);
    for (quint32 i = 0; i < count; ++i) {
        stream >> scene.textures[i].name;
        stream >> scene.textures[i].filePath;
        stream >> scene.textures[i].width;
        stream >> scene.textures[i].height;
        stream >> scene.textures[i].channels;
    }

    return true;
}

bool KNHWriter::write(const QString& path, const KNHScene& scene) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = QString("Cannot create file: %1").arg(path);
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    KNHHeader header;
    header.dataOffset = sizeof(KNHHeader);

    writeHeader(stream, header);
    writeNodes(stream, scene);
    writeMeshes(stream, scene);
    writeMaterials(stream, scene);
    writeTextures(stream, scene);

    return true;
}

bool KNHWriter::writeBinary(const QString& path, const KNHScene& scene) {
    return write(path, scene);
}

bool KNHWriter::writeJson(const QString& path, const KNHScene& scene) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        m_error = QString("Cannot create file: %1").arg(path);
        return false;
    }

    QJsonObject root;
    root["name"] = scene.name;
    root["version"] = 1;

    QJsonArray nodesArray;
    for (const auto& node : scene.nodes) {
        QJsonObject n;
        n["name"] = node.name;
        n["parent"] = node.parentName;
        n["visible"] = node.visible;
        nodesArray.append(n);
    }
    root["nodes"] = nodesArray;

    root["meshCount"] = scene.meshes.size();
    root["materialCount"] = scene.materials.size();
    root["textureCount"] = scene.textures.size();

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));

    return true;
}

void KNHWriter::writeHeader(QDataStream& stream, const KNHHeader& header) {
    stream.writeRawData("KNH", 4);
    stream << header.version;
    stream << header.flags;
    stream << header.dataOffset;
    stream << header.dataSize;
}

void KNHWriter::writeNodes(QDataStream& stream, const KNHScene& scene) {
    stream << quint32(scene.nodes.size());

    for (const auto& node : scene.nodes) {
        stream << node.name;
        stream << node.parentName;

        const QMatrix4x4& mat = node.transform;
        stream << mat(0, 0) << mat(0, 1) << mat(0, 2) << mat(0, 3);
        stream << mat(1, 0) << mat(1, 1) << mat(1, 2) << mat(1, 3);
        stream << mat(2, 0) << mat(2, 1) << mat(2, 2) << mat(2, 3);
        stream << mat(3, 0) << mat(3, 1) << mat(3, 2) << mat(3, 3);

        stream << node.position;
        stream << node.rotation;
        stream << node.scale;
        stream << node.visible;
    }
}

void KNHWriter::writeMeshes(QDataStream& stream, const KNHScene& scene) {
    stream << quint32(scene.meshes.size());

    for (const auto& mesh : scene.meshes) {
        stream << mesh.name;
        stream << mesh.materialName;
        stream << quint32(mesh.vertices.size());

        for (int i = 0; i < mesh.vertices.size(); ++i) {
            stream << mesh.vertices[i];
            stream << mesh.normals[i];
            stream << mesh.uvs[i];
        }

        stream << quint32(mesh.indices.size());
        for (quint32 idx : mesh.indices) {
            stream << idx;
        }
    }
}

void KNHWriter::writeMaterials(QDataStream& stream, const KNHScene& scene) {
    stream << quint32(scene.materials.size());

    for (const auto& mat : scene.materials) {
        stream << mat.name;
        stream << mat.diffuseMap;
        stream << mat.normalMap;
        stream << mat.specularMap;
        stream << mat.ambientColor;
        stream << mat.diffuseColor;
        stream << mat.specularColor;
        stream << mat.shininess;
    }
}

void KNHWriter::writeTextures(QDataStream& stream, const KNHScene& scene) {
    stream << quint32(scene.textures.size());

    for (const auto& tex : scene.textures) {
        stream << tex.name;
        stream << tex.filePath;
        stream << tex.width;
        stream << tex.height;
        stream << tex.channels;
    }
}

bool KNHConverter::toKNH(const QString& inputPath, const QString& outputPath, const QString& sourceFormat) {
    QString ext = sourceFormat.isEmpty() 
        ? QFileInfo(inputPath).suffix().toLower() 
        : sourceFormat;

    if (ext == "knh") {
        if (!QFile::copy(inputPath, outputPath)) {
            qWarning() << "KNHConverter: Failed to copy" << inputPath << "to" << outputPath;
            return false;
        }
        return true;
    } else if (ext == "kn5") {
        return convertFromKN5(inputPath, outputPath);
    } else if (ext == "glb") {
        return convertFromGLB(inputPath, outputPath);
    } else if (ext == "fbx") {
        return convertFromFBX(inputPath, outputPath);
    } else if (ext == "obj") {
        return convertFromOBJ(inputPath, outputPath);
    }

    return false;
}

bool KNHConverter::convertFromOBJ(const QString& objPath, const QString& knhPath) {
    CADOBJParser parser;
    if (!parser.loadFromFile(objPath.toStdString())) {
        return false;
    }

    KNHScene scene;
    scene.name = QFileInfo(objPath).baseName();

    KNHNode rootNode;
    rootNode.name = "Root";
    rootNode.transform.setToIdentity();
    rootNode.position = QVector3D(0, 0, 0);
    rootNode.rotation = QVector3D(0, 0, 0);
    rootNode.scale = QVector3D(1, 1, 1);
    scene.nodes.append(rootNode);

    const auto& objScene = parser.scene();
    for (const auto& srcMesh : objScene.meshes) {
        KNHMesh knhMesh;
        knhMesh.name = QString::fromStdString(srcMesh.name);
        knhMesh.materialName = QString::fromStdString(srcMesh.materialName);

        for (const auto& v : srcMesh.vertices) {
            knhMesh.vertices.append(QVector3D(v.x, v.y, v.z));
        }
        for (const auto& n : srcMesh.normals) {
            knhMesh.normals.append(QVector3D(n.x, n.y, n.z));
        }
        for (const auto& t : srcMesh.texCoords) {
            knhMesh.uvs.append(QVector2D(t.x, t.y));
        }
        for (const auto& idx : srcMesh.indices) {
            knhMesh.indices.append(static_cast<quint32>(idx.x));
            knhMesh.indices.append(static_cast<quint32>(idx.y));
            knhMesh.indices.append(static_cast<quint32>(idx.z));
        }

        scene.meshes.append(knhMesh);
    }

    for (const auto& [matName, srcMat] : objScene.materials) {
        KNHMaterial knhMat;
        knhMat.name = QString::fromStdString(srcMat.name);
        knhMat.diffuseColor = QVector3D(srcMat.Kd.x, srcMat.Kd.y, srcMat.Kd.z);
        knhMat.specularColor = QVector3D(srcMat.Ks.x, srcMat.Ks.y, srcMat.Ks.z);
        knhMat.ambientColor = QVector3D(srcMat.Ka.x, srcMat.Ka.y, srcMat.Ka.z);
        knhMat.shininess = srcMat.Ns;
        knhMat.diffuseMap = QString::fromStdString(srcMat.mapKd);
        knhMat.normalMap = QString::fromStdString(srcMat.mapBump);
        knhMat.specularMap = QString::fromStdString(srcMat.mapKs);
        scene.materials.append(knhMat);
    }

    KNHWriter writer;
    return writer.write(knhPath, scene);
}

bool KNHConverter::convertFromGLB(const QString& glbPath, const QString& knhPath) {
    GLBParser parser;
    if (!parser.loadFromFile(glbPath.toStdString())) {
        return false;
    }

    KNHScene scene;
    scene.name = QFileInfo(glbPath).baseName();

    KNHNode rootNode;
    rootNode.name = "Root";
    rootNode.transform.setToIdentity();
    scene.nodes.append(rootNode);

    const auto& glbScene = parser.scene();
    for (const auto& srcMesh : glbScene.meshes) {
        for (const auto& prim : srcMesh.primitives) {
            KNHMesh knhMesh;
            knhMesh.name = QString::fromStdString(srcMesh.name);

            if (prim.attributes.count("POSITION")) {
                auto verts = parser.getVertices(prim.attributes.at("POSITION"));
                for (const auto& v : verts) {
                    knhMesh.vertices.append(QVector3D(v.x, v.y, v.z));
                }
            }
            if (prim.attributes.count("NORMAL")) {
                auto norms = parser.getNormals(prim.attributes.at("NORMAL"));
                for (const auto& n : norms) {
                    knhMesh.normals.append(QVector3D(n.x, n.y, n.z));
                }
            }
            if (prim.attributes.count("TEXCOORD_0")) {
                auto texs = parser.getTexCoords(prim.attributes.at("TEXCOORD_0"));
                for (const auto& t : texs) {
                    knhMesh.uvs.append(QVector2D(t.x, t.y));
                }
            }
            if (prim.indices != 0xFFFFFFFF) {
                auto idxs = parser.getIndices(prim.indices);
                for (uint32_t idx : idxs) {
                    knhMesh.indices.append(static_cast<quint32>(idx));
                }
            }

            if (prim.material != 0xFFFFFFFF && prim.material < glbScene.materials.size()) {
                const auto& srcMat = glbScene.materials[prim.material];
                knhMesh.materialName = QString::fromStdString(srcMat.name);

                KNHMaterial knhMat;
                knhMat.name = QString::fromStdString(srcMat.name);
                knhMat.diffuseColor = QVector3D(
                    srcMat.baseColorFactor.x,
                    srcMat.baseColorFactor.y,
                    srcMat.baseColorFactor.z
                );
                knhMat.shininess = srcMat.roughnessFactor < 0.01f ? 128.0f : (1.0f - srcMat.roughnessFactor) * 128.0f;
                scene.materials.append(knhMat);
            }

            scene.meshes.append(knhMesh);
        }
    }

    KNHWriter writer;
    return writer.write(knhPath, scene);
}

bool KNHConverter::convertFromFBX(const QString& fbxPath, const QString& knhPath) {
    FBXParser parser;
    if (!parser.loadFromFile(fbxPath.toStdString())) {
        return false;
    }

    KNHScene scene;
    scene.name = QFileInfo(fbxPath).baseName();

    KNHNode rootNode;
    rootNode.name = "Root";
    rootNode.transform.setToIdentity();
    scene.nodes.append(rootNode);

    const auto& fbxScene = parser.scene();
    for (const auto& srcMesh : fbxScene.meshes) {
        KNHMesh knhMesh;
        knhMesh.name = QString::fromStdString(srcMesh.name);
        knhMesh.materialName = QString::fromStdString(srcMesh.materialName);

        for (const auto& v : srcMesh.vertices) {
            knhMesh.vertices.append(QVector3D(v.x, v.y, v.z));
        }
        for (const auto& n : srcMesh.normals) {
            knhMesh.normals.append(QVector3D(n.x, n.y, n.z));
        }
        for (const auto& t : srcMesh.texCoords) {
            knhMesh.uvs.append(QVector2D(t.x, t.y));
        }
        for (uint32_t idx : srcMesh.indices) {
            knhMesh.indices.append(idx);
        }

        scene.meshes.append(knhMesh);
    }

    for (const auto& [matName, srcMat] : fbxScene.materials) {
        KNHMaterial knhMat;
        knhMat.name = QString::fromStdString(srcMat.name);
        knhMat.diffuseColor = QVector3D(
            srcMat.diffuseColor.x,
            srcMat.diffuseColor.y,
            srcMat.diffuseColor.z
        );
        knhMat.specularColor = QVector3D(
            srcMat.specularColor.x,
            srcMat.specularColor.y,
            srcMat.specularColor.z
        );
        knhMat.ambientColor = QVector3D(
            srcMat.ambientColor.x,
            srcMat.ambientColor.y,
            srcMat.ambientColor.z
        );
        knhMat.shininess = srcMat.shininess;
        knhMat.diffuseMap = QString::fromStdString(srcMat.diffuseTexture);
        knhMat.normalMap = QString::fromStdString(srcMat.normalTexture);
        scene.materials.append(knhMat);
    }

    KNHWriter writer;
    return writer.write(knhPath, scene);
}

bool KNHConverter::convertFromKN5(const QString& kn5Path, const QString& knhPath) {
    QFile file(kn5Path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    char magic[4];
    stream.readRawData(magic, 4);
    if (QByteArray(magic, 3) != "KN5") {
        return false;
    }

    quint32 version;
    stream >> version;

    KNHScene scene;
    scene.name = QFileInfo(kn5Path).baseName();

    KNHNode rootNode;
    rootNode.name = "Root";
    rootNode.transform.setToIdentity();
    scene.nodes.append(rootNode);

    quint32 meshCount;
    stream >> meshCount;

    for (quint32 i = 0; i < meshCount; ++i) {
        quint32 nameLen;
        stream >> nameLen;
        QByteArray nameBytes(nameLen, Qt::Uninitialized);
        stream.readRawData(nameBytes.data(), nameLen);
        QString meshName = QString::fromUtf8(nameBytes);

        quint32 vertCount;
        stream >> vertCount;

        KNHMesh knhMesh;
        knhMesh.name = meshName;

        knhMesh.vertices.resize(vertCount);
        knhMesh.normals.resize(vertCount);
        knhMesh.uvs.resize(vertCount);

        for (quint32 j = 0; j < vertCount; ++j) {
            float x, y, z;
            stream >> x >> y >> z;
            knhMesh.vertices[j] = QVector3D(x, y, z);
        }
        for (quint32 j = 0; j < vertCount; ++j) {
            float nx, ny, nz;
            stream >> nx >> ny >> nz;
            knhMesh.normals[j] = QVector3D(nx, ny, nz);
        }
        for (quint32 j = 0; j < vertCount; ++j) {
            float u, v;
            stream >> u >> v;
            knhMesh.uvs[j] = QVector2D(u, v);
        }

        quint32 idxCount;
        stream >> idxCount;
        knhMesh.indices.resize(idxCount);
        for (quint32 j = 0; j < idxCount; ++j) {
            quint16 idx;
            stream >> idx;
            knhMesh.indices[j] = idx;
        }

        quint32 matNameLen;
        stream >> matNameLen;
        if (matNameLen > 0) {
            QByteArray matBytes(matNameLen, Qt::Uninitialized);
            stream.readRawData(matBytes.data(), matNameLen);
            knhMesh.materialName = QString::fromUtf8(matBytes);
        }

        scene.meshes.append(knhMesh);
    }

    KNHWriter writer;
    return writer.write(knhPath, scene);
}

}
