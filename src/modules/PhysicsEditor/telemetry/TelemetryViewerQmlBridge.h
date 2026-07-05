#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include "../../../core/editor/EditorModule.h"

namespace ks {

class TelemetryViewerQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(int lapCount READ lapCount NOTIFY lapCountChanged)
    Q_PROPERTY(QString bestLapTime READ bestLapTime NOTIFY bestLapChanged)

public:
    static TelemetryViewerQmlBridge* instance();

    int lapCount() const { return m_lapCount; }
    QString bestLapTime() const { return m_bestLapTime; }
    QString currentFile() const { return m_currentFile; }

    Q_INVOKABLE bool loadTelemetry(const QString& path);
    Q_INVOKABLE bool exportTelemetry(const QString& path);
    Q_INVOKABLE void analyzeCurrent();
    Q_INVOKABLE QVariantList getLapData() const;
    Q_INVOKABLE QVariantMap getLapDetails(int lapIndex) const;
    Q_INVOKABLE QVariantList compareLaps(int lapA, int lapB);
    Q_INVOKABLE QVariantMap getSpeedTrace(int lapIndex) const;
    Q_INVOKABLE QVariantMap getThrottleTrace(int lapIndex) const;
    Q_INVOKABLE QVariantMap getBrakeTrace(int lapIndex) const;
    Q_INVOKABLE QVariantMap getSteeringTrace(int lapIndex) const;
    Q_INVOKABLE QVariantMap getSectorAnalysis(int lapIndex) const;
    Q_INVOKABLE QVariantList getCornerAnalysis(int lapIndex) const;
    Q_INVOKABLE QVariantMap getImprovementSuggestions() const;
    Q_INVOKABLE QStringList getAvailableSessions() const;

signals:
    void statusMessage(const QString& msg);
    void errorMessage(const QString& msg);
    void telemetryLoaded(const QString& path);
    void analysisComplete();
    void lapCountChanged();
    void bestLapChanged();
    void comparisonReady(const QStringList& diffSummary);

private:
    TelemetryViewerQmlBridge(QObject* parent = nullptr);
    static TelemetryViewerQmlBridge* s_instance;

    struct LapInfo {
        int lapNumber;
        float lapTime;
        float sector1, sector2, sector3;
        float topSpeed;
        float avgSpeed;
        QVector<double> speedTrace;
        QVector<double> throttleTrace;
        QVector<double> brakeTrace;
        QVector<double> steeringTrace;
        struct CornerInfo {
            QString name;
            float entrySpeed = 0, apexSpeed = 0, exitSpeed = 0;
            float minGap = 0;
        };
        QVector<CornerInfo> corners;
    };

    QVector<LapInfo> m_laps;
    int m_lapCount = 0;
    QString m_bestLapTime;
    QString m_currentFile;
    QString m_dataDir;
    QJsonObject m_currentSessionData;
};

class TelemetryViewerModule : public EditorModule {
    Q_OBJECT
public:
    explicit TelemetryViewerModule(QWidget* parent = nullptr);
    ~TelemetryViewerModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Telemetry Viewer"; }
    QString moduleId() const override { return "telemetryViewer"; }
    int getModulePriority() const override { return 30; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;
};

}


