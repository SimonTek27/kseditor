#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QUuid>

class QTimer;

namespace ks {

class AnimationTimeline : public QObject {
    Q_OBJECT
public:
    struct Keyframe {
        int frame = 0;
        float value = 0.0f;
        QString interpolation = "linear";
    };

    struct Track {
        QString id;
        QString name;
        bool locked = false;
        bool muted = false;
        bool solo = false;
        QMap<int, Keyframe> keyframes;
    };

    struct Animation {
        QString id;
        QString name;
        int startFrame = 0;
        int endFrame = 250;
        int frameRate = 30;
        bool loop = false;
    };

    static AnimationTimeline* instance();

    // Animation CRUD
    void createAnimation(const QString& name);
    void deleteAnimation(const QString& id);
    void setCurrentAnimation(const QString& id);
    void renameAnimation(const QString& id, const QString& name);
    void duplicateAnimation(const QString& id);
    QVector<Animation> getAnimations() const;
    Animation getAnimation(const QString& id) const;
    QString currentAnimationId() const { return m_currentAnimationId; }

    // Track management
    void addTrack(const QString& animId, const Track& track);
    void removeTrack(const QString& animId, const QString& trackId);
    void setTrackLocked(const QString& animId, const QString& trackId, bool locked);
    void setTrackMuted(const QString& animId, const QString& trackId, bool muted);
    void setTrackSolo(const QString& animId, const QString& trackId, bool solo);
    QVector<Track> getTracks(const QString& animId) const;

    // Keyframe management
    void addKeyframe(const QString& animId, const QString& trackId, const Keyframe& kf);
    void removeKeyframe(const QString& animId, const QString& trackId, int frame);
    void setKeyframeValue(const QString& animId, const QString& trackId, int frame, float value);
    void setKeyframeInterpolation(const QString& animId, const QString& trackId, int frame, const QString& interp);
    float evaluateTrack(const QString& animId, const QString& trackId, int frame) const;

    // Playback
    int currentFrame() const { return m_currentFrame; }
    void setCurrentFrame(int frame);
    void setPlayRange(int start, int end);
    void getPlayRange(int& start, int& end) const;
    void setFrameRate(int fps);
    int frameRate() const { return m_frameRate; }
    void setLoop(bool loop);
    void play();
    void pause();
    void stop();
    void togglePlayPause();
    void stepForward();
    void stepBackward();
    void goToStart();
    void goToEnd();

    // Baking
    QMap<int, float> bakeTrack(const QString& animId, const QString& trackId, int step) const;

signals:
    void animationCreated(const QString& id);
    void animationDeleted(const QString& id);
    void currentAnimationChanged(const QString& id);
    void currentFrameChanged(int frame);
    void trackAdded(const QString& animId, const QString& trackId);
    void trackRemoved(const QString& animId, const QString& trackId);
    void keyframeAdded(const QString& animId, const QString& trackId, int frame);
    void keyframeRemoved(const QString& animId, const QString& trackId, int frame);
    void keyframeChanged(const QString& animId, const QString& trackId, int frame);
    void playbackStateChanged(bool playing);
    void frameRateChanged(int fps);

private slots:
    void advanceFrame();

private:
    explicit AnimationTimeline(QObject* parent = nullptr);
    ~AnimationTimeline() override;
    Q_DISABLE_COPY(AnimationTimeline)

    static AnimationTimeline* s_instance;

    QTimer* m_playTimer = nullptr;
    QMap<QString, Animation> m_animations;
    QMap<QString, QMap<QString, Track>> m_tracks;
    QString m_currentAnimationId;
    int m_frameRate = 30;
    int m_currentFrame = 0;
    int m_rangeStart = 0;
    int m_rangeEnd = 250;
    bool m_playing = false;
    bool m_loop = false;
};

} // namespace ks
