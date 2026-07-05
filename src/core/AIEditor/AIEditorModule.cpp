#include "AIEditorModule.h"
#include "../../core/sys/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>
#include <QDoubleSpinBox>
#include <QFont>

namespace ks {

AIEditorModule::AIEditorModule(QWidget* parent)
    : EditorModule(parent)
{
}

bool AIEditorModule::initialize()
{
    if (m_initialized) return true;
    m_initialized = true;
    LOG_INFO("AIEditorModule", "Initializing AI Editor module");
    return true;
}

void AIEditorModule::shutdown()
{
    LOG_INFO("AIEditorModule", "Shutting down AI Editor module");
}

QDockWidget* AIEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;
    m_dockWidget = new QDockWidget("AI Editor", mainWindow);
    m_dockWidget->setObjectName("AIEditorDock");

    auto* wrapper = new QWidget();
    auto* mainLayout = new QVBoxLayout(wrapper);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);

    // --- Track / Spline tab ---
    auto* splineTab = new QWidget();
    auto* splineLayout = new QVBoxLayout(splineTab);

    auto* trackGroup = new QGroupBox("Track Selection");
    auto* trackLayout = new QVBoxLayout(trackGroup);
    m_trackSelector = new QComboBox();
    m_trackSelector->setEditable(true);
    m_trackSelector->setPlaceholderText("Select or type track path...");
    connect(m_trackSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIEditorModule::onTrackSelected);
    trackLayout->addWidget(m_trackSelector);

    auto* trackBtnLayout = new QHBoxLayout();
    m_loadTrackBtn = new QPushButton("Load Track");
    m_saveTrackBtn = new QPushButton("Save Track");
    m_saveTrackBtn->setEnabled(false);
    connect(m_loadTrackBtn, &QPushButton::clicked, this, &AIEditorModule::onLoadTrack);
    connect(m_saveTrackBtn, &QPushButton::clicked, this, &AIEditorModule::onSaveTrack);
    trackBtnLayout->addWidget(m_loadTrackBtn);
    trackBtnLayout->addWidget(m_saveTrackBtn);
    trackLayout->addLayout(trackBtnLayout);
    splineLayout->addWidget(trackGroup);

    auto* filesGroup = new QGroupBox("AI Files");
    auto* filesLayout = new QVBoxLayout(filesGroup);
    m_splineFileList = new QListWidget();
    filesLayout->addWidget(m_splineFileList);
    m_splineInfoLabel = new QLabel("No track loaded");
    m_splineInfoLabel->setStyleSheet("color: #888; font-style: italic;");
    filesLayout->addWidget(m_splineInfoLabel);
    splineLayout->addWidget(filesGroup);

    auto* opsGroup = new QGroupBox("Spline Operations");
    auto* opsLayout = new QGridLayout(opsGroup);
    m_smoothBtn = new QPushButton("Smooth Spline");
    m_smoothIterationsSpin = new QSpinBox();
    m_smoothIterationsSpin->setRange(1, 20);
    m_smoothIterationsSpin->setValue(3);
    m_smoothIterationsSpin->setPrefix("Iterations: ");
    connect(m_smoothBtn, &QPushButton::clicked, this, &AIEditorModule::onSmoothSpline);
    opsLayout->addWidget(m_smoothBtn, 0, 0);
    opsLayout->addWidget(m_smoothIterationsSpin, 0, 1);

    m_resampleBtn = new QPushButton("Resample");
    m_targetPointsEdit = new QLineEdit("500");
    m_targetPointsEdit->setPlaceholderText("Target points");
    connect(m_resampleBtn, &QPushButton::clicked, this, &AIEditorModule::onResampleSpline);
    opsLayout->addWidget(m_resampleBtn, 1, 0);
    opsLayout->addWidget(m_targetPointsEdit, 1, 1);
    splineLayout->addWidget(opsGroup);

    splineLayout->addStretch();
    m_tabWidget->addTab(splineTab, "Track & Spline");

    // --- AI Settings tab ---
    auto* aiTab = new QWidget();
    auto* aiLayout = new QVBoxLayout(aiTab);

    auto* paramsGroup = new QGroupBox("AI Parameters");
    auto* paramsLayout = new QGridLayout(paramsGroup);
    paramsLayout->addWidget(new QLabel("Difficulty:"), 0, 0);
    m_difficultySpin = new QSpinBox();
    m_difficultySpin->setRange(0, 100);
    m_difficultySpin->setValue(50);
    connect(m_difficultySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AIEditorModule::onDifficultyChanged);
    paramsLayout->addWidget(m_difficultySpin, 0, 1);

    paramsLayout->addWidget(new QLabel("Aggression:"), 1, 0);
    m_aggressionSpin = new QSpinBox();
    m_aggressionSpin->setRange(0, 100);
    m_aggressionSpin->setValue(50);
    connect(m_aggressionSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AIEditorModule::onAggressionChanged);
    paramsLayout->addWidget(m_aggressionSpin, 1, 1);

    paramsLayout->addWidget(new QLabel("Precision:"), 2, 0);
    m_precisionSpin = new QSpinBox();
    m_precisionSpin->setRange(0, 100);
    m_precisionSpin->setValue(50);
    paramsLayout->addWidget(m_precisionSpin, 2, 1);

    paramsLayout->addWidget(new QLabel("Consistency:"), 3, 0);
    m_consistencySpin = new QSpinBox();
    m_consistencySpin->setRange(0, 100);
    m_consistencySpin->setValue(50);
    paramsLayout->addWidget(m_consistencySpin, 3, 1);
    aiLayout->addWidget(paramsGroup);

    auto* actionGroup = new QGroupBox("AI Pipeline");
    auto* actionLayout = new QVBoxLayout(actionGroup);
    m_analyzeBtn = new QPushButton("Analyze Racing Line");
    m_generateBtn = new QPushButton("Generate AI Line");
    m_optimizeBtn = new QPushButton("Optimize AI Line");
    connect(m_analyzeBtn, &QPushButton::clicked, this, &AIEditorModule::onAnalyzeLine);
    connect(m_generateBtn, &QPushButton::clicked, this, &AIEditorModule::onGenerateAILine);
    connect(m_optimizeBtn, &QPushButton::clicked, this, &AIEditorModule::onOptimizeLine);
    actionLayout->addWidget(m_analyzeBtn);
    actionLayout->addWidget(m_generateBtn);
    actionLayout->addWidget(m_optimizeBtn);
    aiLayout->addWidget(actionGroup);

    m_resultLog = new QTextEdit();
    m_resultLog->setReadOnly(true);
    m_resultLog->setPlaceholderText("AI operation results will appear here...");
    m_resultLog->setMaximumHeight(150);
    aiLayout->addWidget(m_resultLog);

    aiLayout->addStretch();
    m_tabWidget->addTab(aiTab, "AI Settings");

    // --- Profiles tab ---
    auto* profilesTab = new QWidget();
    auto* profilesLayout = new QHBoxLayout(profilesTab);

    auto* profileLeft = new QVBoxLayout();

    // Style filter
    auto* styleLayout = new QHBoxLayout();
    styleLayout->addWidget(new QLabel("Style:"));
    m_profileStyleFilter = new QComboBox();
    m_profileStyleFilter->addItem("All Styles");
    m_profileStyleFilter->addItems({"Aggressive", "Defensive", "Consistent", "Smooth",
                                    "Balanced", "Reckless", "Tactical", "Endurance"});
    connect(m_profileStyleFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AIEditorModule::onProfileStyleFilterChanged);
    styleLayout->addWidget(m_profileStyleFilter);
    profileLeft->addLayout(styleLayout);

    // Tier filter
    auto* tierLayout = new QHBoxLayout();
    tierLayout->addWidget(new QLabel("Tier:"));
    m_profileTierFilter = new QComboBox();
    m_profileTierFilter->addItem("All Tiers");
    m_profileTierFilter->addItems({"Novice", "Amateur", "Intermediate", "Advanced",
                                   "Expert", "Legendary"});
    connect(m_profileTierFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AIEditorModule::onProfileTierFilterChanged);
    tierLayout->addWidget(m_profileTierFilter);
    profileLeft->addLayout(tierLayout);

    m_profileList = new QListWidget();
    m_profileList->setMinimumWidth(180);
    connect(m_profileList, &QListWidget::currentRowChanged, this, &AIEditorModule::onProfileSelected);
    profileLeft->addWidget(m_profileList);

    auto* profileBtnLayout = new QHBoxLayout();
    m_exportProfileBtn = new QPushButton("Export");
    m_compareProfilesBtn = new QPushButton("Compare");
    m_suggestImprovementsBtn = new QPushButton("Suggestions");
    connect(m_exportProfileBtn, &QPushButton::clicked, this, &AIEditorModule::onExportProfile);
    connect(m_compareProfilesBtn, &QPushButton::clicked, this, &AIEditorModule::onCompareProfiles);
    connect(m_suggestImprovementsBtn, &QPushButton::clicked, this, &AIEditorModule::onSuggestImprovements);
    profileBtnLayout->addWidget(m_exportProfileBtn);
    profileBtnLayout->addWidget(m_compareProfilesBtn);
    profileBtnLayout->addWidget(m_suggestImprovementsBtn);
    profileLeft->addLayout(profileBtnLayout);

    profilesLayout->addLayout(profileLeft);

    m_profileDetail = new QTextEdit();
    m_profileDetail->setReadOnly(true);
    profilesLayout->addWidget(m_profileDetail, 1);

    m_tabWidget->addTab(profilesTab, "Profiles");

    // --- Telemetry Training tab ---
    auto* telemetryTab = new QWidget();
    auto* telemetryLayout = new QVBoxLayout(telemetryTab);

    auto* trainingGroup = new QGroupBox("Telemetry Training");
    auto* trainingLayout = new QVBoxLayout(trainingGroup);
    m_loadTelemetryBtn = new QPushButton("Load Telemetry Data...");
    m_analyzeTelemetryBtn = new QPushButton("Analyze Selected Track");
    m_applyTelemetryProfileBtn = new QPushButton("Apply Telemetry Profile");
    m_clearTrainingDataBtn = new QPushButton("Clear All Training Data");
    m_analyzeTelemetryBtn->setEnabled(false);
    m_applyTelemetryProfileBtn->setEnabled(false);
    connect(m_loadTelemetryBtn, &QPushButton::clicked, this, &AIEditorModule::onLoadTelemetry);
    connect(m_analyzeTelemetryBtn, &QPushButton::clicked, this, &AIEditorModule::onAnalyzeTelemetry);
    connect(m_applyTelemetryProfileBtn, &QPushButton::clicked, this, &AIEditorModule::onApplyTelemetryProfile);
    connect(m_clearTrainingDataBtn, &QPushButton::clicked, this, &AIEditorModule::onClearTrainingData);
    trainingLayout->addWidget(m_loadTelemetryBtn);
    trainingLayout->addWidget(m_analyzeTelemetryBtn);
    trainingLayout->addWidget(m_applyTelemetryProfileBtn);
    trainingLayout->addWidget(m_clearTrainingDataBtn);
    telemetryLayout->addWidget(trainingGroup);

    auto* statsGroup = new QGroupBox("Training Statistics");
    auto* statsLayout = new QVBoxLayout(statsGroup);
    m_trainingStatsLabel = new QLabel("No training data loaded");
    m_trainingStatsLabel->setStyleSheet("color: #888; font-style: italic;");
    statsLayout->addWidget(m_trainingStatsLabel);
    m_trainedTrackList = new QListWidget();
    connect(m_trainedTrackList, &QListWidget::currentRowChanged, this, &AIEditorModule::onTrainedTrackSelected);
    statsLayout->addWidget(m_trainedTrackList);
    telemetryLayout->addWidget(statsGroup);

    auto* cornerGroup = new QGroupBox("Corner Analysis");
    auto* cornerLayout = new QVBoxLayout(cornerGroup);
    m_cornerAnalysisText = new QTextEdit();
    m_cornerAnalysisText->setReadOnly(true);
    m_cornerAnalysisText->setPlaceholderText("Select a trained track to see corner analysis...");
    cornerLayout->addWidget(m_cornerAnalysisText);
    telemetryLayout->addWidget(cornerGroup);

    telemetryLayout->addStretch();
    m_tabWidget->addTab(telemetryTab, "Telemetry Training");

    // --- Multi-Car Race tab ---
    auto* raceTab = new QWidget();
    auto* raceLayout = new QVBoxLayout(raceTab);

    auto* setupGroup = new QGroupBox("Race Setup");
    auto* setupLayout = new QGridLayout(setupGroup);
    setupLayout->addWidget(new QLabel("Number of AI Drivers:"), 0, 0);
    m_raceNumDriversSpin = new QSpinBox();
    m_raceNumDriversSpin->setRange(2, 40);
    m_raceNumDriversSpin->setValue(10);
    connect(m_raceNumDriversSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AIEditorModule::onNumDriversChanged);
    setupLayout->addWidget(m_raceNumDriversSpin, 0, 1);

    setupLayout->addWidget(new QLabel("Race Laps:"), 1, 0);
    m_raceLapsSpin = new QSpinBox();
    m_raceLapsSpin->setRange(1, 200);
    m_raceLapsSpin->setValue(10);
    connect(m_raceLapsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AIEditorModule::onRaceLapsChanged);
    setupLayout->addWidget(m_raceLapsSpin, 1, 1);

    setupLayout->addWidget(new QLabel("Track Length (m):"), 2, 0);
    m_trackLengthSpin = new QDoubleSpinBox();
    m_trackLengthSpin->setRange(500, 30000);
    m_trackLengthSpin->setValue(5000.0);
    m_trackLengthSpin->setDecimals(0);
    m_trackLengthSpin->setSuffix(" m");
    connect(m_trackLengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AIEditorModule::onTrackLengthChanged);
    setupLayout->addWidget(m_trackLengthSpin, 2, 1);

    auto* raceBtnLayout = new QHBoxLayout();
    m_startRaceBtn = new QPushButton("Start Race");
    m_stopRaceBtn = new QPushButton("Stop");
    m_resetRaceBtn = new QPushButton("Reset");
    m_stopRaceBtn->setEnabled(false);
    connect(m_startRaceBtn, &QPushButton::clicked, this, &AIEditorModule::onStartRace);
    connect(m_stopRaceBtn, &QPushButton::clicked, this, &AIEditorModule::onStopRace);
    connect(m_resetRaceBtn, &QPushButton::clicked, this, &AIEditorModule::onResetRace);
    raceBtnLayout->addWidget(m_startRaceBtn);
    raceBtnLayout->addWidget(m_stopRaceBtn);
    raceBtnLayout->addWidget(m_resetRaceBtn);
    setupLayout->addLayout(raceBtnLayout, 3, 0, 1, 2);
    raceLayout->addWidget(setupGroup);

    auto* statusGroup = new QGroupBox("Race Status");
    auto* statusLayout = new QGridLayout(statusGroup);
    m_raceStatusLabel = new QLabel("Configure race parameters and press Start");
    m_raceStatusLabel->setStyleSheet("font-weight: bold;");
    statusLayout->addWidget(m_raceStatusLabel, 0, 0, 1, 2);
    m_raceTimeLabel = new QLabel("Time: 0:00");
    statusLayout->addWidget(m_raceTimeLabel, 1, 0);
    m_fastestLapLabel = new QLabel("Fastest Lap: --");
    statusLayout->addWidget(m_fastestLapLabel, 1, 1);
    m_overtakesLabel = new QLabel("Overtakes: 0");
    statusLayout->addWidget(m_overtakesLabel, 1, 2);
    raceLayout->addWidget(statusGroup);

    auto* leaderboardGroup = new QGroupBox("Leaderboard");
    auto* leaderboardLayout = new QVBoxLayout(leaderboardGroup);
    m_raceLeaderboard = new QTableWidget();
    m_raceLeaderboard->setColumnCount(7);
    m_raceLeaderboard->setHorizontalHeaderLabels({"Pos", "Driver", "Lap", "Gap Ahead", "Best Lap", "Tire%", "Fuel%"});
    m_raceLeaderboard->horizontalHeader()->setStretchLastSection(true);
    m_raceLeaderboard->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_raceLeaderboard->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_raceLeaderboard->verticalHeader()->setVisible(false);
    leaderboardLayout->addWidget(m_raceLeaderboard);
    raceLayout->addWidget(leaderboardGroup);

    auto* raceLogGroup = new QGroupBox("Race Events");
    auto* raceLogLayout = new QVBoxLayout(raceLogGroup);
    m_raceEventLog = new QTextEdit();
    m_raceEventLog->setReadOnly(true);
    m_raceEventLog->setPlaceholderText("Race events will appear here...");
    m_raceEventLog->setMaximumHeight(120);
    raceLogLayout->addWidget(m_raceEventLog);
    raceLayout->addWidget(raceLogGroup);

    raceLayout->addStretch();
    m_tabWidget->addTab(raceTab, "Multi-Car Race");

    // Race timer
    m_raceTimer = new QTimer(this);
    m_raceTimer->setInterval(100);
    connect(m_raceTimer, &QTimer::timeout, this, &AIEditorModule::onRaceTick);

    // Status bar
    m_statusLabel = new QLabel("AI Editor ready. Load a track to begin.");
    m_statusLabel->setStyleSheet("color: #888; padding: 4px 0;");
    mainLayout->addWidget(m_statusLabel);

    m_dockWidget->setWidget(wrapper);

    populateTrackList();
    populateProfileList();
    populateTelemetryTracks();

    return m_dockWidget;
}

void AIEditorModule::setupUi()
{
    if (m_statusLabel) m_statusLabel->setText("UI Ready");
}

void AIEditorModule::populateTrackList()
{
    m_trackSelector->clear();
    m_trackSelector->addItem("Browse for track directory...");
    m_trackSelector->addItem("Open recent...");

    QStringList recentTracks;
    recentTracks << "ks_redbull_ring" << "ks_nordschleife" << "ks_monza"
                 << "ks_spa" << "ks_silverstone" << "ks_suzuka";
    for (const auto& t : recentTracks) {
        m_trackSelector->addItem(t);
    }
}

void AIEditorModule::populateTelemetryTracks()
{
    m_trainedTrackList->clear();
    auto* bridge = AIEditorQmlBridge::instance();
    if (!bridge) return;

    auto tracks = bridge->getTrainedTracks();
    if (tracks.isEmpty()) {
        m_trainedTrackList->addItem("(no trained tracks)");
        return;
    }
    for (const auto& t : tracks) {
        auto stats = bridge->getTrackStats(t);
        int laps = stats["lapCount"].toInt();
        double best = stats["bestLapTime"].toDouble();
        m_trainedTrackList->addItem(QString("%1 (%2 laps, best: %3s)")
            .arg(t).arg(laps).arg(best, 0, 'f', 3));
    }
}

void AIEditorModule::populateProfileList()
{
    m_profileList->clear();

    auto profiles = AiBehaviorModel::getBuiltInProfiles();
    int styleFilter = m_profileStyleFilter ? m_profileStyleFilter->currentIndex() - 1 : -1;
    int tierFilter = m_profileTierFilter ? m_profileTierFilter->currentIndex() - 1 : -1;

    for (const auto& p : profiles) {
        auto meta = AiBehaviorModel::computeMeta(p);

        // Apply filters
        if (styleFilter >= 0 && static_cast<int>(meta.style) != styleFilter)
            continue;
        if (tierFilter >= 0 && static_cast<int>(meta.tier) != tierFilter)
            continue;

        QString label = QString("%1  [%2/%3]")
            .arg(p.name)
            .arg(AiBehaviorModel::styleLabel(meta.style))
            .arg(AiBehaviorModel::tierLabel(meta.tier));
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, p.name);
        m_profileList->addItem(item);
    }

    if (m_profileList->count() == 0) {
        m_profileList->addItem("(no matching profiles)");
    }
}

void AIEditorModule::onTrackSelected(int index)
{
    if (index <= 0) return; // 0 = browse, -1 = manual text
    QString text = m_trackSelector->currentText();
    if (text.isEmpty()) return;
    QString trackPath = QDir::current().absoluteFilePath("content/tracks/" + text);
    m_currentTrackPath = trackPath;
    if (QDir(trackPath).exists()) {
        updateSplineInfo();
        m_saveTrackBtn->setEnabled(true);
    }
}

void AIEditorModule::onLoadTrack()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Track Directory",
        QDir::current().absoluteFilePath("content/tracks"));
    if (dir.isEmpty()) return;
    m_currentTrackPath = dir;
    updateSplineInfo();
    m_saveTrackBtn->setEnabled(true);
    m_statusLabel->setText(QString("Loaded track: %1").arg(QDir(dir).dirName()));
}

void AIEditorModule::onSaveTrack()
{
    if (!m_splineManager || m_currentTrackPath.isEmpty()) return;
    if (m_splineManager->save()) {
        m_statusLabel->setText("Track AI data saved.");
    } else {
        m_statusLabel->setText("Failed to save track AI data.");
    }
}

void AIEditorModule::updateSplineInfo()
{
    if (m_currentTrackPath.isEmpty()) {
        m_splineInfoLabel->setText("No track loaded");
        m_splineFileList->clear();
        return;
    }

    m_splineFileList->clear();
    QDir dir(m_currentTrackPath);
    QStringList filters;
    filters << "*.ai" << "*.csv";
    QStringList files = dir.entryList(filters, QDir::Files);
    if (files.isEmpty()) {
        m_splineFileList->addItem("(no AI files found)");
    } else {
        for (const auto& f : files) {
            QFileInfo fi(f);
            qint64 bytes = fi.size();
            m_splineFileList->addItem(QString("%1 (%2 KB)").arg(f).arg(bytes / 1024));
        }
    }

    // Try loading with AiSplineManager
    if (m_splineManager) delete m_splineManager;
    m_splineManager = new AiSplineManager(m_currentTrackPath);
    if (m_splineManager->load()) {
        int pts = m_splineManager->hasFastLane() ? m_splineManager->fastLane().pointCount() : 0;
        float len = m_splineManager->hasFastLane() ? m_splineManager->getFastLaneLength() : 0.0f;
        m_splineInfoLabel->setText(QString("Fast lane: %1 pts, %2 m | %3")
            .arg(pts).arg(len, 0, 'f', 1)
            .arg(m_splineManager->hasBorders() ? "Borders OK" : "No borders"));
    } else {
        m_splineInfoLabel->setText("Track loaded, but AI spline data unavailable");
    }
}

void AIEditorModule::onSmoothSpline()
{
    if (!m_splineManager || !m_splineManager->hasFastLane()) {
        m_statusLabel->setText("No fast lane spline loaded.");
        return;
    }
    int iters = m_smoothIterationsSpin->value();
    if (m_splineManager->smoothFastLane(iters)) {
        m_statusLabel->setText(QString("Spline smoothed (%1 iterations).").arg(iters));
        updateSplineInfo();
    } else {
        m_statusLabel->setText("Smoothing failed.");
    }
}

void AIEditorModule::onResampleSpline()
{
    if (!m_splineManager || !m_splineManager->hasFastLane()) {
        m_statusLabel->setText("No fast lane spline loaded.");
        return;
    }
    int target = m_targetPointsEdit->text().toInt();
    if (target < 10) { m_statusLabel->setText("Invalid target point count."); return; }
    if (m_splineManager->resampleFastLane(target)) {
        m_statusLabel->setText(QString("Spline resampled to %1 points.").arg(target));
        updateSplineInfo();
    } else {
        m_statusLabel->setText("Resampling failed.");
    }
}

void AIEditorModule::onAnalyzeLine()
{
    QString track = m_trackSelector->currentText();
    if (track.isEmpty()) { m_statusLabel->setText("Select a track first."); return; }
    auto* bridge = AIEditorQmlBridge::instance();
    if (bridge) {
        bridge->analyzeLine(track);
        m_resultLog->append(QString("[Analysis] Track: %1").arg(track));
        m_statusLabel->setText("Analysis requested.");
    }
}

void AIEditorModule::onGenerateAILine()
{
    QString track = m_trackSelector->currentText();
    if (track.isEmpty()) { m_statusLabel->setText("Select a track first."); return; }
    auto* bridge = AIEditorQmlBridge::instance();
    if (bridge) {
        bridge->generateAILine(track);
        m_resultLog->append(QString("[Generate] AI line requested for: %1").arg(track));
        m_statusLabel->setText("AI line generation requested.");
    }
}

void AIEditorModule::onOptimizeLine()
{
    QString track = m_trackSelector->currentText();
    if (track.isEmpty()) { m_statusLabel->setText("Select a track first."); return; }
    auto* bridge = AIEditorQmlBridge::instance();
    if (bridge) {
        bridge->optimizeLine(track);
        m_resultLog->append(QString("[Optimize] Optimization requested for: %1").arg(track));
        m_statusLabel->setText("Optimization requested.");
    }
}

void AIEditorModule::onDifficultyChanged(int v)
{
    auto* bridge = AIEditorQmlBridge::instance();
    if (bridge) bridge->setDifficulty(v);
}

void AIEditorModule::onAggressionChanged(int v)
{
    auto* bridge = AIEditorQmlBridge::instance();
    if (bridge) bridge->setAggression(v);
}

void AIEditorModule::onPresetApplied(const QString& name)
{
    m_resultLog->append(QString("[Preset] Applied: %1").arg(name));
    m_statusLabel->setText(QString("Preset applied: %1").arg(name));
}

AiBehaviorModel::AiDriverProfile AIEditorModule::resolveProfile(int row) const
{
    if (row < 0) return AiBehaviorModel::getBalancedProfile();
    QString text = m_profileList->item(row)->text();

    // Strip the filter labels: "Name [Style/Tier]"
    QString name = text.left(text.indexOf("  ["));
    if (name.isEmpty()) name = text;

    // Try built-in profiles first
    auto profiles = AiBehaviorModel::getBuiltInProfiles();
    for (const auto& p : profiles) {
        if (p.name == name) return p;
    }

    // Fallback to file load
    auto loaded = AiBehaviorModel::loadProfile(name);
    if (!loaded.name.isEmpty()) return loaded;

    return AiBehaviorModel::getBalancedProfile();
}

void AIEditorModule::onProfileSelected(int row)
{
    if (row < 0) {
        m_profileDetail->clear();
        return;
    }

    auto profile = resolveProfile(row);
    auto meta = AiBehaviorModel::computeMeta(profile);
    auto score = AiBehaviorModel::scoreProfile(profile);
    auto suggestions = AiBehaviorModel::getImprovementSuggestions(profile);

    QString info;

    // Header
    info += QString("═══ %1 ═══\n\n").arg(profile.name);
    info += QString("Style: %1  |  Tier: %2\n\n")
        .arg(AiBehaviorModel::styleLabel(meta.style))
        .arg(AiBehaviorModel::tierLabel(meta.tier));
    info += QString("▸ %1\n\n").arg(meta.shortDescription.isEmpty() ? meta.description : meta.shortDescription);

    // Radar-style ratings
    info += "── Ratings ──\n";
    info += QString("  Overall:        %1%\n").arg(qRound(score.overallScore * 100));
    for (const auto& kv : score.strengths.mid(0, 5)) {
        info += QString("  %1: %2%\n")
            .arg(kv.first, -18)
            .arg(qRound(kv.second * 100));
    }

    // Raw parameters
    info += "\n── Parameters ──\n";
    info += QString("  Skill:            %1%\n").arg(qRound(profile.skill * 100));
    info += QString("  Aggression:       %1%\n").arg(qRound(profile.aggression * 100));
    info += QString("  Defensive:        %1%\n").arg(qRound(profile.defensive * 100));
    info += QString("  Consistency:      %1%\n").arg(qRound(profile.consistency * 100));
    info += QString("  Mistakes:         %1%\n").arg(qRound(profile.mistakeRate * 100));
    info += QString("  Tire Mgmt:        %1%\n").arg(qRound(profile.tireManagement * 100));
    info += QString("  Fuel Mgmt:        %1%\n").arg(qRound(profile.fuelManagement * 100));
    info += QString("  Wet Skill:        %1%\n").arg(qRound(profile.wetSkill * 100));
    info += QString("  Qualifying Pace:  %1%\n").arg(qRound(profile.qualifyingPace * 100));
    info += QString("  Race Pace:        %1%\n").arg(qRound(profile.racePace * 100));

    // Top suggestions
    if (!suggestions.isEmpty()) {
        info += "\n── Suggested Improvements ──\n";
        for (int i = 0; i < qMin(3, suggestions.size()); ++i) {
            const auto& s = suggestions[i];
            info += QString("  %1: %2% → %3% (%4)\n")
                .arg(s.category, -18)
                .arg(qRound(s.currentValue * 100))
                .arg(qRound(s.suggestedValue * 100))
                .arg(s.reasoning);
        }
    }

    info += "\n── Tags ──\n";
    for (const auto& t : meta.tags) {
        info += QString("  #%1").arg(t);
    }

    m_profileDetail->setText(info);
}

void AIEditorModule::onProfileStyleFilterChanged(int)
{
    populateProfileList();
}

void AIEditorModule::onProfileTierFilterChanged(int)
{
    populateProfileList();
}

void AIEditorModule::onCompareProfiles()
{
    int row = m_profileList->currentRow();
    if (row < 0) {
        m_statusLabel->setText("Select a profile first.");
        return;
    }

    auto profile = resolveProfile(row);

    // Compare against the average built-in profile
    auto allProfiles = AiBehaviorModel::getBuiltInProfiles();
    if (allProfiles.isEmpty()) return;

    AiBehaviorModel::AiDriverProfile avgProfile;
    avgProfile.name = "Average";
    for (const auto& p : allProfiles) {
        avgProfile.skill += p.skill;
        avgProfile.aggression += p.aggression;
        avgProfile.defensive += p.defensive;
        avgProfile.consistency += p.consistency;
        avgProfile.mistakeRate += p.mistakeRate;
        avgProfile.tireManagement += p.tireManagement;
        avgProfile.fuelManagement += p.fuelManagement;
        avgProfile.wetSkill += p.wetSkill;
        avgProfile.qualifyingPace += p.qualifyingPace;
        avgProfile.racePace += p.racePace;
    }
    float n = allProfiles.size();
    avgProfile.skill /= n; avgProfile.aggression /= n; avgProfile.defensive /= n;
    avgProfile.consistency /= n; avgProfile.mistakeRate /= n;
    avgProfile.tireManagement /= n; avgProfile.fuelManagement /= n;
    avgProfile.wetSkill /= n; avgProfile.qualifyingPace /= n; avgProfile.racePace /= n;

    auto comp = AiBehaviorModel::compareProfiles(profile, avgProfile);

    QString info;
    info += QString("═══ Comparison: %1 vs Average ───\n\n")
        .arg(profile.name);
    info += QString("Overall: %1\n\n").arg(comp.summary);

    if (!comp.strengths.isEmpty()) {
        info += "Strengths (vs average):\n";
        for (const auto& s : comp.strengths) {
            info += QString("  + %1: %2\n").arg(s.first, -18).arg(s.second, 0, 'f', 2);
        }
        info += "\n";
    }

    if (!comp.weaknesses.isEmpty()) {
        info += "Weaknesses (vs average):\n";
        for (const auto& w : comp.weaknesses) {
            info += QString("  - %1: %2\n").arg(w.first, -18).arg(w.second, 0, 'f', 2);
        }
    }

    m_profileDetail->setText(info);
    m_statusLabel->setText(QString("Compared %1 to average profile").arg(profile.name));
}

void AIEditorModule::onSuggestImprovements()
{
    int row = m_profileList->currentRow();
    if (row < 0) {
        m_statusLabel->setText("Select a profile first.");
        return;
    }

    auto profile = resolveProfile(row);
    auto suggestions = AiBehaviorModel::getImprovementSuggestions(profile);

    if (suggestions.isEmpty()) {
        m_profileDetail->setText(QString("═══ %1 ═══\n\nNo improvements suggested — this profile is well-balanced!").arg(profile.name));
        m_statusLabel->setText("Profile is already well-balanced");
        return;
    }

    auto optimized = AiBehaviorModel::applySuggestions(profile, suggestions);
    auto comp = AiBehaviorModel::compareProfiles(optimized, profile);

    QString info;
    info += QString("═══ Improvement Plan: %1 ═══\n\n").arg(profile.name);
    info += QString("Applying top suggestions would improve overall by %1 points.\n\n")
        .arg(comp.overallScore, 0, 'f', 2);

    for (int i = 0; i < suggestions.size(); ++i) {
        const auto& s = suggestions[i];
        info += QString("%1. %2: %3% → %4%  (impact: %5%)\n")
            .arg(i + 1)
            .arg(s.category, -18)
            .arg(qRound(s.currentValue * 100))
            .arg(qRound(s.suggestedValue * 100))
            .arg(qRound(s.impact * 100));
        info += QString("   %1\n").arg(s.reasoning);
    }

    info += "\n── Optimized Profile Preview ──\n";
    info += QString("  Overall Rating: %1% → %2%\n")
        .arg(qRound(AiBehaviorModel::overallRating(profile) * 100))
        .arg(qRound(AiBehaviorModel::overallRating(optimized) * 100));
    info += QString("  Skill: %1% → %2%\n")
        .arg(qRound(profile.skill * 100)).arg(qRound(optimized.skill * 100));
    info += QString("  Consistency: %1% → %2%\n")
        .arg(qRound(profile.consistency * 100)).arg(qRound(optimized.consistency * 100));

    m_profileDetail->setText(info);
    m_statusLabel->setText(QString("Generated improvement plan for %1").arg(profile.name));
}

void AIEditorModule::onExportProfile()
{
    int row = m_profileList->currentRow();
    if (row < 0) { m_statusLabel->setText("Select a profile first."); return; }

    auto profile = resolveProfile(row);
    QString defaultName = profile.name.isEmpty() ? "profile" : profile.name;
    QString path = QFileDialog::getSaveFileName(this, "Export Profile",
        defaultName + ".json", "JSON Files (*.json)");
    if (path.isEmpty()) return;

    if (AiBehaviorModel::saveProfile(profile)) {
        m_statusLabel->setText(QString("Profile exported: %1").arg(path));
    } else {
        m_statusLabel->setText("Failed to export profile.");
    }
}

void AIEditorModule::onLoadTelemetry()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
        "Load Telemetry Data", QString(),
        "Telemetry Files (*.csv *.json);;CSV Files (*.csv);;JSON Files (*.json)");
    if (files.isEmpty()) return;

    auto* bridge = AIEditorQmlBridge::instance();
    if (!bridge) return;

    QString track = m_trackSelector->currentText();
    if (track.isEmpty() || track == "Browse for track directory...") {
        track = QInputDialog::getText(this, "Track Name",
            "Enter track name for this telemetry data:");
        if (track.isEmpty()) {
            m_statusLabel->setText("Telemetry import cancelled - no track specified");
            return;
        }
    }

    int totalSamples = 0;
    for (const auto& path : files) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_resultLog->append(QString("[Error] Cannot open: %1").arg(path));
            continue;
        }

        QVariantList samples;
        QTextStream stream(&file);

        if (path.endsWith(".json", Qt::CaseInsensitive)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();
            QJsonArray arr = doc.object()["samples"].toArray();
            for (const auto& v : arr) {
                QJsonObject s = v.toObject();
                QVariantList sl;
                sl << s["t"].toDouble() << s["speed"].toDouble()
                   << s["rpm"].toDouble() << s["gear"].toInt()
                   << s["throttle"].toDouble() << s["brake"].toDouble()
                   << s["steering"].toDouble()
                   << s["posX"].toDouble() << s["posY"].toDouble() << s["posZ"].toDouble()
                   << s["gLat"].toDouble() << s["gLon"].toDouble() << s["gVert"].toDouble();
                samples.append(sl);
            }
        } else {
            bool headerSkipped = false;
            while (!stream.atEnd()) {
                QString line = stream.readLine().trimmed();
                if (line.isEmpty()) continue;
                if (!headerSkipped) { headerSkipped = true; continue; }
                QStringList parts = line.split(',');
                if (parts.size() < 7) continue;
                QVariantList sl;
                sl << parts[0].toFloat() << parts[1].toFloat()
                   << parts[2].toFloat() << parts[3].toInt()
                   << parts[4].toFloat() << parts[5].toFloat()
                   << parts[6].toFloat();
                if (parts.size() > 7) sl << parts[7].toFloat();
                if (parts.size() > 8) sl << parts[8].toFloat();
                if (parts.size() > 9) sl << parts[9].toFloat();
                samples.append(sl);
            }
            file.close();
        }

        if (!samples.isEmpty()) {
            bridge->trainFromTelemetry(track, samples);
            totalSamples += samples.size();
        }
    }

    m_resultLog->append(QString("[Telemetry] Loaded %1 samples for track '%2'")
        .arg(totalSamples).arg(track));
    m_statusLabel->setText(QString("Telemetry loaded: %1 samples for %2").arg(totalSamples).arg(track));
    populateTelemetryTracks();
    m_analyzeTelemetryBtn->setEnabled(true);
    m_applyTelemetryProfileBtn->setEnabled(true);
}

void AIEditorModule::onAnalyzeTelemetry()
{
    int row = m_trainedTrackList->currentRow();
    if (row < 0) { m_statusLabel->setText("Select a trained track first."); return; }

    QString text = m_trainedTrackList->currentItem()->text();
    QString track = text.left(text.indexOf('(')).trimmed();
    if (track.isEmpty()) return;

    auto* bridge = AIEditorQmlBridge::instance();
    if (!bridge) return;

    bridge->analyzeTrackData(track);

    auto stats = bridge->getTrackStats(track);
    m_trainingStatsLabel->setText(QString("Track: %1 | Laps: %2 | Best: %3s | Avg: %4s | Consistency: %5%")
        .arg(track)
        .arg(stats["lapCount"].toInt())
        .arg(stats["bestLapTime"].toDouble(), 0, 'f', 3)
        .arg(stats["averageLapTime"].toDouble(), 0, 'f', 3)
        .arg(stats["consistency"].toDouble() * 100, 0, 'f', 1));

    auto corners = bridge->getCornerAnalysis(track);
    m_cornerAnalysisText->clear();
    if (corners.isEmpty()) {
        m_cornerAnalysisText->setPlainText("No corner data available for this track.");
    } else {
        QString report;
        report += QString("Corner Analysis for %1\n").arg(track);
        report += QString("Total corners detected: %1\n\n").arg(corners.size());
        for (int i = 0; i < corners.size(); ++i) {
            QVariantMap c = corners[i].toMap();
            report += QString("Corner #%1:\n").arg(c["cornerIndex"].toInt());
            report += QString("  Entry: %1 km/h\n").arg(c["entrySpeed"].toDouble(), 0, 'f', 1);
            report += QString("  Apex:  %1 km/h\n").arg(c["apexSpeed"].toDouble(), 0, 'f', 1);
            report += QString("  Exit:  %1 km/h\n").arg(c["exitSpeed"].toDouble(), 0, 'f', 1);
            report += QString("  Brake: %1%\n").arg(c["brakePressure"].toDouble() * 100, 0, 'f', 0);
            report += QString("  Time Lost: %1s\n\n").arg(c["timeLost"].toDouble(), 0, 'f', 3);
        }
        m_cornerAnalysisText->setPlainText(report);
    }

    m_statusLabel->setText(QString("Analysis complete for %1").arg(track));
}

void AIEditorModule::onApplyTelemetryProfile()
{
    int row = m_trainedTrackList->currentRow();
    if (row < 0) { m_statusLabel->setText("Select a trained track first."); return; }

    QString text = m_trainedTrackList->currentItem()->text();
    QString track = text.left(text.indexOf('(')).trimmed();
    if (track.isEmpty()) return;

    auto* bridge = AIEditorQmlBridge::instance();
    if (!bridge) return;

    bridge->applyTelemetryProfile(track);

    auto profile = bridge->getOptimizedDriverProfile(track);
    m_resultLog->append(QString("[Telemetry Profile] Applied profile for %1").arg(track));
    m_resultLog->append(QString("  Skill: %.0f%% | Aggression: %.0f%% | Consistency: %.0f%%")
        .arg(profile["skill"].toDouble() * 100)
        .arg(profile["aggression"].toDouble() * 100)
        .arg(profile["consistency"].toDouble() * 100));

    m_statusLabel->setText(QString("Applied telemetry profile for %1").arg(track));
}

void AIEditorModule::onClearTrainingData()
{
    auto* bridge = AIEditorQmlBridge::instance();
    if (!bridge) return;

    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Clear Training Data",
        "Are you sure you want to clear all telemetry training data?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        bridge->clearTrainingData();
        populateTelemetryTracks();
        m_trainingStatsLabel->setText("No training data loaded");
        m_cornerAnalysisText->clear();
        m_analyzeTelemetryBtn->setEnabled(false);
        m_applyTelemetryProfileBtn->setEnabled(false);
        m_statusLabel->setText("Training data cleared");
    }
}

void AIEditorModule::onTrainedTrackSelected(int row)
{
    if (row < 0) {
        m_analyzeTelemetryBtn->setEnabled(false);
        m_applyTelemetryProfileBtn->setEnabled(false);
        return;
    }

    m_analyzeTelemetryBtn->setEnabled(true);
    m_applyTelemetryProfileBtn->setEnabled(true);

    QString text = m_trainedTrackList->currentItem()->text();
    QString track = text.left(text.indexOf('(')).trimmed();

    auto* bridge = AIEditorQmlBridge::instance();
    if (!bridge) return;

    auto stats = bridge->getTrackStats(track);
    m_trainingStatsLabel->setText(QString("Track: %1 | Laps: %2 | Best: %3s")
        .arg(track)
        .arg(stats["lapCount"].toInt())
        .arg(stats["bestLapTime"].toDouble(), 0, 'f', 3));

    auto corners = bridge->getCornerAnalysis(track);
    if (corners.isEmpty()) {
        m_cornerAnalysisText->setPlainText("No corner data. Click 'Analyze Selected Track'.");
    } else {
        QString report;
        report += QString("%1 corners detected\n").arg(corners.size());
        for (int i = 0; i < corners.size(); ++i) {
            QVariantMap c = corners[i].toMap();
            report += QString("C%1: %2/%3/%4 km/h\n")
                .arg(c["cornerIndex"].toInt())
                .arg(c["entrySpeed"].toDouble(), 0, 'f', 0)
                .arg(c["apexSpeed"].toDouble(), 0, 'f', 0)
                .arg(c["exitSpeed"].toDouble(), 0, 'f', 0);
        }
        m_cornerAnalysisText->setPlainText(report);
    }
}

void AIEditorModule::importFile(const QString& filePath)
{
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    if (ext == "ai") {
        auto spline = AiSplineEditor::loadAiFile(filePath);
        if (spline.isValid()) {
            m_statusLabel->setText(QString("Imported AI spline: %1 (%2 pts)").arg(fi.fileName()).arg(spline.pointCount()));
        }
    } else if (ext == "csv") {
        auto border = AiSplineEditor::loadCsvBorder(filePath);
        if (border.isValid()) {
            m_statusLabel->setText(QString("Imported border: %1 (%2 pts)").arg(fi.fileName()).arg(border.pointCount()));
        }
    } else if (ext == "json") {
        QFile f(filePath);
        if (f.open(QIODevice::ReadOnly)) {
            auto hints = QJsonDocument::fromJson(f.readAll()).object();
            f.close();
            AiSplineEditor::saveAiHints(hints, fi.absolutePath());
            m_statusLabel->setText(QString("Imported AI hints from: %1").arg(fi.fileName()));
        }
    } else {
        m_statusLabel->setText(QString("Unsupported file format: %1").arg(ext));
    }
}

void AIEditorModule::exportFile(const QString& filePath)
{
    if (filePath.isEmpty() || !m_splineManager) return;
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    if (ext == "ai" && m_splineManager->hasFastLane()) {
        AiSplineEditor::saveAiFile(m_splineManager->fastLane(), filePath);
        m_statusLabel->setText(QString("Exported fast lane to: %1").arg(filePath));
    } else if (ext == "csv" && m_splineManager->hasBorders()) {
        if (filePath.contains("side_l")) {
            AiSplineEditor::saveCsvBorder(m_splineManager->leftBorder(), filePath);
        } else if (filePath.contains("side_r")) {
            AiSplineEditor::saveCsvBorder(m_splineManager->rightBorder(), filePath);
        }
        m_statusLabel->setText(QString("Exported border to: %1").arg(filePath));
    } else if (ext == "ai") {
        // No fast lane data to export - write empty valid file
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[HEADER]\nVERSION=1\nPOINTS=0\n\n";
            out << "; No AI line data available. Record a fast lap first.\n";
            file.close();
            m_statusLabel->setText(QString("No AI line data - exported empty file: %1").arg(filePath));
        }
    } else if (ext == "csv") {
        // No border data to export
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "x,y,z\n";
            out << "; No border data available. Record spline borders first.\n";
            file.close();
            m_statusLabel->setText(QString("No border data - exported empty file: %1").arg(filePath));
        }
    } else {
        m_statusLabel->setText(QString("Unsupported export format: %1").arg(ext));
    }
}

void AIEditorModule::onActivation()
{
    if (m_statusLabel) m_statusLabel->setText("Active");
}

void AIEditorModule::onDeactivation()
{
    if (m_statusLabel) m_statusLabel->setText("Inactive");
}

// --- Multi-car race ---

void AIEditorModule::onStartRace()
{
    int numDrivers = m_raceNumDriversSpin->value();
    int laps = m_raceLapsSpin->value();
    float trackLen = static_cast<float>(m_trackLengthSpin->value());

    m_multiCarAI.setupRace(numDrivers, laps, trackLen);

    // Use built-in profiles for realistic distribution
    auto allProfiles = AiBehaviorModel::getBuiltInProfiles();

    // Driver names with character archetypes
    QStringList names = {"Verstappen","Hamilton","Leclerc","Norris","Sainz",
                         "Piastri","Russell","Alonso","Perez","Stroll",
                         "Gasly","Ocon","Tsunoda","Ricciardo","Hulkenberg",
                         "Magnussen","Albon","Sargeant","Bottas","Zhou",
                         "Lawson","Hadjar","Bearman","Antonelli",
                         "Colapinto","Doohan","Bortoleto","Martins",
                         "Crawford","Maini","Browning","Fornaroli",
                         "Shields","Bedrin","Montoya","Dunne",
                         "Fittipaldi","Sato","Green","Woolf"};

    for (int i = 0; i < numDrivers; ++i) {
        auto* d = m_multiCarAI.getDriver(i);
        if (!d) continue;

        if (i < names.size()) d->name = names[i];

        // Distribute profiles: front runners get higher-tier profiles, back markers get lower
        float progress = static_cast<float>(i) / qMax(1, numDrivers - 1);
        int profileIdx = qMin(static_cast<int>(progress * allProfiles.size()), allProfiles.size() - 1);
        d->profile = allProfiles[profileIdx];

        // Add slight randomization for variety
        d->profile.skill = qBound(0.2f,
            d->profile.skill + AiBehaviorModel::randomFloat(-0.08f, 0.08f), 1.0f);
        d->profile.aggression = qBound(0.15f,
            d->profile.aggression + AiBehaviorModel::randomFloat(-0.12f, 0.12f), 1.0f);
        d->profile.consistency = qBound(0.2f,
            d->profile.consistency + AiBehaviorModel::randomFloat(-0.1f, 0.1f), 1.0f);

        // Track position offset (staggered start)
        d->trackProgress = -(i * 2.0f);
    }

    m_raceTimer->start();
    m_startRaceBtn->setEnabled(false);
    m_stopRaceBtn->setEnabled(true);
    m_raceStatusLabel->setText("Race in progress...");
    m_raceEventLog->clear();
    m_raceEventLog->append("Race started with " + QString::number(numDrivers) + " drivers, " +
                           QString::number(laps) + " laps, " +
                           QString::number(trackLen, 'f', 0) + "m track");

    updateSplineInfo();
}

void AIEditorModule::onStopRace()
{
    m_raceTimer->stop();
    m_startRaceBtn->setEnabled(true);
    m_stopRaceBtn->setEnabled(false);
    m_raceStatusLabel->setText("Race stopped");

    m_raceEventLog->append("--- Race stopped ---");
}

void AIEditorModule::onResetRace()
{
    m_raceTimer->stop();
    m_multiCarAI.clearGrid();
    m_startRaceBtn->setEnabled(true);
    m_stopRaceBtn->setEnabled(false);
    m_raceStatusLabel->setText("Configure race parameters and press Start");
    m_raceTimeLabel->setText("Time: 0:00");
    m_fastestLapLabel->setText("Fastest Lap: --");
    m_overtakesLabel->setText("Overtakes: 0");
    m_raceLeaderboard->setRowCount(0);
    m_raceEventLog->clear();
}

void AIEditorModule::onRaceTick()
{
    if (!m_multiCarAI.isRaceComplete()) {
        m_multiCarAI.tick(0.1f);

        auto events = m_multiCarAI.consumeEvents();
        for (const auto& ev : events) {
            QString prefix;
            switch (ev.type) {
                case RaceEvent::LAP_COMPLETED: prefix = "[LAP]"; break;
                case RaceEvent::OVERTAKE:      prefix = "[OVERTAKE]"; break;
                case RaceEvent::PIT_STOP:      prefix = "[PIT]"; break;
                case RaceEvent::DNF:           prefix = "[DNF]"; break;
                case RaceEvent::FINISH:        prefix = "[FINISH]"; break;
                default:                       prefix = "[EVENT]"; break;
            }
            m_raceEventLog->append(prefix + " " + ev.description);
        }
    }

    auto leaderboard = m_multiCarAI.getLeaderboard();
    m_raceLeaderboard->setRowCount(leaderboard.size());

    for (int i = 0; i < leaderboard.size(); ++i) {
        const auto& d = leaderboard[i];

        auto* posItem = new QTableWidgetItem(QString::number(d.position));
        posItem->setTextAlignment(Qt::AlignCenter);
        if (d.dnf) posItem->setText("DNF");
        else if (d.finished) posItem->setText(QString::number(d.position) + "✓");

        auto* nameItem = new QTableWidgetItem(d.name);
        auto* lapItem = new QTableWidgetItem(QString("%1/%2").arg(d.lap).arg(d.totalLaps));
        lapItem->setTextAlignment(Qt::AlignCenter);

        QString gapText = d.distanceToCarAhead < 1e5f
            ? QString("%1s").arg(d.distanceToCarAhead / 10.0f, 0, 'f', 1)
            : "---";

        auto* gapItem = new QTableWidgetItem(gapText);
        gapItem->setTextAlignment(Qt::AlignCenter);

        auto* bestLapItem = new QTableWidgetItem(
            d.bestLapTime < 1e8f ? QString("%1s").arg(d.bestLapTime, 0, 'f', 3) : "---");
        bestLapItem->setTextAlignment(Qt::AlignCenter);

        auto* tireItem = new QTableWidgetItem(QString("%1%").arg(static_cast<int>(d.tireWear)));
        tireItem->setTextAlignment(Qt::AlignCenter);
        if (d.tireWear > 70) tireItem->setForeground(QColor("#ff4444"));

        auto* fuelItem = new QTableWidgetItem(QString("%1%").arg(static_cast<int>(d.fuel)));
        fuelItem->setTextAlignment(Qt::AlignCenter);
        if (d.fuel < 20) fuelItem->setForeground(QColor("#ffaa00"));

        m_raceLeaderboard->setItem(i, 0, posItem);
        m_raceLeaderboard->setItem(i, 1, nameItem);
        m_raceLeaderboard->setItem(i, 2, lapItem);
        m_raceLeaderboard->setItem(i, 3, gapItem);
        m_raceLeaderboard->setItem(i, 4, bestLapItem);
        m_raceLeaderboard->setItem(i, 5, tireItem);
        m_raceLeaderboard->setItem(i, 6, fuelItem);
    }

    m_raceTimeLabel->setText(QString("Time: %1:%2")
        .arg(static_cast<int>(m_multiCarAI.grid().drivers.isEmpty() ? 0 : m_multiCarAI.grid().drivers[0].raceTime) / 60)
        .arg(static_cast<int>(m_multiCarAI.grid().drivers.isEmpty() ? 0 : m_multiCarAI.grid().drivers[0].raceTime) % 60, 2, 10, QChar('0')));

    if (m_multiCarAI.getFastestLapDriver() >= 0) {
        m_fastestLapLabel->setText(QString("Fastest Lap: %1s (%2)")
            .arg(m_multiCarAI.getFastestLap(), 0, 'f', 3)
            .arg(m_multiCarAI.getDriver(m_multiCarAI.getFastestLapDriver())->name));
    }

    m_overtakesLabel->setText(QString("Overtakes: %1 | Pos Changes: %2")
        .arg(m_multiCarAI.getTotalOvertakes())
        .arg(m_multiCarAI.getTotalPositionChanges()));

    if (m_multiCarAI.isRaceComplete()) {
        m_raceTimer->stop();
        m_startRaceBtn->setEnabled(true);
        m_stopRaceBtn->setEnabled(false);
        m_raceStatusLabel->setText("Race complete! See leaderboard for results.");
        m_raceEventLog->append("=== RACE COMPLETE ===");
    }
}

void AIEditorModule::onNumDriversChanged(int v)
{
    m_currentNumDrivers = qMax(2, qMin(40, v));
}

void AIEditorModule::onRaceLapsChanged(int v)
{
    m_raceTotalLaps = qMax(1, v);
}

void AIEditorModule::onTrackLengthChanged(double v)
{
    m_trackLength = qMax(500.0, v);
}

QJsonObject AIEditorModule::serializeProject() const
{
    QJsonObject data;
    data["currentTrackPath"] = m_currentTrackPath;
    data["difficulty"] = m_difficultySpin ? m_difficultySpin->value() : 50;
    data["aggression"] = m_aggressionSpin ? m_aggressionSpin->value() : 50;
    data["precision"] = m_precisionSpin ? m_precisionSpin->value() : 50;
    data["consistency"] = m_consistencySpin ? m_consistencySpin->value() : 50;
    data["numDrivers"] = m_raceNumDriversSpin ? m_raceNumDriversSpin->value() : 1;
    data["raceLaps"] = m_raceLapsSpin ? m_raceLapsSpin->value() : 5;
    data["trackLength"] = m_trackLengthSpin ? m_trackLengthSpin->value() : 0.0;
    return data;
}

void AIEditorModule::deserializeProject(const QJsonObject& data)
{
    m_currentTrackPath = data["currentTrackPath"].toString();
    if (m_difficultySpin) m_difficultySpin->setValue(data["difficulty"].toInt(50));
    if (m_aggressionSpin) m_aggressionSpin->setValue(data["aggression"].toInt(50));
    if (m_precisionSpin) m_precisionSpin->setValue(data["precision"].toInt(50));
    if (m_consistencySpin) m_consistencySpin->setValue(data["consistency"].toInt(50));
    if (m_raceNumDriversSpin) m_raceNumDriversSpin->setValue(data["numDrivers"].toInt(1));
    if (m_raceLapsSpin) m_raceLapsSpin->setValue(data["raceLaps"].toInt(5));
    if (m_trackLengthSpin) m_trackLengthSpin->setValue(data["trackLength"].toDouble(0.0));

    if (!m_currentTrackPath.isEmpty()) {
        onLoadTrack();
    }
}

} // namespace ks
