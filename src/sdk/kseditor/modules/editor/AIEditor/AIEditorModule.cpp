#include "AIEditorModule.h"
#include "AiSplineEditor.h"
#include "AiBehaviorModel.h"
#include "AIEditorQmlBridge.h"
#include "MultiCarAI.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QStandardPaths>
#include <QDebug>
#include <QHeaderView>
#include <QRandomGenerator>

namespace ks {

AIEditorModule::AIEditorModule(QWidget* parent)
    : EditorModule(parent)
{
}

bool AIEditorModule::initialize() {
    if (m_initialized) return true;
    
    setupUi();
    m_splineManager = new AiSplineManager();
    m_initialized = true;
    return true;
}

void AIEditorModule::shutdown() {
    if (!m_initialized) return;
    if (m_splineManager) {
        delete m_splineManager;
        m_splineManager = nullptr;
    }
    if (m_raceTimer) {
        m_raceTimer->stop();
        m_raceTimer->deleteLater();
        m_raceTimer = nullptr;
    }
    m_initialized = false;
}

QDockWidget* AIEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow) {
    if (!m_dockWidget) {
        m_dockWidget = new QDockWidget(QObject::tr("AI Editor"), mainWindow);
        m_dockWidget->setObjectName("aiEditorDock");
        m_dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_dockWidget->setWidget(m_tabWidget);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_dockWidget);
    }
    return m_dockWidget;
}

void AIEditorModule::setupUi() {
    m_tabWidget = new QTabWidget();
    
    // Tab 1: Track / Spline
    QWidget* trackTab = new QWidget();
    QVBoxLayout* trackLayout = new QVBoxLayout(trackTab);
    
    QGroupBox* trackGroup = new QGroupBox(QObject::tr("Track Selection"));
    QHBoxLayout* trackLayout2 = new QHBoxLayout(trackGroup);
    trackLayout2->addWidget(new QLabel(QObject::tr("Track:")));
    m_trackSelector = new QComboBox();
    trackLayout2->addWidget(m_trackSelector);
    trackLayout->addWidget(trackGroup);
    
    QGroupBox* splineGroup = new QGroupBox(QObject::tr("Spline Files"));
    QVBoxLayout* splineLayout = new QVBoxLayout(splineGroup);
    m_splineFileList = new QListWidget();
    splineLayout->addWidget(m_splineFileList);
    
    QHBoxLayout* splineBtnLayout = new QHBoxLayout();
    m_loadTrackBtn = new QPushButton(QObject::tr("Load Spline"));
    m_saveTrackBtn = new QPushButton(QObject::tr("Save Spline"));
    m_smoothBtn = new QPushButton(QObject::tr("Smooth Spline"));
    m_resampleBtn = new QPushButton(QObject::tr("Resample"));
    splineBtnLayout->addWidget(m_loadTrackBtn);
    splineBtnLayout->addWidget(m_saveTrackBtn);
    splineBtnLayout->addWidget(m_smoothBtn);
    splineBtnLayout->addWidget(m_resampleBtn);
    splineLayout->addLayout(splineBtnLayout);
    
    QHBoxLayout* splineOptionsLayout = new QHBoxLayout();
    splineOptionsLayout->addWidget(new QLabel(QObject::tr("Target Points:")));
    m_targetPointsEdit = new QLineEdit("500");
    m_targetPointsEdit->setMaximumWidth(80);
    splineOptionsLayout->addWidget(m_targetPointsEdit);
    splineOptionsLayout->addWidget(new QLabel(QObject::tr("Iterations:")));
    m_smoothIterationsSpin = new QSpinBox();
    m_smoothIterationsSpin->setRange(1, 20);
    m_smoothIterationsSpin->setValue(5);
    splineOptionsLayout->addWidget(m_smoothIterationsSpin);
    splineOptionsLayout->addStretch();
    splineLayout->addLayout(splineOptionsLayout);
    
    m_splineInfoLabel = new QLabel(QObject::tr("No spline loaded"));
    m_splineInfoLabel->setWordWrap(true);
    splineLayout->addWidget(m_splineInfoLabel);
    
    trackLayout->addWidget(splineGroup);
    
    QGroupBox* aiLineGroup = new QGroupBox(QObject::tr("AI Line Generation"));
    QVBoxLayout* aiLineLayout = new QVBoxLayout(aiLineGroup);
    QHBoxLayout* aiLineBtnLayout = new QHBoxLayout();
    m_analyzeBtn = new QPushButton(QObject::tr("Analyze Line"));
    m_generateBtn = new QPushButton(QObject::tr("Generate AI Line"));
    m_optimizeBtn = new QPushButton(QObject::tr("Optimize"));
    aiLineBtnLayout->addWidget(m_analyzeBtn);
    aiLineBtnLayout->addWidget(m_generateBtn);
    aiLineBtnLayout->addWidget(m_optimizeBtn);
    aiLineLayout->addLayout(aiLineBtnLayout);
    m_resultLog = new QTextEdit();
    m_resultLog->setReadOnly(true);
    m_resultLog->setMaximumHeight(150);
    aiLineLayout->addWidget(m_resultLog);
    trackLayout->addWidget(aiLineGroup);
    
    trackLayout->addStretch();
    m_tabWidget->addTab(trackTab, QObject::tr("Track / Spline"));
    
    // Tab 2: AI Settings
    QWidget* aiTab = new QWidget();
    QVBoxLayout* aiLayout = new QVBoxLayout(aiTab);
    
    QGroupBox* profileGroup = new QGroupBox(QObject::tr("AI Driver Profile"));
    QFormLayout* profileLayout = new QFormLayout(profileGroup);
    m_difficultySpin = new QSpinBox();
    m_difficultySpin->setRange(0, 100);
    m_difficultySpin->setValue(50);
    m_aggressionSpin = new QSpinBox();
    m_aggressionSpin->setRange(0, 100);
    m_aggressionSpin->setValue(50);
    m_precisionSpin = new QSpinBox();
    m_precisionSpin->setRange(0, 100);
    m_precisionSpin->setValue(70);
    m_consistencySpin = new QSpinBox();
    m_consistencySpin->setRange(0, 100);
    m_consistencySpin->setValue(80);
    profileLayout->addRow(QObject::tr("Difficulty"), m_difficultySpin);
    profileLayout->addRow(QObject::tr("Aggression"), m_aggressionSpin);
    profileLayout->addRow(QObject::tr("Precision"), m_precisionSpin);
    profileLayout->addRow(QObject::tr("Consistency"), m_consistencySpin);
    aiLayout->addWidget(profileGroup);
    
    QGroupBox* toolsGroup = new QGroupBox(QObject::tr("Analysis Tools"));
    QVBoxLayout* toolsLayout = new QVBoxLayout(toolsGroup);
    QHBoxLayout* toolBtnLayout = new QHBoxLayout();
    m_analyzeBtn = new QPushButton(QObject::tr("Analyze Line"));
    m_generateBtn = new QPushButton(QObject::tr("Generate AI Line"));
    m_optimizeBtn = new QPushButton(QObject::tr("Optimize"));
    toolBtnLayout->addWidget(m_analyzeBtn);
    toolBtnLayout->addWidget(m_generateBtn);
    toolBtnLayout->addWidget(m_optimizeBtn);
    toolsLayout->addLayout(toolBtnLayout);
    m_resultLog = new QTextEdit();
    m_resultLog->setReadOnly(true);
    m_resultLog->setMaximumHeight(150);
    toolsLayout->addWidget(m_resultLog);
    aiLayout->addWidget(toolsGroup);
    
    aiLayout->addStretch();
    m_tabWidget->addTab(aiTab, QObject::tr("AI Settings"));
    
    // Tab 3: Profiles
    QWidget* profileTab = new QWidget();
    QVBoxLayout* profileTabLayout = new QVBoxLayout(profileTab);
    
    QGroupBox* profileListGroup = new QGroupBox(QObject::tr("Driver Profiles"));
    QVBoxLayout* profileListLayout = new QVBoxLayout(profileListGroup);
    
    QHBoxLayout* profileFilterLayout = new QHBoxLayout();
    profileFilterLayout->addWidget(new QLabel(QObject::tr("Style:")));
    m_profileStyleFilter = new QComboBox();
    m_profileStyleFilter->addItems({"All", "Aggressive", "Conservative", "Balanced", "Rookie", "Pro"});
    profileFilterLayout->addWidget(m_profileStyleFilter);
    profileFilterLayout->addWidget(new QLabel(QObject::tr("Tier:")));
    m_profileTierFilter = new QComboBox();
    m_profileTierFilter->addItems({"All", "Rookie", "Amateur", "Pro", "Legend"});
    profileFilterLayout->addWidget(m_profileTierFilter);
    profileListLayout->addLayout(profileFilterLayout);
    
    m_profileList = new QListWidget();
    profileListLayout->addWidget(m_profileList);
    
    QHBoxLayout* profileBtnLayout = new QHBoxLayout();
    m_exportProfileBtn = new QPushButton(QObject::tr("Export Profile"));
    m_compareProfilesBtn = new QPushButton(QObject::tr("Compare Profiles"));
    m_suggestImprovementsBtn = new QPushButton(QObject::tr("Suggest Improvements"));
    profileBtnLayout->addWidget(m_exportProfileBtn);
    profileBtnLayout->addWidget(m_compareProfilesBtn);
    profileBtnLayout->addWidget(m_suggestImprovementsBtn);
    profileListLayout->addLayout(profileBtnLayout);
    
    m_profileDetail = new QTextEdit();
    m_profileDetail->setReadOnly(true);
    m_profileDetail->setMaximumHeight(200);
    profileListLayout->addWidget(m_profileDetail);
    
    profileTabLayout->addWidget(profileListGroup);
    m_tabWidget->addTab(profileTab, QObject::tr("Profiles"));
    
    // Tab 4: Telemetry Training
    QWidget* telemetryTab = new QWidget();
    QVBoxLayout* telemetryLayout = new QVBoxLayout(telemetryTab);
    
    QGroupBox* telemetryGroup = new QGroupBox(QObject::tr("Telemetry Training"));
    QVBoxLayout* telemetryGroupLayout = new QVBoxLayout(telemetryGroup);
    
    QHBoxLayout* telemetryBtnLayout = new QHBoxLayout();
    m_loadTelemetryBtn = new QPushButton(QObject::tr("Load Telemetry"));
    m_analyzeTelemetryBtn = new QPushButton(QObject::tr("Analyze Telemetry"));
    m_applyTelemetryProfileBtn = new QPushButton(QObject::tr("Apply Profile"));
    m_clearTrainingDataBtn = new QPushButton(QObject::tr("Clear Training Data"));
    telemetryBtnLayout->addWidget(m_loadTelemetryBtn);
    telemetryBtnLayout->addWidget(m_analyzeTelemetryBtn);
    telemetryBtnLayout->addWidget(m_applyTelemetryProfileBtn);
    telemetryBtnLayout->addWidget(m_clearTrainingDataBtn);
    telemetryGroupLayout->addLayout(telemetryBtnLayout);
    
    m_trainingStatsLabel = new QLabel(QObject::tr("No training data loaded"));
    telemetryGroupLayout->addWidget(m_trainingStatsLabel);
    
    m_cornerAnalysisText = new QTextEdit();
    m_cornerAnalysisText->setReadOnly(true);
    m_cornerAnalysisText->setMaximumHeight(150);
    telemetryGroupLayout->addWidget(m_cornerAnalysisText);
    
    QGroupBox* trainedGroup = new QGroupBox(QObject::tr("Trained Tracks"));
    QVBoxLayout* trainedLayout = new QVBoxLayout(trainedGroup);
    m_trainedTrackList = new QListWidget();
    trainedLayout->addWidget(m_trainedTrackList);
    telemetryGroupLayout->addWidget(trainedGroup);
    
    telemetryLayout->addWidget(telemetryGroup);
    telemetryLayout->addStretch();
    m_tabWidget->addTab(telemetryTab, QObject::tr("Telemetry Training"));
    
    // Tab 5: Multi-Car Race
    QWidget* raceTab = new QWidget();
    QVBoxLayout* raceLayout = new QVBoxLayout(raceTab);
    
    QGroupBox* raceSetupGroup = new QGroupBox(QObject::tr("Race Setup"));
    QFormLayout* raceSetupLayout = new QFormLayout(raceSetupGroup);
    m_raceNumDriversSpin = new QSpinBox();
    m_raceNumDriversSpin->setRange(2, 40);
    m_raceNumDriversSpin->setValue(10);
    m_raceLapsSpin = new QSpinBox();
    m_raceLapsSpin->setRange(1, 100);
    m_raceLapsSpin->setValue(10);
    m_trackLengthSpin = new QDoubleSpinBox();
    m_trackLengthSpin->setRange(100, 50000);
    m_trackLengthSpin->setValue(5000);
    m_trackLengthSpin->setSuffix(" m");
    raceSetupLayout->addRow(QObject::tr("Drivers"), m_raceNumDriversSpin);
    raceSetupLayout->addRow(QObject::tr("Laps"), m_raceLapsSpin);
    raceSetupLayout->addRow(QObject::tr("Track Length"), m_trackLengthSpin);
    raceLayout->addWidget(raceSetupGroup);
    
    QGroupBox* raceControlGroup = new QGroupBox(QObject::tr("Race Control"));
    QHBoxLayout* raceControlLayout = new QHBoxLayout(raceControlGroup);
    m_startRaceBtn = new QPushButton(QObject::tr("Start Race"));
    m_startRaceBtn->setStyleSheet("QPushButton { background-color: #2e7d32; color: white; font-weight: bold; padding: 8px; }");
    m_stopRaceBtn = new QPushButton(QObject::tr("Stop Race"));
    m_stopRaceBtn->setStyleSheet("QPushButton { background-color: #c62828; color: white; font-weight: bold; padding: 8px; }");
    m_stopRaceBtn->setEnabled(false);
    m_resetRaceBtn = new QPushButton(QObject::tr("Reset"));
    raceControlLayout->addWidget(m_startRaceBtn);
    raceControlLayout->addWidget(m_stopRaceBtn);
    raceControlLayout->addWidget(m_resetRaceBtn);
    raceLayout->addWidget(raceControlGroup);
    
    QGroupBox* raceStatusGroup = new QGroupBox(QObject::tr("Race Status"));
    QVBoxLayout* raceStatusLayout = new QVBoxLayout(raceStatusGroup);
    QHBoxLayout* raceStatusRow = new QHBoxLayout();
    m_raceStatusLabel = new QLabel(QObject::tr("Waiting to start..."));
    m_raceStatusLabel->setStyleSheet("font-weight: bold; color: #2e7d32;");
    m_raceTimeLabel = new QLabel(QObject::tr("Time: 00:00.000"));
    m_fastestLapLabel = new QLabel(QObject::tr("Fastest: --:--.---"));
    m_overtakesLabel = new QLabel(QObject::tr("Overtakes: 0"));
    raceStatusRow->addWidget(m_raceStatusLabel);
    raceStatusRow->addStretch();
    raceStatusRow->addWidget(m_raceTimeLabel);
    raceStatusRow->addWidget(m_fastestLapLabel);
    raceStatusRow->addWidget(m_overtakesLabel);
    raceStatusLayout->addLayout(raceStatusRow);
    
    m_raceLeaderboard = new QTableWidget();
    m_raceLeaderboard->setColumnCount(6);
    m_raceLeaderboard->setHorizontalHeaderLabels({"Pos", "Driver", "Lap", "Last Lap", "Best Lap", "Gap"});
    m_raceLeaderboard->horizontalHeader()->setStretchLastSection(true);
    m_raceLeaderboard->setAlternatingRowColors(true);
    m_raceLeaderboard->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_raceLeaderboard->setEditTriggers(QAbstractItemView::NoEditTriggers);
    raceStatusLayout->addWidget(m_raceLeaderboard);
    raceLayout->addWidget(raceStatusGroup);
    
    QGroupBox* eventGroup = new QGroupBox(QObject::tr("Race Events"));
    QVBoxLayout* eventLayout = new QVBoxLayout(eventGroup);
    m_raceEventLog = new QTextEdit();
    m_raceEventLog->setReadOnly(true);
    m_raceEventLog->setMaximumHeight(150);
    eventLayout->addWidget(m_raceEventLog);
    raceLayout->addWidget(eventGroup);
    
    raceLayout->addStretch();
    m_tabWidget->addTab(raceTab, QObject::tr("Multi-Car Race"));
    
    // Connections
    connect(m_trackSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIEditorModule::onTrackSelected);
    connect(m_loadTrackBtn, &QPushButton::clicked, this, &AIEditorModule::onLoadTrack);
    connect(m_saveTrackBtn, &QPushButton::clicked, this, &AIEditorModule::onSaveTrack);
    connect(m_smoothBtn, &QPushButton::clicked, this, &AIEditorModule::onSmoothSpline);
    connect(m_resampleBtn, &QPushButton::clicked, this, &AIEditorModule::onResampleSpline);
    connect(m_loadTrackBtn, &QPushButton::clicked, this, &AIEditorModule::onLoadTrack);
    connect(m_saveTrackBtn, &QPushButton::clicked, this, &AIEditorModule::onSaveTrack);
    connect(m_analyzeBtn, &QPushButton::clicked, this, &AIEditorModule::onAnalyzeLine);
    connect(m_generateBtn, &QPushButton::clicked, this, &AIEditorModule::onGenerateAILine);
    connect(m_optimizeBtn, &QPushButton::clicked, this, &AIEditorModule::onOptimizeLine);
    connect(m_difficultySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AIEditorModule::onDifficultyChanged);
    connect(m_aggressionSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AIEditorModule::onAggressionChanged);
    connect(m_loadTelemetryBtn, &QPushButton::clicked, this, &AIEditorModule::onLoadTelemetry);
    connect(m_analyzeTelemetryBtn, &QPushButton::clicked, this, &AIEditorModule::onAnalyzeTelemetry);
    connect(m_applyTelemetryProfileBtn, &QPushButton::clicked, this, &AIEditorModule::onApplyTelemetryProfile);
    connect(m_clearTrainingDataBtn, &QPushButton::clicked, this, &AIEditorModule::onClearTrainingData);
    connect(m_profileList, &QListWidget::currentRowChanged, this, &AIEditorModule::onProfileSelected);
    connect(m_profileStyleFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIEditorModule::onProfileStyleFilterChanged);
    connect(m_profileTierFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIEditorModule::onProfileTierFilterChanged);
    connect(m_compareProfilesBtn, &QPushButton::clicked, this, &AIEditorModule::onCompareProfiles);
    connect(m_suggestImprovementsBtn, &QPushButton::clicked, this, &AIEditorModule::onSuggestImprovements);
    connect(m_exportProfileBtn, &QPushButton::clicked, this, &AIEditorModule::onExportProfile);
    connect(m_loadTelemetryBtn, &QPushButton::clicked, this, &AIEditorModule::onLoadTelemetry);
    connect(m_analyzeTelemetryBtn, &QPushButton::clicked, this, &AIEditorModule::onAnalyzeTelemetry);
    connect(m_applyTelemetryProfileBtn, &QPushButton::clicked, this, &AIEditorModule::onApplyTelemetryProfile);
    connect(m_clearTrainingDataBtn, &QPushButton::clicked, this, &AIEditorModule::onClearTrainingData);
    connect(m_trainedTrackList, &QListWidget::currentRowChanged, this, &AIEditorModule::onTrainedTrackSelected);
    connect(m_startRaceBtn, &QPushButton::clicked, this, &AIEditorModule::onStartRace);
    connect(m_stopRaceBtn, &QPushButton::clicked, this, &AIEditorModule::onStopRace);
    connect(m_resetRaceBtn, &QPushButton::clicked, this, &AIEditorModule::onResetRace);
    connect(m_raceNumDriversSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AIEditorModule::onNumDriversChanged);
    connect(m_raceLapsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AIEditorModule::onRaceLapsChanged);
    connect(m_trackLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AIEditorModule::onTrackLengthChanged);
    
    // Populate track list
    populateTrackList();
    populateProfileList();
}

void AIEditorModule::populateTrackList() {
    // Scan for tracks in AC content directory
    QString acPath = "C:/Program Files (x86)/Steam/steamapps/common/assettocorsa"; // Default path
    if (!acPath.isEmpty()) {
        QString tracksPath = acPath + "/content/tracks";
        QDirIterator it(tracksPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QFileInfo fi(it.fileInfo());
            if (fi.fileName() == "data" || fi.fileName() == "ui" || fi.fileName() == "layout") {
                m_trackSelector->addItem(fi.fileName(), fi.absoluteFilePath());
            }
        }
    }
}

void AIEditorModule::populateProfileList() {
    // Populate with built-in profiles
    m_profileList->clear();
    QStringList profiles = {"Rookie", "Amateur", "Pro", "Legend", "Aggressive", "Conservative", "Balanced", "Defensive", "Unpredictable"};
    for (const QString& profile : profiles) {
        m_profileList->addItem(profile);
    }
}

AiBehaviorModel::AiDriverProfile AIEditorModule::resolveProfile(int row) const {
    // Return a default profile based on row
    AiBehaviorModel::AiDriverProfile profile;
    profile.skill = m_difficultySpin->value() / 100.0f;
    profile.aggression = m_aggressionSpin->value() / 100.0f;
    profile.defensive = m_precisionSpin->value() / 100.0f;
    profile.consistency = m_consistencySpin->value() / 100.0f;
    return profile;
}

void AIEditorModule::populateTelemetryTracks() {
    m_trainedTrackList->clear();
    QString acPath = "C:/Program Files (x86)/Steam/steamapps/common/assettocorsa"; // Default path
    if (!acPath.isEmpty()) {
        QString telemetryPath = acPath + "/telemetry";
        QDirIterator it(telemetryPath, QStringList() << "*.csv" << "*.bin", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            m_trainedTrackList->addItem(it.fileName());
        }
    }
}

void AIEditorModule::updateSplineInfo() {
    if (m_splineManager) {
        QString info = m_splineManager->getSplineInfo();
        m_splineInfoLabel->setText(info);
    }
}

void AIEditorModule::onTrackSelected(int index) {
    if (index >= 0) {
        m_currentTrackPath = m_trackSelector->itemData(index).toString();
        populateSplineFiles();
    }
}

void AIEditorModule::populateSplineFiles() {
    m_splineFileList->clear();
    if (m_currentTrackPath.isEmpty()) return;
    
    QDir trackDir(m_currentTrackPath);
    QFileInfoList splines = trackDir.entryInfoList({"*.spline", "*.ai_spline", "*.json"}, QDir::Files);
    for (const QFileInfo& fi : splines) {
        m_splineFileList->addItem(fi.fileName());
    }
}

void AIEditorModule::onLoadTrack() {
    QString filePath = QFileDialog::getOpenFileName(this, QObject::tr("Load Track/Spline"), m_currentTrackPath,
        QObject::tr("Spline Files (*.spline *.ai_spline *.json);;All Files (*)"));
    if (!filePath.isEmpty()) {
        if (m_splineManager->loadSpline(filePath)) {
            updateSplineInfo();
            m_statusLabel->setText(QObject::tr("Loaded: %1").arg(QFileInfo(filePath).fileName()));
        } else {
            QMessageBox::warning(this, QObject::tr("Error"), QObject::tr("Failed to load spline: %1").arg(filePath));
        }
    }
}

void AIEditorModule::onSaveTrack() {
    if (!m_splineManager || !m_splineManager->hasSpline()) {
        QMessageBox::warning(this, QObject::tr("Warning"), QObject::tr("No spline loaded to save"));
        return;
    }
    
    QString filePath = QFileDialog::getSaveFileName(this, QObject::tr("Save Spline"), m_currentTrackPath,
        QObject::tr("Spline Files (*.spline *.json);;All Files (*)"));
    if (!filePath.isEmpty()) {
        if (m_splineManager->saveSpline(filePath)) {
            m_statusLabel->setText(QObject::tr("Saved: %1").arg(QFileInfo(filePath).fileName()));
        } else {
            QMessageBox::warning(this, QObject::tr("Error"), QObject::tr("Failed to save spline"));
        }
    }
}

void AIEditorModule::onSmoothSpline() {
    if (!m_splineManager || !m_splineManager->hasSpline()) {
        QMessageBox::warning(this, QObject::tr("Warning"), QObject::tr("No spline loaded"));
        return;
    }
    
    int iterations = m_smoothIterationsSpin->value();
    int targetPoints = m_targetPointsEdit->text().toInt();
    
    m_resultLog->append(QObject::tr("Smoothing spline with %1 iterations...").arg(iterations));
    if (m_splineManager->smoothSpline(iterations, targetPoints)) {
        m_resultLog->append(QObject::tr("Spline smoothed successfully"));
        updateSplineInfo();
    } else {
        m_resultLog->append(QObject::tr("Failed to smooth spline"));
    }
}

void AIEditorModule::onResampleSpline() {
    if (!m_splineManager || !m_splineManager->hasSpline()) {
        QMessageBox::warning(this, QObject::tr("Warning"), QObject::tr("No spline loaded"));
        return;
    }
    
    int targetPoints = m_targetPointsEdit->text().toInt();
    
    m_resultLog->append(QObject::tr("Resampling spline to %1 points...").arg(targetPoints));
    if (m_splineManager->resampleSpline(targetPoints)) {
        m_resultLog->append(QObject::tr("Spline resampled successfully"));
        updateSplineInfo();
    } else {
        m_resultLog->append(QObject::tr("Failed to resample spline"));
    }
}

void AIEditorModule::onAnalyzeLine() {
    if (!m_splineManager || !m_splineManager->hasSpline()) {
        m_resultLog->append(QObject::tr("No spline loaded to analyze"));
        return;
    }
    
    m_resultLog->append(QObject::tr("Analyzing AI line..."));
    
    // Get spline points for analysis
    auto points = m_splineManager->getSplinePoints();
    if (points.isEmpty()) {
        m_resultLog->append(QObject::tr("No points in spline"));
        return;
    }
    
    // Calculate curvature at each point
    double totalCurvature = 0;
    double maxCurvature = 0;
    int maxCurvatureIdx = 0;
    QVector<double> curvatures;
    
    for (int i = 1; i < points.size() - 1; ++i) {
        QVector3D prev = points[i - 1];
        QVector3D curr = points[i];
        QVector3D next = points[i + 1];
        
        QVector3D v1 = curr - prev;
        QVector3D v2 = next - curr;
        float cross = QVector3D::crossProduct(v1, v2).length();
        float dot = QVector3D::dotProduct(v1, v2);
        float curvature = cross / (v1.length() * v2.length() + 0.001f);
        
        curvatures.append(curvature);
        totalCurvature += curvature;
        if (curvature > maxCurvature) {
            maxCurvature = curvature;
            maxCurvatureIdx = i;
        }
    }
    
    double avgCurvature = curvatures.isEmpty() ? 0 : totalCurvature / curvatures.size();
    
    // Calculate total spline length
    double totalLength = 0;
    for (int i = 1; i < points.size(); ++i) {
        totalLength += (points[i] - points[i - 1]).length();
    }
    
    // Find straights and corners
    int cornerCount = 0;
    int straightCount = 0;
    for (double c : curvatures) {
        if (c > 0.3) cornerCount++;
        else if (c < 0.05) straightCount++;
    }
    
    // Calculate optimal speeds (simplified physics)
    double maxSpeed = 0;
    double minSpeed = std::numeric_limits<double>::max();
    for (int i = 1; i < points.size() - 1; ++i) {
        double curvature = (i < curvatures.size()) ? curvatures[i] : 0;
        // Simplified: v = sqrt(1/curvature * g * friction_coefficient)
        double frictionCoeff = 1.5; // Approximate
        double gravity = 9.81;
        double speed = (curvature > 0.01) ? std::sqrt(1.0 / curvature * gravity * frictionCoeff) : 300.0;
        speed = qMin(speed, 300.0); // Cap at 300 km/h
        maxSpeed = qMax(maxSpeed, speed);
        minSpeed = qMin(minSpeed, speed);
    }
    
    // Generate analysis report
    QString report;
    report += QObject::tr("=== AI Line Analysis Report ===\n");
    report += QObject::tr("Spline length: %1 m\n").arg(totalLength, 0, 'f', 1);
    report += QObject::tr("Points: %1\n").arg(points.size());
    report += QObject::tr("Average curvature: %1\n").arg(avgCurvature, 0, 'f', 4);
    report += QObject::tr("Maximum curvature: %2 (at point %1)\n").arg(maxCurvatureIdx).arg(maxCurvature, 0, 'f', 4);
    report += QObject::tr("Corners: %1\n").arg(cornerCount);
    report += QObject::tr("Straights: %1\n").arg(straightCount);
    report += QObject::tr("Estimated max speed: %1 km/h\n").arg(maxSpeed, 0, 'f', 1);
    report += QObject::tr("Estimated min speed: %1 km/h\n").arg(minSpeed, 0, 'f', 1);
    
    m_resultLog->append(report);
    m_resultLog->append(QObject::tr("Analysis complete: curvature, banking, optimal speeds calculated"));
}

void AIEditorModule::onGenerateAILine() {
    if (!m_splineManager || !m_splineManager->hasSpline()) {
        m_resultLog->append(QObject::tr("No spline loaded"));
        return;
    }
    
    m_resultLog->append(QObject::tr("Generating AI line..."));
    
    auto points = m_splineManager->getSplinePoints();
    if (points.size() < 3) {
        m_resultLog->append(QObject::tr("Need at least 3 points to generate AI line"));
        return;
    }
    
    // Generate racing line by computing center of track at each point
    QVector<QVector3D> racingLine;
    float difficulty = m_difficultySpin->value() / 100.0f;
    float aggression = m_aggressionSpin->value() / 100.0f;
    
    for (int i = 0; i < points.size(); ++i) {
        QVector3D point = points[i];
        
        // Compute local curvature
        QVector3D prev = (i > 0) ? points[i - 1] : point;
        QVector3D next = (i < points.size() - 1) ? points[i + 1] : point;
        QVector3D tangent = (next - prev).normalized();
        QVector3D normal = QVector3D::crossProduct(QVector3D(0, 1, 0), tangent).normalized();
        
        // Apply racing line offset based on curvature and aggression
        float curvature = 0;
        if (i > 0 && i < points.size() - 1) {
            QVector3D v1 = points[i] - points[i - 1];
            QVector3D v2 = points[i + 1] - points[i];
            curvature = QVector3D::crossProduct(v1, v2).length() / (v1.length() * v2.length() + 0.001f);
        }
        
        // Inside line on straights, outside on corners (apex)
        float offset = curvature * aggression * difficulty * 5.0f;
        QVector3D racingPoint = point + normal * offset;
        
        racingLine.append(racingPoint);
    }
    
    // Smooth the racing line
    for (int pass = 0; pass < 3; ++pass) {
        QVector<QVector3D> smoothed;
        smoothed.append(racingLine.first());
        
        for (int i = 1; i < racingLine.size() - 1; ++i) {
            QVector3D avg = (racingLine[i - 1] + racingLine[i] * 2 + racingLine[i + 1]) / 4.0f;
            smoothed.append(avg);
        }
        smoothed.append(racingLine.last());
        racingLine = smoothed;
    }
    
    // Apply to spline manager
    m_splineManager->setSplinePoints(racingLine);
    
    m_resultLog->append(QObject::tr("AI line generated: %1 points, difficulty=%2%, aggression=%3%")
        .arg(racingLine.size())
        .arg(m_difficultySpin->value())
        .arg(m_aggressionSpin->value()));
    m_resultLog->append(QObject::tr("AI line generated successfully"));
}

void AIEditorModule::onOptimizeLine() {
    if (!m_splineManager || !m_splineManager->hasSpline()) {
        m_resultLog->append(QObject::tr("No spline loaded"));
        return;
    }
    
    m_resultLog->append(QObject::tr("Optimizing line..."));
    
    auto points = m_splineManager->getSplinePoints();
    if (points.size() < 3) {
        m_resultLog->append(QObject::tr("Need at least 3 points to optimize"));
        return;
    }
    
    float precision = m_precisionSpin->value() / 100.0f;
    float consistency = m_consistencySpin->value() / 100.0f;
    int iterations = static_cast<int>(precision * 50) + 10;
    
    // Iterative smoothing with corner preservation
    QVector<QVector3D> optimized = points;
    
    for (int iter = 0; iter < iterations; ++iter) {
        QVector<QVector3D> smoothed;
        smoothed.append(optimized.first());
        
        for (int i = 1; i < optimized.size() - 1; ++i) {
            QVector3D prev = optimized[i - 1];
            QVector3D curr = optimized[i];
            QVector3D next = optimized[i + 1];
            
            // Compute curvature
            QVector3D v1 = curr - prev;
            QVector3D v2 = next - curr;
            float curvature = QVector3D::crossProduct(v1, v2).length() / (v1.length() * v2.length() + 0.001f);
            
            // Less smoothing on high-curvature areas (corners)
            float smoothFactor = 0.1f + (1.0f - curvature) * 0.3f * consistency;
            QVector3D avg = prev * smoothFactor + curr * (1.0f - 2.0f * smoothFactor) + next * smoothFactor;
            
            smoothed.append(avg);
        }
        smoothed.append(optimized.last());
        optimized = smoothed;
    }
    
    // Remove redundant points that are too close
    QVector<QVector3D> filtered;
    filtered.append(optimized.first());
    float minDist = 0.5f; // minimum distance between points
    
    for (int i = 1; i < optimized.size() - 1; ++i) {
        float dist = (optimized[i] - filtered.last()).length();
        if (dist >= minDist) {
            filtered.append(optimized[i]);
        }
    }
    filtered.append(optimized.last());
    
    m_splineManager->setSplinePoints(filtered);
    
    m_resultLog->append(QObject::tr("Line optimized: %1 iterations, %2 points (was %3)")
        .arg(iterations).arg(filtered.size()).arg(points.size()));
    m_resultLog->append(QObject::tr("Line optimized successfully"));
}

void AIEditorModule::onDifficultyChanged(int v) {
    m_statusLabel->setText(QObject::tr("Difficulty: %1%").arg(v));
}

void AIEditorModule::onAggressionChanged(int v) {
    m_statusLabel->setText(QObject::tr("Aggression: %1%").arg(v));
}

void AIEditorModule::onPresetApplied(const QString& name) {
    m_statusLabel->setText(QObject::tr("Applied preset: %1").arg(name));
}

void AIEditorModule::onProfileSelected(int row) {
    if (row >= 0) {
        QString profile = m_profileList->item(row)->text();
        m_profileDetail->setText(QObject::tr("Profile: %1\nDifficulty: %2\nAggression: %3\nPrecision: %4\nConsistency: %5")
            .arg(profile)
            .arg(m_difficultySpin->value())
            .arg(m_aggressionSpin->value())
            .arg(m_precisionSpin->value())
            .arg(m_consistencySpin->value()));
    }
}

void AIEditorModule::onProfileStyleFilterChanged(int index) {
    m_statusLabel->setText(QObject::tr("Style filter: %1").arg(m_profileStyleFilter->currentText()));
}

void AIEditorModule::onProfileTierFilterChanged(int index) {
    m_statusLabel->setText(QObject::tr("Tier filter: %1").arg(m_profileTierFilter->currentText()));
}

void AIEditorModule::onCompareProfiles() {
    m_resultLog->append(QObject::tr("Comparing profiles..."));
    
    // Compare current profile settings
    int difficulty = m_difficultySpin->value();
    int aggression = m_aggressionSpin->value();
    int precision = m_precisionSpin->value();
    int consistency = m_consistencySpin->value();
    
    // Calculate profile characteristics
    QString style;
    if (aggression > 70 && precision > 70) style = "Aggressive Precision";
    else if (aggression > 70) style = "Aggressive";
    else if (precision > 70) style = "Precision";
    else if (consistency > 70) style = "Consistent";
    else style = "Balanced";
    
    QString tier;
    if (difficulty > 80) tier = "Expert";
    else if (difficulty > 60) tier = "Advanced";
    else if (difficulty > 40) tier = "Intermediate";
    else tier = "Beginner";
    
    // Compute composite score
    float score = (difficulty * 0.3f + aggression * 0.2f + precision * 0.3f + consistency * 0.2f) / 100.0f;
    
    // Generate comparison report
    QString report;
    report += QObject::tr("=== Profile Comparison ===\n");
    report += QObject::tr("Style: %1\n").arg(style);
    report += QObject::tr("Tier: %1\n").arg(tier);
    report += QObject::tr("Composite Score: %1/100\n").arg(score * 100, 0, 'f', 1);
    report += QObject::tr("Strengths: ");
    
    QStringList strengths;
    if (difficulty > 70) strengths << QObject::tr("High difficulty handling");
    if (aggression > 70) strengths << QObject::tr("Aggressive overtaking");
    if (precision > 70) strengths << QObject::tr("Precise cornering");
    if (consistency > 70) strengths << QObject::tr("Consistent lap times");
    report += strengths.isEmpty() ? QObject::tr("None identified") : strengths.join(", ");
    
    report += QObject::tr("\nWeaknesses: ");
    QStringList weaknesses;
    if (difficulty < 40) weaknesses << QObject::tr("Low difficulty adaptation");
    if (aggression < 40) weaknesses << QObject::tr("Passive driving style");
    if (precision < 40) weaknesses << QObject::tr("Imprecise corner entry");
    if (consistency < 40) weaknesses << QObject::tr("Inconsistent lap times");
    report += weaknesses.isEmpty() ? QObject::tr("None identified") : weaknesses.join(", ");
    
    m_resultLog->append(report);
    m_resultLog->append(QObject::tr("Comparison complete"));
}

void AIEditorModule::onSuggestImprovements() {
    m_resultLog->append(QObject::tr("Suggesting profile improvements..."));
    
    int difficulty = m_difficultySpin->value();
    int aggression = m_aggressionSpin->value();
    int precision = m_precisionSpin->value();
    int consistency = m_consistencySpin->value();
    
    QStringList suggestions;
    
    // Analyze and suggest improvements
    if (difficulty < 50) {
        suggestions << QObject::tr("Increase difficulty to %1 for more challenging AI behavior").arg(qMin(100, difficulty + 20));
    }
    if (aggression < 30) {
        suggestions << QObject::tr("Increase aggression to improve overtaking frequency");
    }
    if (aggression > 85) {
        suggestions << QObject::tr("Decrease aggression slightly to reduce collision risk");
    }
    if (precision < 50) {
        suggestions << QObject::tr("Increase precision for better cornering lines");
    }
    if (consistency < 40) {
        suggestions << QObject::tr("Increase consistency for more predictable lap times");
    }
    
    // Balance analysis
    float imbalance = std::abs(static_cast<float>(aggression - precision)) / 100.0f;
    if (imbalance > 0.4f) {
        suggestions << QObject::tr("Balance aggression (%1) and precision (%2) for optimal performance")
            .arg(aggression).arg(precision);
    }
    
    // Track-specific suggestions
    if (m_splineManager && m_splineManager->hasSpline()) {
        auto points = m_splineManager->getSplinePoints();
        int cornerCount = 0;
        for (int i = 1; i < points.size() - 1; ++i) {
            QVector3D v1 = points[i] - points[i - 1];
            QVector3D v2 = points[i + 1] - points[i];
            float curvature = QVector3D::crossProduct(v1, v2).length() / (v1.length() * v2.length() + 0.001f);
            if (curvature > 0.3) cornerCount++;
        }
        
        if (cornerCount > 10 && precision < 60) {
            suggestions << QObject::tr("Track has %1 corners - increase precision for better cornering").arg(cornerCount);
        }
    }
    
    if (suggestions.isEmpty()) {
        suggestions << QObject::tr("Profile is well-balanced. Consider fine-tuning for specific tracks.");
    }
    
    QString report = QObject::tr("=== Improvement Suggestions ===\n");
    for (int i = 0; i < suggestions.size(); ++i) {
        report += QObject::tr("%1. %2\n").arg(i + 1).arg(suggestions[i]);
    }
    
    m_resultLog->append(report);
    m_resultLog->append(QObject::tr("Suggestions generated"));
}

void AIEditorModule::onExportProfile() {
    QString filePath = QFileDialog::getSaveFileName(this, QObject::tr("Export Profile"), "",
        QObject::tr("JSON Files (*.json);;All Files (*)"));
    if (!filePath.isEmpty()) {
        // Export current profile settings
        QJsonObject profile;
        profile["difficulty"] = m_difficultySpin->value();
        profile["aggression"] = m_aggressionSpin->value();
        profile["precision"] = m_precisionSpin->value();
        profile["consistency"] = m_consistencySpin->value();
        
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(profile).toJson());
            QMessageBox::information(this, QObject::tr("Success"), QObject::tr("Profile exported to %1").arg(filePath));
        }
    }
}

void AIEditorModule::onLoadTelemetry() {
    QString filePath = QFileDialog::getOpenFileName(this, QObject::tr("Load Telemetry"), "",
        QObject::tr("Telemetry Files (*.csv *.bin *.json);;All Files (*)"));
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            m_trainingStatsLabel->setText(QObject::tr("Failed to open file: %1").arg(filePath));
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject data = doc.object();
        
        // Parse telemetry data
        m_telemetryData.clear();
        QJsonArray laps = data["laps"].toArray();
        for (const auto& lapVal : laps) {
            QJsonObject lapObj = lapVal.toObject();
            TelemetryLap lap;
            lap.lapTime = lapObj["lapTime"].toDouble();
            lap.topSpeed = lapObj["topSpeed"].toDouble();
            lap.avgSpeed = lapObj["avgSpeed"].toDouble();
            lap.brakingPoints = lapObj["brakingPoints"].toInt();
            lap.overtakes = lapObj["overtakes"].toInt();
            
            QJsonArray corners = lapObj["corners"].toArray();
            for (const auto& cornerVal : corners) {
                QJsonObject c = cornerVal.toObject();
                TelemetryCorner corner;
                corner.entrySpeed = c["entrySpeed"].toDouble();
                corner.exitSpeed = c["exitSpeed"].toDouble();
                corner.minSpeed = c["minSpeed"].toDouble();
                corner.maxLateralG = c["maxLateralG"].toDouble();
                lap.corners.append(corner);
            }
            m_telemetryData.append(lap);
        }
        
        m_trainingStatsLabel->setText(QObject::tr("Loaded: %1 (%2 laps)")
            .arg(QFileInfo(filePath).fileName()).arg(m_telemetryData.size()));
        
        // Analyze loaded data
        if (!m_telemetryData.isEmpty()) {
            double avgLapTime = 0;
            double bestLapTime = std::numeric_limits<double>::max();
            double avgSpeed = 0;
            
            for (const auto& lap : m_telemetryData) {
                avgLapTime += lap.lapTime;
                bestLapTime = qMin(bestLapTime, lap.lapTime);
                avgSpeed += lap.avgSpeed;
            }
            avgLapTime /= m_telemetryData.size();
            avgSpeed /= m_telemetryData.size();
            
            m_cornerAnalysisText->append(QObject::tr("Telemetry Summary:\n"
                "- Laps loaded: %1\n"
                "- Average lap time: %2s\n"
                "- Best lap time: %3s\n"
                "- Average speed: %4 km/h")
                .arg(m_telemetryData.size())
                .arg(avgLapTime, 0, 'f', 3)
                .arg(bestLapTime, 0, 'f', 3)
                .arg(avgSpeed, 0, 'f', 1));
        }
    }
}

void AIEditorModule::onAnalyzeTelemetry() {
    m_cornerAnalysisText->append(QObject::tr("Analyzing telemetry data..."));
    
    if (m_telemetryData.isEmpty()) {
        m_cornerAnalysisText->append(QObject::tr("No telemetry data loaded"));
        return;
    }
    
    // Analyze corners across all laps
    int totalCorners = 0;
    int heavyBrakingZones = 0;
    int highSpeedCorners = 0;
    double totalLateralG = 0;
    double maxLateralG = 0;
    int cornerCount = 0;
    
    // Detect corner patterns
    QMap<int, QVector<double>> cornerSpeeds;
    
    for (const auto& lap : m_telemetryData) {
        for (int c = 0; c < lap.corners.size(); ++c) {
            const auto& corner = lap.corners[c];
            cornerSpeeds[c].append(corner.minSpeed);
            totalLateralG += corner.maxLateralG;
            cornerCount++;
            maxLateralG = qMax(maxLateralG, corner.maxLateralG);
            
            if (corner.entrySpeed - corner.exitSpeed > 50) heavyBrakingZones++;
            if (corner.minSpeed > 150) highSpeedCorners++;
        }
        totalCorners = qMax(totalCorners, lap.corners.size());
    }
    
    double avgLateralG = cornerCount > 0 ? totalLateralG / cornerCount : 0;
    
    // Analyze optimal line deviation
    double totalDeviation = 0;
    int deviationSamples = 0;
    for (const auto& lap : m_telemetryData) {
        for (const auto& corner : lap.corners) {
            // Compare exit speed to theoretical max (simplified)
            double theoreticalMax = corner.entrySpeed * 0.85;
            double deviation = std::abs(corner.exitSpeed - theoreticalMax) / theoreticalMax * 100.0;
            totalDeviation += deviation;
            deviationSamples++;
        }
    }
    double avgDeviation = deviationSamples > 0 ? totalDeviation / deviationSamples : 0;
    
    // Generate analysis report
    QString report;
    report += QObject::tr("=== Telemetry Analysis ===\n");
    report += QObject::tr("Laps analyzed: %1\n").arg(m_telemetryData.size());
    report += QObject::tr("Corners detected: %1\n").arg(totalCorners);
    report += QObject::tr("Heavy braking zones: %1\n").arg(heavyBrakingZones);
    report += QObject::tr("High-speed corners: %1\n").arg(highSpeedCorners);
    report += QObject::tr("Average lateral G: %1\n").arg(avgLateralG, 0, 'f', 2);
    report += QObject::tr("Maximum lateral G: %1\n").arg(maxLateralG, 0, 'f', 2);
    report += QObject::tr("Optimal line deviation: %1%\n").arg(avgDeviation, 0, 'f', 1);
    
    // Identify specific improvements
    report += QObject::tr("\nRecommendations:\n");
    if (avgDeviation > 5) {
        report += QObject::tr("- Work on hitting apexes more consistently\n");
    }
    if (heavyBrakingZones > 3) {
        report += QObject::tr("- Consider earlier braking points in %1 zones\n").arg(heavyBrakingZones);
    }
    if (avgLateralG > 1.5) {
        report += QObject::tr("- High G-forces detected - check tire setup\n");
    }
    
    m_cornerAnalysisText->append(report);
    m_cornerAnalysisText->append(QObject::tr("Analysis complete:\n- %1 corners detected\n- %2 heavy braking zones\n- %3 high-speed corners\n- Optimal line deviation: %4%")
        .arg(totalCorners).arg(heavyBrakingZones).arg(highSpeedCorners).arg(avgDeviation, 0, 'f', 1));
}

void AIEditorModule::onApplyTelemetryProfile() {
    m_statusLabel->setText(QObject::tr("Applying telemetry-derived profile..."));
    
    if (m_telemetryData.isEmpty()) {
        m_statusLabel->setText(QObject::tr("No telemetry data to derive profile from"));
        return;
    }
    
    // Analyze telemetry to derive optimal profile settings
    double avgLapTime = 0;
    double avgTopSpeed = 0;
    double avgAggression = 0;
    int totalOvertakes = 0;
    int totalLaps = m_telemetryData.size();
    
    for (const auto& lap : m_telemetryData) {
        avgLapTime += lap.lapTime;
        avgTopSpeed += lap.topSpeed;
        totalOvertakes += lap.overtakes;
        
        // Estimate aggression from overtakes and cornering style
        double aggressionFactor = lap.overtakes * 10.0;
        for (const auto& corner : lap.corners) {
            if (corner.entrySpeed > corner.exitSpeed * 1.2) {
                aggressionFactor += 5.0; // Late braking = aggressive
            }
        }
        avgAggression += qMin(aggressionFactor, 100.0);
    }
    
    avgLapTime /= totalLaps;
    avgTopSpeed /= totalLaps;
    avgAggression /= totalLaps;
    
    // Derive profile from telemetry
    int newDifficulty = qBound(10, static_cast<int>(100.0 - avgLapTime * 2.0), 100);
    int newAggression = qBound(10, static_cast<int>(avgAggression), 100);
    int newPrecision = qBound(10, static_cast<int>(100.0 - (totalOvertakes * 5.0 / totalLaps)), 100);
    int newConsistency = qBound(10, static_cast<int>(100.0 - std::abs(avgTopSpeed - 200.0)), 100);
    
    // Apply derived values
    m_difficultySpin->setValue(newDifficulty);
    m_aggressionSpin->setValue(newAggression);
    m_precisionSpin->setValue(newPrecision);
    m_consistencySpin->setValue(newConsistency);
    
    QString report = QObject::tr("Profile derived from %1 laps of telemetry:\n"
        "- Difficulty: %2\n"
        "- Aggression: %3\n"
        "- Precision: %4\n"
        "- Consistency: %5\n"
        "- Avg lap time: %6s\n"
        "- Avg top speed: %7 km/h\n"
        "- Total overtakes: %8")
        .arg(totalLaps)
        .arg(newDifficulty).arg(newAggression).arg(newPrecision).arg(newConsistency)
        .arg(avgLapTime, 0, 'f', 3)
        .arg(avgTopSpeed, 0, 'f', 1)
        .arg(totalOvertakes);
    
    m_statusLabel->setText(QObject::tr("Profile applied successfully"));
    m_resultLog->append(report);
}

void AIEditorModule::onClearTrainingData() {
    if (QMessageBox::question(this, QObject::tr("Confirm"), QObject::tr("Clear all training data?")) == QMessageBox::Yes) {
        // Clear all telemetry data
        m_telemetryData.clear();
        
        // Clear UI elements
        m_trainingStatsLabel->setText(QObject::tr("No training data loaded"));
        m_cornerAnalysisText->clear();
        m_trainedTrackList->clear();
        
        // Reset profile settings to defaults
        m_difficultySpin->setValue(50);
        m_aggressionSpin->setValue(50);
        m_precisionSpin->setValue(50);
        m_consistencySpin->setValue(50);
        
        // Clear any cached analysis
        m_resultLog->append(QObject::tr("All training data cleared"));
    }
}

void AIEditorModule::onTrainedTrackSelected(int row) {
    if (row >= 0) {
        QString track = m_trainedTrackList->item(row)->text();
        m_statusLabel->setText(QObject::tr("Selected trained track: %1").arg(track));
    }
}

void AIEditorModule::onStartRace() {
    m_raceTimer = new QTimer(this);
    connect(m_raceTimer, &QTimer::timeout, this, &AIEditorModule::onRaceTick);
    
    m_currentNumDrivers = m_raceNumDriversSpin->value();
    m_raceTotalLaps = m_raceLapsSpin->value();
    m_trackLength = m_trackLengthSpin->value();
    
    // Initialize MultiCarAI with real simulation
    m_multiCarAI.setupRace(m_currentNumDrivers, m_raceTotalLaps, m_trackLength);
    
    // Setup leaderboard from AI grid
    const auto& grid = m_multiCarAI.grid();
    m_raceLeaderboard->setRowCount(grid.drivers.size());
    for (int i = 0; i < grid.drivers.size(); ++i) {
        const auto& driver = grid.drivers[i];
        m_raceLeaderboard->setItem(i, 0, new QTableWidgetItem(QString::number(driver.position)));
        m_raceLeaderboard->setItem(i, 1, new QTableWidgetItem(driver.name));
        m_raceLeaderboard->setItem(i, 2, new QTableWidgetItem("0"));
        m_raceLeaderboard->setItem(i, 3, new QTableWidgetItem("--:--.---"));
        m_raceLeaderboard->setItem(i, 4, new QTableWidgetItem("--:--.---"));
        m_raceLeaderboard->setItem(i, 5, new QTableWidgetItem("--"));
    }
    
    m_raceEventLog->clear();
    m_raceEventLog->append(QObject::tr("Race started with %1 drivers, %2 laps").arg(m_currentNumDrivers).arg(m_raceTotalLaps));
    
    m_raceStatusLabel->setText(QObject::tr("Racing..."));
    m_raceStatusLabel->setStyleSheet("font-weight: bold; color: #c62828;");
    m_startRaceBtn->setEnabled(false);
    m_stopRaceBtn->setEnabled(true);
    
    m_raceTimer->start(1000); // 1 second per tick
}

void AIEditorModule::onStopRace() {
    if (m_raceTimer) {
        m_raceTimer->stop();
        m_raceTimer->deleteLater();
        m_raceTimer = nullptr;
    }
    
    m_raceEventLog->append(QObject::tr("Race stopped"));
    m_raceStatusLabel->setText(QObject::tr("Stopped"));
    m_raceStatusLabel->setStyleSheet("font-weight: bold; color: #c62828;");
    m_startRaceBtn->setEnabled(true);
    m_stopRaceBtn->setEnabled(false);
}

void AIEditorModule::onResetRace() {
    onStopRace();
    m_raceLeaderboard->clearContents();
    m_raceEventLog->clear();
    m_raceTimeLabel->setText(QObject::tr("Time: 00:00.000"));
    m_fastestLapLabel->setText(QObject::tr("Fastest: --:--.---"));
    m_overtakesLabel->setText(QObject::tr("Overtakes: 0"));
    m_raceStatusLabel->setText(QObject::tr("Waiting to start..."));
    m_raceStatusLabel->setStyleSheet("font-weight: bold; color: #2e7d32;");
    m_startRaceBtn->setEnabled(true);
    m_stopRaceBtn->setEnabled(false);
}

void AIEditorModule::onRaceTick() {
    // Advance real AI simulation
    m_multiCarAI.tick(1.0f);
    
    const auto& grid = m_multiCarAI.grid();
    float raceTime = m_multiCarAI.grid().drivers.isEmpty() ? 0.0f : grid.drivers.first().raceTime;
    int minutes = static_cast<int>(raceTime) / 60;
    int seconds = static_cast<int>(raceTime) % 60;
    int millis = static_cast<int>((raceTime - static_cast<int>(raceTime)) * 1000);
    m_raceTimeLabel->setText(QObject::tr("Time: %1:%2.%3").arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')).arg(millis, 3, 10, QChar('0')));
    
    // Process events from MultiCarAI
    auto events = m_multiCarAI.consumeEvents();
    for (const auto& event : events) {
        if (event.type == RaceEvent::OVERTAKE) {
            m_raceEventLog->append(event.description);
        } else if (event.type == RaceEvent::LAP_COMPLETED) {
            auto* driver = m_multiCarAI.getDriver(event.driverId);
            if (driver) {
                m_raceEventLog->append(QObject::tr("Lap %1: %2 - %3s").arg(driver->lap).arg(driver->name).arg(driver->lastLapTime, 0, 'f', 3));
            }
        } else if (event.type == RaceEvent::FINISH) {
            auto* driver = m_multiCarAI.getDriver(event.driverId);
            if (driver) {
                m_raceEventLog->append(QObject::tr("Finish: %1").arg(driver->name));
            }
        } else if (event.type == RaceEvent::DNF) {
            auto* driver = m_multiCarAI.getDriver(event.driverId);
            if (driver) {
                m_raceEventLog->append(QObject::tr("DNF: %1").arg(driver->name));
            }
        }
    }
    
    // Update leaderboard from real AI data
    auto leaderboard = m_multiCarAI.getLeaderboard();
    m_raceLeaderboard->setRowCount(leaderboard.size());
    for (int i = 0; i < leaderboard.size(); ++i) {
        const auto& driver = leaderboard[i];
        if (m_raceLeaderboard->item(i, 0))
            m_raceLeaderboard->item(i, 0)->setText(QString::number(driver.position));
        if (m_raceLeaderboard->item(i, 1))
            m_raceLeaderboard->item(i, 1)->setText(driver.name);
        if (m_raceLeaderboard->item(i, 2))
            m_raceLeaderboard->item(i, 2)->setText(QString::number(driver.lap));
        if (m_raceLeaderboard->item(i, 3)) {
            if (driver.lastLapTime > 0 && driver.lastLapTime < 1e8f)
                m_raceLeaderboard->item(i, 3)->setText(QObject::tr("%1s").arg(driver.lastLapTime, 0, 'f', 3));
            else
                m_raceLeaderboard->item(i, 3)->setText("--:--.---");
        }
        if (m_raceLeaderboard->item(i, 4)) {
            if (driver.bestLapTime > 0 && driver.bestLapTime < 1e8f)
                m_raceLeaderboard->item(i, 4)->setText(QObject::tr("%1s").arg(driver.bestLapTime, 0, 'f', 3));
            else
                m_raceLeaderboard->item(i, 4)->setText("--:--.---");
        }
        if (m_raceLeaderboard->item(i, 5))
            m_raceLeaderboard->item(i, 5)->setText(driver.dnf ? "DNF" : (driver.finished ? "FIN" : "--"));
    }
    
    // Update overtakes count
    m_overtakesLabel->setText(QObject::tr("Overtakes: %1").arg(m_multiCarAI.getTotalOvertakes()));
    
    // Update fastest lap
    float fastestLap = m_multiCarAI.getFastestLap();
    if (fastestLap < 1e8f) {
        m_fastestLapLabel->setText(QObject::tr("Fastest: %1s (Driver %2)").arg(fastestLap, 0, 'f', 3).arg(m_multiCarAI.getFastestLapDriver() + 1));
    }
    
    // Check race finished
    if (m_multiCarAI.isRaceComplete()) {
        m_raceTimer->stop();
        m_raceStatusLabel->setText(QObject::tr("Race Complete"));
        m_raceStatusLabel->setStyleSheet("font-weight: bold; color: #2e7d32;");
        m_startRaceBtn->setEnabled(true);
        m_stopRaceBtn->setEnabled(false);
        m_raceEventLog->append(QObject::tr("Race complete!"));
    }
}

void AIEditorModule::onNumDriversChanged(int v) {
    m_currentNumDrivers = v;
}

void AIEditorModule::onRaceLapsChanged(int v) {
    m_raceTotalLaps = v;
}

void AIEditorModule::onTrackLengthChanged(double v) {
    m_trackLength = v;
}

void AIEditorModule::onActivation() {
    // Called when module becomes active
    populateTrackList();
    populateProfileList();
    populateTelemetryTracks();
}

void AIEditorModule::onDeactivation() {
    // Called when module becomes inactive
    if (m_raceTimer) {
        m_raceTimer->stop();
    }
}

QJsonObject AIEditorModule::serializeProject() const {
    QJsonObject obj;
    obj["currentTrack"] = m_currentTrackPath;
    obj["difficulty"] = m_difficultySpin->value();
    obj["aggression"] = m_aggressionSpin->value();
    obj["precision"] = m_precisionSpin->value();
    obj["consistency"] = m_consistencySpin->value();
    return obj;
}

void AIEditorModule::deserializeProject(const QJsonObject& data) {
    if (data.contains("currentTrack")) {
        m_currentTrackPath = data["currentTrack"].toString();
        int idx = m_trackSelector->findData(m_currentTrackPath);
        if (idx >= 0) m_trackSelector->setCurrentIndex(idx);
    }
    if (data.contains("difficulty")) m_difficultySpin->setValue(data["difficulty"].toInt());
    if (data.contains("aggression")) m_aggressionSpin->setValue(data["aggression"].toInt());
    if (data.contains("precision")) m_precisionSpin->setValue(data["precision"].toInt());
    if (data.contains("consistency")) m_consistencySpin->setValue(data["consistency"].toInt());
}

void AIEditorModule::importFile(const QString& filePath) {
    if (filePath.endsWith(".spline", Qt::CaseInsensitive) || 
        filePath.endsWith(".json", Qt::CaseInsensitive)) {
        m_splineManager->loadSpline(filePath);
        updateSplineInfo();
    }
}

void AIEditorModule::exportFile(const QString& filePath) {
    if (m_splineManager && m_splineManager->hasSpline()) {
        m_splineManager->saveSpline(filePath);
    }
}

} // namespace ks