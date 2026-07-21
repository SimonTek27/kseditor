#include "AnimationEditorModule.h"
#include "core/editor/ModuleGuiBase.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTimer>
#include <QColorDialog>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <algorithm>
#include <QScrollArea>
#include <QPainter>
#include <QFile>
#include <QVector2D>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>

namespace ks {

AnimationEditorModule::AnimationEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_timelineTab(nullptr)
    , m_animationTree(nullptr)
    , m_keyframeTable(nullptr)
    , m_timelineSlider(nullptr)
    , m_timeSpin(nullptr)
    , m_frameSpin(nullptr)
    , m_playBtn(nullptr)
    , m_pauseBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_loopCheck(nullptr)
    , m_timeLabel(nullptr)
    , m_interpolationCombo(nullptr)
    , m_addTrackBtn(nullptr)
    , m_removeTrackBtn(nullptr)
    , m_durationSpin(nullptr)
    , m_exportBtn(nullptr)
    , m_importBtn(nullptr)
    , m_stateMachineTab(nullptr)
    , m_stateTree(nullptr)
    , m_transitionTable(nullptr)
    , m_addStateBtn(nullptr)
    , m_removeStateBtn(nullptr)
    , m_addTransitionBtn(nullptr)
    , m_stateAnimCombo(nullptr)
    , m_blendTreeTab(nullptr)
    , m_blendTreeNodes(nullptr)
    , m_addBlendNodeBtn(nullptr)
    , m_removeBlendNodeBtn(nullptr)
    , m_blendTypeCombo(nullptr)
    , m_blendParamSpin(nullptr)
    , m_propertiesTab(nullptr)
    , m_propertiesTree(nullptr)
    , m_curvesTab(nullptr)
    , m_curvesPlaceholder(nullptr)
    , m_currentAnimation("")
    , m_currentStateMachine("")
    , m_playing(false)
    , m_currentTime(0.0)
    , m_fps(30)
    , m_playbackTimer(nullptr)
{
    setObjectName("AnimationEditorModule");
}

bool AnimationEditorModule::initialize() {
    if (m_uiBuilt) return true;
    
    m_playbackTimer = new QTimer(this);
    m_playbackTimer->setInterval(1000 / m_fps);
    connect(m_playbackTimer, &QTimer::timeout, this, [this]() {
        if (m_playing) {
            double step = 1.0 / m_fps;
            m_currentTime += step;
            
            AnimationClip* clip = nullptr;
            if (m_animations.contains(m_currentAnimation)) {
                clip = &m_animations[m_currentAnimation];
            }
            
            if (clip && m_currentTime >= clip->duration) {
                if (clip->loop) {
                    m_currentTime = 0.0;
                } else {
                    m_currentTime = clip->duration;
                    stopAnimation();
                }
            }
            
            updatePlaybackUI();
            refreshKeyframeTable();
        }
    });
    
    ModuleGuiBase::initialize();
    return true;
}

void AnimationEditorModule::shutdown() {
    if (m_playing) stopAnimation();
    ModuleGuiBase::shutdown();
}

void AnimationEditorModule::importFile(const QString& filePath) {
    QFileInfo info(filePath);
    if (info.suffix().toLower() == "anim" || info.suffix().toLower() == "json") {
        loadAnimationFromFile(filePath);
    } else {
        logError("Unsupported animation format");
    }
}

void AnimationEditorModule::exportFile(const QString& filePath) {
    if (m_currentAnimation.isEmpty()) {
        logError("No animation selected");
        return;
    }
    saveAnimationToFile(filePath);
}

void AnimationEditorModule::buildUI() {
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3a3a3a; background: #1e1e1e; }"
        "QTabBar::tab { background: #2d2d2d; color: #aaa; padding: 8px 16px; border: 1px solid #3a3a3a; border-bottom: none; }"
        "QTabBar::tab:selected { background: #3a5a8a; color: #fff; }"
        "QTabBar::tab:hover { background: #4a6a9a; }"
    );
    
    setupTimelineTab();
    setupStateMachineTab();
    setupBlendTreeTab();
    setupPropertiesTab();
    setupCurvesTab();
    
    m_tabWidget->addTab(m_timelineTab, "Timeline");
    m_tabWidget->addTab(m_stateMachineTab, "State Machine");
    m_tabWidget->addTab(m_blendTreeTab, "Blend Tree");
    m_tabWidget->addTab(m_propertiesTab, "Properties");
    m_tabWidget->addTab(m_curvesTab, "Curves");
    
    m_mainLayout->insertWidget(1, m_tabWidget, 1);
}

void AnimationEditorModule::setupTimelineTab() {
    m_timelineTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_timelineTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Toolbar
    QGroupBox* toolGroup = createGroupBox("Animation Controls");
    QHBoxLayout* toolLayout = new QHBoxLayout(toolGroup);
    
    QPushButton* newBtn = createButton("New", "success");
    connect(newBtn, &QPushButton::clicked, this, &AnimationEditorModule::onNewAnimation);
    toolLayout->addWidget(newBtn);
    
    QPushButton* dupBtn = createButton("Duplicate");
    connect(dupBtn, &QPushButton::clicked, this, &AnimationEditorModule::onDuplicateAnimation);
    toolLayout->addWidget(dupBtn);
    
    QPushButton* delBtn = createButton("Delete", "danger");
    connect(delBtn, &QPushButton::clicked, this, &AnimationEditorModule::onDeleteAnimation);
    toolLayout->addWidget(delBtn);
    
    toolLayout->addSpacing(20);
    
    m_playBtn = createButton("Play", "success");
    connect(m_playBtn, &QPushButton::clicked, this, &AnimationEditorModule::onPlayPause);
    toolLayout->addWidget(m_playBtn);
    
    m_pauseBtn = createButton("Pause", "warning");
    m_pauseBtn->setVisible(false);
    connect(m_pauseBtn, &QPushButton::clicked, this, &AnimationEditorModule::onPlayPause);
    toolLayout->addWidget(m_pauseBtn);
    
    m_stopBtn = createButton("Stop", "danger");
    connect(m_stopBtn, &QPushButton::clicked, this, &AnimationEditorModule::onStop);
    toolLayout->addWidget(m_stopBtn);
    
    m_loopCheck = createCheckBox("Loop");
    m_loopCheck->setChecked(true);
    connect(m_loopCheck, &QCheckBox::toggled, this, &AnimationEditorModule::onLoopToggled);
    toolLayout->addWidget(m_loopCheck);
    
    toolLayout->addSpacing(20);
    
    toolLayout->addWidget(createLabel("Time:"));
    m_timeSpin = new QDoubleSpinBox();
    m_timeSpin->setRange(0, 9999);
    m_timeSpin->setSuffix(" s");
    m_timeSpin->setDecimals(3);
    m_timeSpin->setSingleStep(0.033);
    m_timeSpin->setMaximumWidth(100);
    connect(m_timeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnimationEditorModule::onTimeChanged);
    toolLayout->addWidget(m_timeSpin);
    
    toolLayout->addWidget(createLabel("Frame:"));
    m_frameSpin = new QSpinBox();
    m_frameSpin->setRange(0, 999999);
    m_frameSpin->setMaximumWidth(80);
    connect(m_frameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AnimationEditorModule::onFrameChanged);
    toolLayout->addWidget(m_frameSpin);
    
    toolLayout->addWidget(createLabel("FPS:"));
    QSpinBox* fpsSpin = new QSpinBox();
    fpsSpin->setRange(1, 120);
    fpsSpin->setValue(30);
    fpsSpin->setMaximumWidth(70);
    connect(fpsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) { m_fps = v; m_playbackTimer->setInterval(1000 / v); });
    toolLayout->addWidget(fpsSpin);
    
    toolLayout->addStretch();
    
    m_exportBtn = createButton("Export");
    connect(m_exportBtn, &QPushButton::clicked, this, &AnimationEditorModule::onExportAnimation);
    toolLayout->addWidget(m_exportBtn);
    
    m_importBtn = createButton("Import");
    connect(m_importBtn, &QPushButton::clicked, this, &AnimationEditorModule::onImportAnimation);
    toolLayout->addWidget(m_importBtn);
    
    layout->addWidget(toolGroup);
    
    // Main content splitter
    QSplitter* mainSplitter = createSplitter();
    
    // Left: Animation list and tracks
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);
    
    QGroupBox* animListGroup = createGroupBox("Animations");
    QVBoxLayout* animListLayout = new QVBoxLayout(animListGroup);
    
    m_animationTree = createTreeWidget({"Name", "Duration", "Tracks", "Loop"});
    m_animationTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_animationTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_animationTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_animationTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_animationTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_animationTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        QList<QTreeWidgetItem*> items = m_animationTree->selectedItems();
        if (!items.isEmpty()) {
            m_currentAnimation = items.first()->text(0);
            refreshKeyframeTable();
            updatePlaybackUI();
        }
    });
    connect(m_animationTree, &QTreeWidget::customContextMenuRequested, this, &AnimationEditorModule::onShowContextMenu);
    animListLayout->addWidget(m_animationTree);
    
    leftLayout->addWidget(animListGroup);
    
    // Tracks
    QGroupBox* tracksGroup = createGroupBox("Tracks");
    QVBoxLayout* tracksLayout = new QVBoxLayout(tracksGroup);
    
    QHBoxLayout* trackBtnLayout = new QHBoxLayout();
    m_addTrackBtn = createButton("Add Track", "success");
    connect(m_addTrackBtn, &QPushButton::clicked, this, &AnimationEditorModule::onAddTrack);
    trackBtnLayout->addWidget(m_addTrackBtn);
    
    m_removeTrackBtn = createButton("Remove Track", "danger");
    connect(m_removeTrackBtn, &QPushButton::clicked, this, &AnimationEditorModule::onRemoveTrack);
    trackBtnLayout->addWidget(m_removeTrackBtn);
    
    trackBtnLayout->addStretch();
    tracksLayout->addLayout(trackBtnLayout);
    
    m_keyframeTable = new QTableWidget();
    m_keyframeTable->setColumnCount(5);
    m_keyframeTable->setHorizontalHeaderLabels({"Track", "Time", "Value", "Interpolation", "Easing"});
    m_keyframeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_keyframeTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_keyframeTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_keyframeTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_keyframeTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_keyframeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_keyframeTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_keyframeTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_keyframeTable, &QTableWidget::customContextMenuRequested, this, &AnimationEditorModule::onShowContextMenu);
    tracksLayout->addWidget(m_keyframeTable);
    
    QHBoxLayout* kfBtnLayout = new QHBoxLayout();
    QPushButton* addKfBtn = createButton("Add Keyframe", "success");
    connect(addKfBtn, &QPushButton::clicked, this, &AnimationEditorModule::onAddKeyframe);
    kfBtnLayout->addWidget(addKfBtn);
    
    QPushButton* removeKfBtn = createButton("Remove Keyframe", "danger");
    connect(removeKfBtn, &QPushButton::clicked, this, &AnimationEditorModule::onRemoveKeyframe);
    kfBtnLayout->addWidget(removeKfBtn);
    
    kfBtnLayout->addStretch();
    
    m_interpolationCombo = createComboBox({"Linear", "Bezier", "Step", "Constant", "Cubic", "Sinusoidal"});
    kfBtnLayout->addWidget(createLabel("Interp:"));
    kfBtnLayout->addWidget(m_interpolationCombo);
    connect(m_interpolationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AnimationEditorModule::onInterpolationChanged);
    
    tracksLayout->addLayout(kfBtnLayout);
    leftLayout->addWidget(tracksGroup, 1);
    
    mainSplitter->addWidget(leftWidget);
    
    // Right: Timeline scrubber and properties
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);
    
    // Timeline scrubber
    QGroupBox* scrubGroup = createGroupBox("Timeline");
    QVBoxLayout* scrubLayout = new QVBoxLayout(scrubGroup);
    
    m_timelineSlider = new QSlider(Qt::Horizontal);
    m_timelineSlider->setRange(0, 10000);
    m_timelineSlider->setTickPosition(QSlider::TicksBelow);
    m_timelineSlider->setTickInterval(1000);
    connect(m_timelineSlider, &QSlider::valueChanged, this, [this](int v) {
        if (!m_currentAnimation.isEmpty() && m_animations.contains(m_currentAnimation)) {
            AnimationClip& clip = m_animations[m_currentAnimation];
            m_currentTime = (v / 10000.0) * clip.duration;
            m_timeSpin->blockSignals(true);
            m_timeSpin->setValue(m_currentTime);
            m_timeSpin->blockSignals(false);
            m_frameSpin->blockSignals(true);
            m_frameSpin->setValue(int(m_currentTime * m_fps));
            m_frameSpin->blockSignals(false);
            refreshKeyframeTable();
        }
    });
    scrubLayout->addWidget(m_timelineSlider);
    
    m_timeLabel = createLabel("0.000s / 1.000s (Frame 0/30)");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet("color: #aaa; font-family: Consolas; font-size: 12px;");
    scrubLayout->addWidget(m_timeLabel);
    
    // Duration control
    QHBoxLayout* durLayout = new QHBoxLayout();
    durLayout->addWidget(createLabel("Duration:"));
    m_durationSpin = new QDoubleSpinBox();
    m_durationSpin->setRange(0.01, 9999);
    m_durationSpin->setSuffix(" s");
    m_durationSpin->setDecimals(3);
    m_durationSpin->setSingleStep(0.1);
    m_durationSpin->setValue(1.0);
    connect(m_durationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double v) {
        if (!m_currentAnimation.isEmpty() && m_animations.contains(m_currentAnimation)) {
            m_animations[m_currentAnimation].duration = v;
            m_timelineSlider->setRange(0, 10000);
            updatePlaybackUI();
        }
    });
    durLayout->addWidget(m_durationSpin);
    durLayout->addStretch();
    scrubLayout->addLayout(durLayout);
    
    rightLayout->addWidget(scrubGroup);
    
    // Properties panel
    QGroupBox* propsGroup = createGroupBox("Animation Properties");
    QFormLayout* propsLayout = new QFormLayout(propsGroup);
    
    QLineEdit* nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("Animation name");
    connect(nameEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (!m_currentAnimation.isEmpty() && m_animations.contains(m_currentAnimation)) {
            AnimationClip clip = m_animations[m_currentAnimation];
            m_animations.remove(m_currentAnimation);
            clip.name = text;
            m_animations[text] = clip;
            m_currentAnimation = text;
            refreshAnimationList();
        }
    });
    propsLayout->addRow("Name:", nameEdit);
    
    QCheckBox* loopCheck = createCheckBox("Loop");
    loopCheck->setChecked(true);
    connect(loopCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_currentAnimation.isEmpty() && m_animations.contains(m_currentAnimation)) {
            m_animations[m_currentAnimation].loop = checked;
        }
    });
    propsLayout->addRow("", loopCheck);
    
    rightLayout->addWidget(propsGroup);
    rightLayout->addStretch();
    
    mainSplitter->addWidget(rightWidget);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 1);
    
    layout->addWidget(mainSplitter, 1);
}

void AnimationEditorModule::setupStateMachineTab() {
    m_stateMachineTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_stateMachineTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    // Toolbar
    QGroupBox* toolGroup = createGroupBox("State Machine");
    QHBoxLayout* toolLayout = new QHBoxLayout(toolGroup);
    
    QPushButton* newSmBtn = createButton("New State Machine", "success");
    connect(newSmBtn, &QPushButton::clicked, this, [this]() {
        bool ok;
        QString name = QInputDialog::getText(this, "New State Machine", "Name:", QLineEdit::Normal, "StateMachine", &ok);
        if (ok && !name.isEmpty()) {
            StateMachine sm;
            sm.name = name;
            m_stateMachines[name] = sm;
            refreshStateList();
        }
    });
    toolLayout->addWidget(newSmBtn);
    
    QPushButton* delSmBtn = createButton("Delete", "danger");
    connect(delSmBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentStateMachine.isEmpty()) {
            m_stateMachines.remove(m_currentStateMachine);
            m_currentStateMachine.clear();
            refreshStateList();
        }
    });
    toolLayout->addWidget(delSmBtn);
    
    toolLayout->addStretch();
    layout->addWidget(toolGroup);
    
    // Main splitter
    QSplitter* splitter = createSplitter();
    
    // States tree
    QGroupBox* statesGroup = createGroupBox("States");
    QVBoxLayout* statesLayout = new QVBoxLayout(statesGroup);
    
    m_stateTree = createTreeWidget({"State", "Animation", "Transitions"});
    m_stateTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_stateTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_stateTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_stateTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_stateTree, &QTreeWidget::customContextMenuRequested, this, &AnimationEditorModule::onStateContextMenu);
    statesLayout->addWidget(m_stateTree);
    
    QHBoxLayout* stateBtnLayout = new QHBoxLayout();
    m_addStateBtn = createButton("Add State", "success");
    connect(m_addStateBtn, &QPushButton::clicked, this, &AnimationEditorModule::onAddState);
    stateBtnLayout->addWidget(m_addStateBtn);
    
    m_removeStateBtn = createButton("Remove State", "danger");
    connect(m_removeStateBtn, &QPushButton::clicked, this, &AnimationEditorModule::onRemoveState);
    stateBtnLayout->addWidget(m_removeStateBtn);
    
    stateBtnLayout->addStretch();
    
    m_stateAnimCombo = createComboBox({"<none>"});
    for (auto it = m_animations.begin(); it != m_animations.end(); ++it) {
        m_stateAnimCombo->addItem(it.key());
    }
    stateBtnLayout->addWidget(createLabel("Animation:"));
    stateBtnLayout->addWidget(m_stateAnimCombo);
    
    statesLayout->addLayout(stateBtnLayout);
    splitter->addWidget(statesGroup);
    
    // Transitions table
    QGroupBox* transGroup = createGroupBox("Transitions");
    QVBoxLayout* transLayout = new QVBoxLayout(transGroup);
    
    m_transitionTable = new QTableWidget();
    m_transitionTable->setColumnCount(5);
    m_transitionTable->setHorizontalHeaderLabels({"From", "To", "Condition", "Duration", "Exit Time"});
    m_transitionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_transitionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_transitionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_transitionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_transitionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    transLayout->addWidget(m_transitionTable);
    
    QHBoxLayout* transBtnLayout = new QHBoxLayout();
    m_addTransitionBtn = createButton("Add Transition", "success");
    connect(m_addTransitionBtn, &QPushButton::clicked, this, &AnimationEditorModule::onAddTransition);
    transBtnLayout->addWidget(m_addTransitionBtn);
    
    QPushButton* remTransBtn = createButton("Remove Transition", "danger");
    connect(remTransBtn, &QPushButton::clicked, this, [this]() {
        QList<QTableWidgetSelectionRange> ranges = m_transitionTable->selectedRanges();
        for (const auto& range : ranges) {
            for (int row = range.bottomRow(); row >= range.topRow(); --row) {
                m_transitionTable->removeRow(row);
            }
        }
    });
    transBtnLayout->addWidget(remTransBtn);
    
    transBtnLayout->addStretch();
    transLayout->addLayout(transBtnLayout);
    
    splitter->addWidget(transGroup);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    
    layout->addWidget(splitter, 1);
}

void AnimationEditorModule::setupBlendTreeTab() {
    m_blendTreeTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_blendTreeTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    
    QGroupBox* toolGroup = createGroupBox("Blend Tree");
    QHBoxLayout* toolLayout = new QHBoxLayout(toolGroup);
    
    toolLayout->addWidget(createLabel("Type:"));
    m_blendTypeCombo = createComboBox({"1D", "2D Simple Directional", "2D Freeform Directional", "2D Freeform Cartesian", "Direct"});
    toolLayout->addWidget(m_blendTypeCombo);
    
    toolLayout->addWidget(createLabel("Parameter:"));
    m_blendParamSpin = new QDoubleSpinBox();
    m_blendParamSpin->setRange(-100, 100);
    m_blendParamSpin->setValue(0);
    m_blendParamSpin->setMaximumWidth(100);
    toolLayout->addWidget(m_blendParamSpin);
    
    m_addBlendNodeBtn = createButton("Add Node", "success");
    connect(m_addBlendNodeBtn, &QPushButton::clicked, this, &AnimationEditorModule::onBlendTreeNodeAdded);
    toolLayout->addWidget(m_addBlendNodeBtn);
    
    m_removeBlendNodeBtn = createButton("Remove Node", "danger");
    connect(m_removeBlendNodeBtn, &QPushButton::clicked, this, [this]() {
        QList<QTreeWidgetItem*> items = m_blendTreeNodes->selectedItems();
        for (QTreeWidgetItem* item : items) delete item;
    });
    toolLayout->addWidget(m_removeBlendNodeBtn);
    
    toolLayout->addStretch();
    layout->addWidget(toolGroup);
    
    m_blendTreeNodes = createTreeWidget({"Node", "Type", "Animation", "Weight/Threshold", "Children"});
    m_blendTreeNodes->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_blendTreeNodes->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_blendTreeNodes->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_blendTreeNodes->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_blendTreeNodes->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    layout->addWidget(m_blendTreeNodes, 1);
}

void AnimationEditorModule::setupPropertiesTab() {
    m_propertiesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_propertiesTab);
    layout->setContentsMargins(8, 8, 8, 8);
    
    QGroupBox* propsGroup = createGroupBox("Properties");
    QVBoxLayout* propsLayout = new QVBoxLayout(propsGroup);
    
    m_propertiesTree = createTreeWidget({"Property", "Value"});
    m_propertiesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_propertiesTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    propsLayout->addWidget(m_propertiesTree);
    
    layout->addWidget(propsGroup);
    
    // Populate when animation selected
    connect(m_animationTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        updateProperties();
    });
}

void AnimationEditorModule::setupCurvesTab() {
    m_curvesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_curvesTab);
    layout->setContentsMargins(8, 8, 8, 8);
    
    m_curvesPlaceholder = createLabel(
        "Curve Editor\n\n"
        "Visual curve editor for animation tracks.\n"
        "Features:\n"
        "- Bezier curve editing with handles\n"
        "- Multiple track overlay\n"
        "- Keyframe manipulation\n"
        "- Tangent editing (auto, free, aligned, broken)\n"
        "- Pre/post infinity modes\n"
        "- Curve scaling and offset\n\n"
        "[Implementation requires custom QWidget with QPainter]",
        "color: #666; font-size: 14px;"
    );
    m_curvesPlaceholder->setAlignment(Qt::AlignCenter);
    m_curvesPlaceholder->setWordWrap(true);
    layout->addWidget(m_curvesPlaceholder);
}

void AnimationEditorModule::refreshAnimationList() {
    m_animationTree->clear();
    m_stateAnimCombo->clear();
    m_stateAnimCombo->addItem("<none>");
    
    for (auto it = m_animations.begin(); it != m_animations.end(); ++it) {
        const AnimationClip& clip = it.value();
        QTreeWidgetItem* item = new QTreeWidgetItem(m_animationTree);
        item->setText(0, clip.name);
        item->setText(1, QString("%1s").arg(clip.duration, 0, 'f', 3));
        item->setText(2, QString::number(clip.tracks.size()));
        item->setText(3, clip.loop ? "Yes" : "No");
        item->setData(0, Qt::UserRole, clip.name);
        
        m_stateAnimCombo->addItem(clip.name);
    }
}

void AnimationEditorModule::refreshKeyframeTable() {
    m_keyframeTable->setRowCount(0);
    
    if (m_currentAnimation.isEmpty() || !m_animations.contains(m_currentAnimation)) return;
    
    AnimationClip& clip = m_animations[m_currentAnimation];
    
    for (auto it = clip.tracks.begin(); it != clip.tracks.end(); ++it) {
        const QString& trackName = it.key();
        const QVector<QPair<double, QVariant>>& keyframes = it.value();
        
        for (const auto& kf : keyframes) {
            int row = m_keyframeTable->rowCount();
            m_keyframeTable->insertRow(row);
            
            m_keyframeTable->setItem(row, 0, new QTableWidgetItem(trackName));
            m_keyframeTable->setItem(row, 1, new QTableWidgetItem(QString("%1s").arg(kf.first, 0, 'f', 3)));
            m_keyframeTable->setItem(row, 2, new QTableWidgetItem(kf.second.toString()));
            m_keyframeTable->setItem(row, 3, new QTableWidgetItem("Linear"));
            m_keyframeTable->setItem(row, 4, new QTableWidgetItem("Ease In/Out"));
            
            // Highlight current frame
            if (qAbs(kf.first - m_currentTime) < 0.02) {
                for (int c = 0; c < 5; ++c) {
                    if (m_keyframeTable->item(row, c)) {
                        m_keyframeTable->item(row, c)->setBackground(QBrush(QColor("#3a5a8a")));
                    }
                }
            }
        }
    }
}

void AnimationEditorModule::refreshStateList() {
    m_stateTree->clear();
    
    if (m_currentStateMachine.isEmpty() || !m_stateMachines.contains(m_currentStateMachine)) return;
    
    StateMachine& sm = m_stateMachines[m_currentStateMachine];
    
    for (auto it = sm.states.begin(); it != sm.states.end(); ++it) {
        const QString& stateName = it.key();
        const QString& animName = it.value();
        
        int transCount = 0;
        for (const auto& t : sm.transitions) {
            if (t.first == stateName || t.second == stateName) transCount++;
        }
        
        QTreeWidgetItem* item = new QTreeWidgetItem(m_stateTree);
        item->setText(0, stateName);
        item->setText(1, animName);
        item->setText(2, QString::number(transCount));
        item->setData(0, Qt::UserRole, stateName);
    }
}

void AnimationEditorModule::updateTimelineScrubber() {
    if (!m_currentAnimation.isEmpty() && m_animations.contains(m_currentAnimation)) {
        AnimationClip& clip = m_animations[m_currentAnimation];
        int sliderPos = clip.duration > 0 ? int((m_currentTime / clip.duration) * 10000) : 0;
        m_timelineSlider->blockSignals(true);
        m_timelineSlider->setValue(sliderPos);
        m_timelineSlider->blockSignals(false);
        
        m_timeLabel->setText(QString("%1s / %2s (Frame %3/%4)")
            .arg(m_currentTime, 0, 'f', 3)
            .arg(clip.duration, 0, 'f', 3)
            .arg(int(m_currentTime * m_fps))
            .arg(int(clip.duration * m_fps)));
    }
}

void AnimationEditorModule::playAnimation() {
    m_playing = true;
    m_playbackTimer->start();
    m_playBtn->setVisible(false);
    m_pauseBtn->setVisible(true);
    log("Playing: " + m_currentAnimation);
}

void AnimationEditorModule::pauseAnimation() {
    m_playing = false;
    m_playbackTimer->stop();
    m_playBtn->setVisible(true);
    m_pauseBtn->setVisible(false);
    log("Paused: " + m_currentAnimation);
}

void AnimationEditorModule::stopAnimation() {
    m_playing = false;
    m_playbackTimer->stop();
    m_currentTime = 0.0;
    m_playBtn->setVisible(true);
    m_pauseBtn->setVisible(false);
    m_timeSpin->blockSignals(true);
    m_timeSpin->setValue(0);
    m_timeSpin->blockSignals(false);
    m_frameSpin->blockSignals(true);
    m_frameSpin->setValue(0);
    m_frameSpin->blockSignals(false);
    updateTimelineScrubber();
    refreshKeyframeTable();
    log("Stopped: " + m_currentAnimation);
}

void AnimationEditorModule::updatePlaybackUI() {
    updateTimelineScrubber();
}

void AnimationEditorModule::updateProperties() {
    m_propertiesTree->clear();
    
    if (m_currentAnimation.isEmpty() || !m_animations.contains(m_currentAnimation)) return;
    
    AnimationClip& clip = m_animations[m_currentAnimation];
    
    QStringList props = {
        "Name", clip.name,
        "Duration", QString("%1s").arg(clip.duration, 0, 'f', 3),
        "Loop", clip.loop ? "Yes" : "No",
        "Track Count", QString::number(clip.tracks.size()),
        "Total Keyframes", QString::number([&clip]() {
            int count = 0;
            for (auto it = clip.tracks.begin(); it != clip.tracks.end(); ++it) count += it.value().size();
            return count;
        }())
    };
    
    for (int i = 0; i < props.size(); i += 2) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_propertiesTree);
        item->setText(0, props[i]);
        item->setText(1, props[i + 1]);
    }
    
    // Track details
    for (auto it = clip.tracks.begin(); it != clip.tracks.end(); ++it) {
        QTreeWidgetItem* trackItem = new QTreeWidgetItem(m_propertiesTree);
        trackItem->setText(0, "Track: " + it.key());
        trackItem->setText(1, QString("%1 keyframes").arg(it.value().size()));
        
        for (const auto& kf : it.value()) {
            QTreeWidgetItem* kfItem = new QTreeWidgetItem(trackItem);
            kfItem->setText(0, QString("  @ %1s").arg(kf.first, 0, 'f', 3));
            kfItem->setText(1, kf.second.toString());
        }
        trackItem->setExpanded(true);
    }
    
    m_propertiesTree->expandAll();
}

// Slots
void AnimationEditorModule::onNewAnimation() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Animation", "Name:", QLineEdit::Normal, "Animation", &ok);
    if (ok && !name.isEmpty()) {
        AnimationClip clip;
        clip.name = name;
        clip.duration = 1.0;
        clip.loop = true;
        m_animations[name] = clip;
        m_currentAnimation = name;
        m_durationSpin->blockSignals(true);
        m_durationSpin->setValue(1.0);
        m_durationSpin->blockSignals(false);
        refreshAnimationList();
        refreshKeyframeTable();
        updatePlaybackUI();
        updateProperties();
        logSuccess("Created animation: " + name);
    }
}

void AnimationEditorModule::onDeleteAnimation() {
    if (m_currentAnimation.isEmpty()) return;
    
    if (confirmAction("Delete Animation", "Delete '" + m_currentAnimation + "'?")) {
        m_animations.remove(m_currentAnimation);
        m_currentAnimation.clear();
        refreshAnimationList();
        refreshKeyframeTable();
        updatePlaybackUI();
        updateProperties();
        log("Deleted animation");
    }
}

void AnimationEditorModule::onDuplicateAnimation() {
    if (m_currentAnimation.isEmpty() || !m_animations.contains(m_currentAnimation)) return;
    
    AnimationClip clip = m_animations[m_currentAnimation];
    QString newName = m_currentAnimation + "_copy";
    int suffix = 1;
    while (m_animations.contains(newName)) {
        newName = m_currentAnimation + "_copy" + QString::number(suffix++);
    }
    clip.name = newName;
    m_animations[newName] = clip;
    m_currentAnimation = newName;
    refreshAnimationList();
    refreshKeyframeTable();
    updatePlaybackUI();
    updateProperties();
    logSuccess("Duplicated animation: " + newName);
}

void AnimationEditorModule::onAddKeyframe() {
    if (m_currentAnimation.isEmpty() || !m_animations.contains(m_currentAnimation)) {
        logError("No animation selected");
        return;
    }
    
    bool ok;
    QString track = QInputDialog::getText(this, "Add Keyframe", "Track name:", QLineEdit::Normal, "position", &ok);
    if (!ok || track.isEmpty()) return;
    
    QVariant value;
    QString type = QInputDialog::getItem(this, "Keyframe Type", "Value type:", 
        {"Float", "Vector2D", "Vector3D", "Quaternion", "Color", "Bool"}, 0, false, &ok);
    if (!ok) return;
    
    if (type == "Float") {
        bool ok2;
        double v = QInputDialog::getDouble(this, "Keyframe Value", "Value:", 0, -1e9, 1e9, 3, &ok2);
        if (ok2) value = v;
    } else if (type == "Vector3D") {
        bool ok2;
        double x = QInputDialog::getDouble(this, "X:", "X:", 0, -1e9, 1e9, 3, &ok2);
        if (!ok2) return;
        double y = QInputDialog::getDouble(this, "Y:", "Y:", 0, -1e9, 1e9, 3, &ok2);
        if (!ok2) return;
        double z = QInputDialog::getDouble(this, "Z:", "Z:", 0, -1e9, 1e9, 3, &ok2);
        if (!ok2) return;
        value = QVector3D(x, y, z);
    } else if (type == "Color") {
        QColor c = QColorDialog::getColor(Qt::white, this, "Keyframe Color");
        if (c.isValid()) value = c;
    } else if (type == "Bool") {
        value = QInputDialog::getItem(this, "Bool", "Value:", {"true", "false"}, 0, false, &ok) == "true";
    }
    
    if (!value.isValid()) return;
    
    AnimationClip& clip = m_animations[m_currentAnimation];
    clip.tracks[track].append(qMakePair(m_currentTime, value));
    
    // Sort keyframes by time
    std::sort(clip.tracks[track].begin(), clip.tracks[track].end(),
        [](const QPair<double, QVariant>& a, const QPair<double, QVariant>& b) {
            return a.first < b.first;
        });
    
    refreshKeyframeTable();
    updateProperties();
    logSuccess("Added keyframe to " + track + " at " + QString::number(m_currentTime, 'f', 3) + "s");
}

void AnimationEditorModule::onRemoveKeyframe() {
    QList<QTableWidgetSelectionRange> ranges = m_keyframeTable->selectedRanges();
    if (ranges.isEmpty()) return;
    
    if (m_currentAnimation.isEmpty() || !m_animations.contains(m_currentAnimation)) return;
    
    AnimationClip& clip = m_animations[m_currentAnimation];
    
    // Collect rows to remove (from bottom to top)
    QSet<int> rowsToRemove;
    for (const auto& range : ranges) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            rowsToRemove.insert(row);
        }
    }
    
    QList<int> sorted_rows = rowsToRemove.values();
    std::sort(sorted_rows.begin(), sorted_rows.end(), std::greater<int>());
    
    // We need to map table rows back to tracks and keyframe indices
    // For simplicity, rebuild tracks from table
    rebuildTracksFromTable();
    
    refreshKeyframeTable();
    updateProperties();
    log("Removed keyframe(s)");
}

void AnimationEditorModule::onPlayPause() {
    if (m_currentAnimation.isEmpty()) return;
    
    if (m_playing) {
        pauseAnimation();
    } else {
        playAnimation();
    }
}

void AnimationEditorModule::onStop() {
    stopAnimation();
}

void AnimationEditorModule::onLoopToggled(bool checked) {
    if (!m_currentAnimation.isEmpty() && m_animations.contains(m_currentAnimation)) {
        m_animations[m_currentAnimation].loop = checked;
    }
}

void AnimationEditorModule::onTimeChanged(double value) {
    m_currentTime = value;
    m_frameSpin->blockSignals(true);
    m_frameSpin->setValue(int(value * m_fps));
    m_frameSpin->blockSignals(false);
    updateTimelineScrubber();
    refreshKeyframeTable();
}

void AnimationEditorModule::onFrameChanged(int frame) {
    double time = frame / double(m_fps);
    m_currentTime = time;
    m_timeSpin->blockSignals(true);
    m_timeSpin->setValue(time);
    m_timeSpin->blockSignals(false);
    updateTimelineScrubber();
    refreshKeyframeTable();
}

void AnimationEditorModule::onAnimationSelected(QTreeWidgetItem* item, int column) {
    Q_UNUSED(column);
    if (item) {
        m_currentAnimation = item->text(0);
        refreshKeyframeTable();
        updatePlaybackUI();
        updateProperties();
    }
}

void AnimationEditorModule::onKeyframeSelected(int row, int column) {
    Q_UNUSED(column);
    // Could show keyframe details in properties
}

void AnimationEditorModule::onInterpolationChanged(int index) {
    // Apply to selected keyframes
    Q_UNUSED(index);
}

void AnimationEditorModule::onAddTrack() {
    if (m_currentAnimation.isEmpty() || !m_animations.contains(m_currentAnimation)) {
        logError("No animation selected");
        return;
    }
    
    bool ok;
    QString track = QInputDialog::getText(this, "Add Track", "Track name:", QLineEdit::Normal, "newTrack", &ok);
    if (ok && !track.isEmpty()) {
        AnimationClip& clip = m_animations[m_currentAnimation];
        if (!clip.tracks.contains(track)) {
            clip.tracks[track] = QVector<QPair<double, QVariant>>();
            refreshKeyframeTable();
            updateProperties();
            logSuccess("Added track: " + track);
        }
    }
}

void AnimationEditorModule::onRemoveTrack() {
    if (m_currentAnimation.isEmpty() || !m_animations.contains(m_currentAnimation)) return;
    
    bool ok;
    QString track = QInputDialog::getItem(this, "Remove Track", "Track:", 
        [this]() {
            QStringList tracks;
            if (m_animations.contains(m_currentAnimation)) {
                for (auto it = m_animations[m_currentAnimation].tracks.begin(); it != m_animations[m_currentAnimation].tracks.end(); ++it) {
                    tracks << it.key();
                }
            }
            return tracks;
        }(), 0, false, &ok);
    if (ok && !track.isEmpty()) {
        m_animations[m_currentAnimation].tracks.remove(track);
        refreshKeyframeTable();
        updateProperties();
        log("Removed track: " + track);
    }
}

void AnimationEditorModule::onAddState() {
    if (m_currentStateMachine.isEmpty() || !m_stateMachines.contains(m_currentStateMachine)) {
        logError("No state machine selected");
        return;
    }
    
    bool ok;
    QString stateName = QInputDialog::getText(this, "Add State", "State name:", QLineEdit::Normal, "State", &ok);
    if (!ok || stateName.isEmpty()) return;
    
    StateMachine& sm = m_stateMachines[m_currentStateMachine];
    sm.states[stateName] = m_stateAnimCombo->currentText() == "<none>" ? "" : m_stateAnimCombo->currentText();
    refreshStateList();
    logSuccess("Added state: " + stateName);
}

void AnimationEditorModule::onRemoveState() {
    if (m_currentStateMachine.isEmpty() || !m_stateMachines.contains(m_currentStateMachine)) return;
    
    QList<QTreeWidgetItem*> items = m_stateTree->selectedItems();
    if (items.isEmpty()) return;
    
    StateMachine& sm = m_stateMachines[m_currentStateMachine];
    for (QTreeWidgetItem* item : items) {
        QString stateName = item->data(0, Qt::UserRole).toString();
        sm.states.remove(stateName);
        // Remove associated transitions
        for (int i = sm.transitions.size() - 1; i >= 0; --i) {
            if (sm.transitions[i].first == stateName || sm.transitions[i].second == stateName) {
                sm.transitions.removeAt(i);
            }
        }
    }
    refreshStateList();
    log("Removed state(s)");
}

void AnimationEditorModule::onAddTransition() {
    if (m_currentStateMachine.isEmpty() || !m_stateMachines.contains(m_currentStateMachine)) return;
    
    StateMachine& sm = m_stateMachines[m_currentStateMachine];
    if (sm.states.size() < 2) {
        logError("Need at least 2 states for a transition");
        return;
    }
    
    bool ok;
    QString from = QInputDialog::getItem(this, "Add Transition", "From:", 
        sm.states.keys(), 0, false, &ok);
    if (!ok) return;
    
    QString to = QInputDialog::getItem(this, "Add Transition", "To:", 
        sm.states.keys(), 0, false, &ok);
    if (!ok) return;
    
    if (from == to) {
        logError("From and To cannot be the same");
        return;
    }
    
    sm.transitions.append(qMakePair(from, to));
    
    int row = m_transitionTable->rowCount();
    m_transitionTable->insertRow(row);
    m_transitionTable->setItem(row, 0, new QTableWidgetItem(from));
    m_transitionTable->setItem(row, 1, new QTableWidgetItem(to));
    m_transitionTable->setItem(row, 2, new QTableWidgetItem(""));
    m_transitionTable->setItem(row, 3, new QTableWidgetItem("0.25"));
    m_transitionTable->setItem(row, 4, new QTableWidgetItem("1.0"));
    
    refreshStateList();
    logSuccess("Added transition: " + from + " -> " + to);
}

void AnimationEditorModule::onBlendTreeNodeAdded() {
    // Add a blend tree node
    QTreeWidgetItem* item = new QTreeWidgetItem(m_blendTreeNodes);
    item->setText(0, "BlendNode_" + QString::number(m_blendTreeNodes->topLevelItemCount() + 1));
    item->setText(1, m_blendTypeCombo->currentText());
    item->setText(2, "<animation>");
    item->setText(3, "0.0");
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    log("Added blend tree node");
}

void AnimationEditorModule::onExportAnimation() {
    if (m_currentAnimation.isEmpty()) return;
    
    QString file = QFileDialog::getSaveFileName(this, "Export Animation", QString(), "Animation (*.anim *.json)");
    if (!file.isEmpty()) {
        saveAnimationToFile(file);
    }
}

void AnimationEditorModule::onImportAnimation() {
    QString file = selectFile("Import Animation", "Animation (*.anim *.json)");
    if (!file.isEmpty()) {
        loadAnimationFromFile(file);
    }
}

void AnimationEditorModule::onShowContextMenu(const QPoint& pos) {
    QTreeWidget* tree = qobject_cast<QTreeWidget*>(sender());
    if (!tree) return;
    
    QTreeWidgetItem* item = tree->itemAt(pos);
    QMenu menu(this);
    
    if (tree == m_animationTree) {
        if (item) {
            QAction* dupAct = menu.addAction("Duplicate");
            connect(dupAct, &QAction::triggered, this, &AnimationEditorModule::onDuplicateAnimation);
            
            QAction* delAct = menu.addAction("Delete");
            connect(delAct, &QAction::triggered, this, &AnimationEditorModule::onDeleteAnimation);
            
            QAction* exportAct = menu.addAction("Export");
            connect(exportAct, &QAction::triggered, this, &AnimationEditorModule::onExportAnimation);
        }
        
        QAction* newAct = menu.addAction("New Animation");
        connect(newAct, &QAction::triggered, this, &AnimationEditorModule::onNewAnimation);
        
        QAction* importAct = menu.addAction("Import");
        connect(importAct, &QAction::triggered, this, &AnimationEditorModule::onImportAnimation);
    } else if (sender() == m_keyframeTable) {
        if (!m_keyframeTable->selectedItems().isEmpty()) {
            QAction* delAct = menu.addAction("Delete Keyframe(s)");
            connect(delAct, &QAction::triggered, this, &AnimationEditorModule::onRemoveKeyframe);
            
            QAction* copyAct = menu.addAction("Copy Keyframe(s)");
            connect(copyAct, &QAction::triggered, this, [this]() {
                // Copy selected keyframes to clipboard as JSON
            });
            
            QAction* pasteAct = menu.addAction("Paste Keyframe(s)");
            connect(pasteAct, &QAction::triggered, this, [this]() {
                // Paste keyframes at current time
            });
        }
    }
    
    menu.exec(tree->viewport()->mapToGlobal(pos));
}

void AnimationEditorModule::onStateContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = m_stateTree->itemAt(pos);
    QMenu menu(this);
    
    QAction* addStateAct = menu.addAction("Add State");
    connect(addStateAct, &QAction::triggered, this, &AnimationEditorModule::onAddState);
    
    if (item) {
        QAction* delAct = menu.addAction("Remove State");
        connect(delAct, &QAction::triggered, this, &AnimationEditorModule::onRemoveState);
        
        QAction* addTransAct = menu.addAction("Add Transition From Here");
        connect(addTransAct, &QAction::triggered, this, &AnimationEditorModule::onAddTransition);
    }
    
    menu.exec(m_stateTree->viewport()->mapToGlobal(pos));
}

void AnimationEditorModule::onTransitionContextMenu(const QPoint& pos) {
    Q_UNUSED(pos);
}

void AnimationEditorModule::rebuildTracksFromTable() {
    if (m_currentAnimation.isEmpty() || !m_animations.contains(m_currentAnimation)) return;
    
    AnimationClip& clip = m_animations[m_currentAnimation];
    clip.tracks.clear();
    
    for (int row = 0; row < m_keyframeTable->rowCount(); ++row) {
        QString track = m_keyframeTable->item(row, 0)->text();
        double time = m_keyframeTable->item(row, 1)->text().replace("s", "").toDouble();
        QString valueStr = m_keyframeTable->item(row, 2)->text();
        
        QVariant value;
        if (valueStr.contains(",")) {
            QStringList parts = valueStr.split(",");
            if (parts.size() == 3) {
                value = QVector3D(parts[0].toDouble(), parts[1].toDouble(), parts[2].toDouble());
            } else if (parts.size() == 2) {
                value = QVector2D(parts[0].toDouble(), parts[1].toDouble());
            }
        } else {
            bool ok;
            double v = valueStr.toDouble(&ok);
            if (ok) value = v;
            else value = valueStr;
        }
        
        clip.tracks[track].append(qMakePair(time, value));
    }
    
    // Sort each track
    for (auto it = clip.tracks.begin(); it != clip.tracks.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(),
            [](const QPair<double, QVariant>& a, const QPair<double, QVariant>& b) {
                return a.first < b.first;
            });
    }
}

void AnimationEditorModule::saveAnimationToFile(const QString& path) {
    QJsonObject obj;
    obj["version"] = 1;
    
    QJsonArray animsArray;
    for (auto it = m_animations.begin(); it != m_animations.end(); ++it) {
        const AnimationClip& clip = it.value();
        QJsonObject animObj;
        animObj["name"] = clip.name;
        animObj["duration"] = clip.duration;
        animObj["loop"] = clip.loop;
        
        QJsonArray tracksArray;
        for (auto tit = clip.tracks.begin(); tit != clip.tracks.end(); ++tit) {
            QJsonObject trackObj;
            trackObj["name"] = tit.key();
            
            QJsonArray kfsArray;
            for (const auto& kf : tit.value()) {
                QJsonObject kfObj;
                kfObj["time"] = kf.first;
                kfObj["value"] = QJsonValue::fromVariant(kf.second);
                kfsArray.append(kfObj);
            }
            trackObj["keyframes"] = kfsArray;
            tracksArray.append(trackObj);
        }
        animObj["tracks"] = tracksArray;
        animsArray.append(animObj);
    }
    obj["animations"] = animsArray;
    
    // State machines
    QJsonArray smArray;
    for (auto it = m_stateMachines.begin(); it != m_stateMachines.end(); ++it) {
        const StateMachine& sm = it.value();
        QJsonObject smObj;
        smObj["name"] = sm.name;
        
        QJsonObject statesObj;
        for (auto sit = sm.states.begin(); sit != sm.states.end(); ++sit) {
            statesObj[sit.key()] = sit.value();
        }
        smObj["states"] = statesObj;
        
        QJsonArray transArray;
        for (const auto& t : sm.transitions) {
            QJsonObject tObj;
            tObj["from"] = t.first;
            tObj["to"] = t.second;
            transArray.append(tObj);
        }
        smObj["transitions"] = transArray;
        
        smArray.append(smObj);
    }
    obj["stateMachines"] = smArray;
    
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        logSuccess("Exported to: " + path);
    }
}

void AnimationEditorModule::loadAnimationFromFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        logError("Failed to open file");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject obj = doc.object();
    
    m_animations.clear();
    
    QJsonArray animsArray = obj["animations"].toArray();
    for (const auto& animVal : animsArray) {
        QJsonObject animObj = animVal.toObject();
        AnimationClip clip;
        clip.name = animObj["name"].toString();
        clip.duration = animObj["duration"].toDouble(1.0);
        clip.loop = animObj["loop"].toBool(true);
        
        QJsonArray tracksArray = animObj["tracks"].toArray();
        for (const auto& trackVal : tracksArray) {
            QJsonObject trackObj = trackVal.toObject();
            QString trackName = trackObj["name"].toString();
            
            QJsonArray kfsArray = trackObj["keyframes"].toArray();
            for (const auto& kfVal : kfsArray) {
                QJsonObject kfObj = kfVal.toObject();
                double time = kfObj["time"].toDouble();
                QVariant value = kfObj["value"].toVariant();
                clip.tracks[trackName].append(qMakePair(time, value));
            }
        }
        
        m_animations[clip.name] = clip;
    }
    
    // State machines
    QJsonArray smArray = obj["stateMachines"].toArray();
    for (const auto& smVal : smArray) {
        QJsonObject smObj = smVal.toObject();
        StateMachine sm;
        sm.name = smObj["name"].toString();
        
        QJsonObject statesObj = smObj["states"].toObject();
        for (auto it = statesObj.begin(); it != statesObj.end(); ++it) {
            sm.states[it.key()] = it.value().toString();
        }
        
        QJsonArray transArray = smObj["transitions"].toArray();
        for (const auto& tVal : transArray) {
            QJsonObject tObj = tVal.toObject();
            sm.transitions.append(qMakePair(tObj["from"].toString(), tObj["to"].toString()));
        }
        
        m_stateMachines[sm.name] = sm;
    }
    
    refreshAnimationList();
    refreshStateList();
    
    if (!m_animations.isEmpty()) {
        m_currentAnimation = m_animations.begin().key();
        m_durationSpin->blockSignals(true);
        m_durationSpin->setValue(m_animations[m_currentAnimation].duration);
        m_durationSpin->blockSignals(false);
    }
    
    refreshKeyframeTable();
    updatePlaybackUI();
    updateProperties();
    
    logSuccess("Loaded from: " + path);
}

void AnimationEditorModule::onActivation() {
    log("Animation Editor activated");
    refreshAnimationList();
    refreshStateList();
}

void AnimationEditorModule::onDeactivation() {
    if (m_playing) stopAnimation();
    log("Animation Editor deactivated");
}

} // namespace ks

#include "AnimationEditorModule.moc"