#pragma once

#include <QVector3D>
#include <QMap>
#include <QVector>
#include <QPair>

namespace ks {

class SceneObject;

// World-space collision triangle used to push cloth out of solid objects.
struct ClothColliderTri {
    QVector3D a, b, c;
    QVector3D n; // outward normal
};

// Verlet-based cloth simulation for mesh objects.
// Each cloth entry snapshots the object's vertices as the rest pose, builds
// distance constraints from the mesh edges and integrates them each step,
// writing the deformed positions back to the SceneMesh. Supports collision
// against a set of solid mesh objects (world-space triangles) and optional
// cloth self-collision.
class ClothSystem {
public:
    ClothSystem();
    ~ClothSystem() = default;

    // Snapshot object mesh (rest pose) and build the spring network.
    // `pinMode`: 0 = none, 1 = top row (highest vertices), 2 = all.
    bool addCloth(int objectId, SceneObject* obj, int pinMode = 1);
    bool removeCloth(int objectId);
    void removeAll();
    bool hasCloth(int objectId) const { return m_entries.contains(objectId); }
    int count() const { return m_entries.size(); }
    QVector<int> clothIds() const { return m_entries.keys(); }
    int pinModeOf(int objectId) const;
    int springCount(int objectId) const;

    // Collision: solid mesh triangles (world space) that the cloth collides with.
    void setColliders(const QVector<ClothColliderTri>& tris);
    void setCollisionEnabled(int objectId, bool enabled);
    bool collisionEnabled(int objectId) const;
    void setSelfCollision(int objectId, bool enabled);
    bool selfCollision(int objectId) const;

    void setGravity(const QVector3D& g);
    QVector3D gravity() const { return m_gravity; }
    void setStiffness(int objectId, float v);
    void setDamping(int objectId, float v);
    void setWind(int objectId, float v);

    void step(float dt, const QMap<int, SceneObject*>& objects);
    void reset(const QMap<int, SceneObject*>& objects);   // restore rest pose
    void clearAll();

private:
    struct Entry {
        QVector<QVector3D> rest;
        QVector<QVector3D> pos;
        QVector<QVector3D> prev;
        QVector<QPair<int, int>> springs;
        QVector<bool> pinned;
        int pinMode = 1;
        float stiffness = 0.9f;
        float damping = 0.2f;
        float wind = 0.0f;
        float collisionRadius = 0.02f;
        bool active = true;
        bool collide = true;
        bool selfCollide = true;
    };

    QMap<int, Entry*> m_entries;
    QVector3D m_gravity = QVector3D(0.0f, -9.8f, 0.0f);
    QVector<ClothColliderTri> m_colliders;

    void writeBack(Entry& e, SceneObject* obj);
};

} // namespace ks
