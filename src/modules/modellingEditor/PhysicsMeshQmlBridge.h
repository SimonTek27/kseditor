#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QQmlEngine>
#include <QQmlContext>

#include "PhysicsMeshGenerator.h"

namespace ks {

// ============================================================================
// PhysicsMeshQmlBridge - QML interface for Physics Mesh Generator
// ============================================================================
// Exposes physics mesh generation capabilities to QML UI.
// Provides methods for generating, validating, and exporting collision meshes.

class PhysicsMeshQmlBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isGenerated READ isGenerated NOTIFY generationStateChanged)
    Q_PROPERTY(int meshCount READ meshCount NOTIFY meshCountChanged)
    Q_PROPERTY(float simplificationRatio READ simplificationRatio WRITE setSimplificationRatio NOTIFY simplificationRatioChanged)
    Q_PROPERTY(int maxConvexParts READ maxConvexParts WRITE setMaxConvexParts NOTIFY maxConvexPartsChanged)
    Q_PROPERTY(bool useVHACD READ useVHACD WRITE setUseVHACD NOTIFY useVHACDChanged)
    Q_PROPERTY(QStringList meshNames READ meshNames NOTIFY meshCountChanged)
    Q_PROPERTY(QVariantList meshInfo READ meshInfo NOTIFY meshCountChanged)

public:
    explicit PhysicsMeshQmlBridge(QObject* parent = nullptr);
    ~PhysicsMeshQmlBridge();

    bool isGenerated() const { return m_isGenerated; }
    int meshCount() const;
    float simplificationRatio() const { return m_generator.simplificationRatio(); }
    int maxConvexParts() const { return m_generator.maxConvexParts(); }
    bool useVHACD() const { return m_generator.useVHACD(); }
    QStringList meshNames() const;
    QVariantList meshInfo() const;

    // QML-callable methods
    Q_INVOKABLE void generateFromMesh(const QVariant& vertices, const QVariant& faces, int collisionType);
    Q_INVOKABLE void generateChassis(const QVariant& vertices, const QVariant& faces);
    Q_INVOKABLE void generateSuspension(const QVariant& vertices, const QVariant& faces);
    Q_INVOKABLE void generateWheels(const QVariant& vertices, const QVariant& faces);
    Q_INVOKABLE void generateInterior(const QVariant& vertices, const QVariant& faces);
    Q_INVOKABLE void generateExtra(const QVariant& vertices, const QVariant& faces, const QString& name);

    Q_INVOKABLE void generateFullCar(const QVariant& chassisVerts, const QVariant& chassisFaces,
                                      const QVariant& suspensionVerts, const QVariant& suspensionFaces,
                                      const QVariant& wheelVerts, const QVariant& wheelFaces);

    Q_INVOKABLE QVariantMap getMeshData(const QString& meshName) const;
    Q_INVOKABLE QVariantList getMeshVertices(const QString& meshName) const;
    Q_INVOKABLE QVariantList getMeshFaces(const QString& meshName) const;

    Q_INVOKABLE void setSimplificationRatio(float ratio);
    Q_INVOKABLE void setMaxConvexParts(int maxParts);
    Q_INVOKABLE void setUseVHACD(bool use);

    Q_INVOKABLE QVariantMap validateMesh(const QString& meshName) const;
    Q_INVOKABLE QVariantMap validateFullCar() const;

    Q_INVOKABLE bool exportAcPhysicsMesh(const QString& filePath) const;
    Q_INVOKABLE bool exportObj(const QString& filePath, int collisionType) const;
    Q_INVOKABLE bool exportJson(const QString& filePath) const;
    Q_INVOKABLE bool importJson(const QString& filePath);

    Q_INVOKABLE void clearAll();

    Q_INVOKABLE QStringList getCollisionTypeNames() const;
    Q_INVOKABLE int getCollisionTypeValue(const QString& name) const;

signals:
    void generationStateChanged();
    void meshCountChanged();
    void simplificationRatioChanged();
    void maxConvexPartsChanged();
    void useVHACDChanged();
    void meshGenerated(const QString& meshName);
    void error(const QString& message);
    void generationProgress(int percent);

private slots:
    void onMeshGenerated();
    void onGenerationError(const QString& message);

private:
    PhysicsMeshGenerator m_generator;
    bool m_isGenerated = false;

    // Convert QML data to internal format
    QVector<PhysicsMeshGenerator::PhysicsVertex> qmlToVertices(const QVariant& data) const;
    QVector<PhysicsMeshGenerator::PhysicsFace> qmlToFaces(const QVariant& data) const;

    // Convert internal format to QML data
    QVariantList verticesToQml(const QVector<PhysicsMeshGenerator::PhysicsVertex>& vertices) const;
    QVariantList facesToQml(const QVector<PhysicsMeshGenerator::PhysicsFace>& faces) const;

    PhysicsMeshGenerator::CollisionType intToCollisionType(int type) const;
};

} // namespace ks
