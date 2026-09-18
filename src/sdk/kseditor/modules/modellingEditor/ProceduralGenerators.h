#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QColor>
#include <QImage>

namespace ks {

class ProceduralTextureGenerator : public QObject
{
    Q_OBJECT

public:
    explicit ProceduralTextureGenerator(QObject* parent = nullptr);
    ~ProceduralTextureGenerator();

    enum TextureType {
        Type_Marble,
        Type_Wood,
        Type_Concrete,
        Type_Asphalt,
        Type_Grass,
        Type_Metal,
        Type_Carbon,
        Type_Plastic,
        Type_Rust,
        Type_Grunge,
        Type_Cotton,
        Type_Silk,
        Type_Denim,
        Type_Leather,
        Type_Rubber,
        Type_Wool,
        Type_Satin,
        Type_Twill
    };

    struct TextureParams {
        TextureType type = Type_Marble;
        int width = 1024;
        int height = 1024;
        float scale = 1.0f;
        float contrast = 1.0f;
        float brightness = 0.5f;
        QVector3D color1 = QVector3D(1, 1, 1);
        QVector3D color2 = QVector3D(0.5, 0.5, 0.5);
        int seed = 0;
    };

    QImage generateTexture(const TextureParams& params);
    QImage generateNormalMap(const QImage& diffuse);

    static QString textureTypeToString(TextureType type);
    static TextureType stringToTextureType(const QString& str);

 signals:
    void generationComplete(const QImage& texture);

private:
    float fbm(float x, float y, int octaves, int seed);
    float turbulence(float x, float y, int octaves, int seed);
    QVector3D marblePattern(float x, float y, int seed, const TextureParams& params);
    QVector3D woodPattern(float x, float y, int seed);
    QVector3D concretePattern(float x, float y, int seed);
    QVector3D asphaltPattern(float x, float y, int seed);
    QVector3D fabricPattern(float x, float y, int seed, TextureType type, const TextureParams& params);
};

class ProceduralMeshGenerator : public QObject
{
    Q_OBJECT

public:
    explicit ProceduralMeshGenerator(QObject* parent = nullptr);
    ~ProceduralMeshGenerator();

    struct MeshData {
        QVector<QVector3D> vertices;
        QVector<QVector3D> normals;
        QVector<QVector2D> texCoords;
        QVector<QVector3D> indices;
    };

    struct MeshParams {
        enum Primitive { Box, Sphere, Cylinder, Cone, Torus, Plane, Grid } primitive = Box;
        float width = 1.0f;
        float height = 1.0f;
        float depth = 1.0f;
        int segments = 32;
        int rings = 16;
    };

    MeshData generateMesh(const MeshParams& params);
    MeshData generateTerrain(int width, int height, float scale, int seed);

 signals:
    void generationComplete(const MeshData& mesh);

private:
    void generateBox(MeshData& mesh, const MeshParams& params);
    void generateSphere(MeshData& mesh, const MeshParams& params);
    void generateCylinder(MeshData& mesh, const MeshParams& params);
    void generateTorus(MeshData& mesh, const MeshParams& params);
    float heightMap(float x, float z, int seed);
};

class ProceduralTrackGenerator : public QObject
{
    Q_OBJECT

public:
    explicit ProceduralTrackGenerator(QObject* parent = nullptr);
    ~ProceduralTrackGenerator();

    struct TrackParams {
        int numPoints = 20;
        float minRadius = 30.0f;
        float maxRadius = 150.0f;
        float totalLength = 3000.0f;
        float width = 12.0f;
        int seed = 0;
        bool closed = true;
        bool includePitLane = true;
    };

    struct TrackPoint {
        QVector3D position;
        QVector3D tangent;
        QVector3D normal;
        float curvature;
        float width;
    };

    struct TrackData {
        QVector<TrackPoint> points;
        QVector<QVector3D> centerLine;
        QVector<QVector3D> leftEdge;
        QVector<QVector3D> rightEdge;
        float totalLength;
    };

    TrackData generateTrack(const TrackParams& params);
    TrackData addChicanes(const TrackData& baseTrack, int count, float intensity);
    TrackData addCrest(const TrackData& baseTrack, float position, float height);

 signals:
    void generationComplete(const TrackData& track);

private:
    TrackPoint calculateTrackPoint(const QVector<QVector2D>& spline, float t);
    float m_trackWidth = 12.0f;
};

class ProceduralCarGenerator : public QObject
{
    Q_OBJECT

public:
    explicit ProceduralCarGenerator(QObject* parent = nullptr);
    ~ProceduralCarGenerator();

    struct CarParams {
        enum BodyStyle { Sedan, Coupe, SUV, Formula, GT } bodyStyle = Coupe;
        float length = 4.5f;
        float width = 1.8f;
        float height = 1.2f;
        float wheelbase = 2.7f;
        float trackWidth = 1.6f;
        float wheelRadius = 0.33f;
        int detailLevel = 2;
    };

    struct CarModel {
        ProceduralMeshGenerator::MeshData body;
        QVector<ProceduralMeshGenerator::MeshData> wheels;
        QVector<ProceduralMeshGenerator::MeshData> aerodynamics;
    };

    CarModel generateCar(const CarParams& params);

 signals:
    void generationComplete(const CarModel& car);
};

class DecalGenerator : public QObject
{
    Q_OBJECT

public:
    explicit DecalGenerator(QObject* parent = nullptr);
    ~DecalGenerator();

    struct DecalParams {
        enum Type { Number, Logo, Stripe, Circle, Rectangle, Text } type = Number;
        float width = 0.5f;
        float height = 0.5f;
        QString text;
        QString fontFamily = "Arial";
        int fontSize = 48;
        QColor color = Qt::white;
        QColor outlineColor = Qt::black;
        float outlineWidth = 2.0f;
    };

    QImage generateDecal(const DecalParams& params);
    QImage generateNumberPlate(const QString& number, const QString& fontFamily);

 signals:
    void generationComplete(const QImage& decal);
};

} // namespace ks
