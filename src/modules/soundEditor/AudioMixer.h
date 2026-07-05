#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>

namespace ks {

namespace audio {

class VCAMixerSystem : public QObject
{
    Q_OBJECT
public:
    explicit VCAMixerSystem(QObject* parent = nullptr) : QObject(parent) {}
    ~VCAMixerSystem() {}

    struct VCA {
        QString id;
        QString name;
        float volume;
        bool muted;
        bool solo;
        QStringList assignedChannels;
    };

    struct MixerChannel {
        QString id;
        QString name;
        float volume;
        float pan;
        bool muted;
        bool solo;
        QString busRoute;
    };

    struct MixerSnapshot {
        QString id;
        QString name;
        QMap<QString, float> channelVolumes;
        QMap<QString, float> vcaVolumes;
        QMap<QString, bool> mutes;
        QMap<QString, bool> solos;
        int fadeTime;
    };

    void createVCA(const QString& name);
    void deleteVCA(const QString& vcaId);
    VCA getVCA(const QString& vcaId) const;
    QVector<VCA> allVCAs() const { return m_vcas.values(); }

    void assignToVCA(const QString& channelId, const QString& vcaId);
    void unassignFromVCA(const QString& channelId, const QString& vcaId);

    void setChannelVolume(const QString& channelId, float volume);
    float getChannelVolume(const QString& channelId) const;
    void setChannelPan(const QString& channelId, float pan);
    void setChannelMute(const QString& channelId, bool mute);
    void setChannelSolo(const QString& channelId, bool solo);

    void saveSnapshot(const QString& name);
    void loadSnapshot(const QString& snapshotId);
    QVector<MixerSnapshot> getSnapshots() const { return m_snapshots; }

    void setMasterVolume(float volume) { m_masterVolume = qBound(0.0f, volume, 1.0f); }
    float masterVolume() const { return m_masterVolume; }

signals:
    void vcaCreated(const QString& vcaId);
    void vcaDeleted(const QString& vcaId);
    void snapshotCreated(const QString& snapshotId);
    void snapshotLoaded(const QString& snapshotId);
    void volumeChanged(const QString& channelId, float volume);

private:
    QMap<QString, VCA> m_vcas;
    QMap<QString, MixerChannel> m_channels;
    QVector<MixerSnapshot> m_snapshots;
    float m_masterVolume = 1.0f;
};

class PrecisionFader : public QWidget
{
    Q_OBJECT
public:
    explicit PrecisionFader(QWidget* parent = nullptr) : QWidget(parent) { setupUI(); }
    ~PrecisionFader() {}

    void setRange(double min, double max);
    void setValue(double value);
    double value() const { return m_value; }
    void setDecimals(int decimals);
    void setStep(double step);
    void setLabel(const QString& label);
    void setSuffix(const QString& suffix);

signals:
    void valueChanged(double value);

private slots:
    void onSpinBoxChanged(double value);
    void onSliderChanged(int value);

private:
    void setupUI();
    void updateFromSpinBox();
    void updateFromSlider();

    double m_value = 0.0;
    double m_min = -96.0;
    double m_max = 6.0;
    double m_step = 0.01;
    int m_decimals = 2;
    QString m_suffix = " dB";

    QSlider* m_slider = nullptr;
    QDoubleSpinBox* m_spinBox = nullptr;
    QLabel* m_labelWidget = nullptr;
};

} // namespace audio
} // namespace ks