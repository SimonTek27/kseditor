#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>
#include <QByteArray>

namespace ks {

struct KNHHeader {
    char magic[4] = {'K', 'N', 'H', '\0'};
    quint32 version = 1;
    quint32 flags = 0;
    quint64 dataOffset = 0;
    quint64 dataSize = 0;
};

struct KNHNode {
    QString name;
    QString parentName;
    QMatrix4x4 transform;
    QVector3D position;
    QVector3D rotation;
    QVector3D scale;
    bool visible = true;
};

struct KNHMaterial {
    QString name;
    QString diffuseMap;
    QString normalMap;
    QString specularMap;
    QVector3D ambientColor;
    QVector3D diffuseColor;
    QVector3D specularColor;
    float shininess = 32.0f;
};

struct KNHMesh {
    QString name;
    QString materialName;
    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<QVector2D> uvs;
    QVector<quint32> indices;
    QVector<QVector3D> tangents;
    QVector<QVector4D> colors;
    QVector<QVector<quint32>> subMeshes;
};

struct KNHLOD {
    float distance;
    KNHMesh mesh;
};

struct KNHTexture {
    QString name;
    QString filePath;
    int width = 0;
    int height = 0;
    int channels = 4;
};

struct KNHAnimation {
    QString name;
    float duration = 0.0f;
    int frameRate = 30;
    QMap<QString, QVector<QMatrix4x4>> nodeTracks;
};

struct KNHCollision {
    QString name;
    QVector<QVector3D> vertices;
    QVector<quint32> indices;
};

struct KNHSkeleton {
    QString name;
    QVector<QString> boneNames;
    QVector<int> parentIndices;
    QVector<QMatrix4x4> inverseBindMatrices;
};

class FileFormat {
public:
    static QString formatExtension() { return ".knh"; }
    static QString formatName() { return "KNH"; }
    static QString formatDescription() { return "Kunos Editor Scene Format"; }

    static bool isValidFormat(const QString& path);
    static bool detectFormat(const QByteArray& header);

    static QStringList supportedReadFormats();
    static QStringList supportedWriteFormats();
};

class KNHScene {
public:
    QString name;
    QVector<KNHNode> nodes;
    QVector<KNHMesh> meshes;
    QVector<KNHMaterial> materials;
    QVector<KNHTexture> textures;
    QVector<KNHLOD> lods;
    QVector<KNHAnimation> animations;
    QVector<KNHCollision> collisions;
    QScopedPointer<KNHSkeleton> skeleton;

    void clear();
    bool isEmpty() const;
    int totalVertexCount() const;
    int totalTriangleCount() const;
};

class KNHReader {
public:
    bool read(const QString& path, KNHScene& scene);
    QString errorString() const { return m_error; }

private:
    bool readHeader(QDataStream& stream, KNHHeader& header);
    bool readNodes(QDataStream& stream, KNHScene& scene);
    bool readMeshes(QDataStream& stream, KNHScene& scene);
    bool readMaterials(QDataStream& stream, KNHScene& scene);
    bool readTextures(QDataStream& stream, KNHScene& scene);

    QString m_error;
};

class KNHWriter {
public:
    bool write(const QString& path, const KNHScene& scene);
    bool writeBinary(const QString& path, const KNHScene& scene);
    bool writeJson(const QString& path, const KNHScene& scene);
    QString errorString() const { return m_error; }

private:
    void writeHeader(QDataStream& stream, const KNHHeader& header);
    void writeNodes(QDataStream& stream, const KNHScene& scene);
    void writeMeshes(QDataStream& stream, const KNHScene& scene);
    void writeMaterials(QDataStream& stream, const KNHScene& scene);
    void writeTextures(QDataStream& stream, const KNHScene& scene);

    QString m_error;
};

class KNHConverter {
public:
    static bool convertFromKN5(const QString& kn5Path, const QString& knhPath);
    static bool convertFromGLB(const QString& glbPath, const QString& knhPath);
    static bool convertFromFBX(const QString& fbxPath, const QString& knhPath);
    static bool convertFromOBJ(const QString& objPath, const QString& knhPath);

    static bool toKNH(const QString& inputPath, const QString& outputPath, const QString& sourceFormat = "");
};

}
