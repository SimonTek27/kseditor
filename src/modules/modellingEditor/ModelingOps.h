#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QMap>
#include <QImage>

#include "Geometry3D.h"

namespace ks {
namespace geometry {

class Modeling3D : public QObject
{
    Q_OBJECT
public:
    explicit Modeling3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Modeling3D() {}

    enum PrimitiveType { Cube, Sphere, Cylinder, Cone, Torus, Plane, Circle };

    Mesh3D* createPrimitive(PrimitiveType type);

    Mesh3D* createCube(float width = 1.0f, float height = 1.0f, float depth = 1.0f);
    Mesh3D* createSphere(float radius = 1.0f, int segments = 32, int rings = 16);
    Mesh3D* createCylinder(float radius = 1.0f, float height = 2.0f, int segments = 32);
    Mesh3D* createCone(float radius = 1.0f, float height = 2.0f, int segments = 32);
    Mesh3D* createTorus(float majorRadius = 1.0f, float minorRadius = 0.3f, int majorSegs = 32, int minorSegs = 16);
    Mesh3D* createPlane(float width = 2.0f, float height = 2.0f, int subdivisions = 1);
    Mesh3D* createCircle(float radius = 1.0f, int segments = 32);

    Mesh3D* createFromHeightmap(const QImage& image, float heightScale = 1.0f);

    void extrude(Mesh3D* mesh, const QVector3D& direction, float distance);
    void bevel(Mesh3D* mesh, float distance, int segments = 1);
    void inset(Mesh3D* mesh, float distance);

    void subdivide(Mesh3D* mesh, int levels);
    void decimate(Mesh3D* mesh, float ratio);
    void triangulate(Mesh3D* mesh);

    void mirror(Mesh3D* mesh, const QVector3D& axis, float pivot = 0.0f);
    void array(Mesh3D* mesh, int count, const QVector3D& offset);
    void screw(Mesh3D* mesh, int steps, float angle, float height);

    enum UVProjection { Planar, Cylindrical, Spherical, Cubic };
    void generateUVs(Mesh3D* mesh, UVProjection projection);

 signals:
    void meshCreated(Mesh3D* mesh);
    void meshModified(Mesh3D* mesh);
};

class Skeleton3D : public QObject
{
    Q_OBJECT
public:
    explicit Skeleton3D(QObject* parent = nullptr) : QObject(parent) {}
    ~Skeleton3D() {}

    struct Bone {
        QString id;
        QString name;
        QString parentId;
        QVector3D head;
        QVector3D tail;
        QQuaternion rotation;
        float length;
        QMatrix4x4 worldMatrix;
    };

    QMatrix4x4 getBoneWorldMatrix(const QString& boneId) const;

    QString addBone(const QString& name, const QString& parentId = QString());
    void removeBone(const QString& boneId);
    Bone getBone(const QString& boneId) const;

    void setBonePosition(const QString& boneId, const QVector3D& pos);
    void setBoneRotation(const QString& boneId, const QQuaternion& rot);

    void calculateFK();
    void calculateIK(const QString& targetBoneId, const QVector3D& targetPos);

    QVector<Bone> allBones() const { return m_bones.values(); }

 signals:
    void boneAdded(const QString& boneId);
    void boneRemoved(const QString& boneId);
    void fkUpdated();

private:
    QMap<QString, Bone> m_bones;
};

} // namespace geometry
} // namespace ks
