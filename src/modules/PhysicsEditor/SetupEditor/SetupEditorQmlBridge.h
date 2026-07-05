#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QVector>
#include <QDateTime>
#include "core/editor/EditorModule.h"

namespace ks {

struct TelemetryDrivingSample {
    QDateTime timestamp;
    float speed = 0;
    float rpm = 0;
    float throttle = 0;
    float brake = 0;
    float steering = 0;
    int gear = 0;
    float lateralG = 0;
    float longitudinalG = 0;
};

struct TelemetryLapForSetup {
    int lapNumber = 0;
    double lapTime = 0;
    double sector1 = 0, sector2 = 0, sector3 = 0;
    float topSpeed = 0;
    float avgSpeed = 0;
    float maxGForce = 0;
    float avgThrottle = 0;
    float avgBrake = 0;
    float avgSteering = 0;
    float throttleTimePercent = 0;
    float brakeTimePercent = 0;
    float coastTimePercent = 0;
    QVector<TelemetryDrivingSample> samples;
};

class SetupEditorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    Q_PROPERTY(int frontBump READ frontBump WRITE setFrontBump NOTIFY frontBumpChanged)
    Q_PROPERTY(int rearBump READ rearBump WRITE setRearBump NOTIFY rearBumpChanged)
    Q_PROPERTY(int frontRebound READ frontRebound WRITE setFrontRebound NOTIFY frontReboundChanged)
    Q_PROPERTY(int rearRebound READ rearRebound WRITE setRearRebound NOTIFY rearReboundChanged)
    Q_PROPERTY(int frontSpring READ frontSpring WRITE setFrontSpring NOTIFY frontSpringChanged)
    Q_PROPERTY(int rearSpring READ rearSpring WRITE setRearSpring NOTIFY rearSpringChanged)
    Q_PROPERTY(int rideHeight READ rideHeight WRITE setRideHeight NOTIFY rideHeightChanged)
    Q_PROPERTY(int frontWing READ frontWing WRITE setFrontWing NOTIFY frontWingChanged)
    Q_PROPERTY(int rearWing READ rearWing WRITE setRearWing NOTIFY rearWingChanged)
    Q_PROPERTY(int preload READ preload WRITE setPreload NOTIFY preloadChanged)
    Q_PROPERTY(int brakeBias READ brakeBias WRITE setBrakeBias NOTIFY brakeBiasChanged)
    Q_PROPERTY(int brakePower READ brakePower WRITE setBrakePower NOTIFY brakePowerChanged)

public:
    static SetupEditorQmlBridge* instance();

    bool isModified() const;

    int frontBump() const;
    int rearBump() const;
    int frontRebound() const;
    int rearRebound() const;
    int frontSpring() const;
    int rearSpring() const;
    int rideHeight() const;
    int frontWing() const;
    int rearWing() const;
    int preload() const;
    int brakeBias() const;
    int brakePower() const;

    Q_INVOKABLE void loadSetup(const QString& path);
    Q_INVOKABLE void saveSetup(const QString& path);
    Q_INVOKABLE void analyzeSetup();
    Q_INVOKABLE void compareWith(const QString& otherPath);
    Q_INVOKABLE void resetToDefault();
    Q_INVOKABLE void applyPreset(const QString& name);
    Q_INVOKABLE void setValue(const QString& key, const QString& value);
    Q_INVOKABLE QString getValue(const QString& key) const;
    Q_INVOKABLE QVariantMap getAllValues() const;
    Q_INVOKABLE QStringList getRecentSetups() const;
    Q_INVOKABLE void exportToCar(const QString& carName, const QString& trackName);
    Q_INVOKABLE QStringList getSetupKeys() const;

    void setFrontBump(int v);
    void setRearBump(int v);
    void setFrontRebound(int v);
    void setRearRebound(int v);
    void setFrontSpring(int v);
    void setRearSpring(int v);
    void setRideHeight(int v);
    void setFrontWing(int v);
    void setRearWing(int v);
    void setPreload(int v);
    void setBrakeBias(int v);
    void setBrakePower(int v);

    // ── Telemetry-driven setup analysis ──────────────────────────────────
    Q_INVOKABLE QVariantMap importTelemetryLap(const QVariantMap& lapData);
    Q_INVOKABLE QVariantList analyzeDrivingStyle();
    Q_INVOKABLE QVariantMap autoOptimizeSetup();
    Q_INVOKABLE QVariantMap getTelemetryAnalysis() const;

signals:
    void statusMessage(const QString& msg);
    void errorMessage(const QString& msg);
    void setupApplied();
    void setupLoaded();
    void setupAnalysisComplete(const QStringList& recommendations);
    void comparisonComplete(const QStringList& differences);
    void valueChanged(const QString& key, const QString& value);
    void modifiedChanged();
    void telemetryOptimizationReady(const QVariantMap& changes);
    void drivingAnalysisComplete(const QVariantList& insights);
    void frontBumpChanged();
    void rearBumpChanged();
    void frontReboundChanged();
    void rearReboundChanged();
    void frontSpringChanged();
    void rearSpringChanged();
    void rideHeightChanged();
    void frontWingChanged();
    void rearWingChanged();
    void preloadChanged();
    void brakeBiasChanged();
    void brakePowerChanged();

private:
    SetupEditorQmlBridge(QObject* parent = nullptr);
    static SetupEditorQmlBridge* s_instance;

    QVariantMap m_currentSetup;
    QString m_currentSetupPath;
    QString m_setupDir;
    bool m_isModified = false;

    int m_frontBump = 50;
    int m_rearBump = 50;
    int m_frontRebound = 50;
    int m_rearRebound = 50;
    int m_frontSpring = 50;
    int m_rearSpring = 50;
    int m_rideHeight = 50;
    int m_frontWing = 50;
    int m_rearWing = 50;
    int m_preload = 50;
    int m_brakeBias = 50;
    int m_brakePower = 50;

    // Telemetry data for setup optimization
    QVector<TelemetryLapForSetup> m_importedLaps;
    TelemetryLapForSetup m_currentTelemetryLap;
    QVector<TelemetryDrivingSample> m_telemetrySamples;
};

class SetupEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit SetupEditorModule(QWidget* parent = nullptr);
    ~SetupEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Setup Editor"; }
    QString moduleId() const override { return "setupEditor"; }
    int getModulePriority() const override { return 35; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

private:
    QString m_filePath;
};

}

