#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QVariant>
#include <QRect>
#include <QPoint>
#include <QSize>
#include <QDateTime>

namespace ks {
namespace editor {

// ─── Dock Widget State ────────────────────────────────────────────────────

struct DockWidgetState {
    QString objectName;
    QString windowTitle;
    Qt::DockWidgetArea area = Qt::LeftDockWidgetArea;
    bool floating = false;
    QRect geometry;
    bool visible = true;
    int tabIndex = -1;
    QString tabBarId;
    bool closable = true;
    bool movable = true;
    bool floatable = true;
};

// ─── Window State ────────────────────────────────────────────────────────

struct MainWindowState {
    QRect geometry;
    QByteArray windowState;  // saveState()
    bool maximized = false;
    bool fullScreen = false;
    QByteArray toolbarStates;  // Toolbar positions/visibility
};

// ─── Editor Workspace ────────────────────────────────────────────────────

struct EditorWorkspace {
    QString workspaceId;
    QString name;
    QString description;
    QString lastModule;
    QString lastProject;
    
    MainWindowState windowState;
    QVector<DockWidgetState> dockWidgets;
    
    // Module-specific UI state
    QMap<QString, QJsonObject> moduleStates;
    
    // Recent files per module
    QMap<QString, QStringList> recentFiles;
    
    QDateTime created;
    QDateTime lastUsed;
    
    // Preferences snapshot
    QJsonObject preferences;
    
    // Active document
    QString activeDocument;
    QStringList openDocuments;
};

// ─── Timeline Editor ─────────────────────────────────────────────────────

struct TimelineKeyframe {
    float time;
    QVariant value;
    QString interpolation = "LINEAR";  // LINEAR, CUBIC, STEP, BEZIER
    QString easing = "NONE";
    bool selected = false;
    QMap<QString, QVariant> tangentData;
};

struct TimelineTrack {
    QString id;
    QString name;
    QString propertyPath;
    QVariant::Type valueType = QVariant::Double;
    QVector<TimelineKeyframe> keyframes;
    bool visible = true;
    bool muted = false;
    bool solo = false;
    QColor color = Qt::cyan;
    float minValue = -std::numeric_limits<float>::infinity();
    float maxValue = std::numeric_limits<float>::infinity();
};

struct TimelineMarker {
    QString id;
    QString name;
    float time;
    QString color = "#FFD700";
    QString comment;
};

class TimelineEditor : public QObject
{
    Q_OBJECT

public:
    static TimelineEditor* instance();

    void setTimeline(const QString& timelineId);
    
    // View
    void setTimeRange(float start, float end);
    void getTimeRange(float& start, float& end) const;
    void setZoom(float zoom);
    float zoom() const { return m_zoom; }
    
    void setHeight(int height);
    int height() const { return m_height; }

    void showFrames(bool show);
    bool isFramesVisible() const { return m_showFrames; }
    void showSeconds(bool show);
    bool isSecondsVisible() const { return m_showSeconds; }
    void setSnapToFrames(bool snap);
    bool isSnapToFramesEnabled() const { return m_snapToFrames; }
    void setSnapToKeyframes(bool snap);
    bool isSnapToKeyframesEnabled() const { return m_snapToKeyframes; }

    // Selection
    void selectAll();
    void deselectAll();
    void invertSelection();
    QVector<int> selectedKeyframes() const { return m_selectedFrames; }

    // Clipboard
    void copySelection();
    void pasteSelection(float atTime);
    void deleteSelection();

    // Playhead
    void setPlayheadStyle(const QString& style);
    QString getPlayheadStyle() const { return m_playheadStyle; }

    // Track height
    void setTrackHeight(int height);
    int trackHeight() const { return m_trackHeight; }

    // Track management
    QString addTrack(const QString& name, const QString& propertyPath, QVariant::Type type);
    void removeTrack(const QString& trackId);
    void setTrackVisible(const QString& trackId, bool visible);
    void setTrackMuted(const QString& trackId, bool muted);
    void setTrackSolo(const QString& trackId, bool solo);
    void setTrackColor(const QString& trackId, const QColor& color);
    
    // Keyframe operations
    QString addKeyframe(const QString& trackId, float time, const QVariant& value);
    void removeKeyframe(const QString& trackId, int keyframeIndex);
    void moveKeyframe(const QString& trackId, int keyframeIndex, float newTime);
    void setKeyframeValue(const QString& trackId, int keyframeIndex, const QVariant& value);
    void setKeyframeInterpolation(const QString& trackId, int keyframeIndex, const QString& interpolation);
    void setKeyframeEasing(const QString& trackId, int keyframeIndex, const QString& easing);

    // Markers
    QString addMarker(const QString& name, float time);
    void removeMarker(const QString& markerId);
    void moveMarker(const QString& markerId, float newTime);

    // Evaluation
    QVariant evaluateTrack(const QString& trackId, float time) const;
    QMap<QString, QVariant> evaluateAll(float time) const;

signals:
    void viewChanged(float start, float end);
    void zoomChanged(float zoom);
    void keyframeAdded(const QString& trackId, int index);
    void keyframeRemoved(const QString& trackId, int index);
    void keyframeModified(const QString& trackId, int index);

private:
    TimelineEditor(QObject* parent = nullptr);
    ~TimelineEditor();
    Q_DISABLE_COPY(TimelineEditor)

    static TimelineEditor* s_instance;

    QString m_currentTimeline;
    float m_timeStart = 0.0f;
    float m_timeEnd = 300.0f;
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
    
    struct TimelineData {
        QMap<QString, TimelineTrack> tracks;
        QVector<TimelineMarker> markers;
        float duration = 300.0f;
        float fps = 30.0f;
    };
    QMap<QString, TimelineData> m_timelines;
};

class TimelineKeyframeEditor : public QObject {
    Q_OBJECT
public:
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
    QVector<int> m_selectedFrames;
    QString m_interpolation = "LINEAR";
    QString m_easing = "NONE";
    bool m_tangentLocked = false;
};

class TimelineTrackControls : public QObject {
    Q_OBJECT
public:
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
    QMap<QString, bool> m_locked;
    QMap<QString, bool> m_muted;
    QMap<QString, bool> m_solo;
    QMap<QString, bool> m_visible;
};

} // namespace editor
} // namespace ks