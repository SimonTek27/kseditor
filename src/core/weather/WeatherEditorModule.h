#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QJsonObject>
#include <QUuid>
#include <QDateTime>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QPointF>
#include <QRandomGenerator>
#include <QGradient>
#include <QWidget>
#include <QDockWidget>
#include <QMainWindow>
#include <QTreeWidget>
#include <QFormLayout>
#include <QLabel>
#include <QTimer>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTimer>
#include <QDebug>
#include <QTreeWidgetItem>
#include <QScrollArea>
#include <QSplitter>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QHeaderView>
#include "../../core/editor/EditorModule.h"

namespace ks {
namespace weather {

// ============================================================================
// Core Weather Types
// ============================================================================

struct WeatherKeyframe {
    QString id;
    double time;              // Time of day (0.0 - 24.0 hours)
    QString type;             // "clear", "cloudy", "overcast", "light_rain", "heavy_rain", "storm", "fog", "snow"
    double cloudCoverage;     // 0.0 - 1.0
    double precipitation;     // 0.0 - 1.0
    double windSpeed;         // m/s
    double windDirection;     // degrees (0-360)
    double temperature;       // Celsius
    double humidity;          // 0.0 - 1.0
    double pressure;          // hPa
    double visibility;        // km
    QString transitionType;   // "linear", "ease_in", "ease_out", "step";
    QMap<QString, double> values;  // Generic values for type-specific properties
    
    static QMap<QString, double> defaultValuesForType(const QString& type);
    bool isValid() const;
};

struct ParticleEffect {
    QString id;
    QString type;             // "rain", "snow", "fog", "mist", "dust", "leaves"
    double intensity = 1.0;   // 0.0 - 1.0
    double coverage = 1.0;    // Screen coverage 0.0 - 1.0
    double particleSize = 1.0;
    QColor color = Qt::white;
    double speed = 1.0;
    QString texturePath;
    QMap<QString, QVariant> customParams;
};

struct WeatherSequence {
    QString id;
    QString name;
    QString description;
    double startTime;         // Start time of sequence
    double duration;          // Duration in hours
    bool loop = true;
    QVector<WeatherKeyframe> keyframes;
    QVector<ParticleEffect> particleEffects;
    bool enabled = true;
};

struct WeatherConfig {
    QString name;
    QString trackName;
    QString author;
    QString description;
    QDateTime created;
    QDateTime modified;
    int version = 1;
    double baseTime = 12.0;   // Base time of day
    double timeMultiplier = 1.0;
    bool dynamicWeather = true;
    double weatherChangeInterval = 2.0; // hours
    QVector<WeatherSequence> sequences;
    QString solConfigPath;    // Path to SOL config file
    QString weatherLuaPath;   // Path to weather script
};

struct WeatherPreset {
    QString name;
    QString category;         // "clear", "cloudy", "rain", "storm", "fog", "snow", "custom"
    QString description;
    WeatherConfig config;
    QColor skyColor = QColor(135, 206, 235);  // Sky blue
    QColor fogColor = QColor(200, 200, 220);
    double fogDensity = 0.0;
    double exposure = 1.0;
    double gamma = 1.0;

    QJsonObject serialize() const;
    void deserialize(const QJsonObject& obj);
};

// ============================================================================
// Weather Config Parser
// ============================================================================

class WeatherConfigParser : public QObject {
    Q_OBJECT
public:
    struct WeatherPreset {
        QString name;
        QString description;
        QString author;
        float timeOfDay = 12.0f;
        float timeMultiplier = 1.0f;
        float ambientTemperature = 20.0f;
        float humidity = 50.0f;
        float windSpeed = 5.0f;
        float windDirection = 0.0f;
        float rainIntensity = 0.0f;
        float cloudIntensity = 0.5f;
    };

    explicit WeatherConfigParser(QObject* parent = nullptr);
    ~WeatherConfigParser() override = default;

    // Static save methods used by WeatherEditor
    static bool saveCspConfig(const WeatherPreset& preset, const QString& configPath);
    static bool savePureConfig(const WeatherPreset& preset, const QString& configPath);
    static bool saveSolConfig(const WeatherPreset& preset, const QString& configPath);

    // Parse CSP/ACC weather config files
    bool parseWeatherConfig(const QString& filePath, WeatherConfig& config, QString* error = nullptr);
    bool parseWeatherLua(const QString& filePath, WeatherConfig& config, QString* error = nullptr);
    bool parseSOLConfig(const QString& filePath, WeatherConfig& config, QString* error = nullptr);
    
    // Write config files
    bool writeWeatherConfig(const QString& filePath, const WeatherConfig& config, QString* error = nullptr);
    bool writeWeatherLua(const QString& filePath, const WeatherConfig& config, QString* error = nullptr);
    bool writeSOLConfig(const QString& filePath, const WeatherConfig& config, QString* error = nullptr);

    // Validation
    bool validateConfig(const WeatherConfig& config, QStringList* errors = nullptr);

signals:
    void parsingProgress(int percent, const QString& message);
    void parsingFinished(bool success, const QString& message);

private:
    bool parseIniSection(QTextStream& in, const QString& sectionName, QMap<QString, QString>& out);
    bool parseLuaTable(const QString& luaCode, const QString& tableName, QJsonObject& out);
    QString interpolateLuaString(const QString& str, const QJsonObject& variables);
    QString generateLuaConfig(const WeatherConfig& config);
    QString generateIniConfig(const WeatherConfig& config);
    QString generateSOLConfig(const WeatherConfig& config);
};

// ============================================================================
// Weather Editor Core
// ============================================================================

class WeatherEditor : public QObject {
    Q_OBJECT
public:
    explicit WeatherEditor(QObject* parent = nullptr);
    ~WeatherEditor() override = default;

    // Project management
    bool createNewConfig(const QString& name);
    bool loadConfig(const QString& filePath);
    bool saveConfig(const QString& filePath);
    bool saveConfigAs(const QString& filePath);
    
    // Sequence management
    QString addSequence(const QString& name, double startTime, double duration);
    bool removeSequence(const QString& sequenceId);
    bool duplicateSequence(const QString& sequenceId, const QString& newName);
    bool reorderSequences(const QVector<QString>& sequenceIds);
    
    // Keyframe management
    QString addKeyframe(const QString& sequenceId, double time, const QString& type = "clear");
    bool removeKeyframe(const QString& sequenceId, const QString& keyframeId);
    bool updateKeyframe(const QString& sequenceId, const QString& keyframeId, const WeatherKeyframe& keyframe);
    bool moveKeyframe(const QString& sequenceId, const QString& keyframeId, double newTime);
    
    // Keyframe interpolation
    WeatherKeyframe interpolateKeyframe(const WeatherSequence& sequence, double time) const;
    QVector<WeatherKeyframe> generateInterpolatedKeyframes(const WeatherSequence& sequence, double interval = 0.25) const;
    
    // Particle effects
    QString addParticleEffect(const QString& sequenceId, const ParticleEffect& effect);
    bool removeParticleEffect(const QString& sequenceId, const QString& effectId);
    bool updateParticleEffect(const QString& sequenceId, const QString& effectId, const ParticleEffect& effect);
    
    // Presets
    void loadBuiltinPresets();
    QVector<WeatherPreset> getBuiltinPresets() const;
    bool applyPreset(const QString& presetName);
    bool saveAsPreset(const QString& presetName, const QString& category = "custom");
    void loadPresetLibrary(const QString& libraryPath);
    void savePresetLibrary(const QString& libraryPath) const;
    
    // Real-time preview
    QImage generatePreview(int width = 512, int height = 512, double time = 12.0) const;
    QImage generatePreviewFrame(const WeatherConfig& config, double time, int width, int height) const;
    
    // Export
    bool exportToCSP(const QString& outputDir);
    bool exportToACC(const QString& outputDir);
    bool exportToSOL(const QString& outputDir);
    bool exportToLua(const QString& filePath);
    bool exportTimeline(const QString& filePath, const QString& format = "json"); // json, csv, xml
    
    // Validation
    bool validateConfig(QStringList* errors = nullptr);
    bool checkKeyframeContinuity(QStringList* warnings = nullptr);
    bool checkTimeConflicts(QStringList* errors = nullptr);
    
    // Serialization
    QJsonObject serialize() const;
    
    // Accessors
    const WeatherConfig& config() const { return m_config; }
    WeatherConfig& config() { return m_config; }
    bool hasUnsavedChanges() const { return m_dirty; }
    void setDirty(bool dirty) { m_dirty = dirty; }
    const QString& currentFile() const { return m_currentFile; }

signals:
    void configChanged();
    void sequenceAdded(const QString& sequenceId);
    void sequenceRemoved(const QString& sequenceId);
    void sequenceChanged(const QString& sequenceId);
    void sequenceSelected(const QString& sequenceId);
    void keyframeAdded(const QString& sequenceId, const QString& keyframeId);
    void keyframeRemoved(const QString& sequenceId, const QString& keyframeId);
    void keyframeChanged(const QString& sequenceId, const QString& keyframeId);
    void keyframeSelected(const QString& sequenceId, const QString& keyframeId);
    void particleEffectAdded(const QString& sequenceId, const QString& effectId);
    void particleEffectRemoved(const QString& sequenceId, const QString& effectId);
    void particleEffectChanged(const QString& sequenceId, const QString& effectId);
    void previewGenerated(const QImage& preview);
    void exportProgress(int percent, const QString& message);
    void exportFinished(bool success, const QString& message);
    void validationFinished(bool valid, const QStringList& errors, const QStringList& warnings);

public slots:
    void onTimeChanged(double time);
    void onSequenceSelectionChanged(const QString& sequenceId);
    void onKeyframeSelectionChanged(const QString& sequenceId, const QString& keyframeId);
    void onAutoSaveTimer();

private:
    void generateId(QString& id) const;
    bool validateKeyframe(const WeatherKeyframe& kf, QString* error) const;
    WeatherKeyframe createDefaultKeyframe(double time) const;
    WeatherSequence createDefaultSequence(const QString& name, double startTime, double duration) const;
    void updateDerivedData();
    
    WeatherConfig m_config;
    QString m_currentFile;
    bool m_dirty = false;
    QVector<WeatherPreset> m_presets;
    
    // Preview cache
    mutable QMap<QString, QImage> m_previewCache;
    int m_cacheSizeLimit = 20;

    // Selection state
    QString m_selectedSequence;
    QString m_selectedKeyframe;
};

// ============================================================================
// Weather Preview Renderer
// ============================================================================

class WeatherPreviewRenderer : public QObject {
    Q_OBJECT
public:
    explicit WeatherPreviewRenderer(QObject* parent = nullptr);
    ~WeatherPreviewRenderer() override;
    
    QImage renderPreview(const WeatherConfig& config, double time, int width, int height) const;
    QImage renderSky(const WeatherConfig& config, double time, int width, int height) const;
    QImage renderParticles(const WeatherConfig& config, double time, int width, int height) const;
    
    // Static helpers
    static QColor interpolateSkyColor(const QVector<WeatherKeyframe>& keyframes, double time);
    static QColor interpolateFogColor(const QVector<WeatherKeyframe>& keyframes, double time);
    static double interpolateValue(const QVector<WeatherKeyframe>& keyframes, double time, 
                                   double (WeatherKeyframe::* member));
    static QVector<ParticleEffect> getActiveParticles(const QVector<ParticleEffect>& effects, double time);
    static QColor lerpColor(const QColor& a, const QColor& b, double t);
    
private:
    void drawGradientSky(QPainter& painter, const QRect& rect, const QColor& top, const QColor& bottom) const;
    void drawSun(QPainter& painter, const QRect& rect, double sunAngle, const QColor& sunColor) const;
    void drawClouds(QPainter& painter, const QRect& rect, double cloudCoverage, double time) const;
    void drawParticles(QPainter& painter, const QRect& rect, const QVector<ParticleEffect>& effects, double time) const;
    void drawRain(QPainter& painter, const QRect& rect, double intensity, double time) const;
    void drawSnow(QPainter& painter, const QRect& rect, double intensity, double time) const;
    void drawFog(QPainter& painter, const QRect& rect, double density, const QColor& color) const;
    void drawLightning(QPainter& painter, const QRect& rect, double intensity) const;
};

// ============================================================================
// Particle System for Weather Effects
// ============================================================================

struct Particle {
    QPointF position;
    QPointF velocity;
    double life = 1.0;
    double maxLife = 1.0;
    QColor color;
    double size = 1.0;
    double rotation = 0.0;
    double rotationSpeed = 0.0;
    bool active = true;
};

class WeatherParticleSystem : public QObject {
    Q_OBJECT
public:
    struct EmitterConfig {
        QRectF spawnArea;
        double spawnRate = 100.0;      // particles per second
        double minLife = 0.5;
        double maxLife = 3.0;
        double minSpeed = 10.0;
        double maxSpeed = 50.0;
        QPointF gravity = {0, 9.81};
        QPointF wind = {0, 0};
        double minSize = 1.0;
        double maxSize = 3.0;
        QGradient colorGradient;
        QString texturePath;
        bool useTexture = false;
    };
    
    explicit WeatherParticleSystem(QObject* parent = nullptr);
    ~WeatherParticleSystem() override;
    
    void setEmitterConfig(const EmitterConfig& config);
    void update(double deltaTime);
    void render(QPainter& painter, const QRectF& viewport) const;
    void emitParticles(int count, const QPointF& position);
    void clear();
    
    int activeParticleCount() const;
    int totalSpawned() const;
    
    void setParticleTexture(const QImage& texture);
    void setColorGradient(const QGradient& gradient);

private:
    void spawnParticles(double deltaTime);
    void updateParticle(Particle& p, double deltaTime);
    bool isInViewport(const Particle& p, const QRectF& viewport) const;
    QColor interpolateGradient(double t) const;
    
    EmitterConfig m_config;
    QVector<Particle> m_particles;
    QImage m_particleTexture;
    QGradient m_colorGradient;
    double m_spawnTimer = 0.0;
    int m_totalSpawned = 0;
    QRandomGenerator* m_rng = nullptr;
};

// ============================================================================
// Weather Editor Module (UI Module Widget)
// ============================================================================

class WeatherEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit WeatherEditorModule(QWidget* parent = nullptr);
    ~WeatherEditorModule();

    QString moduleName() const override { return "Weather Editor"; }
    QString moduleId() const override { return "weather"; }
    int getModulePriority() const override { return 45; }

    bool initialize();
    void shutdown();
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow);
    
    void importFile(const QString& filePath);
    void exportFile(const QString& filePath);
    QJsonObject serializeProject() const;
    void deserializeProject(const QJsonObject& data);

public slots:
    void onActivation();
    void onDeactivation();

private:
    void setupUI();
    void onConfigChanged();
    void updateSequenceTree();
    void updateKeyframeTree(const QString& sequenceName = QString());

    WeatherEditor* m_editor = nullptr;
    bool m_initialized = false;
    QDockWidget* m_dockWidget = nullptr;
    QWidget* m_centralWidget = nullptr;

    // UI elements
    QTreeWidget* m_sequenceTree = nullptr;
    QTreeWidget* m_keyframeTree = nullptr;
    QDoubleSpinBox* m_timeSpin = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QDoubleSpinBox* m_cloudSpin = nullptr;
    QDoubleSpinBox* m_rainSpin = nullptr;
    QDoubleSpinBox* m_windSpeedSpin = nullptr;
    QDoubleSpinBox* m_windDirSpin = nullptr;
    QDoubleSpinBox* m_tempSpin = nullptr;
    QDoubleSpinBox* m_humiditySpin = nullptr;
    QDoubleSpinBox* m_pressureSpin = nullptr;
    QDoubleSpinBox* m_visibilitySpin = nullptr;
    QComboBox* m_transitionCombo = nullptr;
    QComboBox* m_particleCombo = nullptr;

    QWidget* m_keyframeEditorWidget = nullptr;
    QFormLayout* m_keyframeForm = nullptr;
    QDoubleSpinBox* m_kfTimeSpin = nullptr;
    QComboBox* m_kfTypeCombo = nullptr;
    QDoubleSpinBox* m_kfCloudSpin = nullptr;
    QDoubleSpinBox* m_kfRainSpin = nullptr;
    QDoubleSpinBox* m_kfWindSpin = nullptr;
    QDoubleSpinBox* m_kfWindDirSpin = nullptr;
    QDoubleSpinBox* m_kfTempSpin = nullptr;
    QDoubleSpinBox* m_kfHumiditySpin = nullptr;
    QDoubleSpinBox* m_kfPressureSpin = nullptr;
    QDoubleSpinBox* m_kfVisibilitySpin = nullptr;
    QComboBox* m_kfTransitionCombo = nullptr;

    QWidget* m_particleEditorWidget = nullptr;
    QFormLayout* m_particleForm = nullptr;
    QComboBox* m_peTypeCombo = nullptr;
    QDoubleSpinBox* m_peIntensitySpin = nullptr;
    QDoubleSpinBox* m_peCoverageSpin = nullptr;
    QDoubleSpinBox* m_peSizeSpin = nullptr;
    QDoubleSpinBox* m_peSpeedSpin = nullptr;

    QWidget* m_previewWidget = nullptr;
    QLabel* m_previewLabel = nullptr;
    QGraphicsView* m_previewGraphics = nullptr;
    QGraphicsScene* m_previewScene = nullptr;
    QDoubleSpinBox* m_previewTimeSpin = nullptr;
    QCheckBox* m_autoPreviewCheck = nullptr;
    QPushButton* m_skyColorBtn = nullptr;
    QPushButton* m_fogColorBtn = nullptr;
    QDoubleSpinBox* m_fogDensitySpin = nullptr;
    QComboBox* m_presetCombo = nullptr;

    QTimer* m_autoSaveTimer = nullptr;
    QTimer* m_previewTimer = nullptr;
};

} // namespace weather
} // namespace ks