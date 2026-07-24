#include "CADColladaParser.h"
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>

namespace ks {

bool CADColladaParser::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Cannot open file: " + filePath;
        return false;
    }

    m_scene = ColladaScene();
    m_scene.name = QFileInfo(filePath).baseName();

    QXmlStreamReader xml(&file);

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name().toString() == "COLLADA") {
                m_scene.metadata["version"] = xml.attributes().value("version").toString();
            } else if (xml.name().toString() == "library_geometries") {
                parseLibraryGeometries(xml);
            } else if (xml.name().toString() == "library_materials") {
                parseLibraryMaterials(xml);
            } else if (xml.name().toString() == "library_effects") {
                parseLibraryEffects(xml);
            } else if (xml.name().toString() == "library_images") {
                parseLibraryImages(xml);
            }
        }
    }

    if (xml.hasError()) {
        m_lastError = "XML parse error: " + xml.errorString();
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool CADColladaParser::parseLibraryGeometries(QXmlStreamReader& xml)
{
    while (!(xml.isEndElement() && xml.name().toString() == "library_geometries")) {
        xml.readNext();
        if (xml.isStartElement() && xml.name().toString() == "geometry") {
            QString geomId = xml.attributes().value("id").toString();
            while (!(xml.isEndElement() && xml.name().toString() == "geometry")) {
                xml.readNext();
                if (xml.isStartElement() && xml.name().toString() == "mesh") {
                    parseMesh(xml, geomId);
                }
            }
        }
    }
    return true;
}

bool CADColladaParser::parseMesh(QXmlStreamReader& xml, const QString& meshId)
{
    ColladaMesh mesh;
    mesh.name = meshId;

    struct Source {
        QString id;
        QVector<float> data;
        int stride = 1;
        QString params[4];
        int paramCount = 0;
    };
    QMap<QString, Source> sources;

    while (!(xml.isEndElement() && xml.name().toString() == "mesh")) {
        xml.readNext();

        if (xml.isStartElement() && xml.name().toString() == "source") {
            Source src;
            src.id = xml.attributes().value("id").toString();
            while (!(xml.isEndElement() && xml.name().toString() == "source")) {
                xml.readNext();
                if (xml.isStartElement() && xml.name().toString() == "float_array") {
                    int count = xml.attributes().value("count").toInt();
                    src.data = parseFloatArray(xml, count);
                } else if (xml.isStartElement() && xml.name().toString() == "technique_common") {
                    while (!(xml.isEndElement() && xml.name().toString() == "technique_common")) {
                        xml.readNext();
                        if (xml.isStartElement() && xml.name().toString() == "accessor") {
                            src.stride = xml.attributes().value("stride").toInt();
                            if (src.stride == 0) src.stride = 1;
                            int pi = 0;
                            while (!(xml.isEndElement() && xml.name().toString() == "accessor")) {
                                xml.readNext();
                                if (xml.isStartElement() && xml.name().toString() == "param" && pi < 4) {
                                    src.params[pi++] = xml.attributes().value("name").toString();
                                }
                            }
                            src.paramCount = pi;
                        }
                    }
                }
            }
            QString shortId = src.id;
            if (shortId.startsWith('#')) shortId = shortId.mid(1);
            else {
                int hash = shortId.lastIndexOf('#');
                if (hash >= 0) shortId = shortId.mid(hash + 1);
                else shortId = meshId + "-" + shortId;
            }
            if (!sources.contains(shortId)) {
                sources[shortId] = src;
            } else {
                sources[src.id] = src;
            }
        } else if (xml.isStartElement() && xml.name().toString() == "vertices") {
            QString vertId = xml.attributes().value("id").toString();
            QString posSource;
            while (!(xml.isEndElement() && xml.name().toString() == "vertices")) {
                xml.readNext();
                if (xml.isStartElement() && xml.name().toString() == "input") {
                    if (xml.attributes().value("semantic").toString() == "POSITION") {
                        posSource = xml.attributes().value("source").toString();
                        if (posSource.startsWith('#')) posSource = posSource.mid(1);
                    }
                }
            }
            if (!posSource.isEmpty() && sources.contains(posSource)) {
                sources[vertId] = sources[posSource];
            }
        } else if (xml.isStartElement() && xml.name().toString() == "triangles" || xml.name().toString() == "polylist") {
            int triCount = xml.attributes().value("count").toInt();
            QString matSymbol = xml.attributes().value("material").toString();

            struct InputSemantic {
                QString semantic;
                QString source;
                int offset = 0;
                int set = 0;
            };
            QVector<InputSemantic> inputs;
            int maxOffset = 0;

            while (!(xml.isEndElement() && (xml.name().toString() == "triangles" || xml.name().toString() == "polylist"))) {
                xml.readNext();
                if (xml.isStartElement() && xml.name().toString() == "input") {
                    InputSemantic inp;
                    inp.semantic = xml.attributes().value("semantic").toString();
                    inp.source = xml.attributes().value("source").toString();
                    if (inp.source.startsWith('#')) inp.source = inp.source.mid(1);
                    inp.offset = xml.attributes().value("offset").toInt();
                    inp.set = xml.attributes().value("set").toInt();
                    inputs.append(inp);
                    if (inp.offset > maxOffset) maxOffset = inp.offset;
                } else if (xml.isStartElement() && (xml.name().toString() == "p" || xml.name().toString() == "vcount")) {
                    if (xml.name().toString() == "vcount") {
                        while (!(xml.isEndElement() && xml.name().toString() == "vcount")) xml.readNext();
                    } else {
                        QString text = xml.readElementText().trimmed();
                        QStringList tokens = text.split(' ', Qt::SkipEmptyParts);
                        int stride = maxOffset + 1;
                        int pi = 0;
                        QVector<uint32_t> posIndices, normIndices, texIndices;

                        for (int i = 0; i + stride - 1 < tokens.size(); i += stride) {
                            for (const auto& inp : inputs) {
                                int idx = tokens[i + inp.offset].toInt();
                                if (inp.semantic == "VERTEX" || inp.semantic == "POSITION") {
                                    posIndices.append(static_cast<uint32_t>(idx));
                                } else if (inp.semantic == "NORMAL") normIndices.append(static_cast<uint32_t>(idx));
                                else if (inp.semantic == "TEXCOORD") texIndices.append(static_cast<uint32_t>(idx));
                            }
                        }

                        for (int t = 0; t + 2 < posIndices.size(); t += 3) {
                            uint32_t i0 = posIndices[t], i1 = posIndices[t + 1], i2 = posIndices[t + 2];
                            mesh.indices.append(static_cast<uint32_t>(mesh.vertices.size()));
                            for (int vi = 0; vi < 3; ++vi) {
                                uint32_t pi = (vi == 0) ? i0 : (vi == 1) ? i1 : i2;
                                ColladaVertex cv;
                                for (const auto& inp : inputs) {
                                    QString srcId = inp.source;
                                    if (srcId.startsWith('#')) srcId = srcId.mid(1);
                                    if (!sources.contains(srcId)) continue;
                                    auto& src = sources[srcId];
                                    uint32_t idx = (inp.semantic == "VERTEX" || inp.semantic == "POSITION") ? pi :
                                                   (inp.semantic == "NORMAL") ? ((vi < normIndices.size()) ? normIndices[t + vi] : 0) :
                                                   (inp.semantic == "TEXCOORD") ? ((vi < texIndices.size()) ? texIndices[t + vi] : 0) : 0;
                                    int dataIdx = static_cast<int>(idx) * src.stride;
                                    if (dataIdx + 2 < src.data.size()) {
                                        cv.position = Vec3(src.data[dataIdx], src.data[dataIdx + 1], src.data[dataIdx + 2]);
                                    }
                                    if (inp.semantic == "NORMAL" && dataIdx + 2 < src.data.size()) {
                                        cv.normal = Vec3(src.data[dataIdx], src.data[dataIdx + 1], src.data[dataIdx + 2]);
                                    }
                                    if (inp.semantic == "TEXCOORD" && dataIdx + 1 < src.data.size()) {
                                        cv.texCoord = Vec2(src.data[dataIdx], src.data[dataIdx + 1]);
                                    }
                                }
                                mesh.vertices.append(cv);
                            }
                        }
                    }
                }
            }
            mesh.materialId = matSymbol;
        }
    }

    if (!mesh.vertices.isEmpty()) {
        m_scene.meshes.append(mesh);
    }
    return true;
}

QVector<float> CADColladaParser::parseFloatArray(QXmlStreamReader& xml, int count)
{
    QString text = xml.readElementText().trimmed();
    QStringList tokens = text.split(' ', Qt::SkipEmptyParts);
    QVector<float> result;
    result.reserve(qMin(tokens.size(), count));
    for (int i = 0; i < tokens.size() && i < count; ++i) {
        result.append(tokens[i].toFloat());
    }
    return result;
}

bool CADColladaParser::parseLibraryMaterials(QXmlStreamReader& xml)
{
    while (!(xml.isEndElement() && xml.name().toString() == "library_materials")) {
        xml.readNext();
        if (xml.isStartElement() && xml.name().toString() == "material") {
            ColladaMaterial mat;
            mat.id = xml.attributes().value("id").toString();
            mat.name = xml.attributes().value("name").toString();
            while (!(xml.isEndElement() && xml.name().toString() == "material")) {
                xml.readNext();
                if (xml.isStartElement() && xml.name().toString() == "instance_effect") {
                    QString effectUrl = xml.attributes().value("url").toString();
                    if (effectUrl.startsWith('#')) effectUrl = effectUrl.mid(1);
                    mat.name = effectUrl;
                }
            }
            m_scene.materials[mat.id] = mat;
        }
    }
    return true;
}

bool CADColladaParser::parseLibraryEffects(QXmlStreamReader& xml)
{
    while (!(xml.isEndElement() && xml.name().toString() == "library_effects")) {
        xml.readNext();
        if (xml.isStartElement() && xml.name().toString() == "effect") {
            QString effectId = xml.attributes().value("id").toString();
            QString currentParam;
            while (!(xml.isEndElement() && xml.name().toString() == "effect")) {
                xml.readNext();
                if (xml.isStartElement()) {
                    QString name = xml.name().toString();
                    if (name == "profile_COMMON" || name == "technique") continue;
                    if (name == "phong" || name == "lambert" || name == "blinn" || name == "constant") currentParam = name;
                    else if (name == "emission" || name == "ambient" || name == "diffuse" || name == "specular") currentParam = name;
                    else if (name == "shininess" || name == "transparency" || name == "transparent") currentParam = name;
                    else if (name == "color" || name == "float") {
                        QString text = xml.readElementText().trimmed();
                        QStringList vals = text.split(' ', Qt::SkipEmptyParts);
                        for (auto& mat : m_scene.materials) {
                            if (mat.name == effectId || mat.id == effectId) {
                                if (currentParam == "diffuse" && vals.size() >= 3)
                                    mat.diffuse = Vec3(vals[0].toFloat(), vals[1].toFloat(), vals[2].toFloat());
                                else if (currentParam == "specular" && vals.size() >= 3)
                                    mat.specular = Vec3(vals[0].toFloat(), vals[1].toFloat(), vals[2].toFloat());
                                else if (currentParam == "ambient" && vals.size() >= 3)
                                    mat.ambient = Vec3(vals[0].toFloat(), vals[1].toFloat(), vals[2].toFloat());
                                else if (currentParam == "shininess" && vals.size() >= 1)
                                    mat.shininess = vals[0].toFloat();
                                else if (currentParam == "transparency" && vals.size() >= 1)
                                    mat.transparency = vals[0].toFloat();
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool CADColladaParser::parseLibraryImages(QXmlStreamReader& xml)
{
    while (!(xml.isEndElement() && xml.name().toString() == "library_images")) {
        xml.readNext();
        if (xml.isStartElement() && xml.name().toString() == "image") {
            QString imgId = xml.attributes().value("id").toString();
            while (!(xml.isEndElement() && xml.name().toString() == "image")) {
                xml.readNext();
                if (xml.isStartElement() && xml.name().toString() == "init_from") {
                    QString path = xml.readElementText().trimmed();
                    for (auto& mat : m_scene.materials) {
                        if (mat.id == imgId || mat.name == imgId) {
                            mat.diffuseTexture = path;
                        }
                    }
                }
            }
        }
    }
    return true;
}

uint32_t CADColladaParser::parseAccessor(QXmlStreamReader& xml, const QString& sourceId)
{
    return 0;
}

} // namespace ks