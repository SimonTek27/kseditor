#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QVector3D>
#include "../../LiveryEditor/LiveryEditorModule.h"

namespace ks {

struct CarPart {
    QString id;
    QString name;
    QString meshFile;
    QString parentId;
    QVector3D position;
    QVector3D rotation;
    QVector3D scale;
    bool visible = true;
};

struct CarConfig {
    QString id;
    QString name;
    QString carClass;
    int maxSpeed;
    float acceleration;
    float handling;
    float braking;
    QMap<QString, CarPart> parts;
};

struct CarPhysicsData {
    float mass = 1200.0f;
    float fuelCapacity = 80.0f;
    float maxPower = 300.0f;
    float maxRpm = 8000.0f;
    float idleRpm = 850.0f;
    float gearRatio[7] = {3.5f, 2.0f, 1.4f, 1.0f, 0.8f, 0.65f, 3.0f};
    float suspensionTravel = 0.15f;
    float suspensionStiffness = 40000.0f;
    float dampingRate = 4000.0f;
    float tireGrip = 1.0f;
    float brakePower = 5000.0f;
    QString driveType = "RWD";

    QJsonObject toJson() const;
    static CarPhysicsData fromJson(const QJsonObject& obj);
};

class CarEditor : public QObject {
    Q_OBJECT
public:
    static CarEditor* instance();

    void newCar(const QString& name);
    bool loadCar(const QString& path);
    bool saveCar(const QString& path);
    bool exportToAC(const QString& outputPath);

    void setCurrentPart(const QString& partId);
    QString currentPart() const { return m_currentPart; }
    QStringList getCarParts() const { return m_parts.keys(); }

    CarPart* getPart(const QString& partId);
    void addPart(const CarPart& part);
    void removePart(const QString& partId);

    void updatePartTransform(const QString& partId, const QVector3D& pos, const QVector3D& rot);

    bool validateCar(QString& errorMsg);
    QVector<QString> getValidationErrors();

    CarConfig getConfig() const;
    void setConfig(const CarConfig& config);

    void loadLivery(const QString& path);
    void saveLivery(const QString& path);

    bool loadLiveryTexture(const QString& skinPath);
    bool saveLiveryTexture(const QImage& texture, const QString& skinPath);
    QStringList getSkins() const;
    bool setCurrentSkin(const QString& skinName);
    LiverySystem::SkinConfig& liveryConfig() { return m_liveryConfig; }
    LiveryPainter* liveryPainter() { return LiveryEditor::instance()->liveryPainter(); }

    CarPhysicsData& physicsData() { return m_physics; }
    const CarPhysicsData& physicsData() const { return m_physics; }

    void loadPhysics(const QString& path);
    void savePhysics(const QString& path);
    
signals:
    void carLoaded(const QString& path);
    void carSaved(const QString& path);
    void partChanged(const QString& partId);
    void carModified();
    void validationComplete(bool valid, const QVector<QString>& errors);
    void liveryLoaded(const QString& skinName);
    void liverySaved(const QString& skinName);

private:
    explicit CarEditor(QObject* parent = nullptr);
    static CarEditor* s_instance;
    
    QString m_currentCar;
    QString m_currentPart;
    QMap<QString, CarPart> m_parts;
    CarConfig m_config;
    
    LiverySystem::SkinConfig m_liveryConfig;
    CarPhysicsData m_physics;
    
    void loadDefaultParts();
    QVector<QString> validateRequiredParts();
    QVector<QString> validateMeshFiles();
    QVector<QString> validateTransforms();
};

class CarPartGenerator {
public:
    static CarPart createWheel(const QString& name, float diameter);
    static CarPart createBody(const QString& name);
    static CarPart createSpoiler(const QString& name);
    static CarPart createEngine(const QString& name);
    static CarPart createInterior(const QString& name);
};

} // namespace ks