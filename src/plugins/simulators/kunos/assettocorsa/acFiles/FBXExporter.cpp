#include "FBXExporter.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QDateTime>

namespace ks {

// ============================================================
// Helpers
// ============================================================
static QByteArray fbxFloat(float v) {
    return QString::number(double(v), 'f', 8).toUtf8();
}
static QByteArray fbxDouble(double v) {
    return QString::number(v, 'f', 8).toUtf8();
}

// Write a FBX LayerElement block for UVs — supports UV0 and UV1 (UV2)
static QByteArray uvLayerBlock(const QVector<QVector2D>& uvs,
                                const QVector<Face>& faces,
                                int channelIndex,
                                const QString& channelName)
{
    QByteArray b;
    b += "      LayerElementUV: " + QByteArray::number(channelIndex) + " {\n";
    b += "        Version: 101\n";
    b += "        Name: \"" + channelName.toUtf8() + "\"\n";
    b += "        MappingInformationType: \"ByPolygonVertex\"\n";
    b += "        ReferenceInformationType: \"IndexToDirect\"\n";

    // UV array (direct)
    b += "        UV: *" + QByteArray::number(uvs.size() * 2) + " {\n          a: ";
    for (int i = 0; i < uvs.size(); ++i) {
        b += fbxFloat(uvs[i].x()) + "," + fbxFloat(uvs[i].y());
        if (i < uvs.size()-1) b += ",";
    }
    b += "\n        }\n";

    // UV index array (one per polygon vertex)
    QVector<int> uvIdx;
    for (const Face& face : faces)
        for (int idx : face.uvIndices.isEmpty() ? face.indices : face.uvIndices)
            uvIdx.append(idx);

    b += "        UVIndex: *" + QByteArray::number(uvIdx.size()) + " {\n          a: ";
    for (int i = 0; i < uvIdx.size(); ++i) {
        b += QByteArray::number(uvIdx[i]);
        if (i < uvIdx.size()-1) b += ",";
    }
    b += "\n        }\n";
    b += "      }\n";
    return b;
}

// Write a LayerElement for Normals or Tangents
static QByteArray vectorLayerBlock(const QVector<QVector3D>& vecs,
                                    const QString& elementType,
                                    const QString& channelName,
                                    float scale = 1.0f)
{
    if (vecs.isEmpty()) return {};
    QByteArray b;
    b += "      LayerElement" + elementType.toUtf8() + ": 0 {\n";
    b += "        Version: 102\n";
    b += "        Name: \"" + channelName.toUtf8() + "\"\n";
    b += "        MappingInformationType: \"ByPolygonVertex\"\n";
    b += "        ReferenceInformationType: \"Direct\"\n";
    b += "        " + elementType.toUtf8() + ": *" + QByteArray::number(vecs.size()*3) + " {\n";
    b += "          a: ";
    for (int i = 0; i < vecs.size(); ++i) {
        b += fbxFloat(vecs[i].x()*scale) + ","
           + fbxFloat(vecs[i].y()*scale) + ","
           + fbxFloat(vecs[i].z()*scale);
        if (i < vecs.size()-1) b += ",";
    }
    b += "\n        }\n      }\n";
    return b;
}

// ============================================================
// Public API
// ============================================================
bool FBXExporter::exportToFBX(const QString& path,
                               const MeshData& mesh,
                               const FBXExportSettings& settings)
{
    return exportToFBX(path, QVector<MeshData>{mesh}, settings);
}

bool FBXExporter::exportToFBX(const QString& path,
                               const QVector<MeshData>& meshes,
                               const FBXExportSettings& settings)
{
    QString err = validateExportSettings(settings);
    if (!err.isEmpty()) { qWarning() << "FBXExporter:" << err; return false; }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "FBXExporter: Cannot open for writing:" << path;
        return false;
    }

    QByteArray data;
    data += generateFBXHeader("ksEditor", "7.4");

    for (int i = 0; i < meshes.size(); ++i) {
        QString name = meshes[i].name.isEmpty()
                     ? "Mesh_" + QString::number(i)
                     : meshes[i].name;
        data += generateMeshNode(name, meshes[i], settings);
        if (settings.exportMaterials && !meshes[i].materialName.isEmpty()) {
            data += generateMaterialNode(meshes[i].materialName,
                                         meshes[i].diffuseColor,
                                         meshes[i].metallic,
                                         meshes[i].roughness);
        }
    }

    // Close Objects block
    data += "}\n\n";

    // Connections block
    data += "Connections: {\n";
    for (int i = 0; i < meshes.size(); ++i) {
        qint64 meshId = 1000 + i;
        data += "  C: \"OO\"," + QByteArray::number(meshId) + ",0\n";
    }
    data += "}\n";

    file.write(data);
    file.close();
    qInfo() << "FBXExporter: Exported" << meshes.size() << "meshes to" << path;
    return true;
}

FBXExportSettings FBXExporter::getDefaultExportSettings() {
    FBXExportSettings s;
    s.exportMaterials  = true;
    s.exportTextures   = true;
    s.exportAnimations = false;
    s.exportSkinning   = false;
    s.embedTextures    = false;
    s.binaryFormat     = false;
    s.scale            = 1.0f;
    s.axisForward      = 2;
    s.axisUp           = 1;
    s.exportUV2        = true;
    s.exportTangents   = true;
    s.exportNormals    = true;
    return s;
}

QString FBXExporter::validateExportSettings(const FBXExportSettings& s) {
    if (s.scale <= 0) return "Scale must be positive";
    return {};
}

QByteArray FBXExporter::generateFBXHeader(const QString& exporter,
                                           const QString& /*version*/)
{
    QByteArray h;
    h += "; FBX 7.4.0 project file\n";
    h += "; Created by " + exporter.toUtf8() + "\n";
    h += "; " + QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8() + "\n\n";
    h += "FBXHeaderExtension:  {\n";
    h += "  FBXVersion: 7400\n";
    h += "  Creator: \"" + exporter.toUtf8() + "\"\n";
    h += "}\n";
    h += "GlobalSettings:  {\n  Version: 1000\n";
    h += "  Properties70:  {\n";
    h += "    P: \"UpAxis\", \"int\", \"\", \"\",1\n";
    h += "    P: \"FrontAxis\", \"int\", \"\", \"\",2\n";
    h += "    P: \"CoordAxis\", \"int\", \"\", \"\",0\n";
    // Scale factor: detect cm vs m from typical AC KN5 export (1 unit = 1m)
    h += "    P: \"UnitScaleFactor\", \"double\", \"\", \"\",1\n";
    h += "    P: \"OriginalUnitScaleFactor\", \"double\", \"\", \"\",1\n";
    h += "  }\n}\n";
    h += "Documents:  {\n  Document: 1, \"\", \"\" {\n  }\n}\n";
    h += "Definitions:  {\n  Version: 100\n  Count: 2\n";
    h += "  ObjectType: \"Model\" {\n    Count: 1\n  }\n";
    h += "  ObjectType: \"Geometry\" {\n    Count: 1\n  }\n";
    h += "}\n";
    h += "Objects:  {\n";
    return h;
}

QByteArray FBXExporter::generateMeshNode(const QString& name,
                                          const MeshData& mesh,
                                          const FBXExportSettings& settings)
{
    QByteArray node;
    qint64 geoId  = 2000 + qHash(name) % 9000;
    qint64 modelId = 1000 + qHash(name) % 9000;

    // --- Geometry node ---
    node += "  Geometry: " + QByteArray::number(geoId);
    node += ", \"Geometry::" + name.toUtf8() + "\", \"Mesh\" {\n";

    // Vertices
    node += "    Vertices: *" + QByteArray::number(mesh.vertices.size()*3) + " {\n      a: ";
    for (int i = 0; i < mesh.vertices.size(); ++i) {
        const auto& v = mesh.vertices[i].position;
        node += fbxDouble(v.x()*settings.scale) + ","
              + fbxDouble(v.y()*settings.scale) + ","
              + fbxDouble(v.z()*settings.scale);
        if (i < mesh.vertices.size()-1) node += ",";
    }
    node += "\n    }\n";

    // PolygonVertexIndex (FBX: negate last index of each polygon)
    QVector<int> polyIdx;
    for (const Face& face : mesh.faces) {
        int n = face.indices.size();
        for (int j = 0; j < n; ++j) {
            int idx = face.indices[j];
            polyIdx.append(j == n-1 ? -(idx+1) : idx);
        }
    }
    node += "    PolygonVertexIndex: *" + QByteArray::number(polyIdx.size()) + " {\n      a: ";
    for (int i = 0; i < polyIdx.size(); ++i) {
        node += QByteArray::number(polyIdx[i]);
        if (i < polyIdx.size()-1) node += ",";
    }
    node += "\n    }\n";

    // --- LayerElements ---

    // Normals (ByPolygonVertex, Direct)
    if (settings.exportNormals && !mesh.normals.isEmpty()) {
        node += vectorLayerBlock(mesh.normals, "Normal", "");
    }

    // Tangents (needed for ksPerPixelNM and similar AC shaders)
    if (settings.exportTangents && !mesh.tangents.isEmpty()) {
        node += vectorLayerBlock(mesh.tangents, "Tangent", "");
    }
    if (settings.exportTangents && !mesh.bitangents.isEmpty()) {
        node += vectorLayerBlock(mesh.bitangents, "Binormal", "");
    }

    // UV channel 0 (primary)
    if (!mesh.uvs.isEmpty()) {
        node += uvLayerBlock(mesh.uvs, mesh.faces, 0, "UVChannel_1");
    }

    // UV channel 1 — UV2 (damage maps, lightmaps — critical for ksPerPixelMultiMap)
    if (settings.exportUV2 && !mesh.uv2s.isEmpty()) {
        node += uvLayerBlock(mesh.uv2s, mesh.faces, 1, "UVChannel_2");
    }

    // Layer stack — declares which LayerElements belong to layer 0 and 1
    node += "    Layer: 0 {\n      Version: 100\n";
    node += "      LayerElement: { Type: \"LayerElementNormal\", TypedIndex: 0 }\n";
    if (settings.exportTangents && !mesh.tangents.isEmpty()) {
        node += "      LayerElement: { Type: \"LayerElementTangent\", TypedIndex: 0 }\n";
        node += "      LayerElement: { Type: \"LayerElementBinormal\", TypedIndex: 0 }\n";
    }
    if (!mesh.uvs.isEmpty())
        node += "      LayerElement: { Type: \"LayerElementUV\", TypedIndex: 0 }\n";
    node += "    }\n";

    if (settings.exportUV2 && !mesh.uv2s.isEmpty()) {
        node += "    Layer: 1 {\n      Version: 100\n";
        node += "      LayerElement: { Type: \"LayerElementUV\", TypedIndex: 1 }\n";
        node += "    }\n";
    }

    node += "  }\n";

    // --- Model node ---
    node += "  Model: " + QByteArray::number(modelId);
    node += ", \"Model::" + name.toUtf8() + "\", \"Mesh\" {\n";
    node += "    Version: 232\n";
    node += "    Properties70:  {\n";
    node += "      P: \"RotationActive\", \"bool\", \"\", \"\",1\n";
    node += "      P: \"InheritType\", \"enum\", \"\", \"\",1\n";
    node += "    }\n";
    node += "    Shading: T\n";
    node += "    Culling: \"CullingOff\"\n";
    node += "  }\n";

    return node;
}

QByteArray FBXExporter::generateMaterialNode(const QString& name,
                                              const QVector4D& color,
                                              float metallic,
                                              float roughness)
{
    QByteArray node;
    node += "  Material: \"Material::" + name.toUtf8() + "\" {\n";
    node += "    Version: 102\n";
    node += "    ShadingModel: \"phong\"\n";
    node += "    MultiLayer: 0\n";
    node += "    Properties70:  {\n";
    node += QByteArray("      P: \"DiffuseColor\", \"Color\", \"\", \"\",")
          + fbxFloat(color.x()) + "," + fbxFloat(color.y()) + "," + fbxFloat(color.z()) + "\n";
    float spec = 1.0f - roughness;
    node += QByteArray("      P: \"SpecularColor\", \"Color\", \"\", \"\",")
          + fbxFloat(spec) + "," + fbxFloat(spec) + "," + fbxFloat(spec) + "\n";
    node += QByteArray("      P: \"Shininess\", \"double\", \"\", \"\",")
          + fbxDouble(metallic * 100.0) + "\n";
    node += "    }\n  }\n";
    return node;
}

// ============================================================
// FBX Importer — parse ASCII FBX with UV2 + tangent support
// ============================================================
bool FBXImporter::importFromFBX(const QString& path,
                                 MeshData& mesh,
                                 const FBXImportSettings& settings)
{
    QVector<MeshData> meshes;
    bool ok = importFromFBX(path, meshes, settings);
    if (ok && !meshes.isEmpty()) mesh = meshes.first();
    return ok;
}

bool FBXImporter::importFromFBX(const QString& path,
                                 QVector<MeshData>& meshes,
                                 const FBXImportSettings& settings)
{
    QString err = validateImportSettings(settings);
    if (!err.isEmpty()) { qWarning() << "FBXImporter:" << err; return false; }

    QByteArray data = readFileBinary(path);
    if (data.isEmpty()) { qWarning() << "FBXImporter: Cannot read:" << path; return false; }

    QString version;
    if (!parseFBXHeader(data, version)) {
        qWarning() << "FBXImporter: Invalid FBX file";
        return false;
    }

    if (version == "Binary") {
        qWarning() << "FBXImporter: Binary FBX — only ASCII supported in this build";
        return false;
    }

    // ASCII FBX parser — extract Geometry blocks
    QString text = QString::fromUtf8(data);
    QStringList lines = text.split('\n');

    MeshData current;
    bool inGeometry = false;
    bool inVertices = false;
    bool inPolyIdx  = false;
    bool inNormals  = false;
    bool inUV       = false;
    bool inUVIndex  = false;
    int  uvChannel  = 0;
    bool inTangents = false;
    QString accumLine;

    auto parseFloats = [](const QString& s) {
        QVector<float> out;
        const QStringList parts = s.split(',', Qt::SkipEmptyParts);
        for (const QString& p : parts) {
            bool ok; float v = p.trimmed().toFloat(&ok);
            if (ok) out.append(v);
        }
        return out;
    };

    auto parseInts = [](const QString& s) {
        QVector<int> out;
        const QStringList parts = s.split(',', Qt::SkipEmptyParts);
        for (const QString& p : parts) {
            bool ok; int v = p.trimmed().toInt(&ok);
            if (ok) out.append(v);
        }
        return out;
    };

    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();

        if (line.startsWith("Geometry:") && line.contains("\"Mesh\"")) {
            if (inGeometry && !current.vertices.isEmpty()) {
                meshes.append(current);
            }
            current = MeshData();
            inGeometry = true;
            // Extract name
            int q1 = line.indexOf('"'), q2 = line.indexOf('"', q1+1);
            if (q1 >= 0 && q2 > q1) {
                QString n = line.mid(q1+1, q2-q1-1);
                if (n.startsWith("Geometry::")) n = n.mid(10);
                current.name = n;
            }
            continue;
        }

        if (!inGeometry) continue;
        if (line == "}") {
            // End of a layer or block
            inVertices = inPolyIdx = inNormals = inUV = inUVIndex = inTangents = false;
            continue;
        }

        if (line.startsWith("Vertices:"))     { inVertices = true; accumLine = line.section("a:",1); continue; }
        if (line.startsWith("PolygonVertexIndex:")) { inPolyIdx = true; accumLine = line.section("a:",1); continue; }
        if (line.contains("LayerElementNormal")) { inNormals = true; continue; }
        if (line.contains("LayerElementUV:"))  {
            bool ok;
            uvChannel = line.split(':').last().trimmed().toInt(&ok);
            if (!ok) uvChannel = 0;
            continue;
        }
        if (line.startsWith("UV:") && inGeometry) { inUV = true; accumLine = line.section("a:",1); continue; }
        if (line.startsWith("UVIndex:"))    { inUVIndex = true; accumLine = line.section("a:",1); continue; }
        if (line.contains("LayerElementTangent")) { inTangents = true; continue; }

        // Data lines inside arrays
        if (inVertices) {
            accumLine += line.replace("}", "");
            if (!line.contains("}")) continue;
            auto floats = parseFloats(accumLine);
            float scale = settings.scale;
            for (int i = 0; i+2 < floats.size(); i+=3) {
                Vertex v;
                v.position = QVector3D(floats[i]*scale,
                                        floats[i+1]*(settings.flipY?-scale:scale),
                                        floats[i+2]*scale);
                current.vertices.append(v);
            }
            inVertices = false; continue;
        }

        if (inPolyIdx) {
            accumLine += line.replace("}", "");
            if (!line.contains("}")) continue;
            auto ints = parseInts(accumLine);
            Face face; face.indices.reserve(4);
            for (int idx : ints) {
                if (idx < 0) {
                    face.indices.append(-(idx+1));
                    current.faces.append(face);
                    face.indices.clear();
                } else {
                    face.indices.append(idx);
                }
            }
            inPolyIdx = false; continue;
        }

        if (inUV) {
            accumLine += line.replace("}", "");
            if (!line.contains("}")) continue;
            auto floats = parseFloats(accumLine);
            QVector<QVector2D>& uvDst = (uvChannel == 1) ? current.uv2s : current.uvs;
            for (int i = 0; i+1 < floats.size(); i+=2)
                uvDst.append({floats[i], floats[i+1]});
            inUV = false; continue;
        }

        if (inNormals && line.startsWith("a:")) {
            auto floats = parseFloats(line.mid(2));
            for (int i = 0; i+2 < floats.size(); i+=3)
                current.normals.append(QVector3D(floats[i], floats[i+1], floats[i+2]));
            inNormals = false; continue;
        }

        if (inTangents && line.startsWith("a:")) {
            auto floats = parseFloats(line.mid(2));
            for (int i = 0; i+2 < floats.size(); i+=3)
                current.tangents.append(QVector3D(floats[i], floats[i+1], floats[i+2]));
            inTangents = false; continue;
        }
    }

    if (inGeometry && !current.vertices.isEmpty())
        meshes.append(current);

    qInfo() << "FBXImporter: Parsed" << meshes.size() << "meshes from" << path;
    return !meshes.isEmpty();
}

QString FBXImporter::validateImportSettings(const FBXImportSettings& s) {
    if (s.scale <= 0) return "Scale must be positive";
    return {};
}

FBXImportSettings FBXImporter::getDefaultImportSettings() {
    FBXImportSettings s;
    s.importMaterials = true; s.importTextures = true;
    s.importAnimations = true; s.importSkinning = true;
    s.flipY = false; s.convertToYUp = true; s.scale = 1.0f;
    return s;
}

bool FBXImporter::parseFBXHeader(const QByteArray& data, QString& version) {
    if (data.startsWith("; FBX"))                         { version = "ASCII";  return true; }
    if (data.size() >= 21 && data.mid(0,21) == "Kaydara FBX Binary  ") { version = "Binary"; return true; }
    return false;
}

bool FBXImporter::parseMeshNode(const FBXNode& node, MeshData& mesh) {
    if (node.name == "Vertices" || node.name == "VerticesNormals") {
        QByteArray rawData = node.data;
        int floatCount = rawData.size() / sizeof(float);
        const float* floats = reinterpret_cast<const float*>(rawData.constData());

        if (node.name == "Vertices") {
            for (int i = 0; i + 2 < floatCount; i += 3) {
                Vertex v;
                v.position = QVector3D(floats[i], floats[i + 1], floats[i + 2]);
                mesh.vertices.append(v);
            }
        } else {
            for (int i = 0; i + 2 < floatCount && (i / 3) < mesh.vertices.size(); i += 3) {
                mesh.vertices[i / 3].normal = QVector3D(floats[i], floats[i + 1], floats[i + 2]).normalized();
            }
        }
        return true;
    }

    if (node.name == "PolygonVertexIndex") {
        QByteArray rawData = node.data;
        int intCount = rawData.size() / sizeof(int);
        const int* ints = reinterpret_cast<const int*>(rawData.constData());

        Face face;
        for (int i = 0; i < intCount; ++i) {
            int idx = ints[i];
            if (idx < 0) {
                face.indices.append(-(idx + 1));
                if (face.indices.size() >= 3) {
                    mesh.faces.append(face);
                    face.indices.clear();
                }
            } else {
                face.indices.append(idx);
            }
        }
        return true;
    }

    if (node.name == "UV" || node.name == "UVIndex") {
        return true;
    }

    return true;
}

QVector4D FBXImporter::parseMaterialColor(const FBXNode& node) {
    QVector4D color(0.8f, 0.8f, 0.8f, 1.0f);

    for (const FBXNode& child : node.children) {
        if (child.name == "Color") {
            QByteArray rawData = child.data;
            if (rawData.size() >= 16) {
                const float* floats = reinterpret_cast<const float*>(rawData.constData());
                color = QVector4D(floats[0], floats[1], floats[2],
                                  rawData.size() >= 20 ? floats[3] : 1.0f);
            }
        } else if (child.name == "P" && child.properties.size() >= 2) {
            QString propName = child.properties[0];
            if (propName.contains("Color", Qt::CaseInsensitive) ||
                propName.contains("Diffuse", Qt::CaseInsensitive) ||
                propName.contains("Ambient", Qt::CaseInsensitive)) {
                QByteArray rawData = child.data;
                if (rawData.size() >= 16) {
                    const float* floats = reinterpret_cast<const float*>(rawData.constData());
                    color = QVector4D(floats[0], floats[1], floats[2],
                                      rawData.size() >= 20 ? floats[3] : 1.0f);
                }
            }
        }
    }

    return color;
}

QByteArray FBXImporter::readFileBinary(const QString& path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

FBXNode FBXImporter::parseNode(const QByteArray& data, int& offset)
{
    FBXNode node;
    if (offset + 13 > data.size()) return node;

    // Read node header: endOffset (4), numProperties (4), propertyListLen (4), nameLen (1)
    quint32 endOffset = *reinterpret_cast<const quint32*>(data.constData() + offset);
    quint32 numProperties = *reinterpret_cast<const quint32*>(data.constData() + offset + 4);
    quint32 propertyListLen = *reinterpret_cast<const quint32*>(data.constData() + offset + 8);
    quint8 nameLen = static_cast<quint8>(data[offset + 12]);

    if (endOffset == 0 || endOffset > static_cast<quint32>(data.size())) return node;
    if (offset + 13 + nameLen > data.size()) return node;

    // Read node name
    node.name = QString::fromLatin1(data.mid(offset + 13, nameLen));
    int pos = offset + 13 + nameLen;

    // Read properties
    for (quint32 i = 0; i < numProperties && pos < endOffset; ++i) {
        if (pos + 1 > data.size()) break;
        char typeChar = data[pos++];

        switch (typeChar) {
        case 'Y': { // 16-bit int
            if (pos + 2 > data.size()) break;
            qint16 val = *reinterpret_cast<const qint16*>(data.constData() + pos);
            node.properties[QString::number(i)] = QString::number(val);
            pos += 2;
            break;
        }
        case 'C': { // boolean (1 byte)
            if (pos + 1 > data.size()) break;
            bool val = data[pos] != 0;
            node.properties[QString::number(i)] = val ? "1" : "0";
            pos += 1;
            break;
        }
        case 'I': { // 32-bit int
            if (pos + 4 > data.size()) break;
            qint32 val = *reinterpret_cast<const qint32*>(data.constData() + pos);
            node.properties[QString::number(i)] = QString::number(val);
            pos += 4;
            break;
        }
        case 'F': { // 32-bit float
            if (pos + 4 > data.size()) break;
            float val = *reinterpret_cast<const float*>(data.constData() + pos);
            node.properties[QString::number(i)] = QString::number(double(val), 'f', 6);
            pos += 4;
            break;
        }
        case 'D': { // 64-bit double
            if (pos + 8 > data.size()) break;
            double val = *reinterpret_cast<const double*>(data.constData() + pos);
            node.properties[QString::number(i)] = QString::number(val, 'f', 12);
            pos += 8;
            break;
        }
        case 'L': { // 64-bit int
            if (pos + 8 > data.size()) break;
            qint64 val = *reinterpret_cast<const qint64*>(data.constData() + pos);
            node.properties[QString::number(i)] = QString::number(val);
            pos += 8;
            break;
        }
        case 'R': { // raw binary data
            if (pos + 4 > data.size()) break;
            quint32 len = *reinterpret_cast<const quint32*>(data.constData() + pos);
            pos += 4;
            if (pos + static_cast<int>(len) > data.size()) break;
            node.properties[QString::number(i)] = QString::fromLatin1(data.mid(pos, len));
            pos += len;
            break;
        }
        case 'S': { // string
            if (pos + 4 > data.size()) break;
            quint32 len = *reinterpret_cast<const quint32*>(data.constData() + pos);
            pos += 4;
            if (pos + static_cast<int>(len) > data.size()) break;
            node.properties[QString::number(i)] = QString::fromUtf8(data.mid(pos, len));
            pos += len;
            break;
        }
        default:
            // Unknown property type, skip to end
            pos = endOffset;
            break;
        }
    }

    // Read children (if this node has children)
    if (pos < endOffset && endOffset - pos > 13) {
        while (pos < endOffset - 13) {
            int childEnd = *reinterpret_cast<const quint32*>(data.constData() + pos);
            if (childEnd == 0 || childEnd > static_cast<quint32>(data.size()) || childEnd <= pos) break;
            FBXNode child = parseNode(data, pos);
            if (!child.name.isEmpty())
                node.children.append(child);
            pos = childEnd;
        }
    }

    offset = endOffset;
    return node;
}

} // namespace ks
