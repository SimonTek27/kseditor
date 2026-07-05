#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QTimer>

struct MeshData;

namespace ks {

struct Keyframe {
    int frame = 0;
    float value = 0.f;
    float intensity = 0.f;
    float ease = 0.f;
    float tangentIn = 0.f;
    float tangentOut = 0.f;
    bool selected = false;

    enum Interpolation {
        InterpolationConstant,
        InterpolationLinear,
        InterpolationBezier,
        InterpolationElastic,
        InterpolationBack,
        InterpolationQuadratic
    };

    Interpolation interpolation = InterpolationBezier;
    QString easing = QString();
};

struct PathAnimationPoint {
    QVector3D position;
    QVector3D handleLeft;
    QVector3D handleRight;
};

struct GraphCurve {
    QString name;
    QVector<Keyframe> keyframes;

    float evaluate(float frame) const;
    void sortKeyframes();
    void addKeyframe(const Keyframe& keyframe);
    void removeKeyframe(int frame);
    Keyframe* getKeyframe(int frame);
};

class AnimationTimeline : public QObject
{
    Q_OBJECT

public:
    struct Keyframe {
        int frame = 0;
        float value = 0.f;
        QString interpolation = QStringLiteral("linear");
        QString easing;
        float tangentIn = 0.f;
        float tangentOut = 0.f;
    };

    struct Track {
        QString id;
        QString name;
        QString property;
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
    void setCurrentFrame(int frame);
    void setPlayRange(int start, int end);
    void getPlayRange(int& start, int& end) const;
    void setFrameRate(int fps);
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
    QMap<int, float> bakeTrack(const QString& animId, const QString& trackId, int step = 1) const;

    // Accessors
    int frameStart() const { return m_rangeStart; }
    int frameEnd() const { return m_rangeEnd; }
    int currentFrame() const { return m_currentFrame; }
    int fps() const { return m_frameRate; }
    bool isPlaying() const { return m_playing; }
    bool isLooping() const { return m_loop; }
    QString currentAnimationId() const { return m_currentAnimationId; }

signals:
    void animationCreated(const QString& id);
    void animationDeleted(const QString& id);
    void currentAnimationChanged(const QString& id);
    void trackAdded(const QString& animId, const QString& trackId);
    void trackRemoved(const QString& animId, const QString& trackId);
    void keyframeAdded(const QString& animId, const QString& trackId, int frame);
    void keyframeRemoved(const QString& animId, const QString& trackId, int frame);
    void keyframeChanged(const QString& animId, const QString& trackId, int frame);
    void currentFrameChanged(int frame);
    void frameRateChanged(int fps);
    void playbackStateChanged(bool playing);

private:
    AnimationTimeline(QObject* parent = nullptr);
    ~AnimationTimeline();
    Q_DISABLE_COPY(AnimationTimeline)

    void advanceFrame();

    static AnimationTimeline* s_instance;

    QMap<QString, Animation> m_animations;
    QMap<QString, QMap<QString, Track>> m_tracks;
    QString m_currentAnimationId;

    int m_currentFrame = 0;
    int m_frameRate = 30;
    int m_rangeStart = 0;
    int m_rangeEnd = 250;
    bool m_playing = false;
    bool m_loop = false;
    QTimer* m_playTimer = nullptr;
};

enum class ExtrapolationMode {
    Constant,
    Linear,
    Cycle,
    CycleRelative,
    Oscillate
};

class GraphEditor : public QObject
{
    Q_OBJECT

public:
    explicit GraphEditor(QObject* parent = nullptr);
    ~GraphEditor();

    struct FCurve {
        QString dataPath;
        QString actionName;
        int arrayIndex;
        int dimensionCount;

        QVector<Keyframe> keyframes;

        ExtrapolationMode extrapolationBefore = ExtrapolationMode::Constant;
        ExtrapolationMode extrapolationAfter = ExtrapolationMode::Constant;

        void addKeyframe(int frame, float value, float intensity = 0.0f, float ease = 0.0f);
        void removeKeyframe(int frame);

        float evaluate(float frame) const;
        QVector3D getKeyframeTangent(int index) const;
    };

    void setActiveAction(const QString& name);
    QString getActiveAction() const { return m_activeAction; }

    QVector<FCurve> getCurves() const { return m_curves; }
    FCurve* getCurve(const QString& dataPath);

    void addCurve(const QString& dataPath);
    void removeCurve(const QString& dataPath);

    void setInterpolation(const QString& dataPath, Keyframe::Interpolation interp);
    void setExtrapolation(const QString& dataPath, int mode);

    void snapToFrames();
    void smoothKeyframes(const QString& dataPath);

    bool bakeAction(const MeshData& input, int startFrame, int endFrame, const QMap<int, QMatrix4x4>& transforms);

signals:
    void curveAdded(const QString& dataPath);
    void curveRemoved(const QString& dataPath);
    void curveUpdated(const QString& dataPath);

private:
    QString m_activeAction;
    QVector<FCurve> m_curves;

    float interpolateBezier(const QVector<Keyframe>& keys, float frame) const;
};

class DopeSheet : public QObject
{
    Q_OBJECT

public:
    explicit DopeSheet(QObject* parent = nullptr);
    ~DopeSheet();

    struct Action {
        QString name;
        int frameStart;
        int frameEnd;
        int activeKeyframe;
        bool selected;
    };

    struct DopeSheetChannel {
        QString name;
        enum class ChannelType { Object, Bone, ShapeKey, material,texture, modifier, driver };
        ChannelType type;
        QVector<Action> actions;
        QVector<Keyframe> keyframes;
        bool expanded;
    };

    void setActions(const QVector<Action>& actions);
    QVector<DopeSheetChannel> getChannels() const { return m_channels; }

    void setSummaryChannel(int start, int end);
    void autoBlend();

    void selectKeyframesInRange(int start, int end);
    void deleteSelectedKeyframes();
    void copySelectedKeyframes();
    void pasteKeyframes();

signals:
    void channelAdded(int index);
    void channelRemoved(int index);

private:
    QVector<DopeSheetChannel> m_channels;
    QVector<Keyframe> m_clipboard;
    int m_summaryStart;
    int m_summaryEnd;
};

class NLAEditor : public QObject
{
    Q_OBJECT

public:
    explicit NLAEditor(QObject* parent = nullptr);
    ~NLAEditor();

    struct NLAStrip {
        QString name;
        QString actionName;
        int frameStart;
        int frameEnd;
        float blendIn;
        float blendOut;
        float blendMode;

        enum class BlendMode { Replace, Add, Subtract, Multiply };
        bool isBlending;
        bool isReversed;
        bool isMuted;
    };

    struct NLATrack {
        QString name;
        QVector<NLAStrip> strips;
        bool locked;
        bool muted;
        bool selected;
    };

    void addTrack(const QString& name);
    void removeTrack(int index);

    void addStrip(int trackIndex, const NLAStrip& strip);
    void removeStrip(int trackIndex, int stripIndex);

    void moveStrip(int fromTrack, int toTrack, int stripIndex);
    void scaleStrip(int trackIndex, int stripIndex, float scale);

    void setBlending(int trackIndex, int stripIndex, float blendIn, float blendOut);

    QVector<NLATrack> getTracks() const { return m_tracks; }

    void setActiveAction(const QString& actionName);
    void pushToStack(const QString& actionName);
    void popFromStack();

signals:
    void tracksChanged();

private:
    QVector<NLATrack> m_tracks;
    QString m_activeAction;
    QVector<QString> m_actionStack;
};

class DriversEditor : public QObject
{
    Q_OBJECT

signals:
    void driverEvaluated(const QString& path, float result);

public:
    explicit DriversEditor(QObject* parent = nullptr);
    ~ DriversEditor();

    struct DriverVariable {
        QString name;
        enum class Type { SingleProperty, TransformChannel, Distance };
        Type type;

        QString dataSource;
        QString dataPath;
        int arrayIndex;

        enum class TransformType { Location, Rotation, Scale };
        TransformType transformType;
        QString transformObject;

        QString drvTarget;
        QString propertyPath;
        float value = 0.0f;
        float distance = 0.0f;
    };

    struct Driver {
        QString dataPath;
        int arrayIndex;
        int dimensionCount;

        enum class Type { Average, Sum, Minimum, Maximum };
        Type type;

        enum class Mode { Scripted, Averaging, Minimum, Maximum, Multiply };
        Mode mode;

        QVector<DriverVariable> variables;

        float expression;
    };

    void addDriver(const QString& dataPath, Driver::Type type = Driver::Type::Average);
    void removeDriver(const QString& dataPath);

    void addVariable(const QString& driverPath, const DriverVariable& variable);
    void removeVariable(const QString& driverPath, const QString& variableName);

    void setExpression(const QString& driverPath, const QString& expression);

    void evaluateDriver(const Driver& driver, float time);

signals:
    void driverUpdated(const QString& path);

private:
    QMap<QString, Driver> m_drivers;
    GraphEditor* m_graphEditor = nullptr;

    float evaluateVariable(const DriverVariable& variable, float time);
};

}