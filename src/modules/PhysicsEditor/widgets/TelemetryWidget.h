#pragma once

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QUdpSocket>
#include <QVector>

namespace ks {

class TelemetryWidget : public QWidget {
    Q_OBJECT
public:
    explicit TelemetryWidget(QWidget* parent = nullptr);
    ~TelemetryWidget();

    struct TelemetrySample {
        double speed = 0;
        double rpm = 0;
        int gear = 0;
        double throttle = 0;
        double brake = 0;
        double tyreTemp[4] = {0, 0, 0, 0};
        double timestamp = 0;
    };

    void startSession(const QString& carFolder);
    void stopSession();
    void loadTelemetryConfig(const QString& path);
    void receiveTelemetrySample(const TelemetrySample& sample);

signals:
    void sessionStarted();
    void sessionStopped();
    void sampleReceived(const TelemetrySample& sample);

private slots:
    void onDataReceived();
    void onStartStopClicked();
    void onCalibrateClicked();

private:
    void buildUI();
    void updateGauge(double value, QProgressBar* bar, QLabel* label,
                     double min, double max, const QString& unit);

    QLabel*    m_sessionLabel  = nullptr;
    QPushButton* m_startBtn    = nullptr;
    QPushButton* m_calibrateBtn = nullptr;

    // Speed
    QProgressBar* m_speedBar  = nullptr;
    QLabel*        m_speedLabel = nullptr;

    // RPM
    QProgressBar* m_rpmBar     = nullptr;
    QLabel*        m_rpmLabel  = nullptr;

    // Tyre temps
    QTableWidget* m_tyreTemps  = nullptr;

    // Brake/ throttle bars
    QProgressBar* m_brakeBar  = nullptr;
    QProgressBar* m_throttleBar = nullptr;

    // Gear
    QLabel* m_gearLabel = nullptr;

    bool    m_sessionActive = false;
    QUdpSocket* m_udpSocket = nullptr;
    QString m_telemetryIp = "127.0.0.1";
    int m_telemetryPort = 9996;
    bool m_useUDP = true;
};

} // namespace ks