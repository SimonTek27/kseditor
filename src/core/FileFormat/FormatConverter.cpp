#include "FormatConverter.h"
#include <QFileInfo>
#include <QDir>
#include <QProgressDialog>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtGlobal>
#include <algorithm>

#include "assettocorsa/KN5Types.h"
#include "assettocorsa/KN5Parser.h"
#include "FBXParser.h"
#include "GLBParser.h"
#include "CADOBJParser.h"

namespace ks {
using fileformat::MeshData;
using fileformat::MeshSubmesh;
using fileformat::MeshMaterial;
using fileformat::MeshBone;
using fileformat::MeshVertex;

// Bring KN5Parser types into ks namespace for easier access
using KN5Parser::KN5File;
using KN5Parser::Material;
using KN5Parser::Mesh;
using KN5Parser::KN5ParserImpl;
using KN5Parser::SubMesh;
using KN5Parser::Bone;
using KN5Parser::KN5_MAGIC;
using KN5Parser::KN5_VERSION;
using KN5Parser::KN5ParserImpl;

#include "../mesh/MeshOperations.h"

FormatConverter::FormatConverter(QObject* parent)
    : QObject(parent) {
}

FormatConverter::~FormatConverter() {
}

QStringList FormatConverter::supportedConversions() {
    return QStringList() << "KN5 to FBX" << "FBX to KN5"
                       << "KN5 to GLB" << "GLB to KN5"
                       << "FBX to GLB" << "GLB to FBX"
                       << "OBJ to KN5" << "KN5 to OBJ"
                       << "FBX to OBJ" << "OBJ to FBX";
}

FormatConverter::ConversionType FormatConverter::detectConversion(const QString& fromExt, const QString& toExt) {
    QString f = fromExt.toLower();
    QString t = toExt.toLower();

    if (f == "kn5" && t == "fbx") return ConversionType::KN5toFBX;
    if (f == "fbx" && t == "kn5") return ConversionType::FBXtoKN5;
    if (f == "kn5" && t == "glb") return ConversionType::KN5toGLB;
    if (f == "glb" && t == "kn5") return ConversionType::GLBtoKN5;
    if (f == "fbx" && t == "glb") return ConversionType::FBXtoGLB;
    if (f == "glb" && t == "fbx") return ConversionType::GLBtoFBX;
    if (f == "obj" && t == "kn5") return ConversionType::OBJtoKN5;
    if (f == "kn5" && t == "obj") return ConversionType::KN5toOBJ;
    if (f == "fbx" && t == "obj") return ConversionType::FBXtoOBJ;
    if (f == "obj" && t == "fbx") return ConversionType::OBJtoFBX;

    return ConversionType::None;
}

bool FormatConverter::convert(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    QFileInfo inputInfo(inputPath);
    QString fromExt = inputInfo.suffix().toLower();
    QString toExt = QFileInfo(outputPath).suffix().toLower();

    ConversionType type = detectConversion(fromExt, toExt);

    switch (type) {
        case ConversionType::KN5toFBX:
            return convertKN5ToFBX(inputPath, outputPath, options);
        case ConversionType::FBXtoKN5:
            return convertFBXToKN5(inputPath, outputPath, options);
        case ConversionType::KN5toGLB:
            return convertKN5ToGLB(inputPath, outputPath, options);
        case ConversionType::GLBtoKN5:
            return convertGLBToKN5(inputPath, outputPath, options);
        case ConversionType::OBJtoKN5:
            return convertOBJToKN5(inputPath, outputPath, options);
        case ConversionType::KN5toOBJ:
            return convertKN5ToOBJ(inputPath, outputPath, options);
        case ConversionType::FBXtoGLB:
            return convertFBXToGLB(inputPath, outputPath, options);
        case ConversionType::GLBtoFBX:
            return convertGLBToFBX(inputPath, outputPath, options);
        case ConversionType::FBXtoOBJ:
            return convertFBXToOBJ(inputPath, outputPath, options);
        case ConversionType::OBJtoFBX:
            return convertOBJToFBX(inputPath, outputPath, options);
        default:
            emit conversionError("Unsupported conversion: " + fromExt + " -> " + toExt);
            return false;
    }
}

bool FormatConverter::convertToMeshData(const QString& inputPath, MeshData& outData, QString* error) {
    QString ext = QFileInfo(inputPath).suffix().toLower();

    if (ext == "kn5") {
        return kn5ToModel(inputPath, outData);
    }

    if (ext == "fbx") {
        FBXParser parser;
        if (!parser.loadFromFile(inputPath.toStdString())) {
            if (error) *error = "Failed to parse FBX file";
            return false;
        }
        const FBXScene& scene = parser.scene();
        for (const auto& fbxMesh : scene.meshes) {
            MeshData mesh;
            mesh.name = QString::fromStdString(fbxMesh.name);
            MeshSubmesh sub;
            for (const auto& v : fbxMesh.vertices) {
                MeshVertex mv;
                mv.px = v.x; mv.py = v.y; mv.pz = v.z;
                mesh.vertices.append(mv);
            }
            for (const auto& n : fbxMesh.normals) {
                if (n.x < mesh.vertices.size())
                    { mesh.vertices[n.x].nx = n.x; mesh.vertices[n.x].ny = n.y; mesh.vertices[n.x].nz = n.z; }
            }
            mesh.indices = QVector<uint32_t>(fbxMesh.indices.begin(), fbxMesh.indices.end());
            sub.indexCount = fbxMesh.indices.size();
            mesh.submeshes.append(sub);
            outData = mesh;
        }
        return !scene.meshes.empty();
    }

    if (ext == "glb") {
        GLBParser parser;
        if (!parser.loadFromFile(inputPath.toStdString())) {
            if (error) *error = "Failed to parse GLB file";
            return false;
        }
        const GLBScene& scene = parser.scene();
        for (const auto& glbMesh : scene.meshes) {
            MeshData mesh;
            mesh.name = QString::fromStdString(glbMesh.name);
            for (const auto& prim : glbMesh.primitives) {
                auto posIt = prim.attributes.find("POSITION");
                if (posIt != prim.attributes.end()) {
                    auto verts = parser.getVertices(posIt->second);
                    for (const auto& v : verts) {
                        MeshVertex mv;
                        mv.px = v.x; mv.py = v.y; mv.pz = v.z;
                        mesh.vertices.append(mv);
                    }
                }
                auto normIt = prim.attributes.find("NORMAL");
                if (normIt != prim.attributes.end()) {
                    auto norms = parser.getNormals(normIt->second);
                    for (int i = 0; i < norms.size() && i < mesh.vertices.size(); ++i) {
                        mesh.vertices[i].nx = norms[i].x;
                        mesh.vertices[i].ny = norms[i].y;
                        mesh.vertices[i].nz = norms[i].z;
                    }
                }
                auto uvIt = prim.attributes.find("TEXCOORD_0");
                if (uvIt != prim.attributes.end()) {
                    auto uvs = parser.getTexCoords(uvIt->second);
                    for (int i = 0; i < uvs.size() && i < mesh.vertices.size(); ++i) {
                        mesh.vertices[i].u0 = uvs[i].x;
                        mesh.vertices[i].v0 = 1.0f - uvs[i].y;
                    }
                }
                if (prim.indices != 0xFFFFFFFF) {
                    auto idx = parser.getIndices(prim.indices);
                    uint32_t base = mesh.vertices.size();
                    for (auto& i : idx) mesh.indices.append(base + i);
                    MeshSubmesh sub;
                    sub.indexOffset = mesh.indices.size() - idx.size();
                    sub.indexCount = idx.size();
                    mesh.submeshes.append(sub);
                }
            }
            outData = mesh;
        }
        return !scene.meshes.empty();
    }

    if (ext == "obj") {
        CADOBJParser parser;
        if (!parser.loadFromFile(inputPath.toStdString())) {
            if (error) *error = "Failed to parse OBJ file";
            return false;
        }
        const CADOBJScene& scene = parser.scene();
        for (const auto& objMesh : scene.meshes) {
            MeshData mesh;
            mesh.name = QString::fromStdString(objMesh.name.empty() ? "Mesh" : objMesh.name);
            for (const auto& v : objMesh.vertices) {
                MeshVertex mv;
                mv.px = v.x; mv.py = v.y; mv.pz = v.z;
                mesh.vertices.append(mv);
            }
            for (int ni = 0; ni < objMesh.normals.size() && ni < mesh.vertices.size(); ++ni) {
                const auto& n = objMesh.normals[ni];
                mesh.vertices[ni].nx = n.x;
                mesh.vertices[ni].ny = n.y;
                mesh.vertices[ni].nz = n.z;
            }
            if (!objMesh.indices.empty()) {
                for (const auto& tri : objMesh.indices) {
                    mesh.indices.append(static_cast<uint32_t>(tri.x));
                }
                MeshSubmesh sub;
                sub.indexCount = objMesh.indices.size();
                mesh.submeshes.append(sub);
            }
            outData = mesh;
        }
        return !scene.meshes.empty();
    }

    if (error) *error = "Unsupported format: " + ext;
    return false;
}

bool FormatConverter::convertFromMeshData(const MeshData& data, const QString& outputPath, QString* error) {
    QString ext = QFileInfo(outputPath).suffix().toLower();
    if (ext == "kn5") return modelToKN5(data, outputPath);
    if (ext == "obj") return modelToOBJ(data, outputPath);
    if (ext == "fbx") return modelToFBX(data, outputPath);
    if (ext == "gltf" || ext == "glb") return modelToGLTF(data, outputPath);
    if (error) *error = "Unsupported output format: " + ext;
    return false;
}

bool FormatConverter::modelToOBJ(const MeshData& model, const QString& outputPath) {
    QStringList objLines;
    objLines << "# Generated from MeshData";
    objLines << "";

    QString mtlFileName = QFileInfo(outputPath).baseName() + ".mtl";
    QString mtlPath = QFileInfo(outputPath).dir().path() + "/" + mtlFileName;

    if (!model.materials.isEmpty()) {
        QStringList mtlLines;
        mtlLines << "# Materials Export";
        for (const auto& mat : model.materials) {
            mtlLines << "";
            mtlLines << QString("newmtl %1").arg(mat.name);
            mtlLines << "Ka 0.2 0.2 0.2";
            mtlLines << "Kd 0.8 0.8 0.8";
            mtlLines << "Ks 0.2 0.2 0.2";
            mtlLines << "Ns 32.0";
            mtlLines << "d 1.0";
        }
        QFile mtlFile(mtlPath);
        if (mtlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            mtlFile.write(mtlLines.join("\n").toUtf8());
            mtlFile.close();
            objLines << "mtllib " + mtlFileName;
            objLines << "";
        }
    }

    quint32 vertexOffset = 1;

    for (const auto& sm : model.submeshes) {
        if (!sm.materialName.isEmpty()) {
            for (int mi = 0; mi < model.materials.size(); ++mi) {
                if (model.materials[mi].name == sm.materialName) {
                    objLines << QString("usemtl %1").arg(model.materials[mi].name);
                    break;
                }
            }
        }

        for (uint32_t i = 0; i + 2 < sm.indexCount; i += 3) {
            uint32_t idx0 = model.indices[sm.indexOffset + i];
            uint32_t idx1 = model.indices[sm.indexOffset + i + 1];
            uint32_t idx2 = model.indices[sm.indexOffset + i + 2];

            int i0 = static_cast<int>(idx0) + 1;
            int i1 = static_cast<int>(idx1) + 1;
            int i2 = static_cast<int>(idx2) + 1;

            objLines << QString("f %1/%1/%1 %2/%2/%2 %3/%3/%3")
                .arg(i0 + vertexOffset - 1)
                .arg(i1 + vertexOffset - 1)
                .arg(i2 + vertexOffset - 1);
        }
    }

    // Write vertices with normals and UVs
    for (const auto& v : model.vertices) {
        objLines << QString("v %1 %2 %3").arg(v.px).arg(v.py).arg(v.pz);
    }
    for (const auto& v : model.vertices) {
        objLines << QString("vt %1 %2").arg(v.u0).arg(1.0f - v.v0);
    }
    for (const auto& v : model.vertices) {
        objLines << QString("vn %1 %2 %3").arg(v.nx).arg(v.ny).arg(v.nz);
    }

    QFile objFile(outputPath);
    if (!objFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    objFile.write(objLines.join("\n").toUtf8());
    return true;
}

bool FormatConverter::modelToFBX(const MeshData& model, const QString& outputPath) {
    QStringList fbxLines;
    fbxLines << "FBX 7.4.0 project";
    fbxLines << "";
    fbxLines << "Nodes: 1";
    fbxLines << "";
    fbxLines << "Node: \"Root\" {";
    fbxLines << "    Model: \"Mesh\" {";
    fbxLines << QString("        Vertices: * %1").arg(model.vertices.size());

    for (const auto& v : model.vertices) {
        fbxLines << QString("        v %1 %2 %3").arg(v.px).arg(v.py).arg(v.pz);
    }

    if (!model.vertices.isEmpty()) {
        fbxLines << QString("        Normals: * %1").arg(model.vertices.size());
        for (const auto& v : model.vertices) {
            fbxLines << QString("        n %1 %2 %3").arg(v.nx).arg(v.ny).arg(v.nz);
        }
    }

    if (!model.vertices.isEmpty()) {
        fbxLines << QString("        UV: * %1").arg(model.vertices.size());
        for (const auto& v : model.vertices) {
            fbxLines << QString("        uv %1 %2").arg(v.u0).arg(v.v0);
        }
    }

    if (!model.indices.isEmpty()) {
        int triCount = model.indices.size() / 3;
        fbxLines << QString("        Polygons: * %1").arg(triCount);
        for (int i = 0; i + 2 < model.indices.size(); i += 3) {
            fbxLines << QString("        p %1 %2 %3").arg(model.indices[i]).arg(model.indices[i + 1]).arg(model.indices[i + 2]);
        }
    }

    fbxLines << "    }";
    fbxLines << "}";

    QFile fbxFile(outputPath);
    if (!fbxFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    fbxFile.write(fbxLines.join("\n").toUtf8());
    return true;
}

bool FormatConverter::modelToGLTF(const MeshData& model, const QString& outputPath) {
    QJsonObject gltf;
    gltf["asset"] = QJsonObject{{"version", "2.0"}, {"generator", "ksEditor FormatConverter"}};

    QJsonArray buffers;
    QJsonArray bufferViews;
    QJsonArray accessors;
    QJsonArray meshes;
    QJsonArray nodes;

    QByteArray bufferData;
    int bufferOffset = 0;

    auto addBufferView = [&](int byteLength, int target) -> int {
        QJsonObject bv;
        bv["buffer"] = 0;
        bv["byteOffset"] = bufferOffset;
        bv["byteLength"] = byteLength;
        bv["target"] = target;
        int idx = bufferViews.size();
        bufferViews.append(bv);
        bufferOffset += byteLength;
        return idx;
    };

    auto addAccessor = [&](int bufferView, int componentType, int count, const QString& type, const QJsonValue& min = QJsonValue(), const QJsonValue& max = QJsonValue()) -> int {
        QJsonObject acc;
        acc["bufferView"] = bufferView;
        acc["byteOffset"] = 0;
        acc["componentType"] = componentType;
        acc["count"] = count;
        acc["type"] = type;
        if (!min.isNull()) acc["min"] = min;
        if (!max.isNull()) acc["max"] = max;
        int idx = accessors.size();
        accessors.append(acc);
        return idx;
    };

    QJsonArray primitives;
    int posAccessor = -1, normAccessor = -1, uvAccessor = -1, idxAccessor = -1;

    if (!model.vertices.isEmpty()) {
        QVector<float> positions;
        positions.reserve(model.vertices.size() * 3);
        for (const auto& v : model.vertices) {
            positions << v.px << v.py << v.pz;
        }
        QByteArray posData(reinterpret_cast<const char*>(positions.constData()), positions.size() * sizeof(float));
        int posBufferView = addBufferView(posData.size(), 34962);
        bufferData.append(posData);
        posAccessor = addAccessor(posBufferView, 5126, model.vertices.size(), "VEC3");

        if (!model.vertices.isEmpty()) {
            QVector<float> normals;
            normals.reserve(model.vertices.size() * 3);
            for (const auto& v : model.vertices) {
                normals << v.nx << v.ny << v.nz;
            }
            QByteArray normData(reinterpret_cast<const char*>(normals.constData()), normals.size() * sizeof(float));
            int normBufferView = addBufferView(normData.size(), 34962);
            bufferData.append(normData);
            normAccessor = addAccessor(normBufferView, 5126, model.vertices.size(), "VEC3");
        }

        if (!model.vertices.isEmpty()) {
            QVector<float> uvs;
            uvs.reserve(model.vertices.size() * 2);
            for (const auto& v : model.vertices) {
                uvs << v.u0 << v.v0;
            }
            QByteArray uvData(reinterpret_cast<const char*>(uvs.constData()), uvs.size() * sizeof(float));
            int uvBufferView = addBufferView(uvData.size(), 34962);
            bufferData.append(uvData);
            uvAccessor = addAccessor(uvBufferView, 5126, model.vertices.size(), "VEC2");
        }
    }

    if (!model.indices.isEmpty()) {
        QVector<quint16> indices;
        indices.reserve(model.indices.size());
        for (int idx : model.indices) indices.append(static_cast<quint16>(idx));
        QByteArray idxData(reinterpret_cast<const char*>(indices.constData()), indices.size() * sizeof(quint16));
        int idxBufferView = addBufferView(idxData.size(), 34963);
        bufferData.append(idxData);
        idxAccessor = addAccessor(idxBufferView, 5123, model.indices.size(), "SCALAR");
    }

    QJsonObject primitive;
    QJsonObject attributes;
    if (posAccessor >= 0) attributes["POSITION"] = posAccessor;
    if (normAccessor >= 0) attributes["NORMAL"] = normAccessor;
    if (uvAccessor >= 0) attributes["TEXCOORD_0"] = uvAccessor;
    primitive["attributes"] = attributes;
    if (idxAccessor >= 0) primitive["indices"] = idxAccessor;
    primitives.append(primitive);

    QJsonObject meshObj;
    meshObj["primitives"] = primitives;
    meshObj["name"] = model.name.isEmpty() ? "Mesh" : model.name;
    meshes.append(meshObj);

    QJsonObject node;
    node["mesh"] = 0;
    node["name"] = model.name.isEmpty() ? "Mesh" : model.name;
    nodes.append(node);

    QJsonObject buffer;
    buffer["byteLength"] = bufferData.size();
    buffer["uri"] = QString("data:application/octet-stream;base64,") + bufferData.toBase64();
    buffers.append(buffer);

    gltf["buffers"] = buffers;
    gltf["bufferViews"] = bufferViews;
    gltf["accessors"] = accessors;
    gltf["meshes"] = meshes;
    gltf["nodes"] = nodes;
    gltf["scenes"] = QJsonArray{QJsonObject{{"nodes", QJsonArray{0}}}};
    gltf["scene"] = 0;

    QJsonDocument doc(gltf);
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

// ============================================================================
// KN5 <-> MeshData bridge
// ============================================================================

bool FormatConverter::kn5ToModel(const QString& inputPath, MeshData& model) {
    QString err;
    auto kn5File = KN5ParserImpl::parse(inputPath, &err);
    if (!kn5File.isValid()) return false;

    for (auto& mesh : kn5File.meshes) {
        mesh.decodeVertices();
    }

    for (const auto& kn5Mesh : kn5File.meshes) {
        MeshData mesh;
        mesh.name = kn5Mesh.name;

        for (int i = 0; i < kn5Mesh.positions.size(); ++i) {
            MeshVertex mv;
            mv.px = kn5Mesh.positions[i].x();
            mv.py = kn5Mesh.positions[i].y();
            mv.pz = kn5Mesh.positions[i].z();
            if (i < kn5Mesh.normals.size()) {
                mv.nx = kn5Mesh.normals[i].x();
                mv.ny = kn5Mesh.normals[i].y();
                mv.nz = kn5Mesh.normals[i].z();
            }
            if (i < kn5Mesh.uv0.size()) {
                mv.u0 = kn5Mesh.uv0[i].x();
                mv.v0 = kn5Mesh.uv0[i].y();
            }
            mesh.vertices.append(mv);
        }

        const quint16* idx = reinterpret_cast<const quint16*>(kn5Mesh.indexData.constData());
        int nIdx = kn5Mesh.indexData.size() / 2;
        for (int i = 0; i < nIdx; ++i)
            mesh.indices.append(idx[i]);

        for (const auto& sm : kn5Mesh.subMeshes) {
            MeshSubmesh sub;
            sub.indexOffset = sm.indexOffset;
            sub.indexCount = sm.indexCount;
            if (sm.materialIndex < (quint32)kn5File.materials.size())
                sub.materialName = kn5File.materials[sm.materialIndex].name;
            mesh.submeshes.append(sub);
        }

        model = mesh;
        break;
    }

    for (const auto& mat : kn5File.materials) {
        MeshMaterial mm;
        mm.name = mat.name;
        mm.shaderName = mat.shaderName;
        for (auto it = mat.textureMapping.begin(); it != mat.textureMapping.end(); ++it)
            mm.textures[it.key()] = it.value();
        model.materials.append(mm);
    }

    for (const auto& bone : kn5File.bones) {
        MeshBone mb;
        mb.name = bone.name;
        mb.parentIndex = bone.parentIndex;
        memcpy(mb.matrix, bone.matrix, sizeof(float) * 16);
        model.bones.append(mb);
    }

    return true;
}

bool FormatConverter::modelToKN5(const MeshData& model, const QString& outputPath) {
    KN5File kn5File;
    kn5File.header.magic = KN5_MAGIC;
    kn5File.header.version = KN5_VERSION;

    for (const auto& mm : model.materials) {
        Material mat;
        mat.id = kn5File.materials.size();
        mat.name = mm.name;
        mat.shaderName = mm.shaderName;
        mat.textureMapping = mm.textures;
        kn5File.materials.push_back(mat);
    }

    {
        Mesh mesh;
        mesh.name = model.name.isEmpty() ? "Mesh" : model.name;

        for (const auto& v : model.vertices) {
            mesh.positions.push_back({v.px, v.py, v.pz});
            mesh.normals.push_back({v.nx, v.ny, v.nz});
            mesh.uv0.push_back({v.u0, v.v0});
        }

        mesh.indexData.resize(model.indices.size() * sizeof(quint16));
        quint16* idxPtr = reinterpret_cast<quint16*>(mesh.indexData.data());
        for (int i = 0; i < model.indices.size(); ++i)
            idxPtr[i] = static_cast<quint16>(model.indices[i]);

        for (const auto& sm : model.submeshes) {
            SubMesh sub;
            sub.vertexOffset = 0;
            sub.vertexCount = mesh.positions.size();
            sub.indexOffset = sm.indexOffset;
            sub.indexCount = sm.indexCount;
            for (int mi = 0; mi < model.materials.size(); ++mi) {
                if (model.materials[mi].name == sm.materialName) {
                    sub.materialIndex = mi;
                    break;
                }
            }
            mesh.subMeshes.push_back(sub);
        }

        kn5File.meshes.push_back(mesh);
    }

    for (const auto& mb : model.bones) {
        Bone bone;
        bone.name = mb.name;
        bone.parentIndex = mb.parentIndex;
        memcpy(bone.matrix, mb.matrix, sizeof(float) * 16);
        kn5File.bones.push_back(bone);
    }

    return KN5ParserImpl::write(outputPath, kn5File);
}

// ============================================================================
// KN5 <-> OBJ Conversion
// ============================================================================

bool FormatConverter::convertKN5ToOBJ(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    emit progressChanged(0.1f);

    QString err;
    KN5File kn5File = KN5ParserImpl::parse(inputPath, &err);
    if (!kn5File.isValid()) {
        emit conversionError("Failed to read KN5 file: " + err);
        return false;
    }

    for (auto& mesh : kn5File.meshes) {
        mesh.decodeVertices();
    }

    emit progressChanged(0.3f);

    QStringList objLines;
    objLines << "# Generated from KN5 file";
    objLines << "# Source: " + inputPath;
    objLines << "";

    QString mtlFileName = QFileInfo(outputPath).baseName() + ".mtl";
    QString mtlPath = QFileInfo(outputPath).dir().path() + "/" + mtlFileName;

    if (options.preserveMaterials && !kn5File.materials.isEmpty()) {
        QStringList mtlLines;
        mtlLines << "# KN5 Materials Export";
        for (const auto& mat : kn5File.materials) {
            mtlLines << "";
            mtlLines << QString("newmtl %1").arg(mat.name);
            mtlLines << "Ka 0.2 0.2 0.2";
            mtlLines << "Kd 0.8 0.8 0.8";
            mtlLines << "Ks 0.2 0.2 0.2";
            mtlLines << "Ns 32.0";
            mtlLines << "d 1.0";
        }
        QFile mtlFile(mtlPath);
        if (mtlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            mtlFile.write(mtlLines.join("\n").toUtf8());
            mtlFile.close();
            objLines << "mtllib " + mtlFileName;
            objLines << "";
        }
    }

    quint32 vertexOffset = 1;
    quint32 normalOffset = 1;
    quint32 texcoordOffset = 1;

    for (const auto& mesh : kn5File.meshes) {
        objLines << QString("# Mesh: %1").arg(mesh.name);
        objLines << QString("g %1").arg(mesh.name);

        for (const auto& pos : mesh.positions) {
            objLines << QString("v %1 %2 %3").arg(pos.x()).arg(pos.y()).arg(pos.z());
        }

        for (const auto& norm : mesh.normals) {
            objLines << QString("vn %1 %2 %3").arg(norm.x()).arg(norm.y()).arg(norm.z());
        }

        if (options.preserveUVs) {
            for (const auto& uv : mesh.uv0) {
                objLines << QString("vt %1 %2").arg(uv.x()).arg(1.0f - uv.y());
            }
        }

        if (options.preserveMaterials && !mesh.subMeshes.isEmpty()) {
            for (const auto& submesh : mesh.subMeshes) {
                if (submesh.materialIndex < (quint32)kn5File.materials.size()) {
                    objLines << QString("usemtl %1").arg(kn5File.materials[submesh.materialIndex].name);
                }

                const quint16* indices = (const quint16*)mesh.indexData.constData();
                quint32 numIndices = mesh.indexData.size() / sizeof(quint16);

                for (quint32 i = submesh.indexOffset; i < submesh.indexOffset + submesh.indexCount && i < numIndices; i += 3) {
                    if (i + 2 < numIndices) {
                        quint32 i0 = indices[i] + vertexOffset;
                        quint32 i1 = indices[i + 1] + vertexOffset;
                        quint32 i2 = indices[i + 2] + vertexOffset;

                        if (options.preserveUVs && !mesh.uv0.isEmpty()) {
                            objLines << QString("f %1/%2/%3 %4/%5/%6 %7/%8/%9")
                                .arg(i0).arg(i0).arg(i0 + normalOffset - 1)
                                .arg(i1).arg(i1).arg(i1 + normalOffset - 1)
                                .arg(i2).arg(i2).arg(i2 + normalOffset - 1);
                        } else {
                            objLines << QString("f %1//%2 %3//%4 %5//%6")
                                .arg(i0).arg(i0 + normalOffset - 1)
                                .arg(i1).arg(i1 + normalOffset - 1)
                                .arg(i2).arg(i2 + normalOffset - 1);
                        }
                    }
                }
            }
        }

        vertexOffset += mesh.positions.size();
        normalOffset += mesh.normals.size();
        if (options.preserveUVs) texcoordOffset += mesh.uv0.size();
    }

    emit progressChanged(0.8f);

    QFile objFile(outputPath);
    if (!objFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit conversionError("Cannot write OBJ file: " + outputPath);
        return false;
    }
    objFile.write(objLines.join("\n").toUtf8());
    objFile.close();

    emit progressChanged(1.0f);
    return true;
}

bool FormatConverter::convertOBJToKN5(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    emit progressChanged(0.1f);

    CADOBJParser objParser;
    if (!objParser.loadFromFile(inputPath.toStdString())) {
        emit conversionError("Failed to parse OBJ file: " + inputPath);
        return false;
    }

    emit progressChanged(0.3f);

    const CADOBJScene& objScene = objParser.scene();

    KN5File kn5File;
    kn5File.header.magic = KN5_MAGIC;
    kn5File.header.version = KN5_VERSION;

    int matIdx = 0;
    for (const auto& objMatPair : objScene.materials) {
        const auto& objMat = objMatPair.second;
        Material mat;
        mat.id = matIdx++;
        mat.name = QString::fromStdString(objMatPair.first);
        mat.shaderName = "ksPerPixelMultiMap";
        mat.type = Material::Type::Normal;
        mat.alphaBlending = (objMat.d < 1.0f);
        kn5File.materials.push_back(mat);
    }

    emit progressChanged(0.5f);

    for (const auto& objMesh : objScene.meshes) {
        Mesh mesh;
        mesh.name = QString::fromStdString(objMesh.name.empty() ? "Mesh" : objMesh.name);
        mesh.castShadows = true;
        mesh.isVisible = true;

        for (const auto& v : objMesh.vertices) {
            mesh.positions.push_back({v.x, v.y, v.z});
        }
        for (const auto& n : objMesh.normals) {
            mesh.normals.push_back({n.x, n.y, n.z});
        }
        for (const auto& uv : objMesh.texCoords) {
            mesh.uv0.push_back(QVector2D(uv.x, 1.0f - uv.y));
        }

        if (!objMesh.indices.empty()) {
            mesh.indexData.resize(objMesh.indices.size() * sizeof(quint16));
            quint16* idxPtr = (quint16*)mesh.indexData.data();
            for (size_t i = 0; i < objMesh.indices.size(); ++i) {
                idxPtr[i] = (quint16)((uint32_t)objMesh.indices[i].x);
            }
        }

        SubMesh submesh;
        submesh.materialIndex = objScene.materials.count(objMesh.materialName) > 0 ? matIdx - 1 : 0;
        submesh.vertexOffset = 0;
        submesh.vertexCount = objMesh.vertices.size();
        submesh.indexOffset = 0;
        submesh.indexCount = objMesh.indices.size();
        mesh.subMeshes.push_back(submesh);

        kn5File.meshes.push_back(mesh);
    }

    emit progressChanged(0.8f);

    if (!KN5ParserImpl::write(outputPath, kn5File)) {
        emit conversionError("Failed to write KN5 file: " + outputPath);
        return false;
    }

    emit progressChanged(1.0f);
    return true;
}

// ============================================================================
// KN5 <-> GLB Conversion
// ============================================================================

bool FormatConverter::convertKN5ToGLB(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    emit progressChanged(0.1f);

    QString err;
    auto kn5File = KN5ParserImpl::parse(inputPath, &err);
    if (!kn5File.isValid()) {
        emit conversionError("Failed to read KN5 file: " + err);
        return false;
    }

    for (auto& mesh : kn5File.meshes) {
        mesh.decodeVertices();
    }

    emit progressChanged(0.3f);

    QByteArray binBuffer;
    QJsonArray accessors;
    QJsonArray bufferViews;
    QJsonArray meshes;
    QJsonArray nodes;
    QJsonArray materialsJson;

    int nodeIdx = 0;
    int accessorIdx = 0;
    int bufferViewIdx = 0;

    for (const auto& mat : kn5File.materials) {
        QJsonObject matObj;
        matObj["name"] = mat.name;
        QJsonObject pbr;
        QJsonObject baseColorFactor;
        baseColorFactor["r"] = 0.8;
        baseColorFactor["g"] = 0.8;
        baseColorFactor["b"] = 0.8;
        baseColorFactor["a"] = mat.alphaBlending ? 0.5 : 1.0;
        pbr["baseColorFactor"] = baseColorFactor;
        pbr["metallicFactor"] = 0.0;
        pbr["roughnessFactor"] = 1.0;
        matObj["pbrMetallicRoughness"] = pbr;
        materialsJson.append(matObj);
    }

    emit progressChanged(0.5f);

    for (const auto& mesh : kn5File.meshes) {
        QJsonObject meshObj;
        meshObj["name"] = mesh.name;
        QJsonArray primitives;

        QVector<QVector3D> allPositions;
        QVector<QVector3D> allNormals;
        QVector<QVector2D> allUVs;
        QVector<quint32> allIndices;

        if (!mesh.subMeshes.isEmpty()) {
            for (const auto& submesh : mesh.subMeshes) {
                const quint16* indices = reinterpret_cast<const quint16*>(mesh.indexData.constData());
                quint32 numIndices = mesh.indexData.size() / sizeof(quint16);

                QJsonObject primObj;
                if (options.preserveMaterials && submesh.materialIndex < (quint32)kn5File.materials.size()) {
                    primObj["material"] = (int)submesh.materialIndex;
                }

                int posViewIdx = bufferViewIdx;
                QByteArray posData;
                QSet<quint16> subIndices;
                for (quint32 i = submesh.indexOffset; i < submesh.indexOffset + submesh.indexCount && i < numIndices; i++) {
                    subIndices.insert(indices[i]);
                }
                QVector<quint16> sortedIndices = subIndices.values();
                std::sort(sortedIndices.begin(), sortedIndices.end());

                for (quint16 idx : sortedIndices) {
                    if (idx < (quint16)mesh.positions.size()) {
                        const auto& p = mesh.positions[idx];
                        float pos[3] = {p.x(), p.y(), p.z()};
                        posData.append(reinterpret_cast<const char*>(pos), 12);
                    }
                }

                QJsonObject posBV;
                posBV["buffer"] = 0;
                posBV["byteOffset"] = binBuffer.size();
                posBV["byteLength"] = posData.size();
                posBV["target"] = 34962;
                bufferViews.append(posBV);

                binBuffer.append(posData);

                QJsonObject posAcc;
                posAcc["bufferView"] = bufferViewIdx;
                posAcc["componentType"] = 5126;
                posAcc["count"] = sortedIndices.size();
                posAcc["type"] = "VEC3";
                float minVal[3] = {1e30f, 1e30f, 1e30f};
                float maxVal[3] = {-1e30f, -1e30f, -1e30f};
                for (int i = 0; i < posData.size(); i += 12) {
                    float v[3];
                    memcpy(v, posData.constData() + i, 12);
                    for (int c = 0; c < 3; c++) {
                        minVal[c] = qMin(minVal[c], v[c]);
                        maxVal[c] = qMax(maxVal[c], v[c]);
                    }
                }
                QJsonArray minArr, maxArr;
                for (int c = 0; c < 3; c++) { minArr.append(minVal[c]); maxArr.append(maxVal[c]); }
                posAcc["min"] = minArr;
                posAcc["max"] = maxArr;
                accessors.append(posAcc);
                int posAccIdx = accessorIdx++;
                bufferViewIdx++;

                {
                    QJsonObject attrs = primObj["attributes"].toObject();
                    attrs["POSITION"] = posAccIdx;
                    primObj["attributes"] = attrs;
                }

                if (options.preserveNormals && !mesh.normals.isEmpty()) {
                    QByteArray normData;
                    for (quint16 idx : sortedIndices) {
                        if (idx < (quint16)mesh.normals.size()) {
                            const auto& n = mesh.normals[idx];
                            float norm[3] = {n.x(), n.y(), n.z()};
                            normData.append(reinterpret_cast<const char*>(norm), 12);
                        }
                    }
                    QJsonObject normBV;
                    normBV["buffer"] = 0;
                    normBV["byteOffset"] = binBuffer.size();
                    normBV["byteLength"] = normData.size();
                    normBV["target"] = 34962;
                    bufferViews.append(normBV);
                    binBuffer.append(normData);

                    QJsonObject normAcc;
                    normAcc["bufferView"] = bufferViewIdx;
                    normAcc["componentType"] = 5126;
                    normAcc["count"] = sortedIndices.size();
                    normAcc["type"] = "VEC3";
                    accessors.append(normAcc);
                    {
                        QJsonObject attrs = primObj["attributes"].toObject();
                        attrs["NORMAL"] = accessorIdx++;
                        primObj["attributes"] = attrs;
                    }
                    bufferViewIdx++;
                }

                if (options.preserveUVs && !mesh.uv0.isEmpty()) {
                    QByteArray uvData;
                    for (quint16 idx : sortedIndices) {
                        if (idx < (quint16)mesh.uv0.size()) {
                            const auto& uv = mesh.uv0[idx];
                            float uvCoord[2] = {uv.x(), 1.0f - uv.y()};
                            uvData.append(reinterpret_cast<const char*>(uvCoord), 8);
                        }
                    }
                    QJsonObject uvBV;
                    uvBV["buffer"] = 0;
                    uvBV["byteOffset"] = binBuffer.size();
                    uvBV["byteLength"] = uvData.size();
                    uvBV["target"] = 34962;
                    bufferViews.append(uvBV);
                    binBuffer.append(uvData);

                    QJsonObject uvAcc;
                    uvAcc["bufferView"] = bufferViewIdx;
                    uvAcc["componentType"] = 5126;
                    uvAcc["count"] = sortedIndices.size();
                    uvAcc["type"] = "VEC2";
                    accessors.append(uvAcc);
                    {
                        QJsonObject attrs = primObj["attributes"].toObject();
                        attrs["TEXCOORD_0"] = accessorIdx++;
                        primObj["attributes"] = attrs;
                    }
                    bufferViewIdx++;
                }

                QByteArray idxData;
                for (int i = 0; i < sortedIndices.size(); i++) {
                    quint32 sequentialIdx = i;
                    idxData.append(reinterpret_cast<const char*>(&sequentialIdx), 4);
                }

                QJsonObject idxBV;
                idxBV["buffer"] = 0;
                idxBV["byteOffset"] = binBuffer.size();
                idxBV["byteLength"] = idxData.size();
                idxBV["target"] = 34963;
                bufferViews.append(idxBV);
                binBuffer.append(idxData);

                QJsonObject idxAcc;
                idxAcc["bufferView"] = bufferViewIdx;
                idxAcc["componentType"] = 5125;
                idxAcc["count"] = sortedIndices.size();
                idxAcc["type"] = "SCALAR";
                accessors.append(idxAcc);
                primObj["indices"] = accessorIdx++;
                bufferViewIdx++;

                primitives.append(primObj);
            }
        } else {
            QJsonObject primObj;

            QByteArray posData;
            for (int i = 0; i < mesh.positions.size(); i++) {
                const auto& p = mesh.positions[i];
                float pos[3] = {p.x(), p.y(), p.z()};
                posData.append(reinterpret_cast<const char*>(pos), 12);
            }

            QJsonObject posBV;
            posBV["buffer"] = 0;
            posBV["byteOffset"] = binBuffer.size();
            posBV["byteLength"] = posData.size();
            posBV["target"] = 34962;
            bufferViews.append(posBV);
            binBuffer.append(posData);

            float minVal[3] = {1e30f, 1e30f, 1e30f};
            float maxVal[3] = {-1e30f, -1e30f, -1e30f};
            for (int i = 0; i < posData.size(); i += 12) {
                float v[3]; memcpy(v, posData.constData() + i, 12);
                for (int c = 0; c < 3; c++) { minVal[c] = qMin(minVal[c], v[c]); maxVal[c] = qMax(maxVal[c], v[c]); }
            }

            QJsonObject posAcc;
            posAcc["bufferView"] = bufferViewIdx;
            posAcc["componentType"] = 5126;
            posAcc["count"] = mesh.positions.size();
            posAcc["type"] = "VEC3";
            QJsonArray minArr, maxArr;
            for (int c = 0; c < 3; c++) { minArr.append(minVal[c]); maxArr.append(maxVal[c]); }
            posAcc["min"] = minArr; posAcc["max"] = maxArr;
            accessors.append(posAcc);
            {
                QJsonObject attrs = primObj["attributes"].toObject();
                attrs["POSITION"] = accessorIdx++;
                primObj["attributes"] = attrs;
            }
            bufferViewIdx++;

            if (options.preserveNormals && !mesh.normals.isEmpty()) {
                QByteArray normData;
                for (const auto& n : mesh.normals) {
                    float norm[3] = {n.x(), n.y(), n.z()};
                    normData.append(reinterpret_cast<const char*>(norm), 12);
                }
                QJsonObject normBV;
                normBV["buffer"] = 0; normBV["byteOffset"] = binBuffer.size();
                normBV["byteLength"] = normData.size(); normBV["target"] = 34962;
                bufferViews.append(normBV); binBuffer.append(normData);
                QJsonObject normAcc;
                normAcc["bufferView"] = bufferViewIdx; normAcc["componentType"] = 5126;
                normAcc["count"] = mesh.normals.size(); normAcc["type"] = "VEC3";
                accessors.append(normAcc);
                {
                    QJsonObject attrs = primObj["attributes"].toObject();
                    attrs["NORMAL"] = accessorIdx++;
                    primObj["attributes"] = attrs;
                }
                bufferViewIdx++;
            }

            if (options.preserveUVs && !mesh.uv0.isEmpty()) {
                QByteArray uvData;
                for (const auto& uv : mesh.uv0) {
                    float uvCoord[2] = {uv.x(), 1.0f - uv.y()};
                    uvData.append(reinterpret_cast<const char*>(uvCoord), 8);
                }
                QJsonObject uvBV;
                uvBV["buffer"] = 0; uvBV["byteOffset"] = binBuffer.size();
                uvBV["byteLength"] = uvData.size(); uvBV["target"] = 34962;
                bufferViews.append(uvBV); binBuffer.append(uvData);
                QJsonObject uvAcc;
                uvAcc["bufferView"] = bufferViewIdx; uvAcc["componentType"] = 5126;
                uvAcc["count"] = mesh.uv0.size(); uvAcc["type"] = "VEC2";
                accessors.append(uvAcc);
                {
                    QJsonObject attrs = primObj["attributes"].toObject();
                    attrs["TEXCOORD_0"] = accessorIdx++;
                    primObj["attributes"] = attrs;
                }
                bufferViewIdx++;
            }

            if (!mesh.indexData.isEmpty()) {
                const quint16* idx = reinterpret_cast<const quint16*>(mesh.indexData.constData());
                int numIdx = mesh.indexData.size() / sizeof(quint16);
                QByteArray idxData;
                for (int i = 0; i < numIdx; i++) {
                    quint32 sequentialIdx = i;
                    idxData.append(reinterpret_cast<const char*>(&sequentialIdx), 4);
                }
                QJsonObject idxBV;
                idxBV["buffer"] = 0; idxBV["byteOffset"] = binBuffer.size();
                idxBV["byteLength"] = idxData.size(); idxBV["target"] = 34963;
                bufferViews.append(idxBV); binBuffer.append(idxData);
                QJsonObject idxAcc;
                idxAcc["bufferView"] = bufferViewIdx; idxAcc["componentType"] = 5125;
                idxAcc["count"] = numIdx; idxAcc["type"] = "SCALAR";
                accessors.append(idxAcc);
                primObj["indices"] = accessorIdx++; bufferViewIdx++;
            }

            primitives.append(primObj);
        }

        meshObj["primitives"] = primitives;
        meshes.append(meshObj);

        QJsonObject nodeObj;
        nodeObj["mesh"] = nodeIdx;
        nodes.append(nodeObj);
        nodeIdx++;
    }

    emit progressChanged(0.8f);

    while (binBuffer.size() % 4 != 0) {
        binBuffer.append('\0');
    }

    QJsonObject gltf;
    QJsonObject asset;
    asset["version"] = "2.0";
    asset["generator"] = "ksEditor";
    gltf["asset"] = asset;

    QJsonArray scenes;
    QJsonObject sceneObj;
    QJsonArray sceneNodes;
    for (int i = 0; i < nodeIdx; i++) sceneNodes.append(i);
    sceneObj["nodes"] = sceneNodes;
    scenes.append(sceneObj);
    gltf["scenes"] = scenes;
    gltf["scene"] = 0;
    gltf["nodes"] = nodes;
    gltf["meshes"] = meshes;
    gltf["accessors"] = accessors;
    gltf["bufferViews"] = bufferViews;
    gltf["materials"] = materialsJson;

    QJsonArray buffers;
    QJsonObject bufObj;
    bufObj["byteLength"] = binBuffer.size();
    buffers.append(bufObj);
    gltf["buffers"] = buffers;

    QJsonDocument doc(gltf);

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit conversionError("Cannot write GLB file: " + outputPath);
        return false;
    }

    QByteArray jsonChunk = doc.toJson(QJsonDocument::Compact);
    while (jsonChunk.size() % 4 != 0) {
        jsonChunk.append(' ');
    }

    quint32 totalLength = 12 + 8 + jsonChunk.size() + 8 + binBuffer.size();

    file.write("glTF", 4);
    quint32 version = 2;
    file.write(reinterpret_cast<const char*>(&version), 4);
    file.write(reinterpret_cast<const char*>(&totalLength), 4);

    quint32 jsonChunkLen = jsonChunk.size();
    quint32 jsonType = 0x4E4F534A;
    file.write(reinterpret_cast<const char*>(&jsonChunkLen), 4);
    file.write(reinterpret_cast<const char*>(&jsonType), 4);
    file.write(jsonChunk);

    quint32 binChunkLen = binBuffer.size();
    quint32 binType = 0x004E4942;
    file.write(reinterpret_cast<const char*>(&binChunkLen), 4);
    file.write(reinterpret_cast<const char*>(&binType), 4);
    file.write(binBuffer);

    file.close();

    emit progressChanged(1.0f);
    return true;
}

bool FormatConverter::convertGLBToKN5(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    emit progressChanged(0.1f);

    GLBParser glbParser;
    if (!glbParser.loadFromFile(inputPath.toStdString())) {
        emit conversionError("Failed to parse GLB file: " + inputPath);
        return false;
    }

    emit progressChanged(0.3f);

    const GLBScene& glbScene = glbParser.scene();

    KN5File kn5File;
    kn5File.header.magic = KN5_MAGIC;
    kn5File.header.version = KN5_VERSION;

    int matIdx = 0;
    for (const auto& glbMat : glbScene.materials) {
        Material mat;
        mat.id = matIdx++;
        mat.name = QString::fromStdString(glbMat.name);
        mat.shaderName = "ksPerPixelMultiMap";
        mat.properties["diffuse_r"] = QString::number(glbMat.baseColorFactor.x);
        mat.properties["diffuse_g"] = QString::number(glbMat.baseColorFactor.y);
        mat.properties["diffuse_b"] = QString::number(glbMat.baseColorFactor.z);
        kn5File.materials.push_back(mat);
    }

    emit progressChanged(0.5f);

    for (const auto& glbMesh : glbScene.meshes) {
        Mesh mesh;
        mesh.name = QString::fromStdString(glbMesh.name);
        mesh.castShadows = true;
        mesh.isVisible = true;

        for (const auto& prim : glbMesh.primitives) {
            auto posIt = prim.attributes.find("POSITION");
            if (posIt != prim.attributes.end()) {
                std::vector<Vec3> vertices = glbParser.getVertices(posIt->second);
                for (const auto& v : vertices) {
                    mesh.positions.push_back({v.x, v.y, v.z});
                }
            }

            auto normIt = prim.attributes.find("NORMAL");
            if (normIt != prim.attributes.end()) {
                std::vector<Vec3> normals = glbParser.getNormals(normIt->second);
                for (const auto& n : normals) {
                    mesh.normals.push_back({n.x, n.y, n.z});
                }
            }

            auto uvIt = prim.attributes.find("TEXCOORD_0");
            if (uvIt != prim.attributes.end()) {
                std::vector<Vec2> texCoords = glbParser.getTexCoords(uvIt->second);
                for (const auto& uv : texCoords) {
                    mesh.uv0.push_back(QVector2D(uv.x, 1.0f - uv.y));
                }
            }

            if (prim.indices != 0xFFFFFFFF) {
                std::vector<uint32_t> indices = glbParser.getIndices(prim.indices);
                mesh.indexData.resize(indices.size() * sizeof(quint16));
                quint16* idxPtr = reinterpret_cast<quint16*>(mesh.indexData.data());
                for (size_t i = 0; i < indices.size(); i++) {
                    idxPtr[i] = static_cast<quint16>(indices[i]);
                }

                SubMesh submesh;
                submesh.materialIndex = (prim.material != 0xFFFFFFFF) ? prim.material : 0;
                submesh.vertexOffset = 0;
                submesh.vertexCount = mesh.positions.size();
                submesh.indexOffset = 0;
                submesh.indexCount = indices.size();
                mesh.subMeshes.push_back(submesh);
            }
        }

        kn5File.meshes.push_back(mesh);
    }

    emit progressChanged(0.8f);

    if (!KN5ParserImpl::write(outputPath, kn5File)) {
        emit conversionError("Failed to write KN5 file: " + outputPath);
        return false;
    }

    emit progressChanged(1.0f);
    return true;
}

// ============================================================================
// KN5 <-> FBX Conversion
// ============================================================================

bool FormatConverter::convertKN5ToFBX(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    emit progressChanged(0.1f);

    QString err;
    KN5File kn5File = KN5ParserImpl::parse(inputPath, &err);
    if (!kn5File.isValid()) {
        emit conversionError("Failed to read KN5 file: " + err);
        return false;
    }

    for (auto& mesh : kn5File.meshes) {
        mesh.decodeVertices();
    }

    emit progressChanged(0.3f);

    QFile fbxFile(outputPath);
    if (!fbxFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit conversionError("Cannot write FBX file: " + outputPath);
        return false;
    }
    QTextStream out(&fbxFile);
    out << "; FBX ASCII export from KN5\n";
    out << "FBXHeaderExtension: { FBXHeaderVersion: 1003; FBXVersion: 7400; }\n";
    out << "GlobalSettings: { Version: 1000; }\n";
    out << "Documents: { Document: { Name: \"" << QFileInfo(inputPath).baseName() << "\"; }\n";

    int nodeId = 0;
    for (int mi = 0; mi < kn5File.meshes.size(); ++mi) {
        auto& mesh = kn5File.meshes[mi];
        const auto& positions = mesh.positions;
        const auto& normals = mesh.normals;
        const auto& uv0 = mesh.uv0;
        const quint16* idx = reinterpret_cast<const quint16*>(mesh.indexData.constData());
        int nIdx = mesh.indexData.size() / 2;
        QString meshName = mesh.name.isEmpty() ? QString("Mesh_%1").arg(mi) : mesh.name;

        out << "Model: " << nodeId << ", \"" << meshName << "\" {\n";
        out << "  Version: 232; Count: " << positions.size() << ";\n";

        // Vertices
        out << "  Vertices: {";
        for (int v = 0; v < positions.size(); ++v) {
            if (v % 3 == 0 && v > 0) out << "\n    ";
            out << positions[v].x() << "," << positions[v].y() << "," << positions[v].z();
            if (v < positions.size() - 1) out << ",";
        }
        out << "}\n";

        // Normals
        out << "  LayerElementNormal: 0 { Version: 101; Name: \"\"; MappingInformationType: \"ByPolygonVertex\"; "
               "ReferenceInformationType: \"Direct\"; Normals: {";
        for (int pv = 0; pv < nIdx; ++pv) {
            if (pv % 3 == 0 && pv > 0) out << "\n      ";
            int vi = std::min<int>(idx[pv], static_cast<int>(normals.size()) - 1);
            out << normals[vi].x() << "," << normals[vi].y() << "," << normals[vi].z();
            if (pv < nIdx - 1) out << ",";
        }
        out << "}}\n";

        // UVs
        out << "  LayerElementUV: 0 { Version: 101; Name: \"UVMap\"; MappingInformationType: \"ByPolygonVertex\"; "
               "ReferenceInformationType: \"Direct\"; UV: {";
        for (int pv = 0; pv < nIdx; ++pv) {
            if (pv % 6 == 0 && pv > 0) out << "\n      ";
            int vi = std::min<int>(idx[pv], static_cast<int>(uv0.size()) - 1);
            out << uv0[vi].x() << "," << uv0[vi].y();
            if (pv < nIdx - 1) out << ",";
        }
        out << "}}\n";

        // Faces (polygons) as triangles
        out << "  PolygonVertexIndex: {";
        for (int f = 0; f < nIdx; f += 3) {
            if (f % 12 == 0 && f > 0) out << "\n    ";
            out << idx[f] << "," << idx[f + 1] << "," << -(idx[f + 2] + 1);
            if (f + 3 < nIdx) out << ",";
        }
        out << "}\n";

        // Layers
        out << "  Layer: 0 { Version: 100; LayerElement: { TypedIndex: 0; } }\n";
        out << "}\n"; // end Model
        nodeId++;
    }

    out << "Connections: {}\n";
    fbxFile.close();

    emit progressChanged(1.0f);
    return true;
}

bool FormatConverter::convertFBXToKN5(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    emit progressChanged(0.1f);

    FBXParser fbxParser;
    if (!fbxParser.loadFromFile(inputPath.toStdString())) {
        emit conversionError("Failed to parse FBX file: " + inputPath);
        return false;
    }

    emit progressChanged(0.3f);

    const FBXScene& fbxScene = fbxParser.scene();

    KN5File kn5File;
    kn5File.header.magic = KN5_MAGIC;
    kn5File.header.version = KN5_VERSION;

    int matIdx = 0;
    for (const auto& fbxMatPair : fbxScene.materials) {
        const auto& fbxMat = fbxMatPair.second;
        Material mat;
        mat.id = matIdx++;
        mat.name = QString::fromStdString(fbxMatPair.first);
        mat.shaderName = "ksPerPixelMultiMap";
        kn5File.materials.push_back(mat);
    }

    emit progressChanged(0.6f);

    for (const auto& fbxMesh : fbxScene.meshes) {
        Mesh mesh;
        mesh.name = QString::fromStdString(fbxMesh.name);
        mesh.castShadows = true;

        for (const auto& v : fbxMesh.vertices) {
            mesh.positions.push_back({v.x, v.y, v.z});
        }
        for (const auto& n : fbxMesh.normals) {
            mesh.normals.push_back({n.x, n.y, n.z});
        }
        for (const auto& uv : fbxMesh.texCoords) {
            mesh.uv0.push_back(QVector2D(uv.x, 1.0f - uv.y));
        }

        if (!fbxMesh.indices.empty()) {
            mesh.indexData.resize(fbxMesh.indices.size() * sizeof(quint16));
            quint16* idxPtr = (quint16*)mesh.indexData.data();
            for (size_t i = 0; i < fbxMesh.indices.size(); ++i) {
                idxPtr[i] = (quint16)fbxMesh.indices[i];
            }
        }

        SubMesh submesh;
        submesh.materialIndex = fbxMesh.materialName.empty() ? 0 : matIdx - 1;
        submesh.vertexOffset = 0;
        submesh.vertexCount = fbxMesh.vertices.size();
        submesh.indexOffset = 0;
        submesh.indexCount = fbxMesh.indices.size();
        mesh.subMeshes.push_back(submesh);

        kn5File.meshes.push_back(mesh);
    }

    emit progressChanged(0.8f);

    if (!KN5ParserImpl::write(outputPath, kn5File)) {
        emit conversionError("Failed to write KN5 file: " + outputPath);
        return false;
    }

    emit progressChanged(1.0f);
    return true;
}

// ============================================================================
// Cross-format conversions (via KN5 as intermediate)
// ============================================================================

bool FormatConverter::convertFBXToGLB(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    QString tmpKN5 = outputPath + ".tmp.kn5";
    if (!convertFBXToKN5(inputPath, tmpKN5, options)) {
        return false;
    }
    if (!convertKN5ToGLB(tmpKN5, outputPath, options)) {
        QFile::remove(tmpKN5);
        return false;
    }
    QFile::remove(tmpKN5);
    return true;
}

bool FormatConverter::convertGLBToFBX(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    QString tmpKN5 = outputPath + ".tmp.kn5";
    if (!convertGLBToKN5(inputPath, tmpKN5, options)) {
        return false;
    }
    if (!convertKN5ToFBX(tmpKN5, outputPath, options)) {
        QFile::remove(tmpKN5);
        return false;
    }
    QFile::remove(tmpKN5);
    return true;
}

bool FormatConverter::convertFBXToOBJ(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    QString tmpKN5 = outputPath + ".tmp.kn5";
    if (!convertFBXToKN5(inputPath, tmpKN5, options)) {
        return false;
    }
    if (!convertKN5ToOBJ(tmpKN5, outputPath, options)) {
        QFile::remove(tmpKN5);
        return false;
    }
    QFile::remove(tmpKN5);
    return true;
}

bool FormatConverter::convertOBJToFBX(const QString& inputPath, const QString& outputPath, const ConversionOptions& options) {
    QString tmpKN5 = outputPath + ".tmp.kn5";
    if (!convertOBJToKN5(inputPath, tmpKN5, options)) {
        return false;
    }
    if (!convertKN5ToFBX(tmpKN5, outputPath, options)) {
        QFile::remove(tmpKN5);
        return false;
    }
    QFile::remove(tmpKN5);
    return true;
}

// ============================================================================
// BatchConverter
// ============================================================================

BatchConverter::BatchConverter(QObject* parent)
    : QObject(parent) {
}

BatchConverter::~BatchConverter() {
}

void BatchConverter::addFile(const QString& path) {
    m_queue.append(path);
}

void BatchConverter::addDirectory(const QString& path, bool recursive) {
    QDir dir(path);
    QStringList filters;
    filters << "*.kn5" << "*.fbx" << "*.glb" << "*.obj";

    if (recursive) {
        QDirIterator it(path, filters, QDir::Files | QDir::Dirs, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            if (it.fileInfo().isFile()) {
                m_queue.append(it.filePath());
            }
        }
    } else {
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
        for (const auto& info : files) {
            m_queue.append(info.filePath());
        }
    }
}

void BatchConverter::clearQueue() {
    m_queue.clear();
    m_successCount = 0;
    m_failureCount = 0;
}

void BatchConverter::setOutputFormat(const QString& format) {
    m_outputFormat = format;
}

void BatchConverter::setOutputDirectory(const QString& dir) {
    m_outputDir = dir;
}

void BatchConverter::setOptions(const ConversionOptions& options) {
    m_options = options;
}

void BatchConverter::startConversion() {
    m_cancelled = false;
    m_successCount = 0;
    m_failureCount = 0;

    for (int i = 0; i < m_queue.size() && !m_cancelled; ++i) {
        QString inputPath = m_queue[i];
        QFileInfo info(inputPath);
        QString baseName = info.baseName();
        QString outputPath = m_outputDir + "/" + baseName + "." + m_outputFormat;

        float progress = (float)i / m_queue.size();
        emit progressChanged(progress, inputPath);

        FormatConverter converter;
        bool success = converter.convert(inputPath, outputPath, m_options);

        if (success) {
            m_successCount++;
            emit fileConverted(inputPath, outputPath);
        } else {
            m_failureCount++;
            emit conversionError(inputPath, "Conversion failed");
        }
    }

    emit conversionComplete(m_successCount, m_failureCount);
}

void BatchConverter::cancelConversion() {
    m_cancelled = true;
}

} // namespace ks
