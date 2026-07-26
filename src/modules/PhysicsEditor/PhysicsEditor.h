#pragma once

// ============================================================================
// PhysicsEditor — Assetto Corsa car editor (INI files + ACD)
// Inspired by AssettoTools: side menu, file tree, syntax-highlighted editor
// ============================================================================

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QTreeWidget>
#include <QTextEdit>
#include <QTreeWidgetItem>
#include <QSortFilterProxyModel>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QListWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QFileSystemModel>
#include <QCompleter>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QTableWidget>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QProgressBar>
#include <QStackedWidget>
#include <QTimer>
#include <QDial>
#include <QUdpSocket>

#include "../../core/editor/EditorModule.h"
#include "../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaConfig.h"
#include "../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaSetup.h"
#include "../../plugins/simulators/kunos/assettocorsa/ksAssettoCorsaIni.h"
#include "../../core/sys/UndoStack.h"
#include "../../core/Graphics/SceneData.h"
#include "../../core/Graphics/SceneObject.h"
#include "../../core/Graphics/SceneGraph.h"
#include "TireCurveEditor.h"

namespace ks {
using KsSetupData     = ::ks::kunos::KsSetupData;
using KsSetupManager  = ::ks::kunos::KsSetupManager;
using KsGameSettings  = ::ks::kunos::KsGameSettings;
using KsIniDocument   = ::ks::plugins::kunos::ks::KsIniDocument;
using KsIniSection    = ::ks::plugins::kunos::ks::KsIniSection;
using ::QSyntaxHighlighter;

// ─────────────────────────────────────────────────────────────────────────────
// IniSyntaxHighlighter — syntax highlighting for AC .ini files
// ─────────────────────────────────────────────────────────────────────────────
class IniSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit IniSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };
    QVector<HighlightingRule> m_rules;
    QTextCharFormat m_sectionFormat;
    QTextCharFormat m_keyFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_stringFormat;
};

// ─────────────────────────────────────────────────────────────────────────────
// IniEditorWidget — text editor with line numbers and highlighting
// ─────────────────────────────────────────────────────────────────────────────
class IniEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit IniEditorWidget(QWidget* parent = nullptr);
    ~IniEditorWidget();

    bool loadFile(const QString& path);
    bool saveFile(const QString& path);
    QString content() const { return m_textEdit->toPlainText(); }
    void setContent(const QString& text);
    bool isModified() const { return m_modified; }

signals:
    void contentChanged();
    void fileSaved(const QString& path);
    void fileLoaded(const QString& path);

private slots:
    void onTextChanged();
    void onSave();

private:
    void setupHighlighter();

    QTextEdit*       m_textEdit  = nullptr;
    QLabel*         m_lineLabel = nullptr;
    QLabel*         m_statusLabel = nullptr;
    QString         m_currentFile;
    bool            m_modified  = false;
    IniSyntaxHighlighter* m_highlighter = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// FileTreeWidget — shows car folder contents (data/ + .ini files)
// ─────────────────────────────────────────────────────────────────────────────
class FileTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit FileTreeWidget(QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    void refresh();

signals:
    void fileSelected(const QString& absolutePath);

private slots:
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    QString m_rootPath;
    static const QStringList s_carDataExtensions;
};

// ─────────────────────────────────────────────────────────────────────────────
// CarBrowserWidget — left sidebar: search + car list
// ─────────────────────────────────────────────────────────────────────────────
class CarBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit CarBrowserWidget(QWidget* parent = nullptr);

    void setCarsPath(const QString& path);
    QString currentCar() const { return m_currentCar; }

signals:
    void carSelected(const QString& carFolder);
    void carReloadRequested(const QString& carFolder);

private slots:
    void onSearchTextChanged(const QString& text);
    void onCarItemClicked(QListWidgetItem* item);
    void onReloadClicked();

private:
    void refreshCarList(const QString& filter = QString());

    QString         m_carsPath;
    QString         m_currentCar;
    QLineEdit*      m_searchBox;
    QListWidget*    m_carList;
    QPushButton*    m_reloadBtn;
    QLabel*         m_pathLabel;
};

// ─────────────────────────────────────────────────────────────────────────────
// TyresTableWidget — tyre pressure/temperature editor
// ─────────────────────────────────────────────────────────────────────────────
class TyresTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit TyresTableWidget(QWidget* parent = nullptr);

    void loadFromIni(const QMap<QString, QString>& data);
    void saveToIni(QMap<QString, QString>& data);

signals:
    void changed();

private:
    QTableWidget* m_table;
    QStringList m_rows;
};

// ─────────────────────────────────────────────────────────────────────────────
// AcdManagerWidget — ACD extract/repack UI
// ─────────────────────────────────────────────────────────────────────────────
class AcdManagerWidget : public QWidget {
    Q_OBJECT
public:
    explicit AcdManagerWidget(QWidget* parent = nullptr);

    void setCarPath(const QString& path);

signals:
    void acdExtracted(const QString& folder);

private slots:
    void onExtract();
    void onRepack();

private:
    QString createKey(const QString& folderName) const;
    QByteArray decryptAcd(const QByteArray& data, const QString& key) const;
    QByteArray encryptAcd(const QByteArray& data, const QString& key) const;

    QString  m_carPath;
    QLabel*  m_acdStatus;
    QPushButton* m_extractBtn;
    QPushButton* m_repackBtn;
    QTextEdit*   m_logEdit;
};

// ─────────────────────────────────────────────────────────────────────────────
// LutCurveWidget — loads and renders .LUT (tyre temperature/friction curves)
// ─────────────────────────────────────────────────────────────────────────────
class LutCurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit LutCurveWidget(QWidget* parent = nullptr);

    bool loadLutFile(const QString& path);
    void setData(const QVector<double>& xData, const QVector<double>& yData,
                 const QString& xLabel, const QString& yLabel);
    void clear();

signals:
    void pointSelected(int index, double x, double y);

private slots:
    void onPointHovered(const QPointF& point, bool isHovering);

private:
    void buildUI();
    void renderChart();
    QVector<double> parseLutFile(const QString& content) const;

    QChart*      m_chart     = nullptr;
    QChartView*  m_chartView = nullptr;
    QLineSeries* m_series    = nullptr;
    QLabel*               m_infoLabel = nullptr;
    QComboBox*            m_lutSelector = nullptr;
    QString               m_currentLut;
    QVector<double>       m_xData;
    QVector<double>       m_yData;
};

// ─────────────────────────────────────────────────────────────────────────────
// EngineCurveWidget — power/torque curve editor
// ─────────────────────────────────────────────────────────────────────────────
class EngineCurveWidget : public QWidget {
    Q_OBJECT
public:
    explicit EngineCurveWidget(QWidget* parent = nullptr);

    void loadFromData(const QVector<double>& rpm,
                      const QVector<double>& power,
                      const QVector<double>& torque);
    void saveToData(QVector<double>& rpm,
                    QVector<double>& power,
                    QVector<double>& torque);
    bool loadFromIni(const QString& carFolder);

signals:
    void curveChanged();

private slots:
    void onAddPoint();
    void onRemovePoint();
    void onClearCurve();
    void onExportCurve();
    void onImportCurve();

private:
    void buildUI();
    void updateChart();
    QVector<double> smoothCurve(const QVector<double>& values, int steps) const;

    QChart*      m_chart     = nullptr;
    QChartView*  m_chartView = nullptr;
    QLineSeries* m_powerSeries = nullptr;
    QLineSeries* m_torqueSeries = nullptr;

    QTableWidget*          m_pointsTable = nullptr;
    QVector<double>        m_rpm;
    QVector<double>        m_power;
    QVector<double>        m_torque;

    QLabel*                m_maxPowerLabel = nullptr;
    QLabel*                m_maxTorqueLabel = nullptr;
    QLabel*                m_maxRpmLabel = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// TelemetryWidget — live telemetry display via UDP shared memory
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
// CarSetupCompareWidget — side-by-side setup comparison
// ─────────────────────────────────────────────────────────────────────────────
class CarSetupCompareWidget : public QWidget {
    Q_OBJECT
public:
    explicit CarSetupCompareWidget(QWidget* parent = nullptr);

    void loadSetupA(const QString& path);
    void loadSetupB(const QString& path);
    void clear();

signals:
    void setupLoaded(const QString& which, const QString& path);

private slots:
    void onBrowseA();
    void onBrowseB();
    void onRefreshDiff();

private:
    QVector<QPair<QString, double>> buildRows(const KsSetupData& s) const;
    void populateTable();

    QLabel*           m_labelA = nullptr;
    QLabel*           m_labelB = nullptr;
    QPushButton*       m_browseA = nullptr;
    QPushButton*       m_browseB = nullptr;
    QTableWidget*      m_table   = nullptr;
    KsSetupData        m_setupA;
    KsSetupData        m_setupB;
    QString            m_pathA;
    QString            m_pathB;
};

// ─────────────────────────────────────────────────────────────────────────────
// AcdBrowserWidget — browse contents of decrypted ACD archive
// ─────────────────────────────────────────────────────────────────────────────
class AcdBrowserWidget : public QWidget {
    Q_OBJECT
public:
    explicit AcdBrowserWidget(QWidget* parent = nullptr);

    void setExtractedPath(const QString& path);
    void refresh();

signals:
    void fileSelected(const QString& path);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int col);
    void onExportSelected();

private:
    void buildUI();
    QString detectFileType(const QString& ext) const;

    QTreeWidget*      m_tree        = nullptr;
    QTextEdit*        m_preview     = nullptr;
    QPushButton*      m_exportBtn   = nullptr;
    QString           m_extractPath;
};

// ─────────────────────────────────────────────────────────────────────────────
// SuspGeometryWidget — suspension geometry solver (camber gain, IC, roll center)
// ─────────────────────────────────────────────────────────────────────────────
class SuspGeometryWidget : public QWidget {
    Q_OBJECT
public:
    explicit SuspGeometryWidget(QWidget* parent = nullptr);

    void loadFromSetup(const KsSetupData& setup);
    void setWheelBase(double wb) { m_wheelBase = wb; }
    void setTrackWidth(double tw) { m_trackWidth = tw; }

signals:
    void geometryCalculated();

private slots:
    void onRecalculate();
    void onExportDiagram();

private:
    void buildUI();
    double calcCamberGain(double bump, double scrubRadius,
                          double upperArmLength, double lowerArmLength,
                          double upperAngle, double lowerAngle) const;
    QPointF calcInstantCenter(double upperLength, double lowerLength,
                             double upperAngle, double lowerAngle,
                             double lowerMountY) const;

    struct WheelGeometry {
        double camberGain;
        double scrubRadius;
        double instantCenterX;
        double instantCenterY;
        double rollCenterHeight;
    };

    QVector<QDoubleSpinBox*> m_inputs;
    QTableWidget* m_resultsTable = nullptr;
    QChart* m_camberChart = nullptr;
    QChartView* m_camberChartView = nullptr;

    double m_wheelBase = 2.7;
    double m_trackWidth = 1.6;
    QVector<double> m_bumpValues;
    QVector<double> m_camberFL, m_camberFR, m_camberRL, m_camberRR;
};

// ─────────────────────────────────────────────────────────────────────────────
// FfbPreviewWidget — live force feedback visualization (from AC SDK)
// ─────────────────────────────────────────────────────────────────────────────
class FfbPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit FfbPreviewWidget(QWidget* parent = nullptr);

    void startPreview();
    void stopPreview();

signals:
    void ffbLevelChanged(double level);

private slots:
    void onDataUpdate();
    void onSimulateLoad(double angle);
    void onLoadPreset(const QString& preset);
    void onExportFfbCurve();

private:
    void buildUI();
    void drawGauge(double current, double max, QPainter* p, const QRect& rect);
    double calculateFfbTorque(double angle, double speed, double damping) const;

    QChart* m_chart = nullptr;
    QChartView* m_chartView = nullptr;
    QLineSeries* m_ffbSeries = nullptr;
    QTimer* m_updateTimer = nullptr;

    QDial* m_angleDial = nullptr;
    QDial* m_speedDial = nullptr;
    QDial* m_dampingDial = nullptr;
    QLabel* m_torqueLabel = nullptr;
    QLabel* m_peakLabel = nullptr;
    QProgressBar* m_ffbBar = nullptr;

    QVector<double> m_ffbHistory;
    double m_peakFfb = 0;
    double m_currentAngle = 0;
    double m_currentSpeed = 0;
    double m_currentDamping = 0.5;
    bool m_previewActive = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// CarValidatorWidget — validates car data and flags issues
// ─────────────────────────────────────────────────────────────────────────────
class CarValidatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit CarValidatorWidget(QWidget* parent = nullptr);

    void validateCar(const QString& carFolder);
    void validateIni(const QString& iniPath);

signals:
    void validationComplete(int errors, int warnings);

private slots:
    void onRunValidation();
    void onExportReport();
    void onFixSuggested(int issueIndex);

private:
    void buildUI();
    QString checkTyrePressures(const KsIniDocument& doc) const;
    QString checkSuspensionGeometry(const KsSetupData& setup) const;
    QString checkEngineData(const KsIniDocument& doc) const;
    QString checkAeroBalance(const KsSetupData& setup) const;
    QString checkMassDistribution(const KsSetupData& setup) const;
    void populateIssues(const QStringList& errors, const QStringList& warnings);

    struct ValidationIssue {
        QString category;
        QString message;
        QString severity;
        QString suggestedFix;
    };

    QTextEdit* m_reportEdit = nullptr;
    QListWidget* m_issuesList = nullptr;
    QTableWidget* m_summaryTable = nullptr;
    QPushButton* m_runBtn = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QString m_lastCarFolder;
    QVector<ValidationIssue> m_issues;
};

// ─────────────────────────────────────────────────────────────────────────────
// TyreTempModelWidget — tyre temp model (thermal simulation visualization)
// ─────────────────────────────────────────────────────────────────────────────
class TyreTempModelWidget : public QWidget {
    Q_OBJECT
public:
    explicit TyreTempModelWidget(QWidget* parent = nullptr);

    void loadFromIni(const QString& carFolder);
    void simulateLap(double ambientTemp, double trackTemp,
                     double avgSpeed, int laps);

signals:
    void simulationComplete(const QVector<double>& finalTemps);

private slots:
    void onStartSim();
    void onResetSim();
    void onLoadTrackPreset(const QString& track);

private:
    void buildUI();
    void updateChart();
    void updateTable(int laps);
    double estimateTempRise(double speed, double load, double friction) const;
    double estimateCooling(double tyreTemp, double ambientTemp) const;

    QChart* m_tempChart = nullptr;
    QChartView* m_tempChartView = nullptr;
    QLineSeries* m_tempFL = nullptr;
    QLineSeries* m_tempFR = nullptr;
    QLineSeries* m_tempRL = nullptr;
    QLineSeries* m_tempRR = nullptr;

    QTableWidget* m_tempTable = nullptr;
    QDoubleSpinBox* m_ambientInput = nullptr;
    QDoubleSpinBox* m_trackTempInput = nullptr;
    QDoubleSpinBox* m_avgSpeedInput = nullptr;
    QSpinBox* m_lapsInput = nullptr;
    QPushButton* m_simBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    QVector<double> m_tempsFL, m_tempsFR, m_tempsRL, m_tempsRR;
    double m_lastPressure = 32.0;

    double m_lastSimDurationMs = 0;
    int m_lastSimStepCount = 0;
    double m_avgStepMs = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// PhysicsEditorModule — main module, side menu + stacked content
// ─────────────────────────────────────────────────────────────────────────────
class PhysicsEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit PhysicsEditorModule(QWidget* parent = nullptr);

    QString moduleName() const override { return "Physics Editor"; }
    QString moduleId()   const override { return "ks.physics_editor"; }
    bool    initialize() override;
    void    shutdown() override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

public slots:
    void onCarSelected(const QString& carFolder);
    void onFileSelected(const QString& path);
    void onOpenCarsFolder();
    void onSaveCurrentFile();
    void onExportCar();
    void onImportCar();
    void onShowLutEditor();
    void onShowEngineCurve();
    void onShowTelemetry();
void onShowSetupCompare();
    void onShowAcdBrowser();
    void onShowTireCurveEditor();
    void onShowSuspGeometry();
    void onShowFfbPreview();
    void onShowCarValidator();
    void onShowTyreTempModel();
    double estimateLapTime(double trackLengthM, double avgCornerSpeedKmh,
                           double avgStraightSpeedKmh, double accelMs2,
                           double brakeDecelMs2, int cornerCount);

private:
    void buildUI();
    void updateWindowTitle();
    bool loadCarIniFiles(const QString& carFolder);
    void populateFileTree(const QString& carFolder);
    bool saveCurrentIni();

    QString                        m_carsPath;
    QString                        m_currentCar;
    QString                        m_currentFile;

    CarBrowserWidget*              m_carBrowser;
    FileTreeWidget*                m_fileTree;
    QStackedWidget*               m_contentStack;
    IniEditorWidget*              m_iniEditor;
    TyresTableWidget*             m_tyresEditor;
    AcdManagerWidget*             m_acdManager;
    LutCurveWidget*               m_lutEditor;
    EngineCurveWidget*            m_engineEditor;
    TelemetryWidget*              m_telemetryWidget;
    CarSetupCompareWidget*        m_setupCompare;
    AcdBrowserWidget*            m_acdBrowser;
    SuspGeometryWidget*           m_suspGeometry;
    FfbPreviewWidget*             m_ffbPreview;
    CarValidatorWidget*          m_validator;
    TyreTempModelWidget*         m_tyreTempModel;
    TireCurveEditor*             m_tireCurveEditor;
    QLabel*                       m_statusLabel;
    QLabel*                       m_carLabel;
};

} // namespace ks