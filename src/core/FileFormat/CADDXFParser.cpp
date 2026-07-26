#include "CADDXFParser.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>

namespace ks {

bool CADDXFParser::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    m_scene.name = QFileInfo(filePath).baseName();
    return parseContent(content);
}

bool CADDXFParser::parseContent(const QString& content)
{
    // Parse DXF group code/value pairs
    QStringList lines = content.split('\n', Qt::SkipEmptyParts);
    QVector<QPair<int, QString>> groups;

    for (int i = 0; i + 1 < lines.size(); i += 2) {
        int code = lines[i].trimmed().toInt();
        QString value = lines[i + 1].trimmed();
        groups.append({code, value});
    }

    m_scene.version = "Unknown";

    // Parse DXF structure
    for (int pos = 0; pos < groups.size(); ) {
        int code = groups[pos].first;
        QString value = groups[pos].second;

        if (code == 0 && value == "SECTION") {
            pos++;
            if (pos < groups.size() && groups[pos].first == 2) {
                QString sectionName = groups[pos].second;
                pos++;

                if (sectionName == "HEADER") {
                    while (pos < groups.size()) {
                        if (groups[pos].first == 0 && groups[pos].second == "ENDSEC") break;
                        if (groups[pos].first == 9) {
                            QString varName = groups[pos].second;
                            pos++;
                            if (varName == "$ACADVER" && pos < groups.size()) {
                                pos++;
                                if (pos < groups.size()) m_scene.version = groups[pos].second;
                            }
                        }
                        pos++;
                    }
                } else if (sectionName == "TABLES") {
                    while (pos < groups.size()) {
                        if (groups[pos].first == 0 && groups[pos].second == "ENDSEC") break;
                        if (groups[pos].first == 0 && groups[pos].second == "LAYER") {
                            pos++;
                            DXFLayer layer;
                            layer.name = "0";
                            while (pos < groups.size()) {
                                if (groups[pos].first == 0) break;
                                if (groups[pos].first == 2) layer.name = groups[pos].second;
                                if (groups[pos].first == 62) layer.colorNumber = groups[pos].second.toInt();
                                if (groups[pos].first == 70) {
                                    int flags = groups[pos].second.toInt();
                                    layer.isFrozen = flags & 1;
                                    layer.isLocked = flags & 4;
                                }
                                pos++;
                            }
                            m_scene.layers.append(layer);
                        } else {
                            pos++;
                        }
                    }
                } else if (sectionName == "ENTITIES") {
                    parseEntities(groups, pos);
                } else {
                    pos++;
                }
            }
        } else if (code == 0 && value == "EOF") {
            break;
        } else {
            pos++;
        }
    }

    return true;
}

void CADDXFParser::parseEntities(const QVector<QPair<int, QString>>& groups, int& pos)
{
    QString currentEntityType;

    while (pos < groups.size()) {
        if (groups[pos].first == 0 && groups[pos].second == "ENDSEC") {
            pos++;
            return;
        }

        if (groups[pos].first == 0) {
            currentEntityType = groups[pos].second;
            pos++;

            if (currentEntityType == "3DFACE") {
                DXFMesh mesh;
                mesh.name = QString("3DFace_%1").arg(m_scene.meshes.size());
                Vec3 pts[4];
                int ptCount = 0;

                while (pos < groups.size()) {
                    if (groups[pos].first == 0) break;
                    int code = groups[pos].first;
                    float val = groups[pos].second.toFloat();

                    if (code == 10) pts[0].x = val;
                    else if (code == 20) pts[0].y = val;
                    else if (code == 30) pts[0].z = val;
                    else if (code == 11) pts[1].x = val;
                    else if (code == 21) pts[1].y = val;
                    else if (code == 31) pts[1].z = val;
                    else if (code == 12) pts[2].x = val;
                    else if (code == 22) pts[2].y = val;
                    else if (code == 32) pts[2].z = val;
                    else if (code == 13) pts[3].x = val;
                    else if (code == 23) pts[3].y = val;
                    else if (code == 33) pts[3].z = val;
                    else if (code == 8) mesh.layerName = groups[pos].second;

                    pos++;
                }

                if (ptCount == 0) ptCount = 4;
                int base = mesh.vertices.size();
                mesh.vertices.append(pts[0]);
                mesh.vertices.append(pts[1]);
                mesh.vertices.append(pts[2]);
                mesh.indices.append(base);
                mesh.indices.append(base + 1);
                mesh.indices.append(base + 2);

                if (pts[3] != pts[2]) {
                    mesh.vertices.append(pts[3]);
                    mesh.indices.append(base);
                    mesh.indices.append(base + 2);
                    mesh.indices.append(base + 3);
                }

                if (!mesh.vertices.isEmpty()) m_scene.meshes.append(mesh);

            } else if (currentEntityType == "POLYLINE" || currentEntityType == "LWPOLYLINE") {
                DXFMesh mesh;
                mesh.name = QString("Polyline_%1").arg(m_scene.meshes.size());
                parsePolyline(groups, pos, mesh);
                if (!mesh.vertices.isEmpty()) m_scene.meshes.append(mesh);

            } else if (currentEntityType == "LINE") {
                DXFMesh mesh;
                mesh.name = QString("Line_%1").arg(m_scene.meshes.size());
                Vec3 p1, p2;
                QString layer;

                while (pos < groups.size()) {
                    if (groups[pos].first == 0) break;
                    int code = groups[pos].first;
                    float val = groups[pos].second.toFloat();
                    if (code == 10) p1.x = val; else if (code == 20) p1.y = val; else if (code == 30) p1.z = val;
                    else if (code == 11) p2.x = val; else if (code == 21) p2.y = val; else if (code == 31) p2.z = val;
                    else if (code == 8) layer = groups[pos].second;
                    pos++;
                }

                mesh.vertices.append(p1);
                mesh.vertices.append(p2);
                mesh.indices.append(0);
                mesh.indices.append(1);
                mesh.layerName = layer;
                if (!mesh.vertices.isEmpty()) m_scene.meshes.append(mesh);

            } else if (currentEntityType == "INSERT") {
                parseInsert(groups, pos);
            } else {
                // Skip unknown entity
                while (pos < groups.size()) {
                    if (groups[pos].first == 0) break;
                    pos++;
                }
            }
        } else {
            pos++;
        }
    }
}

void CADDXFParser::parsePolyline(const QVector<QPair<int, QString>>& groups, int& pos, DXFMesh& mesh)
{
    int flags = 0;
    double elev = 0;
    double thickness = 0;

    while (pos < groups.size()) {
        if (groups[pos].first == 0) {
            QString name = groups[pos].second;
            if (name == "VERTEX" || name == "SEQEND") break;
            if (name == "END") break;
        }
        if (groups[pos].first == 70) flags = groups[pos].second.toInt();
        else if (groups[pos].first == 38) elev = groups[pos].second.toDouble();
        else if (groups[pos].first == 39) thickness = groups[pos].second.toDouble();
        else if (groups[pos].first == 8) mesh.layerName = groups[pos].second;
        pos++;
    }

    // Collect vertices
    bool poly3D = (flags & 8) || (flags & 16);
    QVector<Vec3> verts;

    while (pos < groups.size()) {
        if (groups[pos].first == 0) {
            if (groups[pos].second == "SEQEND" || groups[pos].second == "END") { pos++; break; }
            if (groups[pos].second == "VERTEX") {
                pos++;
                Vec3 v;
                while (pos < groups.size()) {
                    if (groups[pos].first == 0) break;
                    if (groups[pos].first == 10) v.x = groups[pos].second.toFloat();
                    else if (groups[pos].first == 20) v.y = groups[pos].second.toFloat();
                    else if (groups[pos].first == 30) v.z = groups[pos].second.toFloat();
                    pos++;
                }
                if (!poly3D) v.z = static_cast<float>(elev);
                verts.append(v);
                continue;
            }
        }
        pos++;
    }

    bool closed = (flags & 1) != 0;
    for (int i = 0; i + 1 < verts.size(); ++i) {
        int base = mesh.vertices.size();
        mesh.vertices.append(verts[i]);
        mesh.vertices.append(verts[i + 1]);
        mesh.indices.append(base);
        mesh.indices.append(base + 1);
    }
    if (closed && verts.size() >= 2) {
        int base = mesh.vertices.size();
        mesh.vertices.append(verts.last());
        mesh.vertices.append(verts.first());
        mesh.indices.append(base);
        mesh.indices.append(base + 1);
    }
}

void CADDXFParser::parseInsert(const QVector<QPair<int, QString>>& groups, int& pos)
{
    // BLOCK INSERT - simplified, just records the entity
    DXFEntity ent;
    ent.type = "INSERT";

    while (pos < groups.size()) {
        if (groups[pos].first == 0) break;
        ent.values[groups[pos].first] = groups[pos].second;
        pos++;
    }

    m_scene.entities.append(ent);
}

} // namespace ks