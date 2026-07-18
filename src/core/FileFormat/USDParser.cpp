#include "AdvancedFormats.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QDebug>
#include <QVector3D>
#include <QQuaternion>
#include <QRegularExpression>

namespace ks {
namespace fileformat {

// Simple USD ASCII (.usda) parser - for USD binary (.usd) would need USD library
bool USDParser::read(const QString& filePath, USDStage& stage, QString* error) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = "Cannot open file: " + filePath;
        return false;
    }

    QString content = file.readAll();
    file.close();

    // Parse USD ASCII format (simplified)
    // Real implementation would use USD C++ library
    QStringList lines = content.split('\n');
    
    QString currentPrimPath;
    USDPrim currentPrim;
    bool inPrim = false;
    bool inAttributes = false;
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) continue;
        
        // Def prim
        if (trimmed.startsWith("def ")) {
            if (inPrim && !currentPrim.path.empty()) {
                stage.prims.append(currentPrim);
            }
            
            // Parse: def "Type" "path" ( { ... }
            QRegularExpression defRx(R"rx(def\s+"(\w+)"\s+"([^"]+)"\s*\{?)rx");
            QRegularExpressionMatch defMatch = defRx.match(trimmed);
            if (defMatch.hasMatch()) {
                currentPrim = USDPrim();
                currentPrim.typeName = defMatch.captured(1).toStdString();
                currentPrim.path = defMatch.captured(2).toStdString();
                inPrim = true;
                inAttributes = true;
            }
        }
        // End of prim
        else if (trimmed == "}" && inPrim) {
            if (!currentPrim.path.empty()) {
                stage.prims.append(currentPrim);
            }
            inPrim = false;
            inAttributes = false;
        }
        // Attribute
        else if (inPrim && inAttributes && trimmed.contains("=")) {
            // Parse: type name = value
            QRegularExpression attrRx(R"((\w+)\s+(\w+)\s*=\s*(.+))");
            QRegularExpressionMatch attrMatch = attrRx.match(trimmed);
            if (attrMatch.hasMatch()) {
                QString attrType = attrMatch.captured(1);
                QString attrName = attrMatch.captured(2);
                QString attrValue = attrMatch.captured(3).trimmed();
                
                QVariant value;
                if (attrType == "float") value = attrValue.toFloat();
                else if (attrType == "double") value = attrValue.toDouble();
                else if (attrType == "int") value = attrValue.toInt();
                else if (attrType == "string") value = attrValue.mid(1, attrValue.length() - 2); // remove quotes
                else if (attrType == "float3") {
                    // Parse (x, y, z)
                    QRegularExpression vecRx(R"(\(([^,]+),\s*([^,]+),\s*([^)]+)\))");
                    QRegularExpressionMatch vecMatch = vecRx.match(attrValue);
                    if (vecMatch.hasMatch()) {
                        value = QVector3D(vecMatch.captured(1).toFloat(), vecMatch.captured(2).toFloat(), vecMatch.captured(3).toFloat());
                    }
                }
                
                if (value.isValid()) {
                    currentPrim.attributes[attrName.toStdString()] = value;
                }
            }
        }
    }
    
    if (inPrim && !currentPrim.path.empty()) {
        stage.prims.append(currentPrim);
    }

    // Extract meshes, materials from prims
    for (const auto& prim : stage.prims) {
        if (prim.typeName == "Mesh") {
            USDMesh mesh;
            mesh.primPath = prim.path;
            
            // Get points
            if (prim.attributes.contains("points")) {
                QVariant v = prim.attributes["points"];
                if (v.canConvert<QVector<QVector3D>>()) {
                    mesh.points = v.value<QVector<QVector3D>>();
                }
            }
            // Get normals, uvs, etc.
            
            stage.meshes.append(mesh);
        }
        else if (prim.typeName == "Material") {
            USDMaterial mat;
            mat.primPath = prim.path;
            
            // Parse PBR inputs
            if (prim.attributes.contains("inputs:diffuseColor")) {
                mat.diffuseColor = prim.attributes["inputs:diffuseColor"].value<QVector3D>();
            }
            if (prim.attributes.contains("inputs:metallic")) {
                mat.metallic = prim.attributes["inputs:metallic"].toFloat();
            }
            if (prim.attributes.contains("inputs:roughness")) {
                mat.roughness = prim.attributes["inputs:roughness"].toFloat();
            }
            
            stage.materials.append(mat);
        }
    }

    return true;
}

bool USDParser::write(const QString& filePath, const USDStage& stage, QString* error) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = "Cannot write file: " + filePath;
        return false;
    }

    QTextStream out(&file);
    out << "#usda 1.0\n";
    out << "(\n";
    out << "    defaultPrim = \"root\"\n";
    out << "    startTimeCode = " << stage.startTimeCode << "\n";
    out << "    endTimeCode = " << stage.endTimeCode << "\n";
    out << "    timeCodesPerSecond = " << stage.timeCodesPerSecond << "\n";
    out << ")\n\n";

    // Write prims
    for (const auto& prim : stage.prims) {
        out << "def \"" << QString::fromStdString(prim.typeName) << "\" \"" 
            << QString::fromStdString(prim.path) << "\" {\n";
        
        for (auto it = prim.attributes.constBegin(); it != prim.attributes.constEnd(); ++it) {
            QString key = QString::fromStdString(it.key());
            QVariant value = it.value();
            
            if (value.typeId() == QMetaType::Float) {
                out << "    float " << key << " = " << value.toFloat() << "\n";
            } else if (value.typeId() == QMetaType::Double) {
                out << "    double " << key << " = " << value.toDouble() << "\n";
            } else if (value.typeId() == QMetaType::Int) {
                out << "    int " << key << " = " << value.toInt() << "\n";
            } else if (value.typeId() == QMetaType::QString) {
                out << "    string " << key << " = \"" << value.toString() << "\"\n";
            } else if (value.typeId() == QMetaType::QVector3D) {
                QVector3D v = value.value<QVector3D>();
                out << "    float3 " << key << " = (" << v.x() << ", " << v.y() << ", " << v.z() << ")\n";
            }
        }
        out << "}\n\n";
    }

    file.close();
    return true;
}

bool USDParser::usdMeshToMeshData(const USDMesh& mesh, MeshData& outData) {
    outData.vertices.reserve(mesh.points.size());
    
    for (int i = 0; i < mesh.points.size(); ++i) {
        MeshVertex v{};
        v.px = mesh.points[i].x();
        v.py = mesh.points[i].y();
        v.pz = mesh.points[i].z();
        if (i < mesh.normals.size()) {
            v.nx = mesh.normals[i].x();
            v.ny = mesh.normals[i].y();
            v.nz = mesh.normals[i].z();
        }
        if (i < mesh.uvs.size()) {
            v.u0 = mesh.uvs[i].x();
            v.v0 = mesh.uvs[i].y();
        }
        outData.vertices.append(v);
    }
    
    // Build index list from faceVertexCounts and faceVertexIndices
    for (int idx : mesh.faceVertexIndices) {
        outData.indices.append(static_cast<uint32_t>(idx));
    }
    
    // Build submeshes from faceMaterialIndices
    int indexOffset = 0;
    for (int i = 0; i < mesh.faceVertexCounts.size(); ++i) {
        int count = mesh.faceVertexCounts[i];
        if (i < mesh.faceMaterialIndices.size()) {
            MeshSubmesh sub;
            sub.indexOffset = indexOffset;
            sub.indexCount = count;
            outData.submeshes.append(sub);
        }
        indexOffset += count;
    }
    
    return true;
}

bool USDParser::meshDataToUsdMesh(const MeshData& data, USDMesh& outMesh) {
    outMesh.points.reserve(data.vertices.size());
    outMesh.normals.reserve(data.vertices.size());
    outMesh.uvs.reserve(data.vertices.size());
    
    for (const auto& v : data.vertices) {
        outMesh.points.append(QVector3D(v.px, v.py, v.pz));
        outMesh.normals.append(QVector3D(v.nx, v.ny, v.nz));
        outMesh.uvs.append(QVector2D(v.u0, v.v0));
    }
    
    // Build face data from submeshes
    for (const auto& sub : data.submeshes) {
        outMesh.faceVertexCounts.append(sub.indexCount);
        for (uint32_t i = 0; i < sub.indexCount; ++i) {
            uint32_t idx = sub.indexOffset + i;
            if (idx < data.indices.size()) {
                outMesh.faceVertexIndices.append(data.indices[idx]);
            }
        }
        outMesh.faceMaterialIndices.append(outMesh.faceVertexCounts.size() - 1);
    }
    
    // If no submeshes, treat all indices as one face group
    if (data.submeshes.isEmpty() && !data.indices.isEmpty()) {
        outMesh.faceVertexCounts.append(data.indices.size());
        for (int idx : data.indices) {
            outMesh.faceVertexIndices.append(idx);
        }
    }
    
    return true;
}

bool USDParser::usdMaterialToPBR(const USDMaterial& mat, MaterialData& outMat) {
    outMat.name = QString::fromStdString(mat.primPath);
    outMat.baseColorFactor = QVector4D(mat.diffuseColor.x(), mat.diffuseColor.y(), mat.diffuseColor.z(), 1.0f);
    outMat.metallic = mat.metallic;
    outMat.roughness = mat.roughness;
    outMat.emissiveFactor = QVector3D(0, 0, 0);
    
    outMat.baseColorTexture = QString::fromStdString(mat.diffuseTexture);
    outMat.normalTexture = QString::fromStdString(mat.normalTexture);
    outMat.metallicRoughnessTexture = QString::fromStdString(mat.metallicRoughnessTexture);
    outMat.emissiveTexture = QString::fromStdString(mat.emissiveTexture);
    
    return true;
}

bool USDParser::pbrToUsdMaterial(const MaterialData& mat, USDMaterial& outMat) {
    outMat.diffuseColor = {mat.baseColorFactor.x(), mat.baseColorFactor.y(), mat.baseColorFactor.z()};
    outMat.metallic = mat.metallic;
    outMat.roughness = mat.roughness;
    
    outMat.diffuseTexture = mat.baseColorTexture.toStdString();
    outMat.normalTexture = mat.normalTexture.toStdString();
    outMat.metallicRoughnessTexture = mat.metallicRoughnessTexture.toStdString();
    outMat.emissiveTexture = mat.emissiveTexture.toStdString();
    
    return true;
}

QVariant USDParser::sampleAnimation(const USDAnimation& anim, double time) {
    if (anim.timeSamples.isEmpty()) return QVariant();
    
    // Find surrounding keyframes
    int i = 0;
    while (i < anim.timeSamples.size() && anim.timeSamples[i] < time) {
        ++i;
    }
    
    if (i == 0) return anim.values[0];
    if (i >= anim.timeSamples.size()) return anim.values.last();
    
    double t0 = anim.timeSamples[i - 1];
    double t1 = anim.timeSamples[i];
    double t = (time - t0) / (t1 - t0);
    
    QVariant v0 = anim.values[i - 1];
    QVariant v1 = anim.values[i];
    
    // Linear interpolation for compatible types
    if (v0.typeId() == QMetaType::Float && v1.typeId() == QMetaType::Float) {
        return v0.toFloat() * (1 - t) + v1.toFloat() * t;
    } else if (v0.typeId() == QMetaType::QVector3D && v1.typeId() == QMetaType::QVector3D) {
        QVector3D a = v0.value<QVector3D>();
        QVector3D b = v1.value<QVector3D>();
        return QVector3D(a.x() * (1 - t) + b.x() * t,
                         a.y() * (1 - t) + b.y() * t,
                         a.z() * (1 - t) + b.z() * t);
    } else if (v0.typeId() == QMetaType::QQuaternion && v1.typeId() == QMetaType::QQuaternion) {
        return QQuaternion::slerp(v0.value<QQuaternion>(), v1.value<QQuaternion>(), static_cast<float>(t));
    }
    
    return v0;
}

QVector<std::string> USDParser::findPrimsByType(const USDStage& stage, const std::string& typeName) {
    QVector<std::string> result;
    for (const auto& prim : stage.prims) {
        if (prim.typeName == typeName) {
            result.append(prim.path);
        }
    }
    return result;
}

} // namespace fileformat
} // namespace ks