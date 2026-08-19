#pragma once

#include <QVector3D>
#include <QMap>
#include <QVector>

namespace ks {

class SceneObject;

// Runtime rigid body dynamics (Bullet) for scene objects.
// Each body wraps a SceneObject, gets an initial pose from it and writes its
// transform back to the object on every step.
class RigidBodySystem {
public:
    RigidBodySystem();
    ~RigidBodySystem();
    RigidBodySystem(const RigidBodySystem&) = delete;
    RigidBodySystem& operator=(const RigidBodySystem&) = delete;

    void setGravity(const QVector3D& g);
    QVector3D gravity() const;

    // Shape types used when creating a body from an object's mesh.
    enum ShapeType { Box = 0, Sphere = 1, ConvexHull = 2 };

    // Creates a collision shape from the object's mesh bounds (Box/Sphere) or
    // vertices (ConvexHull) and adds a rigid body at the object's current pose.
    // `kinematic=true` → mass forced to 0 and follows the object each step.
    // Otherwise `mass=0` → static, `mass>0` → dynamic (simulated).
    bool addBody(int objectId, SceneObject* obj, int shapeType, float mass, bool kinematic);
    bool removeBody(int objectId);
    void removeAll();
    bool hasBody(int objectId) const { return m_entries.contains(objectId); }
    int count() const { return m_entries.size(); }
    QVector<int> bodyIds() const { return m_entries.keys(); }
    int shapeTypeOf(int objectId) const;

    void setBodyKinematic(int objectId, bool kinematic);
    bool setBodyMass(int objectId, float mass);

    void sync(const QMap<int, SceneObject*>& objects);                    // object → body (kinematic/static)
    void step(float dt, const QMap<int, SceneObject*>& objects);          // advance world + write back
    void reset(const QMap<int, SceneObject*>& objects);                   // teleport bodies, zero velocities
    void clearAll();

private:
    struct Entry;
    QMap<int, Entry*> m_entries;
    void* m_world = nullptr; // btDiscreteDynamicsWorld* kept opaque here
};

} // namespace ks