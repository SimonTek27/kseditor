#include "CADPLYParser.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>

namespace ks {

bool CADPLYParser::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 15) {
        m_lastError = "File too small";
        return false;
    }

    QString header = QString::fromLatin1(data.left(200));
    if (!header.startsWith("ply", Qt::CaseInsensitive)) {
        m_lastError = "Not a valid PLY file";
        return false;
    }

    if (header.contains("format ascii 1.0")) {
        m_scene.isBinary = false;
        return parseASCII(data);
    } else if (header.contains("format binary_little_endian 1.0")) {
        m_scene.isBinary = true;
        return parseBinaryLE(data);
    } else if (header.contains("format binary_big_endian 1.0")) {
        m_scene.isBinary = true;
        m_lastError = "Big-endian PLY not supported";
        return false;
    }

    m_lastError = "Unknown PLY format";
    return false;
}

bool CADPLYParser::parseASCII(const QByteArray& data)
{
    QString content = QString::fromLatin1(data);
    QStringList lines = content.split('\n', Qt::SkipEmptyParts);

    int lineIdx = 0;
    int vertexCount = 0, faceCount = 0;
    bool hasNormals = false, hasTexCoords = false, hasColors = false;
    QVector<QString> vertexProps;

    for (; lineIdx < lines.size(); ++lineIdx) {
        QString line = lines[lineIdx].trimmed();
        if (line == "end_header") { ++lineIdx; break; }

        if (line.startsWith("element vertex")) {
            vertexCount = line.mid(QString("element vertex").length()).trimmed().toInt();
        } else if (line.startsWith("element face")) {
            faceCount = line.mid(QString("element face").length()).trimmed().toInt();
        } else if (line.startsWith("property")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString propName = parts.last();
                if (propName == "nx" || propName == "ny" || propName == "nz") hasNormals = true;
                if (propName == "s" || propName == "t" || propName == "u" || propName == "v") hasTexCoords = true;
                if (propName == "red" || propName == "green" || propName == "blue" || propName == "r" || propName == "g" || propName == "b") hasColors = true;
                vertexProps.append(propName);
            }
        }
    }

    m_scene.vertexCount = vertexCount;
    m_scene.faceCount = faceCount;

    auto getPropertyIndex = [&](const QString& name) -> int {
        for (int i = 0; i < vertexProps.size(); ++i)
            if (vertexProps[i] == name) return i;
        return -1;
    };

    int px = getPropertyIndex("x"), py = getPropertyIndex("y"), pz = getPropertyIndex("z");
    int nx = getPropertyIndex("nx"), ny = getPropertyIndex("ny"), nz = getPropertyIndex("nz");
    int uIdx = getPropertyIndex("s");
    if (uIdx < 0) uIdx = getPropertyIndex("u");
    int vIdx = getPropertyIndex("t");
    if (vIdx < 0) vIdx = getPropertyIndex("v");
    int rIdx = getPropertyIndex("red");
    if (rIdx < 0) rIdx = getPropertyIndex("r");
    int gIdx = getPropertyIndex("green");
    if (gIdx < 0) gIdx = getPropertyIndex("g");
    int bIdx = getPropertyIndex("blue");
    if (bIdx < 0) bIdx = getPropertyIndex("b");

    for (int i = 0; i < vertexCount && lineIdx < lines.size(); ++i, ++lineIdx) {
        QStringList vals = lines[lineIdx].trimmed().split(' ', Qt::SkipEmptyParts);
        PLYVertex v;
        if (px >= 0 && px < vals.size()) v.position.x = vals[px].toFloat();
        if (py >= 0 && py < vals.size()) v.position.y = vals[py].toFloat();
        if (pz >= 0 && pz < vals.size()) v.position.z = vals[pz].toFloat();
        if (nx >= 0 && nx < vals.size()) v.normal.x = vals[nx].toFloat();
        if (ny >= 0 && ny < vals.size()) v.normal.y = vals[ny].toFloat();
        if (nz >= 0 && nz < vals.size()) v.normal.z = vals[nz].toFloat();
        if (uIdx >= 0 && uIdx < vals.size()) v.texCoord.x = vals[uIdx].toFloat();
        if (vIdx >= 0 && vIdx < vals.size()) v.texCoord.y = vals[vIdx].toFloat();
        if (rIdx >= 0 && rIdx < vals.size()) { v.color.x = vals[rIdx].toFloat() / 255.0f; v.hasColor = true; }
        if (gIdx >= 0 && gIdx < vals.size()) v.color.y = vals[gIdx].toFloat() / 255.0f;
        if (bIdx >= 0 && bIdx < vals.size()) v.color.z = vals[bIdx].toFloat() / 255.0f;
        m_scene.mesh.vertices.append(v);
    }

    for (int i = 0; i < faceCount && lineIdx < lines.size(); ++i, ++lineIdx) {
        QStringList vals = lines[lineIdx].trimmed().split(' ', Qt::SkipEmptyParts);
        if (vals.isEmpty()) continue;
        int nVerts = vals[0].toInt();
        PLYFace face;
        for (int j = 1; j <= nVerts && j < vals.size(); ++j) {
            face.vertexIndices.append(vals[j].toInt());
        }
        m_scene.mesh.faces.append(face);
    }

    return true;
}

bool CADPLYParser::parseBinaryLE(const QByteArray& data)
{
    int pos = 0;
    int vertexCount = 0, faceCount = 0;
    bool hasNormals = false, hasTexCoords = false, hasColors = false;
    QVector<QString> vertexProps;
    QVector<int> propSizes;
    int headerEnd = 0;

    QString headerContent;
    int newlinePos;
    while ((newlinePos = data.indexOf('\n', pos)) >= 0) {
        QByteArray lineData = data.mid(pos, newlinePos - pos);
        if (pos > 0 && data[pos - 1] == '\r') {
            lineData = data.mid(pos, newlinePos - pos - 1);
        }
        QString line = QString::fromLatin1(lineData).trimmed();
        pos = newlinePos + 1;

        if (pos > 0 && data[pos - 2] == '\r') ++pos;

        if (line == "end_header") { headerEnd = pos; break; }

        if (line.startsWith("element vertex")) {
            vertexCount = line.mid(QString("element vertex").length()).trimmed().toInt();
        } else if (line.startsWith("element face")) {
            faceCount = line.mid(QString("element face").length()).trimmed().toInt();
        } else if (line.startsWith("property")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                QString propName = parts.last();
                vertexProps.append(propName);
                if (parts[1] == "char" || parts[1] == "uchar") propSizes.append(1);
                else if (parts[1] == "short" || parts[1] == "ushort") propSizes.append(2);
                else if (parts[1] == "int" || parts[1] == "uint") propSizes.append(4);
                else if (parts[1] == "float") propSizes.append(4);
                else if (parts[1] == "double") propSizes.append(8);
                else propSizes.append(4);
                if (propName == "nx" || propName == "ny" || propName == "nz") hasNormals = true;
                if (propName == "s" || propName == "t" || propName == "u" || propName == "v") hasTexCoords = true;
                if (propName == "red" || propName == "green" || propName == "blue" || propName == "r" || propName == "g" || propName == "b") hasColors = true;
            }
        }
    }

    m_scene.vertexCount = vertexCount;
    m_scene.faceCount = faceCount;

    int vertexStride = 0;
    for (int s : propSizes) vertexStride += s;

    auto getPropOffset = [&](const QString& name) -> int {
        int offset = 0;
        for (int i = 0; i < vertexProps.size(); ++i) {
            if (vertexProps[i] == name) return offset;
            offset += propSizes[i];
        }
        return -1;
    };

    int offPx = getPropOffset("x"), offPy = getPropOffset("y"), offPz = getPropOffset("z");
    int offNx = getPropOffset("nx"), offNy = getPropOffset("ny"), offNz = getPropOffset("nz");
    int offU = getPropOffset("s"); if (offU < 0) offU = getPropOffset("u");
    int offV = getPropOffset("t"); if (offV < 0) offV = getPropOffset("v");
    int offR = getPropOffset("red"); if (offR < 0) offR = getPropOffset("r");
    int offG = getPropOffset("green"); if (offG < 0) offG = getPropOffset("g");
    int offB = getPropOffset("blue"); if (offB < 0) offB = getPropOffset("b");

    pos = headerEnd;
    for (int i = 0; i < vertexCount && pos + vertexStride <= data.size(); ++i, pos += vertexStride) {
        PLYVertex v;
        auto readFloat = [&](int offset) -> float {
            if (offset < 0) return 0.0f;
            float val;
            memcpy(&val, data.constData() + pos + offset, sizeof(float));
            return val;
        };

        v.position.x = readFloat(offPx);
        v.position.y = readFloat(offPy);
        v.position.z = readFloat(offPz);
        v.normal.x = readFloat(offNx);
        v.normal.y = readFloat(offNy);
        v.normal.z = readFloat(offNz);
        v.texCoord.x = readFloat(offU);
        v.texCoord.y = readFloat(offV);

        if (offR >= 0 && offG >= 0 && offB >= 0) {
            uint8_t r = *(uint8_t*)(data.constData() + pos + offR);
            uint8_t g = *(uint8_t*)(data.constData() + pos + offG);
            uint8_t b = *(uint8_t*)(data.constData() + pos + offB);
            v.color = Vec3(r / 255.0f, g / 255.0f, b / 255.0f);
            v.hasColor = true;
        }

        m_scene.mesh.vertices.append(v);
    }

    for (int i = 0; i < faceCount && pos < data.size(); ++i) {
        uint8_t nVerts;
        if (pos + 1 > data.size()) break;
        memcpy(&nVerts, data.constData() + pos, 1);
        pos += 1;

        PLYFace face;
        for (int j = 0; j < nVerts && pos + 4 <= data.size(); ++j, pos += 4) {
            int idx;
            memcpy(&idx, data.constData() + pos, 4);
            face.vertexIndices.append(idx);
        }
        m_scene.mesh.faces.append(face);
    }

    return true;
}

} // namespace ks