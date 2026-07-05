#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QVector3D>

namespace ks {

struct RigVertex {
    QVector3D position;
    QVector3D normal;
};

using RigMeshData = QVector<RigVertex>;

class RigGenerator : public QObject {
    Q_OBJECT
public:
    explicit RigGenerator(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~RigGenerator() = default;

    virtual void generateRig(QJsonObject& config) {}
    virtual bool validateRig(const QJsonObject& config) const { return true; }

    bool generateTireRigFromParams(const QJsonObject& params);
    bool generateEngineRigFromParams(const QJsonObject& params);
    QJsonObject createDefaultTireRigParams();
    QJsonObject createDefaultEngineRigParams();
    bool validateTireRigParams(const QJsonObject& params, QString& error);
    bool validateEngineRigParams(const QJsonObject& params, QString& error);
    void createTireSpringRig(int meshId, const QJsonObject& params);
    void createEngineCrankshaft(int meshId, const QJsonObject& params);
    void createPistonRig(int meshId, const QJsonObject& params);
    void createConnectingRodRig(int meshId, const QJsonObject& params);

    // Procedural mesh generation
    RigMeshData generateTireMesh(double radius, double width, int segments = 24);
    RigMeshData generatePistonMesh(double bore, double height, int segments = 16);
    RigMeshData generateConnectingRodMesh(double length, double bigEndDiam, double smallEndDiam);
    RigMeshData generateCrankshaftMesh(double throw_, double rodLength, int cylinders);

signals:
    void rigGenerated(const QString& type, const QJsonObject& params);
    void rigCreated(const QString& type, int meshId, const QJsonObject& params);
};

} // namespace ks