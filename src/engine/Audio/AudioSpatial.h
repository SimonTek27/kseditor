#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVector3D>
#include <QMap>
#include <QJsonObject>

namespace ks { namespace audio {

// ============================================================================
// Enums
// ============================================================================

enum class SpeakerPosition {
    FrontLeft,
    FrontCenter,
    FrontRight,
    SideLeft,
    SideRight,
    BackLeft,
    BackCenter,
    BackRight,
    LFE,
    TopFrontLeft,
    TopFrontRight,
    TopBackLeft,
    TopBackRight
};

enum class ChannelLayout {
    Mono,
    Stereo,
    Stereo_1,
    Surround_5_1,
    Surround_7_1,
    Atmos_7_1_4
};

// ============================================================================
// SpeakerInfo
// ============================================================================

struct SpeakerInfo {
    SpeakerPosition position = SpeakerPosition::FrontLeft;
    float azimuth = 0.0f;
    float elevation = 0.0f;

    static SpeakerInfo forPosition(SpeakerPosition pos);
};

// ============================================================================
// SpatialSource
// ============================================================================

struct SpatialSource {
    QString id;
    QString name;
    QVector3D position;
    QVector3D velocity;
    float volume = 1.0f;
    float spread = 0.0f;
    float distanceFactor = 1.0f;
    float rolloffFactor = 1.0f;
    float innerAngle = 360.0f;
    float outerAngle = 360.0f;
    float outerGain = 0.0f;
    float dopplerFactor = 1.0f;
    bool isEmitting = true;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
};

// ============================================================================
// SpatialListener
// ============================================================================

struct SpatialListener {
    QVector3D position;
    QVector3D forward{0.0f, 0.0f, -1.0f};
    QVector3D up{0.0f, 1.0f, 0.0f};
    QVector3D velocity;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
};

// ============================================================================
// SpatialBus
// ============================================================================

struct SpatialBus {
    QString id;
    QString name;
    ChannelLayout layout = ChannelLayout::Stereo;
    float volume = 1.0f;
    bool muted = false;
    QString panningLaw = "equal_power";
    float spread = 0.0f;
    bool occlusionEnabled = false;
    float occlusionFactor = 0.0f;
    float occlusionLowPass = 5000.0f;
    float reverbSend = 0.0f;
    QVector<SpeakerInfo> speakers;

    static SpatialBus createStereo(const QString& name);
    static SpatialBus create5_1(const QString& name);
    static SpatialBus create7_1(const QString& name);

    void initializeSpeakers();
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);
};

// ============================================================================
// SpatialSurroundSystem
// ============================================================================

class SpatialSurroundSystem : public QObject {
    Q_OBJECT

public:
    explicit SpatialSurroundSystem(QObject* parent = nullptr);

    void setListener(const SpatialListener& listener);
    SpatialListener listener() const { return m_listener; }

    void addSource(const SpatialSource& source);
    void removeSource(const QString& sourceId);
    void updateSource(const QString& sourceId, const QVector3D& position);
    SpatialSource* getSource(const QString& sourceId);
    QVector<SpatialSource*> getAllSources() const;

    void addBus(const SpatialBus& bus);
    void removeBus(const QString& busId);
    SpatialBus* getBus(const QString& busId);
    QVector<SpatialBus*> getAllBuses() const;

    QMap<SpeakerPosition, float> calculatePanning(
        const SpatialSource& source, const SpatialBus& bus) const;
    float calculateDoppler(const SpatialSource& source) const;
    float calculateAttenuation(const SpatialSource& source) const;
    float calculateOcclusion(const SpatialSource& source,
                             const SpatialBus& bus) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

signals:
    void listenerChanged();
    void sourceAdded(const QString& id);
    void sourceRemoved(const QString& id);
    void sourceMoved(const QString& id, const QVector3D& position);
    void busAdded(const QString& id);
    void busRemoved(const QString& id);

private:
    void applyPanningLaw(
        float azimuth, float spread,
        const QVector<SpeakerInfo>& speakers,
        QMap<SpeakerPosition, float>& gains) const;

    SpatialListener m_listener;
    QVector<SpatialSource> m_sources;
    QVector<SpatialBus> m_buses;
};

}} // namespace ks::audio
