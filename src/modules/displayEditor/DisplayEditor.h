#pragma once
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QColor>
#include <QVector>
#include <QString>
#include <QMap>
#include <QTimer>
#include <QEasingCurve>
#include <QJsonObject>
#include <QJsonArray>

enum class ElementType {
    TEXT,
    IMAGE,
    BAR,
    CIRCLE,
    DIGIT_GROUP,
    ANIMATED_TEXT,
    PROGRESS_RING
};

enum class DataSource {
    SPEED,
    RPM,
    GEAR,
    LAP_TIME,
    BEST_LAP,
    LAST_LAP,
    DELTA,
    FUEL,
    TIRE_TEMP,
    WATER_TEMP,
    OIL_TEMP,
    TURBO_BOOST,
    SPEED_KMH,
    SPEED_MPH,
    CURRENT_LAP,
    LAP_COUNT,
    POSITION,
    BRAKE_BIAS,
    TIRE_PRESSURE,
    STEERING_ANGLE,
    SUSPENSION_TRAVEL
};

enum class AnimationType {
    NONE,
    FADE_IN,
    FADE_OUT,
    SLIDE_IN,
    SLIDE_OUT,
    BOUNCE,
    PULSE,
    ROTATE,
    SCALE,
    SHAKE,
    TYPEWRITER,
    COLOR_CYCLE,
    PHYSICS_DRIVEN
};

struct AnimationConfig {
    AnimationType type = AnimationType::NONE;
    int durationMs = 500;
    int delayMs = 0;
    bool loop = false;
    bool pingPong = false;
    QEasingCurve::Type easing = QEasingCurve::Linear;
    QPointF slideOffset;
    double scaleFrom = 1.0;
    double scaleTo = 1.0;
    double rotationFrom = 0.0;
    double rotationTo = 0.0;
    QColor colorFrom;
    QColor colorTo;
    double minOpacity = 0.0;
    double maxOpacity = 1.0;
    double pulseMinScale = 0.95;
    double pulseMaxScale = 1.05;
    int shakeIntensity = 3;
    double physicsMultiplier = 1.0;
    DataSource physicsSource = DataSource::RPM;
};

struct DisplayElement {
    QString id;
    ElementType type;
    DataSource source;
    QPoint position;
    QSize size;
    QColor color;
    QColor backgroundColor;
    int fontSize;
    int decimalPlaces;
    QString fontFamily;
    bool visible;
    int updateInterval; // milliseconds
    QString imagePath;
    int minValue;
    int maxValue;
    AnimationConfig animation;
    
    DisplayElement() : type(ElementType::TEXT), 
                       source(DataSource::SPEED),
                       position(0,0),
                       size(100,30),
                       color(Qt::white),
                       backgroundColor(Qt::transparent),
                       fontSize(24),
                       decimalPlaces(0),
                       fontFamily("Arial"),
                       visible(true),
                       updateInterval(100),
                       minValue(0),
                       maxValue(100) {}
};

// Animation runtime state
struct AnimationState {
    AnimationConfig config;
    double elapsedMs = 0;
    double progress = 0;
    bool running = false;
    bool forward = true;
    QPointF currentOffset;
    double currentScale = 1.0;
    double currentRotation = 0.0;
    QColor currentColor;
    double currentOpacity = 1.0;
    double physicsValue = 0.0;
};

class ksDisplayEditor : public QObject
{
    Q_OBJECT

public:
    explicit ksDisplayEditor(QObject *parent = nullptr);
    ~ksDisplayEditor();

    // File operations
    bool loadFromFile(const QString& filePath);
    bool saveToFile(const QString& filePath);
    bool loadFromLua(const QString& luaFilePath);
    bool saveToLua(const QString& luaFilePath);
    
    // Element management
    void addElement(const DisplayElement& element);
    void removeElement(const QString& elementId);
    void updateElement(const QString& elementId, const DisplayElement& element);
    DisplayElement* getElement(const QString& elementId);
    QVector<DisplayElement>& getAllElements();
    void clearElements();
    
    // Display settings
    void setDisplayName(const QString& name);
    QString getDisplayName() const;
    void setDisplaySize(const QSize& size);
    QSize getDisplaySize() const;
    void setBackgroundImage(const QString& imagePath);
    QString getBackgroundImage() const;
    void setBackgroundColor(const QColor& color);
    QColor getBackgroundColor() const;
    
    // Validation
    bool validateConfig();
    QStringList getErrors() const;
    
    // Export
    bool exportToIni(const QString& filePath);
    bool exportToJson(const QString& filePath);
    bool exportAsImage(const QString& filePath, int width = 0, int height = 0);
    
    // ── Animation System ─────────────────────────────────────────────────
    void setAnimation(const QString& elementId, const AnimationConfig& config);
    void removeAnimation(const QString& elementId);
    void setPhysicsValue(const QString& elementId, double value);
    void pauseAnimations();
    void resumeAnimations();
    void stopAllAnimations();
    AnimationState getAnimationState(const QString& elementId) const;
    
    // ── Physics-driven values ────────────────────────────────────────────
    void updatePhysicsValue(DataSource source, double value);
    double getPhysicsValue(DataSource source) const;
    void setPhysicsUpdateInterval(int ms);
    
signals:
    void elementAdded(const QString& elementId);
    void elementRemoved(const QString& elementId);
    void elementUpdated(const QString& elementId);
    void displayChanged();
    void animationStarted(const QString& elementId);
    void animationStopped(const QString& elementId);
    void animationLooped(const QString& elementId);
    
private slots:
    void onAnimationTick();

private:
    QString m_displayName;
    QSize m_displaySize;
    QString m_backgroundImage;
    QColor m_backgroundColor;
    QVector<DisplayElement> m_elements;
    mutable QStringList m_errors;
    
    // Parsing methods
    bool parseIniFile(const QString& content);
    bool parseLuaFile(const QString& content);
    bool loadFromJson(const QJsonObject& json);
    void parseIniSection(const QString& section, const QMap<QString, QString>& values);
    QString generateLuaScript() const;
    QString generateIniContent() const;
    QString generateJsonContent() const;
    
    // Helper methods
    QString elementTypeToString(ElementType type) const;
    ElementType stringToElementType(const QString& str) const;
    QString dataSourceToString(DataSource source) const;
    DataSource stringToDataSource(const QString& str) const;
    QString colorToHex(const QColor& color) const;
    QColor hexToColor(const QString& hex) const;
    
    // ── Animation system ─────────────────────────────────────────────────
    QTimer* m_animationTimer;
    QMap<QString, AnimationState> m_animations;
    QMap<DataSource, double> m_physicsValues;
    int m_physicsUpdateIntervalMs = 50;
    bool m_animationsPaused = false;
    
    void updateAnimationState(AnimationState& state, double dtMs);
    QPointF computeSlideOffset(const AnimationConfig& cfg, double progress);
    double computeEasedValue(const AnimationConfig& cfg, double progress);
    QColor interpolateColor(const QColor& from, const QColor& to, double t);
};