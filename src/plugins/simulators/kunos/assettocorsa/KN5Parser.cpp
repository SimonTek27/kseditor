#include "KN5Parser.h"
#include "KN5Types.h"
#include <QString>
#include <QVector>
#include <QStringList>
#include <QJsonObject>
#include <QMap>
#include <QByteArray>
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <cmath>
#include <limits>

/**
 * @brief KN5 file format parser for Assetto Corsa
 * 
 * KN5 is the native mesh format for Assetto Corsa.
 * Based on analysis of KN5 files and AcTools.
 */

namespace KN5Parser {

// Shader name constants (used only in implementations)
static const QString SHADER_PER_PIXEL = "ksPerPixel";
static const QString SHADER_PER_PIXEL_NM = "ksPerPixelNM";
static const QString SHADER_PER_PIXEL_MULTI = "ksPerPixelMultiMap";
static const QString SHADER_MULTILAYER = "ksMultilayer";
static const QString SHADER_SIMPLE = "ksSimple";

// Implementations
// ============================================================================

QString KN5ParserImpl::m_lastError;

KN5File KN5ParserImpl::parse(const QString& filePath, QString* error) {
    KN5File kn5;
    kn5.filePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QString("Cannot open file: %1").arg(filePath);
        if (error) *error = m_lastError;
        return KN5File();
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    try {
        // Read header
        FileHeader& h = kn5.header;
        stream >> h.magic;
        stream >> h.version;
        stream >> h.flags;
        stream >> h.textureCount;
        stream >> h.materialCount;
        stream >> h.nodeCount;
        stream >> h.headerSize;
        stream >> h.nodeOffset;
        stream >> h.textureOffset;
        stream >> h.vertexBufferOffset;
        stream >> h.indexBufferOffset;
        stream >> h.vertexBufferSize;
        stream >> h.indexBufferSize;

        if (h.magic != KN5_MAGIC) {
            m_lastError = QString("Invalid KN5 magic: 0x%1").arg(h.magic, 0, 16);
            if (error) *error = m_lastError;
            return KN5File();
        }

        if (h.version != KN5_VERSION) {
            m_lastError = QString("Unsupported KN5 version: %1").arg(h.version);
            if (error) *error = m_lastError;
            return KN5File();
        }

        // Read textures
        kn5.textures.resize(h.textureCount);
        for (quint32 i = 0; i < h.textureCount; i++) {
            parseTexture(kn5, stream);
        }

        // Read materials
        kn5.materials.resize(h.materialCount);
        for (quint32 i = 0; i < h.materialCount; i++) {
            parseMaterial(kn5, stream);
        }

        // Read meshes
        kn5.meshes.resize(h.nodeCount);
        for (quint32 i = 0; i < h.nodeCount; i++) {
            parseMesh(kn5, stream);
        }

        // Read world matrix
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                stream >> kn5.worldMatrix.m[i][j];
            }
        }

    } catch (const std::exception& e) {
        m_lastError = QString("Parse error: %1").arg(e.what());
        if (error) *error = m_lastError;
        return KN5File();
    }

    file.close();
    if (error) *error = QString();
    return kn5;
}

bool KN5ParserImpl::write(const QString& filePath, const KN5File& kn5) {
    QFile outFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        m_lastError = QString("Cannot create file: %1").arg(filePath);
        return false;
    }

    QDataStream stream(&outFile);
    stream.setByteOrder(QDataStream::LittleEndian);

    try {
        // Write header
        FileHeader h = kn5.header;
        h.magic = KN5_MAGIC;
        h.version = KN5_VERSION;
        h.textureCount = kn5.textures.size();
        h.materialCount = kn5.materials.size();
        h.nodeCount = kn5.meshes.size();

        stream << h.magic;
        stream << h.version;
        stream << h.flags;
        stream << h.textureCount;
        stream << h.materialCount;
        stream << h.nodeCount;
        stream << h.headerSize;
        stream << h.nodeOffset;
        stream << h.textureOffset;
        stream << h.vertexBufferOffset;
        stream << h.indexBufferOffset;
        stream << h.vertexBufferSize;
        stream << h.indexBufferSize;

        // Write textures
        for (const auto& tex : kn5.textures) {
            QByteArray nameBytes = tex.name.toUtf8();
            stream << (quint32)nameBytes.size();
            stream.writeRawData(nameBytes.constData(), nameBytes.size());
            stream << tex.width;
            stream << tex.height;
            stream << tex.format;
            stream << tex.mipmapCount;
            stream << (quint32)tex.data.size();
            stream.writeRawData(tex.data.constData(), tex.data.size());
        }

        // Write materials
        for (const auto& mat : kn5.materials) {
            stream << mat.id;
            
            QByteArray nameBytes = mat.name.toUtf8();
            stream << (quint32)nameBytes.size();
            stream.writeRawData(nameBytes.constData(), nameBytes.size());

            QByteArray shaderBytes = mat.shaderName.toUtf8();
            stream << (quint32)shaderBytes.size();
            stream.writeRawData(shaderBytes.constData(), shaderBytes.size());

            stream << (quint32)mat.type;

            stream << (quint32)mat.properties.size();
            for (auto it = mat.properties.begin(); it != mat.properties.end(); ++it) {
                QByteArray key = it.key().toUtf8();
                QByteArray val = it.value().toUtf8();
                stream << (quint32)key.size();
                stream.writeRawData(key.constData(), key.size());
                stream << (quint32)val.size();
                stream.writeRawData(val.constData(), val.size());
            }

            stream << (quint32)mat.textureMapping.size();
            for (auto it = mat.textureMapping.begin(); it != mat.textureMapping.end(); ++it) {
                QByteArray slot = it.key().toUtf8();
                QByteArray tex = it.value().toUtf8();
                stream << (quint32)slot.size();
                stream.writeRawData(slot.constData(), slot.size());
                stream << (quint32)tex.size();
                stream.writeRawData(tex.constData(), tex.size());
            }
        }

        // Write meshes
        for (const auto& mesh : kn5.meshes) {
            QByteArray nameBytes = mesh.name.toUtf8();
            stream << (quint32)nameBytes.size();
            stream.writeRawData(nameBytes.constData(), nameBytes.size());
            stream << (quint32)0; // transform type
            stream << mesh.nodeIndex;
            stream << mesh.castShadows;
            stream << mesh.isVisible;
            stream << mesh.isTransparent;
            stream << (quint32)mesh.materialType;

            // Write bounding box
            stream << mesh.boundingMin.x;
            stream << mesh.boundingMin.y;
            stream << mesh.boundingMin.z;
            stream << mesh.boundingMax.x;
            stream << mesh.boundingMax.y;
            stream << mesh.boundingMax.z;
            stream << mesh.boundingRadius;

            // Write vertex layout (including UV2, tangents, bone weights if present)
            // Rebuild layout to ensure UV2/tangent/bone slots are included
            {
                using AT = AttributeType;
                VertexLayout& vl = const_cast<Mesh&>(mesh).vertexLayout;
                if (!vl.has(AT::TexCoord1) && !mesh.uv1.isEmpty()) {
                    quint8 offs = (quint8)vl.vertexSize;
                    vl.attributes.append(std::make_pair(AT::TexCoord1, offs));
                    vl.vertexSize += 8;
                }
                if (!vl.has(AT::Tangent) && !mesh.tangents.isEmpty()) {
                    quint8 offs = (quint8)vl.vertexSize;
                    vl.attributes.append(std::make_pair(AT::Tangent, offs));
                    vl.vertexSize += 12;
                }
                if (!vl.has(AT::Bitangent) && !mesh.bitangents.isEmpty()) {
                    quint8 offs = (quint8)vl.vertexSize;
                    vl.attributes.append(std::make_pair(AT::Bitangent, offs));
                    vl.vertexSize += 12;
                }
                if (!vl.has(AT::BoneWeight) && !mesh.boneWeights.isEmpty()) {
                    quint8 offs = (quint8)vl.vertexSize;
                    vl.attributes.append(std::make_pair(AT::BoneWeight, offs));
                    vl.vertexSize += 16;
                    offs = (quint8)vl.vertexSize;
                    vl.attributes.append(std::make_pair(AT::BoneIndex, offs));
                    vl.vertexSize += 4;
                }
            }
            stream << (quint32)mesh.vertexLayout.attributes.size();
            for (const auto& attr : mesh.vertexLayout.attributes) {
                stream << (quint32)attr.first;
                stream << (quint32)attr.second;
                // Attribute size in bytes
                quint32 attrBytes = 0;
                switch (attr.first) {
                case AttributeType::Position:
                case AttributeType::Normal:
                case AttributeType::Tangent:
                case AttributeType::Bitangent: attrBytes = 12; break;
                case AttributeType::TexCoord0:
                case AttributeType::TexCoord1: attrBytes =  8; break;
                case AttributeType::BoneWeight: attrBytes = 16; break;
                case AttributeType::BoneIndex:
                case AttributeType::Color:      attrBytes =  4; break;
                default: attrBytes = 4; break;
                }
                stream << attrBytes;
            }

            // Write vertex data
            stream << (quint32)mesh.getVertexCount();
            stream << (quint32)mesh.vertexData.size();
            stream.writeRawData(mesh.vertexData.constData(), mesh.vertexData.size());

            // Write index data
            stream << (quint32)(mesh.indexData.size() / 2);
            stream << (quint32)mesh.indexData.size();
            stream.writeRawData(mesh.indexData.constData(), mesh.indexData.size());

            // Write submeshes
            stream << (quint32)mesh.subMeshes.size();
            for (const auto& sub : mesh.subMeshes) {
                stream << sub.materialIndex;
                stream << sub.vertexOffset;
                stream << sub.vertexCount;
                stream << sub.indexOffset;
                stream << sub.indexCount;
                stream << sub.boundingMin.x;
                stream << sub.boundingMin.y;
                stream << sub.boundingMin.z;
                stream << sub.boundingMax.x;
                stream << sub.boundingMax.y;
                stream << sub.boundingMax.z;
            }
        }

        // Write world matrix
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                stream << kn5.worldMatrix.m[i][j];
            }
        }

    } catch (const std::exception& e) {
        m_lastError = QString("Write error: %1").arg(e.what());
        outFile.close();
        return false;
    }

    outFile.close();
    return true;
}

bool KN5ParserImpl::isValid(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 magic, version;
    stream >> magic >> version;

    return magic == KN5_MAGIC && version == KN5_VERSION;
}

void KN5ParserImpl::parseTexture(KN5File& out, QDataStream& stream) {
    Texture tex;

    quint32 nameLength;
    stream >> nameLength;
    QByteArray nameBytes;
    nameBytes.resize(nameLength);
    stream.readRawData(nameBytes.data(), nameLength);
    tex.name = QString::fromUtf8(nameBytes);

    stream >> tex.width;
    stream >> tex.height;
    stream >> tex.format;
    stream >> tex.mipmapCount;

    quint32 dataSize;
    stream >> dataSize;
    tex.data.resize(dataSize);
    stream.readRawData(tex.data.data(), dataSize);

    out.textureNames.append(tex.name);
    out.textures.append(tex);
}

void KN5ParserImpl::parseMaterial(KN5File& out, QDataStream& stream) {
    Material mat;

    stream >> mat.id;

    quint32 nameLength;
    stream >> nameLength;
    QByteArray nameBytes;
    nameBytes.resize(nameLength);
    stream.readRawData(nameBytes.data(), nameLength);
    mat.name = QString::fromUtf8(nameBytes);

    quint32 shaderLength;
    stream >> shaderLength;
    nameBytes.resize(shaderLength);
    stream.readRawData(nameBytes.data(), shaderLength);
    mat.shaderName = QString::fromUtf8(nameBytes);

    stream >> (quint32&)mat.type;

    quint32 propCount;
    stream >> propCount;
    for (quint32 i = 0; i < propCount; i++) {
        quint32 keyLen, valLen;
        stream >> keyLen;
        QByteArray key(keyLen, 0);
        stream.readRawData(key.data(), keyLen);

        stream >> valLen;
        QByteArray val(valLen, 0);
        stream.readRawData(val.data(), valLen);

        mat.properties[QString::fromUtf8(key)] = QString::fromUtf8(val);
    }

    quint32 texCount;
    stream >> texCount;
    for (quint32 i = 0; i < texCount; i++) {
        quint32 slotLen, texLen;
        stream >> slotLen;
        QByteArray slot(slotLen, 0);
        stream.readRawData(slot.data(), slotLen);

        stream >> texLen;
        QByteArray tex(texLen, 0);
        stream.readRawData(tex.data(), texLen);

        mat.textureMapping[QString::fromUtf8(slot)] = QString::fromUtf8(tex);
    }

    out.materials.append(mat);
}

void KN5ParserImpl::parseMesh(KN5File& out, QDataStream& stream) {
    Mesh mesh;

    quint32 nameLength;
    stream >> nameLength;
    QByteArray nameBytes;
    nameBytes.resize(nameLength);
    stream.readRawData(nameBytes.data(), nameLength);
    mesh.name = QString::fromUtf8(nameBytes);

    quint32 transformType;
    stream >> transformType;
    stream >> mesh.nodeIndex;
    stream >> mesh.castShadows;
    stream >> mesh.isVisible;
    stream >> mesh.isTransparent;
    stream >> (quint32&)mesh.materialType;

    // Read bounding box
    stream >> mesh.boundingMin.x;
    stream >> mesh.boundingMin.y;
    stream >> mesh.boundingMin.z;
    stream >> mesh.boundingMax.x;
    stream >> mesh.boundingMax.y;
    stream >> mesh.boundingMax.z;
    stream >> mesh.boundingRadius;

    // Read vertex layout
    quint32 attrCount;
    stream >> attrCount;
    mesh.vertexLayout.vertexSize = 0;
    for (quint32 i = 0; i < attrCount; i++) {
        quint32 attrType, attrOffset, attrSize;
        stream >> attrType;
        stream >> attrOffset;
        stream >> attrSize;
        mesh.vertexLayout.attributes.append(std::make_pair((AttributeType)attrType, (quint8)attrOffset));
        mesh.vertexLayout.vertexSize += attrSize;
    }

    // Read vertex data
    quint32 vertexCount, vertexBufferSize;
    stream >> vertexCount;
    stream >> vertexBufferSize;
    mesh.vertexData.resize(vertexBufferSize);
    stream.readRawData(mesh.vertexData.data(), vertexBufferSize);

    // Read index data
    quint32 indexCount, indexBufferSize;
    stream >> indexCount;
    stream >> indexBufferSize;
    mesh.indexData.resize(indexBufferSize);
    stream.readRawData(mesh.indexData.data(), indexBufferSize);

    // Read submeshes
    quint32 subMeshCount;
    stream >> subMeshCount;
    for (quint32 i = 0; i < subMeshCount; i++) {
        SubMesh sub;
        stream >> sub.materialIndex;
        stream >> sub.vertexOffset;
        stream >> sub.vertexCount;
        stream >> sub.indexOffset;
        stream >> sub.indexCount;
        stream >> sub.boundingMin.x;
        stream >> sub.boundingMin.y;
        stream >> sub.boundingMin.z;
        stream >> sub.boundingMax.x;
        stream >> sub.boundingMax.y;
        stream >> sub.boundingMax.z;
        mesh.subMeshes.append(sub);
    }

    out.meshes.append(mesh);
}

void MeshHelper::computeBoundingBox(const KN5File& kn5, Vector3& min, Vector3& max) {
    min = Vector3(std::numeric_limits<float>::max(), 
                  std::numeric_limits<float>::max(), 
                  std::numeric_limits<float>::max());
    max = Vector3(-std::numeric_limits<float>::max(), 
                  -std::numeric_limits<float>::max(), 
                  -std::numeric_limits<float>::max());

    for (const auto& mesh : kn5.meshes) {
        min.x = qMin(min.x, mesh.boundingMin.x);
        min.y = qMin(min.y, mesh.boundingMin.y);
        min.z = qMin(min.z, mesh.boundingMin.z);
        max.x = qMax(max.x, mesh.boundingMax.x);
        max.y = qMax(max.y, mesh.boundingMax.y);
        max.z = qMax(max.z, mesh.boundingMax.z);
    }
}

void MeshHelper::computeBoundingSphere(const KN5File& kn5, Vector3& center, float& radius) {
    Vector3 min, max;
    computeBoundingBox(kn5, min, max);
    center = (min + max) * 0.5f;
    Vector3 diff;
    diff.x = max.x - min.x;
    diff.y = max.y - min.y;
    diff.z = max.z - min.z;
    radius = diff.length() * 0.5f;
}

// Mesh inline method implementations (moved from header to avoid duplicate definitions)
quint32 KN5Parser::Mesh::getVertexCount() const {
    const quint32 stride = vertexLayout.vertexSize > 0 ? vertexLayout.vertexSize : 44;
    return stride ? (quint32)vertexData.size() / stride : 0;
}

quint32 KN5Parser::Mesh::getTriangleCount() const {
    return (quint32)indexData.size() / 6; // quint16 indices, 3 per tri
}

quint32 MeshHelper::getTotalTriangles(const KN5File& kn5) {
    quint32 total = 0;
    for (const auto& mesh : kn5.meshes) {
        total += mesh.getTriangleCount();
    }
    return total;
}

quint32 MeshHelper::getTotalVertices(const KN5File& kn5) {
    quint32 total = 0;
    for (const auto& mesh : kn5.meshes) {
        total += mesh.getVertexCount();
    }
    return total;
}

} // namespace KN5Parser