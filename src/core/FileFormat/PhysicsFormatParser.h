#pragma once

#include <QString>
#include <QVector>
#include "Math/MathCore.h"

namespace ks {

struct PhysicsConvexHull {
    QString name;
    QVector<Vec3> vertices;
    QVector<uint32_t> indices;
    float mass = 1.0f;
    Vec3 centerOfMass;
    float volume = 0.0f;
    Vec3 inertiaTensor;
};

struct PhysicsShape {
    QString name;
    QString type; // "box", "sphere", "capsule", "cylinder", "convex", "mesh", "compound"
    float dimensions[3] = {1, 1, 1};
    float radius = 0.5f;
    float height = 1.0f;
    float position[3] = {0, 0, 0};
    float rotation[3] = {0, 0, 0};
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.1f;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    bool isTrigger = false;
    bool isKinematic = false;
    QString collisionGroup = "default";
    QString collisionMask = "all";
    PhysicsConvexHull convexHull;
};

struct PhysicsConstraint {
    QString name;
    QString type; // "fixed", "hinge", "slider", "spring", "ball"
    int bodyA = -1;
    int bodyB = -1;
    float pivotA[3], pivotB[3];
    float axisA[3], axisB[3];
    float lowLimit = -1.0f;
    float highLimit = 1.0f;
    float stiffness = 0.0f;
    float damping = 0.0f;
    bool enableLimit = false;
    bool enableMotor = false;
    float motorTarget = 0.0f;
    float motorMaxForce = 0.0f;
};

struct PhysicsFile {
    QString version = "1.0";
    float gravity[3] = {0, -9.81f, 0};
    float defaultFriction = 0.5f;
    float defaultRestitution = 0.1f;
    QVector<PhysicsShape> shapes;
    QVector<PhysicsConstraint> constraints;
    QMap<QString, QString> metadata;
};

class PhysicsFormatParser {
public:
    static bool load(const QString& filePath, PhysicsFile& outPhysics);
    static bool save(const QString& filePath, const PhysicsFile& physics);
    static QString lastError() { return s_lastError; }

private:
    static QString s_lastError;

    static PhysicsShape parseShape(const class QJsonObject& obj);
    static class QJsonObject serializeShape(const PhysicsShape& shape);
    static PhysicsConstraint parseConstraint(const class QJsonObject& obj);
    static class QJsonObject serializeConstraint(const PhysicsConstraint& constraint);
};

} // namespace ks