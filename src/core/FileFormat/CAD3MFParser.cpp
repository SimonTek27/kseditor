#include "CAD3MFParser.h"
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QDataStream>

namespace ks {

// Minimal ZIP local file header parser for 3MF (no external dep)
static QMap<QString, QByteArray> extractZipEntries(const QByteArray& data)
{
    QMap<QString, QByteArray> entries;
    int pos = 0;

    while (pos + 30 <= data.size()) {
        if (data[pos] != 'P' || data[pos + 1] != 'K') break;

        quint16 version, flags, method, modTime, modDate;
        quint32 crc32, compSize, uncompSize, nameLen, extraLen;

        QDataStream ds(data.mid(pos + 4, 26));
        ds.setByteOrder(QDataStream::LittleEndian);
        ds >> version >> flags >> method >> modTime >> modDate
           >> crc32 >> compSize >> uncompSize >> nameLen >> extraLen;

        if (pos + 30 + nameLen + extraLen + compSize > (quint32)data.size()) break;

        QString name = QString::fromLatin1(data.mid(pos + 30, nameLen));
        QByteArray entryData = data.mid(pos + 30 + nameLen + extraLen, compSize);

        if (method == 0) {
            entries[name] = entryData;
        } else if (method == 8) {
            // Deflate - for a real implementation, use zlib via QByteArray::qUncompress
            // For now, store compressed; real impl would need zlib
            entries[name] = entryData;
        }

        pos += 30 + nameLen + extraLen + compSize;
    }

    return entries;
}

bool CAD3MFParser::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 4 || data.left(2) != "PK") {
        m_lastError = "Not a valid 3MF file (ZIP archive expected)";
        return false;
    }

    m_scene.name = QFileInfo(filePath).baseName();
    return parseZip(data);
}

bool CAD3MFParser::parseZip(const QByteArray& data)
{
    auto entries = extractZipEntries(data);

    QByteArray modelData;
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it.key().endsWith(".model")) {
            modelData = it.value();
            break;
        }
    }

    if (modelData.isEmpty()) {
        // Try to find any .model entry by searching for 3D/3DModel.model
        if (entries.contains("3D/3DModel.model")) modelData = entries["3D/3DModel.model"];
        else if (entries.contains("/3D/3DModel.model")) modelData = entries["/3D/3DModel.model"];
    }

    if (modelData.isEmpty()) {
        m_lastError = "No .model file found in 3MF archive";
        return false;
    }

    QXmlStreamReader xml(modelData);

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement()) {
            QStringView name = xml.name();

            if (name == "model") {
                QString unit = xml.attributes().value("unit").toString();
                if (unit.isEmpty()) unit = "millimeter";
            } else if (name == "metadata") {
                QString attrName = xml.attributes().value("name").toString();
                QString value = xml.readElementText();
                if (attrName == "Title") m_scene.name = value;
            } else if (name == "object") {
                QString objId = xml.attributes().value("id").toString();
                QString objName = xml.attributes().value("name").toString();
                QString objType = xml.attributes().value("type").toString();
                ThreeMFMesh mesh;
                mesh.name = objName.isEmpty() ? objId : objName;

                while (!(xml.isEndElement() && xml.name() == "object")) {
                    xml.readNext();
                    if (xml.isStartElement() && xml.name() == "mesh") {
                        bool inMesh = true;
                        while (inMesh) {
                            xml.readNext();
                            if (xml.isEndElement() && xml.name() == "mesh") inMesh = false;
                            if (xml.isStartElement() && xml.name() == "vertices") {
                                while (!(xml.isEndElement() && xml.name() == "vertices")) {
                                    xml.readNext();
                                    if (xml.isStartElement() && xml.name() == "vertex") {
                                        Vec3 v;
                                        v.x = xml.attributes().value("x").toFloat();
                                        v.y = xml.attributes().value("y").toFloat();
                                        v.z = xml.attributes().value("z").toFloat();
                                        mesh.vertices.append(v);
                                    }
                                }
                            }
                            if (xml.isStartElement() && xml.name() == "triangles") {
                                while (!(xml.isEndElement() && xml.name() == "triangles")) {
                                    xml.readNext();
                                    if (xml.isStartElement() && xml.name() == "triangle") {
                                        auto attrs = xml.attributes();
                                        uint32_t i1 = attrs.value("v1").toUInt();
                                        uint32_t i2 = attrs.value("v2").toUInt();
                                        uint32_t i3 = attrs.value("v3").toUInt();
                                        mesh.indices.append(i1);
                                        mesh.indices.append(i2);
                                        mesh.indices.append(i3);
                                        QString pid = attrs.value("pid").toString();
                                        if (!pid.isEmpty()) mesh.materialId = pid;
                                    }
                                }
                            }
                        }
                    }
                }

                if (!mesh.vertices.isEmpty()) {
                    m_scene.meshes.append(mesh);
                }
            } else if (name == "basematerials") {
                while (!(xml.isEndElement() && xml.name() == "basematerials")) {
                    xml.readNext();
                    if (xml.isStartElement() && xml.name() == "base") {
                        ThreeMFMaterial mat;
                        mat.name = xml.attributes().value("name").toString();
                        QString displayColor = xml.attributes().value("displaycolor").toString();
                        if (displayColor.startsWith('#')) {
                            bool ok;
                            uint32_t hexColor = displayColor.mid(1).toUInt(&ok, 16);
                            if (ok) {
                                mat.color.x = ((hexColor >> 16) & 0xFF) / 255.0f;
                                mat.color.y = ((hexColor >> 8) & 0xFF) / 255.0f;
                                mat.color.z = (hexColor & 0xFF) / 255.0f;
                                if (displayColor.length() == 9) {
                                    mat.opacity = ((hexColor >> 24) & 0xFF) / 255.0f;
                                }
                            }
                        }
                        m_scene.materials[mat.name] = mat;
                    }
                }
            }
        }
    }

    if (xml.hasError()) {
        m_lastError = "XML parse error: " + xml.errorString();
        return false;
    }

    return true;
}

} // namespace ks