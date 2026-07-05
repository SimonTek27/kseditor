#pragma once

#include "../../core/editor/EditorModule.h"
#include "AiSplineEditor.h"
#include "AiBehaviorModel.h"
#include "AIEditorQmlBridge.h"
#include "MultiCarAI.h"
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <QDockWidget>
#include <QTableWidget>
#include <QTimer>

namespace ks {

class AIEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit AIEditorModule(QWidget* parent = nullptr);
    ~AIEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "AI Editor"; }
    QString moduleId() const override { return "aiEditor"; }
    int getModulePriority() const override { return 15; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;
    void onActivation() override;
    void onDeactivation() override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

private slots:
    void onTrackSelected(int index);
    void onLoadTrack();
    void onSaveTrack();
    void onSmoothSpline();
    void onResampleSpline();
    void onAnalyzeLine();
    void onGenerateAILine();
    void onOptimizeLine();
    void onDifficultyChanged(int v);
    void onAggressionChanged(int v);
    void onPresetApplied(const QString& name);
    void onProfileSelected(int row);
    void onProfileStyleFilterChanged(int index);
    void onProfileTierFilterChanged(int index);
    void onCompareProfiles();
    void onSuggestImprovements();
    void onExportProfile();
    void onLoadTelemetry();
    void onAnalyzeTelemetry();
    void onApplyTelemetryProfile();
    void onClearTrainingData();
    void onTrainedTrackSelected(int row);

    // Multi-car race
    void onStartRace();
    void onStopRace();
    void onResetRace();
    void onRaceTick();
    void onNumDriversChanged(int v);
    void onRaceLapsChanged(int v);
    void onTrackLengthChanged(double v);

private:
    void setupUi();
    void populateTrackList();
    void populateProfileList();
    AiBehaviorModel::AiDriverProfile resolveProfile(int row) const;
    void populateTelemetryTracks();
    void updateSplineInfo();

    QDockWidget* m_dockWidget = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Track / spline tab
    QComboBox* m_trackSelector = nullptr;
    QListWidget* m_splineFileList = nullptr;
    QPushButton* m_loadTrackBtn = nullptr;
    QPushButton* m_saveTrackBtn = nullptr;
    QPushButton* m_smoothBtn = nullptr;
    QPushButton* m_resampleBtn = nullptr;
    QLabel* m_splineInfoLabel = nullptr;
    QLineEdit* m_targetPointsEdit = nullptr;
    QSpinBox* m_smoothIterationsSpin = nullptr;

    // AI settings tab
    QSpinBox* m_difficultySpin = nullptr;
    QSpinBox* m_aggressionSpin = nullptr;
    QSpinBox* m_precisionSpin = nullptr;
    QSpinBox* m_consistencySpin = nullptr;
    QPushButton* m_analyzeBtn = nullptr;
    QPushButton* m_generateBtn = nullptr;
    QPushButton* m_optimizeBtn = nullptr;
    QTextEdit* m_resultLog = nullptr;

    // ── Profiles tab ────────────────────────────────────────────────────────
    QListWidget* m_profileList = nullptr;
    QTextEdit* m_profileDetail = nullptr;
    QPushButton* m_exportProfileBtn = nullptr;
    QComboBox* m_profileStyleFilter = nullptr;
    QComboBox* m_profileTierFilter = nullptr;
    QPushButton* m_compareProfilesBtn = nullptr;
    QPushButton* m_suggestImprovementsBtn = nullptr;

    // Telemetry training tab
    QPushButton* m_loadTelemetryBtn = nullptr;
    QPushButton* m_analyzeTelemetryBtn = nullptr;
    QPushButton* m_applyTelemetryProfileBtn = nullptr;
    QPushButton* m_clearTrainingDataBtn = nullptr;
    QLabel* m_trainingStatsLabel = nullptr;
    QTextEdit* m_cornerAnalysisText = nullptr;
    QListWidget* m_trainedTrackList = nullptr;

    AiSplineManager* m_splineManager = nullptr;
    QString m_currentTrackPath;
    bool m_initialized = false;

    // Race state
    int m_currentNumDrivers = 10;
    int m_raceTotalLaps = 10;
    double m_trackLength = 5000.0;

    // Multi-car race
    ks::MultiCarAI m_multiCarAI;
    QTimer* m_raceTimer = nullptr;
    QTableWidget* m_raceLeaderboard = nullptr;
    QLabel* m_raceStatusLabel = nullptr;
    QLabel* m_raceTimeLabel = nullptr;
    QLabel* m_fastestLapLabel = nullptr;
    QLabel* m_overtakesLabel = nullptr;
    QSpinBox* m_raceNumDriversSpin = nullptr;
    QSpinBox* m_raceLapsSpin = nullptr;
    QDoubleSpinBox* m_trackLengthSpin = nullptr;
    QPushButton* m_startRaceBtn = nullptr;
    QPushButton* m_stopRaceBtn = nullptr;
    QPushButton* m_resetRaceBtn = nullptr;
    QTextEdit* m_raceEventLog = nullptr;
};

} // namespace ks
