#include "CAD3DSParser.h"
#include <QFile>
#include <QDataStream>
#include <QFileInfo>

namespace ks {

static quint16 readWord(const QByteArray& data, int pos)
{
    if (pos + 2 > data.size()) return 0;
    return *reinterpret_cast<const quint16*>(data.constData() + pos);
}

static quint32 readDWord(const QByteArray& data, int pos)
{
    if (pos + 4 > data.size()) return 0;
    return *reinterpret_cast<const quint32*>(data.constData() + pos);
}

static float readFloat(const QByteArray& data, int pos)
{
    if (pos + 4 > data.size()) return 0.0f;
    float v;
    memcpy(&v, data.constData() + pos, 4);
    return v;
}

static QString readCStr(const QByteArray& data, int& pos)
{
    QString result;
    while (pos < data.size() && data[pos] != 0) {
        result += QChar(data[pos]);
        pos++;
    }
    if (pos < data.size()) pos++;
    return result;
}

// 3DS chunk IDs
enum {
    CHUNK_MAIN = 0x4D4D,
    CHUNK_3DEDITOR = 0x3D3D,
    CHUNK_KEYFRAMER = 0xB000,
    CHUNK_OBJECT = 0x4000,
    CHUNK_TRIMESH = 0x4100,
    CHUNK_VERTLIST = 0x4110,
    CHUNK_FACELIST = 0x4120,
    CHUNK_FACEMAT = 0x4130,
    CHUNK_MAPLIST = 0x4140,
    CHUNK_SMOOTHGROUP = 0x4150,
    CHUNK_MATNAME = 0xAFFF,
    CHUNK_MATAMBIENT = 0xA010,
    CHUNK_MATDIFFUSE = 0xA020,
    CHUNK_MATSPECULAR = 0xA030,
    CHUNK_MATSHININESS = 0xA040,
    CHUNK_MATSHIN2PCT = 0xA041,
    CHUNK_MATTRANSPARENCY = 0xA050,
    CHUNK_MATCOLOR_RGB = 0x0010,
    CHUNK_MATCOLOR_FLOAT = 0x0012,
    CHUNK_MAPTEXTURE1 = 0xA200,
    CHUNK_MAPTEXTURE2 = 0xA230,
    CHUNK_MAPFILENAME = 0xA300,
    CHUNK_TRACKPOS = 0xB020,
    CHUNK_TRACKROT = 0xB021,
    CHUNK_TRACKSCALE = 0xB022,
    CHUNK_COLOR_RGB = 0x0010,
    CHUNK_COLOR_F = 0x0012,
    CHUNK_PERCENT_INT = 0x0030,
    CHUNK_PERCENT_FLOAT = 0x0031
};

bool CAD3DSParser::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.size() < 6) {
        m_lastError = "File too small";
        return false;
    }

    m_scene.name = QFileInfo(filePath).baseName();

    int pos = 0;
    return parseChunks(data, pos, data.size());
}

bool CAD3DSParser::parseChunks(const QByteArray& data, int& pos, int endPos)
{
    while (pos + 6 <= endPos) {
        quint16 id = readWord(data, pos);
        quint32 len = readDWord(data, pos + 2);
        if (len < 6 || pos + (int)len > endPos) {
            pos += 6;
            continue;
        }
        int chunkEnd = pos + len;

        switch (id) {
        case CHUNK_MAIN:
        case CHUNK_3DEDITOR:
        case CHUNK_KEYFRAMER:
            pos += 6;
            parseChunks(data, pos, chunkEnd);
            break;

        case CHUNK_OBJECT: {
            pos += 6;
            QString objName = readCStr(data, pos);
            TDSObject obj;
            obj.name = objName;
            int objStart = pos;
            while (pos < chunkEnd) {
                if (pos + 6 > chunkEnd) break;
                quint16 subId = readWord(data, pos);
                quint32 subLen = readDWord(data, pos + 2);
                if (subLen < 6) break;
                int subEnd = pos + subLen;
                if (subId == CHUNK_TRIMESH) {
                    pos += 6;
                    parseTriMesh(data, pos, subEnd, obj.mesh);
                } else if (subId == CHUNK_TRACKPOS || subId == CHUNK_TRACKROT || subId == CHUNK_TRACKSCALE) {
                    // Skip keyframe track data
                    pos += subLen;
                } else {
                    pos += subLen;
                }
            }
            m_scene.objects.append(obj);
            pos = chunkEnd;
            break;
        }

        case CHUNK_MATNAME:
        case CHUNK_MATAMBIENT:
        case CHUNK_MATDIFFUSE:
        case CHUNK_MATSPECULAR:
        case CHUNK_MATSHININESS:
        case CHUNK_MATSHIN2PCT:
        case CHUNK_MATTRANSPARENCY:
        case CHUNK_MAPTEXTURE1:
        case CHUNK_MAPTEXTURE2:
        case CHUNK_MAPFILENAME:
            parseMatEntry(data, pos, chunkEnd);
            break;

        default:
            pos += (int)len;
            break;
        }

        if (pos < chunkEnd) pos = chunkEnd;
    }

    return true;
}

bool CAD3DSParser::parseNamedObject(const QByteArray& data, int& pos, int endPos)
{
    return parseChunks(data, pos, endPos);
}

bool CAD3DSParser::parseTriMesh(const QByteArray& data, int& pos, int endPos, TDSMesh& mesh)
{
    QVector<Vec3> vertices;
    QVector<uint32_t> faceIndices;
    QVector<Vec2> texCoords;

    while (pos + 6 <= endPos) {
        quint16 id = readWord(data, pos);
        quint32 len = readDWord(data, pos + 2);
        if (len < 6) break;
        int chunkEnd = pos + len;

        switch (id) {
        case CHUNK_VERTLIST: {
            pos += 6;
            quint16 count = readWord(data, pos); pos += 2;
            for (int i = 0; i < count && pos + 12 <= chunkEnd; ++i) {
                Vec3 v;
                v.x = readFloat(data, pos); pos += 4;
                v.y = readFloat(data, pos); pos += 4;
                v.z = readFloat(data, pos); pos += 4;
                vertices.append(v);
            }
            break;
        }
        case CHUNK_FACELIST: {
            pos += 6;
            quint16 count = readWord(data, pos); pos += 2;
            for (int i = 0; i < count && pos + 8 <= chunkEnd; ++i) {
                uint32_t i1 = readWord(data, pos); pos += 2;
                uint32_t i2 = readWord(data, pos); pos += 2;
                uint32_t i3 = readWord(data, pos); pos += 2;
                pos += 2; // face flags
                faceIndices.append(i1);
                faceIndices.append(i2);
                faceIndices.append(i3);
            }
            break;
        }
        case CHUNK_MAPLIST: {
            pos += 6;
            quint16 count = readWord(data, pos); pos += 2;
            for (int i = 0; i < count && pos + 8 <= chunkEnd; ++i) {
                Vec2 uv;
                uv.x = readFloat(data, pos); pos += 4;
                uv.y = 1.0f - readFloat(data, pos); pos += 4;
                texCoords.append(uv);
            }
            break;
        }
        case CHUNK_FACEMAT: {
            pos += 6;
            QString matName = readCStr(data, pos);
            mesh.materialName = matName;
            quint16 count = readWord(data, pos); pos += 2;
            pos += count * 2; // skip face indices
            break;
        }
        default:
            pos = chunkEnd;
            break;
        }

        if (pos < chunkEnd) pos = chunkEnd;
    }

    // Build mesh data
    for (uint32_t idx : faceIndices) {
        if (idx < (uint32_t)vertices.size()) {
            TDSVertex v;
            v.position = vertices[idx];
            if (idx < (uint32_t)texCoords.size())
                v.texCoord = texCoords[idx];
            mesh.vertices.append(v);
            mesh.indices.append(mesh.vertices.size() - 1);
        }
    }

    return true;
}

bool CAD3DSParser::parseMatEntry(const QByteArray& data, int& pos, int endPos)
{
    static TDSMaterial currentMat;
    static bool inMat = false;

    if (pos + 6 > endPos) return false;
    quint16 id = readWord(data, pos);
    quint32 len = readDWord(data, pos + 2);

    switch (id) {
    case CHUNK_MATNAME: {
        pos += 6;
        TDSMaterial mat;
        mat.name = readCStr(data, pos);
        m_scene.materials.append(mat);
        currentMat = mat;
        inMat = true;
        break;
    }
    case CHUNK_MATAMBIENT:
    case CHUNK_MATDIFFUSE:
    case CHUNK_MATSPECULAR: {
        pos += 6;
        Vec3 color;
        int subPos = pos;
        while (subPos + 6 <= (int)(pos + len - 6)) {
            quint16 subId = readWord(data, subPos);
            quint32 subLen = readDWord(data, subPos + 2);
            if (subId == CHUNK_COLOR_F && subLen >= 14) {
                color.x = readFloat(data, subPos + 6);
                color.y = readFloat(data, subPos + 10);
                color.z = readFloat(data, subPos + 14);
            } else if (subId == CHUNK_COLOR_RGB && subLen >= 9) {
                color.x = (quint8)data[subPos + 6] / 255.0f;
                color.y = (quint8)data[subPos + 7] / 255.0f;
                color.z = (quint8)data[subPos + 8] / 255.0f;
            }
            subPos += subLen;
        }
        if (!m_scene.materials.isEmpty()) {
            auto& mat = m_scene.materials.last();
            if (id == CHUNK_MATAMBIENT) mat.ambient = color;
            else if (id == CHUNK_MATDIFFUSE) mat.diffuse = color;
            else if (id == CHUNK_MATSPECULAR) mat.specular = color;
        }
        pos += len;
        break;
    }
    case CHUNK_MATSHININESS:
    case CHUNK_MATSHIN2PCT: {
        pos += 6;
        int subPos = pos;
        while (subPos + 6 <= (int)(pos + len - 6)) {
            quint16 subId = readWord(data, subPos);
            quint32 subLen = readDWord(data, subPos + 2);
            if (subId == CHUNK_PERCENT_INT && !m_scene.materials.isEmpty()) {
                m_scene.materials.last().shininess = (quint8)data[subPos + 6] / 100.0f;
            } else if (subId == CHUNK_PERCENT_FLOAT && !m_scene.materials.isEmpty()) {
                float val = readFloat(data, subPos + 6);
                m_scene.materials.last().shininess = val;
            }
            subPos += subLen;
        }
        pos += len;
        break;
    }
    case CHUNK_MATTRANSPARENCY: {
        pos += 6;
        int subPos = pos;
        while (subPos + 6 <= (int)(pos + len - 6)) {
            quint16 subId = readWord(data, subPos);
            quint32 subLen = readDWord(data, subPos + 2);
            if (subId == CHUNK_PERCENT_INT && !m_scene.materials.isEmpty()) {
                m_scene.materials.last().transparency = (quint8)data[subPos + 6] / 100.0f;
            } else if (subId == CHUNK_PERCENT_FLOAT && !m_scene.materials.isEmpty()) {
                m_scene.materials.last().transparency = readFloat(data, subPos + 6);
            }
            subPos += subLen;
        }
        pos += len;
        break;
    }
    case CHUNK_MAPFILENAME: {
        pos += 6;
        QString filename = readCStr(data, pos);
        if (!m_scene.materials.isEmpty()) {
            m_scene.materials.last().textureMap1 = filename;
        }
        break;
    }
    default:
        pos += len;
        break;
    }

    return true;
}

} // namespace ks