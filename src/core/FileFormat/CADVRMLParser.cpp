#include "CADVRMLParser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

namespace ks {

bool CADVRMLParser::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    content = content.trimmed();
    if (!content.startsWith("#VRML")) {
        m_lastError = "Not a valid VRML file";
        return false;
    }

    return parseContent(content);
}

bool CADVRMLParser::parseContent(const QString& content)
{
    m_scene.name = "VRML Scene";

    // Extract VRML version
    QRegularExpression versionRx(R"(#VRML\s+V(\S+))");
    auto vm = versionRx.match(content);
    if (vm.hasMatch()) m_scene.version = vm.captured(1);

    // Extract DEF names and their blocks
    QRegularExpression defRx(R"(DEF\s+(\S+)\s+(\w+)\s*\{)");
    auto defIt = defRx.globalMatch(content);
    QMap<QString, QString> defBlocks;

    while (defIt.hasNext()) {
        auto match = defIt.next();
        QString defName = match.captured(1);
        QString defType = match.captured(2);
        int braceStart = match.capturedEnd(0) - 1;
        int braceDepth = 1;
        int braceEnd = braceStart + 1;
        while (braceDepth > 0 && braceEnd < content.length()) {
            if (content[braceEnd] == '{') braceDepth++;
            else if (content[braceEnd] == '}') braceDepth--;
            braceEnd++;
        }
        QString block = content.mid(braceStart, braceEnd - braceStart);
        defBlocks[defName] = block;
    }

    // Extract inline Shape blocks not inside DEF
    QRegularExpression shapeRx(R"(Shape\s*\{)");
    auto shapeIt = shapeRx.globalMatch(content);
    QSet<int> usedPositions;

    while (shapeIt.hasNext()) {
        auto match = shapeIt.next();
        int braceStart = match.capturedEnd(0) - 1;

        bool insideDef = false;
        for (auto it = defBlocks.begin(); it != defBlocks.end(); ++it) {
            int defPos = content.indexOf("DEF " + it.key());
            if (defPos >= 0 && braceStart > defPos && braceStart < defPos + it.value().length() + 20) {
                insideDef = true;
                break;
            }
        }
        if (insideDef) continue;

        int braceDepth = 1;
        int braceEnd = braceStart + 1;
        while (braceDepth > 0 && braceEnd < content.length()) {
            if (content[braceEnd] == '{') braceDepth++;
            else if (content[braceEnd] == '}') braceDepth--;
            braceEnd++;
        }
        QString block = content.mid(braceStart, braceEnd - braceStart);
        VRMLMesh mesh;
        mesh.name = QString("Mesh_%1").arg(m_scene.meshes.size());
        parseIndexedFaceSet(mesh.name, block, mesh);
        if (!mesh.vertices.isEmpty()) {
            m_scene.meshes.append(mesh);
        }
    }

    // Process DEF blocks that are Shapes
    for (auto it = defBlocks.begin(); it != defBlocks.end(); ++it) {
        if (it.value().contains("IndexedFaceSet") || it.value().contains("IndexedLineSet")) {
            VRMLMesh mesh;
            mesh.name = it.key();
            parseIndexedFaceSet(it.key(), it.value(), mesh);
            if (!mesh.vertices.isEmpty()) {
                m_scene.meshes.append(mesh);
            }
        }
    }

    // Extract materials (Appearance blocks)
    QRegularExpression appearanceRx(R"(Appearance\s*\{([^}]*)\})");
    auto appIt = appearanceRx.globalMatch(content);
    while (appIt.hasNext()) {
        auto match = appIt.next();
        QString block = match.captured(1);
        VRMLMaterial mat;

        QRegularExpression diffuseRx(R"(diffuseColor\s+([\d.eE+-]+)\s+([\d.eE+-]+)\s+([\d.eE+-]+))");
        auto dm = diffuseRx.match(block);
        if (dm.hasMatch()) {
            mat.diffuseColor = Vec3(dm.captured(1).toFloat(), dm.captured(2).toFloat(), dm.captured(3).toFloat());
        }

        QRegularExpression specRx(R"(specularColor\s+([\d.eE+-]+)\s+([\d.eE+-]+)\s+([\d.eE+-]+))");
        auto sm = specRx.match(block);
        if (sm.hasMatch()) {
            mat.specularColor = Vec3(sm.captured(1).toFloat(), sm.captured(2).toFloat(), sm.captured(3).toFloat());
        }

        QRegularExpression shininessRx(R"(shininess\s+([\d.eE+-]+))");
        auto shm = shininessRx.match(block);
        if (shm.hasMatch()) mat.shininess = shm.captured(1).toFloat();

        QRegularExpression transpRx(R"(transparency\s+([\d.eE+-]+))");
        auto tm = transpRx.match(block);
        if (tm.hasMatch()) mat.transparency = tm.captured(1).toFloat();

        mat.name = QString("Material_%1").arg(m_scene.materials.size());
        m_scene.materials[mat.name] = mat;
    }

    return true;
}

bool CADVRMLParser::parseIndexedFaceSet(const QString& defName, const QString& content, VRMLMesh& mesh)
{
    QRegularExpression coordRx(R"(coord\s*Coordinate\s*\{\s*point\s*\[([^\]]*)\])");
    auto cm = coordRx.match(content);
    if (!cm.hasMatch()) {
        QRegularExpression coordDirectRx(R"(coordIndex\s*\[([^\]]*)\])");
        auto cdm = coordDirectRx.match(content);
        if (!cdm.hasMatch()) return false;
    }

    QVector<Vec3> coordPoints;

    QRegularExpression pointRx(R"(point\s*\[([^\]]*)\])");
    auto pm = pointRx.match(content);
    if (pm.hasMatch()) {
        QString pointsStr = pm.captured(1).trimmed();
        QStringList tokens = pointsStr.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
        for (int i = 0; i + 2 < tokens.size(); i += 3) {
            Vec3 v;
            v.x = tokens[i].toFloat();
            v.y = tokens[i + 1].toFloat();
            v.z = tokens[i + 2].toFloat();
            coordPoints.append(v);
        }
    }

    QVector<Vec3> normals;
    QRegularExpression normalRx(R"(normal\s*Normal\s*\{\s*vector\s*\[([^\]]*)\])");
    auto nm = normalRx.match(content);
    if (nm.hasMatch()) {
        QString normStr = nm.captured(1).trimmed();
        QStringList tokens = normStr.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
        for (int i = 0; i + 2 < tokens.size(); i += 3) {
            normals.append(Vec3(tokens[i].toFloat(), tokens[i + 1].toFloat(), tokens[i + 2].toFloat()));
        }
    }

    QVector<Vec2> texCoords;
    QRegularExpression texCoordRx(R"(texCoord\s*TextureCoordinate\s*\{\s*point\s*\[([^\]]*)\])");
    auto tc = texCoordRx.match(content);
    if (tc.hasMatch()) {
        QString tcStr = tc.captured(1).trimmed();
        QStringList tokens = tcStr.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
        for (int i = 0; i + 1 < tokens.size(); i += 2) {
            texCoords.append(Vec2(tokens[i].toFloat(), tokens[i + 1].toFloat()));
        }
    }

    QRegularExpression coordIdxRx(R"(coordIndex\s*\[([^\]]*)\])");
    auto ci = coordIdxRx.match(content);
    if (!ci.hasMatch() || coordPoints.isEmpty()) return false;

    QString idxStr = ci.captured(1).trimmed();
    QStringList tokens = idxStr.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);

    QVector<uint32_t> faceIndices;
    int normalIdx = 0;
    for (const QString& token : tokens) {
        if (token == "-1") {
            for (int k = 2; k < faceIndices.size(); ++k) {
                uint32_t i0 = faceIndices[0];
                uint32_t i1 = faceIndices[k - 1];
                uint32_t i2 = faceIndices[k];

                mesh.vertices.append({coordPoints[i0], (normals.isEmpty() ? Vec3() : normals[normalIdx % normals.size()]), (texCoords.isEmpty() ? Vec2() : texCoords[qMin((int)i0, texCoords.size() - 1)])});
                mesh.vertices.append({coordPoints[i1], (normals.isEmpty() ? Vec3() : normals[(normalIdx + 1) % normals.size()]), (texCoords.isEmpty() ? Vec2() : texCoords[qMin((int)i1, texCoords.size() - 1)])});
                mesh.vertices.append({coordPoints[i2], (normals.isEmpty() ? Vec3() : normals[(normalIdx + 2) % normals.size()]), (texCoords.isEmpty() ? Vec2() : texCoords[qMin((int)i2, texCoords.size() - 1)])});
                mesh.indices.append(mesh.vertices.size() - 3);
                mesh.indices.append(mesh.vertices.size() - 2);
                mesh.indices.append(mesh.vertices.size() - 1);
                normalIdx += 3;
            }
            faceIndices.clear();
        } else {
            faceIndices.append(token.toUInt());
        }
    }

    return !mesh.vertices.isEmpty();
}

} // namespace ks