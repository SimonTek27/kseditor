#include "KsConverters.h"

#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QList>
#include <cmath>

namespace ks {

// ---------------------------------------------------------------------------
// Type definitions (mirrors of the corresponding types in assettocorsa.h)
// ---------------------------------------------------------------------------

struct KsMeshVertex {
    float position[3];
    float normal[3];
    float texcoord[2];
    float tangent[4];
    float color[4];
    int boneIds[4];
    float boneWeights[4];

    KsMeshVertex() {
        position[0] = position[1] = position[2] = 0;
        normal[0] = 0; normal[1] = 1; normal[2] = 0;
        texcoord[0] = texcoord[1] = 0;
        tangent[0] = tangent[1] = tangent[2] = 0; tangent[3] = 1;
        color[0] = color[1] = color[2] = 1; color[3] = 1;
        for (int i = 0; i < 4; i++) {
            boneIds[i] = -1;
            boneWeights[i] = 0;
        }
    }
};

struct KsMeshFace {
    int indices[3];
    int materialId;
    int smoothingGroup;

    KsMeshFace() : materialId(0), smoothingGroup(0) {
        indices[0] = indices[1] = indices[2] = 0;
    }
};

struct KsMeshMaterial {
    QString name;
    QString shader;
    QString diffuseMap;
    QString normalMap;
    QString specularMap;
    QString emissiveMap;

    float diffuse[4];
    float specular[4];
    float ambient[4];
    float emissive[4];

    float opacity;
    float shininess;
    float bumpStrength;

    KsMeshMaterial() : opacity(1.0f), shininess(32.0f), bumpStrength(1.0f) {
        diffuse[0] = diffuse[1] = diffuse[2] = 0.8f; diffuse[3] = 1.0f;
        specular[0] = specular[1] = specular[2] = 0.5f; specular[3] = 1.0f;
        ambient[0] = ambient[1] = ambient[2] = 0.2f; ambient[3] = 1.0f;
        emissive[0] = emissive[1] = emissive[2] = 0; emissive[3] = 1.0f;
    }
};

class KsMeshData {
public:
    QList<KsMeshVertex> vertices;
    QList<KsMeshFace> faces;
    QList<KsMeshMaterial> materials;

    QString name;
    QString sourceFile;

    float boundingMin[3];
    float boundingMax[3];
    float boundingRadius;

    int vertexCount() const { return vertices.size(); }
    int faceCount() const { return faces.size(); }

    void clear() {
        vertices.clear();
        faces.clear();
        materials.clear();
    }

    void calculateBounds() {
        if (vertices.isEmpty()) return;

        boundingMin[0] = boundingMin[1] = boundingMin[2] = 1e9f;
        boundingMax[0] = boundingMax[1] = boundingMax[2] = -1e9f;

        for (const auto& v : vertices) {
            for (int i = 0; i < 3; i++) {
                if (v.position[i] < boundingMin[i]) boundingMin[i] = v.position[i];
                if (v.position[i] > boundingMax[i]) boundingMax[i] = v.position[i];
            }
        }

        boundingRadius = 0;
        float center[3] = {
            (boundingMin[0] + boundingMax[0]) * 0.5f,
            (boundingMin[1] + boundingMax[1]) * 0.5f,
            (boundingMin[2] + boundingMax[2]) * 0.5f
        };

        for (const auto& v : vertices) {
            float dx = v.position[0] - center[0];
            float dy = v.position[1] - center[1];
            float dz = v.position[2] - center[2];
            float dist = sqrt(dx*dx + dy*dy + dz*dz);
            if (dist > boundingRadius) boundingRadius = dist;
        }
    }
};

class KsMeshUtils {
public:
    static bool loadFromOBJ(const QString& path, KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QTextStream in(&file);

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith("#")) continue;

            QStringList parts = line.split(" ");
            QString cmd = parts[0];

            if (cmd == "v") {
                KsMeshVertex v;
                v.position[0] = parts[1].toFloat();
                v.position[1] = parts[2].toFloat();
                v.position[2] = parts[3].toFloat();
                mesh->vertices.append(v);
            }
            else if (cmd == "vn") {
                if (mesh->vertices.size() > 0) {
                    KsMeshVertex& v = mesh->vertices.last();
                    v.normal[0] = parts[1].toFloat();
                    v.normal[1] = parts[2].toFloat();
                    v.normal[2] = parts[3].toFloat();
                }
            }
            else if (cmd == "vt") {
                if (mesh->vertices.size() > 0) {
                    KsMeshVertex& v = mesh->vertices.last();
                    v.texcoord[0] = parts[1].toFloat();
                    v.texcoord[1] = parts[2].toFloat();
                }
            }
            else if (cmd == "f") {
                KsMeshFace f;
                QStringList v0 = parts[1].split("/");
                QStringList v1 = parts[2].split("/");
                QStringList v2 = parts[3].split("/");

                f.indices[0] = v0[0].toInt() - 1;
                f.indices[1] = v1[0].toInt() - 1;
                f.indices[2] = v2[0].toInt() - 1;

                mesh->faces.append(f);
            }
        }

        file.close();
        mesh->calculateBounds();
        return true;
    }

    static bool saveToOBJ(const QString& path, const KsMeshData* mesh) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QTextStream out(&file);
        out << "# OBJ Export\n";
        out << "# Vertices: " << mesh->vertexCount() << "\n";
        out << "# Faces: " << mesh->faceCount() << "\n\n";

        for (const auto& v : mesh->vertices) {
            out << "v " << v.position[0] << " " << v.position[1] << " " << v.position[2] << "\n";
        }
        out << "\n";

        for (const auto& v : mesh->vertices) {
            out << "vn " << v.normal[0] << " " << v.normal[1] << " " << v.normal[2] << "\n";
        }
        out << "\n";

        for (const auto& v : mesh->vertices) {
            out << "vt " << v.texcoord[0] << " " << v.texcoord[1] << "\n";
        }
        out << "\n";

        for (const auto& f : mesh->faces) {
            out << "f " << f.indices[0] + 1 << " " << f.indices[1] + 1 << " " << f.indices[2] + 1 << "\n";
        }

        file.close();
        return true;
    }
};

// ---------------------------------------------------------------------------
// Converter implementations
// ---------------------------------------------------------------------------

bool KsConverter::exportToOBJ(const QString& path, const KsMeshData* mesh) {
    return KsMeshUtils::saveToOBJ(path, mesh);
}

bool KsConverter::importFromOBJ(const QString& path, KsMeshData* mesh) {
    return KsMeshUtils::loadFromOBJ(path, mesh);
}

bool KsConverter::exportToJSON(const QString& path, const KsMeshData* mesh) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QJsonObject root;
    root["name"] = mesh->name;
    root["vertexCount"] = mesh->vertices.size();
    root["faceCount"] = mesh->faces.size();

    QJsonArray vertices;
    for (const auto& v : mesh->vertices) {
        QJsonArray vertex;
        vertex.append(v.position[0]);
        vertex.append(v.position[1]);
        vertex.append(v.position[2]);
        vertex.append(v.normal[0]);
        vertex.append(v.normal[1]);
        vertex.append(v.normal[2]);
        vertex.append(v.texcoord[0]);
        vertex.append(v.texcoord[1]);
        vertices.append(vertex);
    }
    root["vertices"] = vertices;

    QJsonArray faces;
    for (const auto& f : mesh->faces) {
        QJsonArray face;
        face.append(f.indices[0]);
        face.append(f.indices[1]);
        face.append(f.indices[2]);
        faces.append(face);
    }
    root["faces"] = faces;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

bool KsConverter::importFromJSON(const QString& path, KsMeshData* mesh) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) return false;

    QJsonObject root = doc.object();
    mesh->name = root["name"].toString();

    QJsonArray vertices = root["vertices"].toArray();
    for (const QJsonValue& v : vertices) {
        QJsonArray arr = v.toArray();
        KsMeshVertex vert;
        vert.position[0] = arr[0].toDouble();
        vert.position[1] = arr[1].toDouble();
        vert.position[2] = arr[2].toDouble();
        if (arr.size() > 5) {
            vert.normal[0] = arr[3].toDouble();
            vert.normal[1] = arr[4].toDouble();
            vert.normal[2] = arr[5].toDouble();
            if (arr.size() > 7) {
                vert.texcoord[0] = arr[6].toDouble();
                vert.texcoord[1] = arr[7].toDouble();
            }
        }
        mesh->vertices.append(vert);
    }

    QJsonArray faces = root["faces"].toArray();
    for (const QJsonValue& f : faces) {
        QJsonArray arr = f.toArray();
        KsMeshFace face;
        face.indices[0] = arr[0].toInt();
        face.indices[1] = arr[1].toInt();
        face.indices[2] = arr[2].toInt();
        mesh->faces.append(face);
    }

    mesh->calculateBounds();
    return true;
}

bool KsConverter::exportToXML(const QString& path, const KsMeshData* mesh) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("mesh");
    xml.writeAttribute("name", mesh->name);
    xml.writeTextElement("vertexCount", QString::number(mesh->vertices.size()));
    xml.writeTextElement("faceCount", QString::number(mesh->faces.size()));

    xml.writeStartElement("vertices");
    for (const auto& v : mesh->vertices) {
        xml.writeStartElement("vertex");
        xml.writeAttribute("x", QString::number(v.position[0]));
        xml.writeAttribute("y", QString::number(v.position[1]));
        xml.writeAttribute("z", QString::number(v.position[2]));
        xml.writeEndElement();
    }
    xml.writeEndElement();

    xml.writeStartElement("faces");
    for (const auto& f : mesh->faces) {
        xml.writeStartElement("face");
        xml.writeAttribute("v0", QString::number(f.indices[0]));
        xml.writeAttribute("v1", QString::number(f.indices[1]));
        xml.writeAttribute("v2", QString::number(f.indices[2]));
        xml.writeEndElement();
    }
    xml.writeEndElement();

    xml.writeEndElement();
    xml.writeEndDocument();

    file.close();
    return true;
}

bool KsConverter::importFromXML(const QString& path, KsMeshData* mesh) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QXmlStreamReader xml(&file);
    if (!xml.readNextStartElement()) return false;

    if (xml.name() != "mesh") return false;
    mesh->name = xml.attributes().value("name").toString();

    while (xml.readNextStartElement()) {
        if (xml.name() == "vertices") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "vertex") {
                    KsMeshVertex v;
                    v.position[0] = xml.attributes().value("x").toDouble();
                    v.position[1] = xml.attributes().value("y").toDouble();
                    v.position[2] = xml.attributes().value("z").toDouble();
                    mesh->vertices.append(v);
                    xml.skipCurrentElement();
                }
            }
        } else if (xml.name() == "faces") {
            while (xml.readNextStartElement()) {
                if (xml.name() == "face") {
                    KsMeshFace f;
                    f.indices[0] = xml.attributes().value("v0").toInt();
                    f.indices[1] = xml.attributes().value("v1").toInt();
                    f.indices[2] = xml.attributes().value("v2").toInt();
                    mesh->faces.append(f);
                    xml.skipCurrentElement();
                }
            }
        } else {
            xml.skipCurrentElement();
        }
    }

    file.close();
    if (xml.hasError()) {
        qWarning() << "XML parse error in mesh:" << xml.errorString();
        return false;
    }
    mesh->calculateBounds();
    return true;
}

bool KsConverter::exportToSTL(const QString& path, const KsMeshData* mesh, bool ascii) {
    if (ascii) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QTextStream out(&file);
        out << "solid default\n";

        for (const auto& f : mesh->faces) {
            const auto& v0 = mesh->vertices[f.indices[0]];
            const auto& v1 = mesh->vertices[f.indices[1]];
            const auto& v2 = mesh->vertices[f.indices[2]];

            float ax = v1.position[0] - v0.position[0];
            float ay = v1.position[1] - v0.position[1];
            float az = v1.position[2] - v0.position[2];
            float bx = v2.position[0] - v0.position[0];
            float by = v2.position[1] - v0.position[1];
            float bz = v2.position[2] - v0.position[2];

            float nx = ay * bz - az * by;
            float ny = az * bx - ax * bz;
            float nz = ax * by - ay * bx;
            float len = sqrt(nx*nx + ny*ny + nz*nz);
            if (len > 0) {
                nx /= len; ny /= len; nz /= len;
            }

            out << "facet normal " << nx << " " << ny << " " << nz << "\n";
            out << "  outer loop\n";
            out << "    vertex " << v0.position[0] << " " << v0.position[1] << " " << v0.position[2] << "\n";
            out << "    vertex " << v1.position[0] << " " << v1.position[1] << " " << v1.position[2] << "\n";
            out << "    vertex " << v2.position[0] << " " << v2.position[1] << " " << v2.position[2] << "\n";
            out << "  endloop\n";
            out << "endfacet\n";
        }

        out << "endsolid\n";
        file.close();
    } else {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) return false;

        QDataStream out(&file);
        out.setByteOrder(QDataStream::LittleEndian);

        char header[80] = {0};
        file.write(header, 80);

        quint32 faceCount = mesh->faces.size();
        out << faceCount;

        for (const auto& f : mesh->faces) {
            const auto& v0 = mesh->vertices[f.indices[0]];
            const auto& v1 = mesh->vertices[f.indices[1]];
            const auto& v2 = mesh->vertices[f.indices[2]];

            float ax = v1.position[0] - v0.position[0];
            float ay = v1.position[1] - v0.position[1];
            float az = v1.position[2] - v0.position[2];
            float bx = v2.position[0] - v0.position[0];
            float by = v2.position[1] - v0.position[1];
            float bz = v2.position[2] - v0.position[2];

            float nx = ay * bz - az * by;
            float ny = az * bx - ax * bz;
            float nz = ax * by - ay * bx;
            float len = sqrt(nx*nx + ny*ny + nz*nz);
            if (len > 0) {
                nx /= len; ny /= len; nz /= len;
            }

            out << (float)nx << (float)ny << (float)nz;
            out << (float)v0.position[0] << (float)v0.position[1] << (float)v0.position[2];
            out << (float)v1.position[0] << (float)v1.position[1] << (float)v1.position[2];
            out << (float)v2.position[0] << (float)v2.position[1] << (float)v2.position[2];
            out << (quint16)0;
        }

        file.close();
    }

    return true;
}

bool KsConverter::importFromSTL(const QString& path, KsMeshData* mesh, bool* isBinary) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QTextStream in(&file);
    QString firstLine = in.readLine().trimmed();

    bool binary = false;
    if (!firstLine.startsWith("solid")) {
        binary = true;
    }
    file.close();

    if (isBinary) *isBinary = binary;

    if (binary) {
        if (!file.open(QIODevice::ReadOnly)) return false;

        QDataStream in(&file);
        in.setByteOrder(QDataStream::LittleEndian);

        file.seek(80);
        quint32 faceCount;
        in >> faceCount;

        for (quint32 i = 0; i < faceCount; i++) {
            float nx, ny, nz;
            float v0[3], v1[3], v2[3];
            quint16 attr;

            in >> nx >> ny >> nz;
            in >> v0[0] >> v0[1] >> v0[2];
            in >> v1[0] >> v1[1] >> v1[2];
            in >> v2[0] >> v2[1] >> v2[2];
            in >> attr;

            KsMeshVertex vert0, vert1, vert2;
            for (int j = 0; j < 3; j++) {
                vert0.position[j] = v0[j];
                vert1.position[j] = v1[j];
                vert2.position[j] = v2[j];
                vert0.normal[j] = nx;
                vert1.normal[j] = ny;
                vert2.normal[j] = nz;
            }

            int baseIdx = mesh->vertices.size();
            mesh->vertices.append(vert0);
            mesh->vertices.append(vert1);
            mesh->vertices.append(vert2);

            KsMeshFace f;
            f.indices[0] = baseIdx;
            f.indices[1] = baseIdx + 1;
            f.indices[2] = baseIdx + 2;
            mesh->faces.append(f);
        }

        file.close();
    } else {
        if (!file.open(QIODevice::ReadOnly)) return false;

        QTextStream in(&file);
        QString line;

        QList<float> stlVerticesX;
        QList<float> stlVerticesY;
        QList<float> stlVerticesZ;
        float snx, sny, snz;

        while (!(line = in.readLine()).isNull()) {
            line = line.trimmed();
            if (line.startsWith("facet normal")) {
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() >= 5) {
                    snx = parts[2].toFloat();
                    sny = parts[3].toFloat();
                    snz = parts[4].toFloat();
                }
            } else if (line.startsWith("vertex")) {
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() >= 4) {
                    stlVerticesX.append(parts[1].toFloat());
                    stlVerticesY.append(parts[2].toFloat());
                    stlVerticesZ.append(parts[3].toFloat());
                }
            } else if (line.startsWith("endfacet")) {
                if (stlVerticesX.size() >= 3) {
                    int baseIdx = mesh->vertices.size();

                    for (int i = 0; i < 3; i++) {
                        KsMeshVertex vert;
                        vert.position[0] = stlVerticesX[i];
                        vert.position[1] = stlVerticesY[i];
                        vert.position[2] = stlVerticesZ[i];
                        vert.normal[0] = snx;
                        vert.normal[1] = sny;
                        vert.normal[2] = snz;
                        mesh->vertices.append(vert);
                    }

                    KsMeshFace f;
                    f.indices[0] = baseIdx;
                    f.indices[1] = baseIdx + 1;
                    f.indices[2] = baseIdx + 2;
                    mesh->faces.append(f);
                }
                stlVerticesX.clear();
                stlVerticesY.clear();
                stlVerticesZ.clear();
            }
        }

        file.close();
    }

    mesh->calculateBounds();
    return true;
}

bool KsKN5Converter::exportToKN5(const QString& path, const KsMeshData* mesh) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    out << (quint32)0x354E4B;
    out << (quint32)2;
    out << (quint32)mesh->vertexCount();
    out << (quint32)mesh->faceCount();
    out << (quint32)1;

    for (const auto& v : mesh->vertices) {
        out << (float)v.position[0];
        out << (float)v.position[1];
        out << (float)v.position[2];
    }

    for (const auto& v : mesh->vertices) {
        out << (float)v.normal[0];
        out << (float)v.normal[1];
        out << (float)v.normal[2];
    }

    for (const auto& v : mesh->vertices) {
        out << (float)v.texcoord[0];
        out << (float)v.texcoord[1];
    }

    for (const auto& f : mesh->faces) {
        out << (quint32)f.indices[0];
        out << (quint32)f.indices[1];
        out << (quint32)f.indices[2];
    }

    file.close();
    return true;
}

bool KsKN5Converter::importFromKN5(const QString& path, KsMeshData* mesh) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    quint32 magic, version;
    in >> magic;
    in >> version;

    if (magic != 0x354E4B) {
        file.close();
        return false;
    }

    quint32 vertexCount, faceCount, materialCount;
    in >> vertexCount;
    in >> faceCount;
    in >> materialCount;

    QList<float> positions;
    for (quint32 i = 0; i < vertexCount; i++) {
        float x, y, z;
        in >> x >> y >> z;
        positions.append(x); positions.append(y); positions.append(z);
    }

    QList<float> normals;
    for (quint32 i = 0; i < vertexCount; i++) {
        float nx, ny, nz;
        in >> nx >> ny >> nz;
        normals.append(nx); normals.append(ny); normals.append(nz);
    }

    QList<float> texcoords;
    for (quint32 i = 0; i < vertexCount; i++) {
        float u, v;
        in >> u >> v;
        texcoords.append(u); texcoords.append(v);
    }

    for (quint32 i = 0; i < vertexCount; i++) {
        KsMeshVertex vert;
        vert.position[0] = positions[i*3];
        vert.position[1] = positions[i*3+1];
        vert.position[2] = positions[i*3+2];
        vert.normal[0] = normals[i*3];
        vert.normal[1] = normals[i*3+1];
        vert.normal[2] = normals[i*3+2];
        vert.texcoord[0] = texcoords[i*2];
        vert.texcoord[1] = texcoords[i*2+1];
        mesh->vertices.append(vert);
    }

    for (quint32 i = 0; i < faceCount; i++) {
        KsMeshFace f;
        in >> f.indices[0] >> f.indices[1] >> f.indices[2];
        mesh->faces.append(f);
    }

    file.close();
    mesh->calculateBounds();
    return true;
}

bool KsModelConverter::convert(const QString& inputPath, const QString& outputPath, KsImportFormat from, KsExportFormat to) {
    KsMeshData* mesh = new KsMeshData();
    bool success = false;

    switch (from) {
        case Import_OBJ:
            success = KsConverter::importFromOBJ(inputPath, mesh);
            break;
        case Import_STL:
            success = KsConverter::importFromSTL(inputPath, mesh);
            break;
        case Import_KN5:
            success = KsKN5Converter::importFromKN5(inputPath, mesh);
            break;
        default:
            delete mesh;
            return false;
    }

    if (!success) {
        delete mesh;
        return false;
    }

    switch (to) {
        case Format_OBJ:
            success = KsConverter::exportToOBJ(outputPath, mesh);
            break;
        case Format_STL:
            success = KsConverter::exportToSTL(outputPath, mesh);
            break;
        case Format_JSON:
            success = KsConverter::exportToJSON(outputPath, mesh);
            break;
        case Format_XML:
            success = KsConverter::exportToXML(outputPath, mesh);
            break;
        default:
            success = false;
    }

    delete mesh;
    return success;
}

QString KsModelConverter::detectFormat(const QString& path) {
    QFileInfo info(path);
    QString ext = info.suffix().toLower();

    if (ext == "obj") return "OBJ";
    if (ext == "fbx") return "FBX";
    if (ext == "gltf" || ext == "glb") return "GLTF";
    if (ext == "dae") return "DAE";
    if (ext == "stl") return "STL";
    if (ext == "3ds") return "3DS";
    if (ext == "kn5") return "KN5";
    if (ext == "json") return "JSON";
    if (ext == "xml") return "XML";

    return "Unknown";
}

QStringList KsModelConverter::getSupportedImportFormats() {
    return QStringList() << "OBJ" << "STL" << "KN5" << "GLTF" << "FBX" << "DAE";
}

QStringList KsModelConverter::getSupportedExportFormats() {
    return QStringList() << "OBJ" << "STL" << "JSON" << "XML" << "GLTF" << "FBX" << "DAE" << "3DS";
}

QString KsModelConverter::exportFormatExtension(KsExportFormat fmt) {
    switch (fmt) {
        case Format_OBJ: return "obj";
        case Format_FBX: return "fbx";
        case Format_GLTF: return "gltf";
        case Format_DAE: return "dae";
        case Format_STL: return "stl";
        case Format_3DS: return "3ds";
        case Format_JSON: return "json";
        case Format_XML: return "xml";
    }
    return "obj";
}

int KsBatchConverter::batchConvert(const QString& inputDir, const QString& outputDir, KsImportFormat from, KsExportFormat to, const QString& extFilter) {
    QDir inDir(inputDir);
    if (!inDir.exists()) return 0;

    QDir outDir(outputDir);
    if (!outDir.exists()) {
        QDir().mkpath(outputDir);
    }

    QStringList files = inDir.entryList(QStringList() << extFilter, QDir::Files);

    int converted = 0;
    for (const QString& file : files) {
        QString inputPath = inDir.absoluteFilePath(file);
        QFileInfo info(file);
        QString outputPath = outDir.absoluteFilePath(info.baseName() + "." + KsModelConverter::exportFormatExtension(to));

        if (KsModelConverter::convert(inputPath, outputPath, from, to)) {
            converted++;
        }
    }

    return converted;
}

int KsBatchConverter::batchConvertAll(const QString& inputDir, const QString& outputDir, KsExportFormat to) {
    QDir inDir(inputDir);
    if (!inDir.exists()) return 0;

    QDir outDir(outputDir);
    if (!outDir.exists()) {
        QDir().mkpath(outputDir);
    }

    QStringList files = inDir.entryList(QDir::Files);

    int converted = 0;
    for (const QString& file : files) {
        QString inputPath = inDir.absoluteFilePath(file);
        QString format = KsModelConverter::detectFormat(file);

        QFileInfo info(file);
        QString outputPath = outDir.absoluteFilePath(info.baseName() + ".obj");

        KsMeshData* mesh = new KsMeshData();
        bool success = false;

        if (format == "OBJ") {
            success = KsConverter::importFromOBJ(inputPath, mesh);
        } else if (format == "STL") {
            success = KsConverter::importFromSTL(inputPath, mesh);
        } else if (format == "KN5") {
            success = KsKN5Converter::importFromKN5(inputPath, mesh);
        }

        if (success) {
            switch (to) {
                case Format_OBJ:
                    outputPath = outDir.absoluteFilePath(info.baseName() + ".obj");
                    success = KsConverter::exportToOBJ(outputPath, mesh);
                    break;
                case Format_STL:
                    outputPath = outDir.absoluteFilePath(info.baseName() + ".stl");
                    success = KsConverter::exportToSTL(outputPath, mesh);
                    break;
                case Format_JSON:
                    outputPath = outDir.absoluteFilePath(info.baseName() + ".json");
                    success = KsConverter::exportToJSON(outputPath, mesh);
                    break;
                default:
                    success = false;
            }

            if (success) converted++;
        }

        delete mesh;
    }

    return converted;
}

} // namespace ks
