#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QElapsedTimer>
#include <QVariantMap>
#include <QVariantList>
#include <QPair>
#include <memory>

#include "../PhysicsSimulator.h"
#include "TelemetryQmlBridge.h"

namespace ks {

class TelemetryFeedbackBridge : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isActive READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool referenceLoaded READ referenceLoaded NOTIFY referenceChanged)
    Q_PROPERTY(double correlationPercent READ correlationPercent NOTIFY metricsUpdated)
    Q_PROPERTY(double speedRMSE READ speedRMSE NOTIFY metricsUpdated)
    Q_PROPERTY(double lateralGRMSE READ lateralGRMSE NOTIFY metricsUpdated)
    Q_PROPERTY(double longitudinalGRMSE READ longitudinalGRMSE NOTIFY metricsUpdated)
    Q_PROPERTY(double lapTimeSim READ lapTimeSim NOTIFY lapCompared)
    Q_PROPERTY(double lapTimeRef READ lapTimeRef NOTIFY lapCompared)
    Q_PROPERTY(double lapTimeDelta READ lapTimeDelta NOTIFY lapCompared)
    Q_PROPERTY(int lapCount READ lapCount NOTIFY lapCountChanged)
    Q_PROPERTY(double currentLapTime READ currentLapTime NOTIFY lapTimeUpdated)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)

public:
    static TelemetryFeedbackBridge* instance();

    bool isActive() const { return m_active; }
    bool referenceLoaded() const { return m_referenceLoaded; }
    double correlationPercent() const { return m_correlationPercent; }
    double speedRMSE() const { return m_speedRMSE; }
    double lateralGRMSE() const { return m_lateralGRMSE; }
    double longitudinalGRMSE() const { return m_longitudinalGRMSE; }
    double lapTimeSim() const { return m_lapTimeSim; }
    double lapTimeRef() const { return m_lapTimeRef; }
    double lapTimeDelta() const { return m_lapTimeDelta; }
    int lapCount() const { return m_lapCount; }
    double currentLapTime() const { return m_currentLapTime; }
    double progress() const { return m_simProgress; }

    Q_INVOKABLE void startFeedback();
    Q_INVOKABLE void stopFeedback();
    Q_INVOKABLE void reset();

    Q_INVOKABLE bool loadReferenceTelemetry(const QString& path);
    Q_INVOKABLE void clearReference();

    Q_INVOKABLE QVariantList getSimSpeedTrace() const;
    Q_INVOKABLE QVariantList getRefSpeedTrace() const;
    Q_INVOKABLE QVariantList getSimThrottleTrace() const;
    Q_INVOKABLE QVariantList getSimBrakeTrace() const;
    Q_INVOKABLE QVariantList getSimSteeringTrace() const;
    Q_INVOKABLE QVariantMap getValidationSummary() const;

    Q_INVOKABLE QVariantList getSectorComparison() const;
    Q_INVOKABLE QVariantMap getLapAnalysis(int lapIndex) const;

signals:
    void activeChanged();
    void referenceChanged();
    void metricsUpdated();
    void lapCompared(int lapNumber, double simTime, double refTime, double delta);
    void lapCountChanged();
    void lapTimeUpdated(double currentTime);
    void feedbackStarted();
    void feedbackStopped();
    void statusMessage(const QString& msg);
    void progressChanged();
    void sampleRecorded(double speed, double rpm, double throttle,
                        double brake, double steering, double latG, double lonG);

private:
    explicit TelemetryFeedbackBridge(QObject* parent = nullptr);
    static TelemetryFeedbackBridge* s_instance;

    void onSimStateUpdated(const SimulationState& state);
    void onLapCompleted(double simLapTime);
    void computeValidationMetrics();

    struct TelemetrySample {
        double speed = 0;
        double rpm = 0;
        double throttle = 0;
        double brake = 0;
        double steering = 0;
        double latG = 0;
        double lonG = 0;
        double time = 0;
    };

    struct ReferenceLap {
        int lapNumber = 0;
        double lapTime = 0;
        QVector<TelemetrySample> samples;
    };

    QVector<TelemetrySample> m_simSamples;
    QVector<ReferenceLap> m_referenceLaps;
    QVector<TelemetrySample> m_currentSimSamples;

    bool m_active = false;
    bool m_referenceLoaded = false;

    double m_correlationPercent = 0;
    double m_speedRMSE = 0;
    double m_lateralGRMSE = 0;
    double m_longitudinalGRMSE = 0;
    double m_lapTimeSim = 0;
    double m_lapTimeRef = 0;
    double m_lapTimeDelta = 0;
    int m_lapCount = 0;
    double m_currentLapTime = 0;
    double m_simProgress = 0;
    double m_sampleTimeAccum = 0;
    phys_Simulator* m_simulator = nullptr;
    int m_feedbackLapCount = 0;
};

} // namespace ks
