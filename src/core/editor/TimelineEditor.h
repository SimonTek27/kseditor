#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QVector>

namespace ks {

class AnimationTimeline;

class TimelineEditor : public QObject
{
    Q_OBJECT

public:
    static TimelineEditor* instance();

    void setAnimationTimeline(AnimationTimeline* timeline);

    void setFrameView(int start, int end);
    void getFrameView(int& start, int& end) const;

    void setZoom(float zoom);
    float getZoom() const { return m_zoom; }

    void setHeight(int height);
    int getHeight() const { return m_height; }

    void showFrames(bool show);
    bool isFramesVisible() const { return m_showFrames; }

    void showSeconds(bool show);
    bool isSecondsVisible() const { return m_showSeconds; }

    void setSnapToFrames(bool snap);
    bool isSnapToFramesEnabled() const { return m_snapToFrames; }

    void setSnapToKeyframes(bool snap);
    bool isSnapToKeyframesEnabled() const { return m_snapToKeyframes; }

    void selectAll();
    void deselectAll();
    void invertSelection();

    void copySelection();
    void pasteSelection();
    void deleteSelection();

    void setPlayheadStyle(const QString& style);
    QString getPlayheadStyle() const { return m_playheadStyle; }

    void setTrackHeight(int height);
    int getTrackHeight() const { return m_trackHeight; }

signals:
    void viewChanged(int start, int end);
    void zoomChanged(float zoom);

private:
    TimelineEditor(QObject* parent = nullptr);
    ~TimelineEditor();
    Q_DISABLE_COPY(TimelineEditor)

    static TimelineEditor* s_instance;

    AnimationTimeline* m_timeline = nullptr;
    int m_frameViewStart = 0;
    int m_frameViewEnd = 300;
    float m_zoom = 1.0f;
    int m_height = 400;
    bool m_showFrames = true;
    bool m_showSeconds = true;
    bool m_snapToFrames = true;
    bool m_snapToKeyframes = true;
    QString m_playheadStyle = "line";
    int m_trackHeight = 30;

    QVector<int> m_selectedFrames;
    QVector<QPair<int, int>> m_clipboard;
};

class TimelineKeyframeEditor : public QObject
{
    Q_OBJECT

public:
    static TimelineKeyframeEditor* instance();

    void setSelectedKeyframes(const QVector<int>& frames);
    QVector<int> getSelectedKeyframes() const { return m_selectedFrames; }

    void setInterpolation(const QString& interpolation);
    QString getInterpolation() const { return m_interpolation; }

    void setEasing(const QString& easing);
    QString getEasing() const { return m_easing; }

    void setTangentLocked(bool locked);
    bool isTangentLocked() const { return m_tangentLocked; }

    void moveSelectedKeyframes(int delta);
    void scaleSelectedKeyframes(float scale, int origin);

    void flipSelectedKeyframes();

    void setValues(float value);
    void offsetValues(float offset);
    void scaleValues(float scale);

    void bakeKeyframes();

signals:
    void keyframesSelected(const QVector<int>& frames);
    void keyframesModified();

private:
    TimelineKeyframeEditor(QObject* parent = nullptr);
    ~TimelineKeyframeEditor();
    Q_DISABLE_COPY(TimelineKeyframeEditor)

    static TimelineKeyframeEditor* s_instance;

    QVector<int> m_selectedFrames;
    QString m_interpolation = "linear";
    QString m_easing = "none";
    bool m_tangentLocked = false;
};

class TimelineTrackControls : public QObject
{
    Q_OBJECT

public:
    static TimelineTrackControls* instance();

    void setTrackLocked(const QString& trackId, bool locked);
    bool isTrackLocked(const QString& trackId) const;

    void setTrackMuted(const QString& trackId, bool muted);
    bool isTrackMuted(const QString& trackId) const;

    void setTrackSolo(const QString& trackId, bool solo);
    bool isTrackSolo(const QString& trackId) const;

    void setTrackVisible(const QString& trackId, bool visible);
    bool isTrackVisible(const QString& trackId) const;

    void muteAllExcept(const QString& trackId);
    void unmuteAll();

    void soloAllExcept(const QString& trackId);
    void unsoloAll();

signals:
    void trackLockChanged(const QString& trackId);
    void trackMuteChanged(const QString& trackId);
    void trackSoloChanged(const QString& trackId);
    void trackVisibilityChanged(const QString& trackId);

private:
    TimelineTrackControls(QObject* parent = nullptr);
    ~TimelineTrackControls();
    Q_DISABLE_COPY(TimelineTrackControls)

    static TimelineTrackControls* s_instance;

    QMap<QString, bool> m_locked;
    QMap<QString, bool> m_muted;
    QMap<QString, bool> m_solo;
    QMap<QString, bool> m_visible;
};

} // namespace ks