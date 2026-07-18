#include "../editor/EditorModule.h"
#include "WeatherEditorModule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QTreeWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QTabWidget>
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
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QCheckBox>
#include <QHeaderView>
#include <QRegularExpression>

namespace ks {
namespace weather {

WeatherEditorModule::WeatherEditorModule(QWidget* parent)
    : EditorModule(parent)
    , m_editor(new WeatherEditor(this))
{
}

WeatherEditorModule::~WeatherEditorModule() = default;

bool WeatherEditorModule::initialize() {
    if (m_initialized) return true;
    
    setupUI();
    connect(m_editor, &WeatherEditor::configChanged, this, &WeatherEditorModule::onConfigChanged);
    m_initialized = true;
    return true;
}

void WeatherEditorModule::shutdown() {
    if (!m_initialized) return;
    m_editor->deleteLater();
    m_initialized = false;
}

QDockWidget* WeatherEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow) {
    if (!m_dockWidget) {
        m_dockWidget = new QDockWidget(tr("Weather Editor"), mainWindow);
        m_dockWidget->setObjectName("weatherEditorDock");
        m_dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        m_dockWidget->setWidget(m_centralWidget);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_dockWidget);
    }
    return m_dockWidget;
}

void WeatherEditorModule::setupUI() {
    m_centralWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);
    
    // Toolbar
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    QPushButton* newBtn = new QPushButton(tr("New"));
    QPushButton* loadBtn = new QPushButton(tr("Load"));
    QPushButton* saveBtn = new QPushButton(tr("Save"));
    QPushButton* saveAsBtn = new QPushButton(tr("Save As"));
    QPushButton* exportBtn = new QPushButton(tr("Export"));
    QPushButton* previewBtn = new QPushButton(tr("Preview"));
    QPushButton* validateBtn = new QPushButton(tr("Validate"));
    
    toolbarLayout->addWidget(newBtn);
    toolbarLayout->addWidget(loadBtn);
    toolbarLayout->addWidget(saveBtn);
    toolbarLayout->addWidget(saveAsBtn);
    toolbarLayout->addWidget(exportBtn);
    toolbarLayout->addWidget(previewBtn);
    toolbarLayout->addWidget(validateBtn);
    toolbarLayout->addStretch();
    mainLayout->addLayout(toolbarLayout);
    
    // Main content area with splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter, 1);
    
    // Left panel - Sequences and Keyframes
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    
    // Sequence list
    QGroupBox* seqGroup = new QGroupBox(tr("Weather Sequences"));
    QVBoxLayout* seqLayout = new QVBoxLayout(seqGroup);
    m_sequenceTree = new QTreeWidget();
    m_sequenceTree->setHeaderLabels({tr("Sequence"), tr("Start"), tr("Duration"), tr("Loops")});
    m_sequenceTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    seqLayout->addWidget(m_sequenceTree);
    
    QHBoxLayout* seqBtnLayout = new QHBoxLayout();
    QPushButton* addSeqBtn = new QPushButton(tr("Add"));
    QPushButton* removeSeqBtn = new QPushButton(tr("Remove"));
    QPushButton* dupSeqBtn = new QPushButton(tr("Duplicate"));
    seqBtnLayout->addWidget(addSeqBtn);
    seqBtnLayout->addWidget(removeSeqBtn);
    seqBtnLayout->addWidget(dupSeqBtn);
    seqBtnLayout->addStretch();
    seqLayout->addLayout(seqBtnLayout);
    leftLayout->addWidget(seqGroup);
    
    // Keyframe editor
    QGroupBox* kfGroup = new QGroupBox(tr("Keyframes"));
    QVBoxLayout* kfLayout = new QVBoxLayout(kfGroup);
    m_keyframeTree = new QTreeWidget();
    m_keyframeTree->setHeaderLabels({tr("Time"), tr("Type"), tr("Clouds"), tr("Rain"), tr("Wind"), tr("Temp"), tr("Transition")});
    m_keyframeTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_keyframeTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    kfLayout->addWidget(m_keyframeTree);
    
    QHBoxLayout* kfBtnLayout = new QHBoxLayout();
    QPushButton* addKfBtn = new QPushButton(tr("Add"));
    QPushButton* removeKfBtn = new QPushButton(tr("Remove"));
    QPushButton* editKfBtn = new QPushButton(tr("Edit"));
    QPushButton* moveKfBtn = new QPushButton(tr("Move"));
    kfBtnLayout->addWidget(addKfBtn);
    kfBtnLayout->addWidget(removeKfBtn);
    kfBtnLayout->addWidget(editKfBtn);
    kfBtnLayout->addWidget(moveKfBtn);
    kfBtnLayout->addStretch();
    kfLayout->addLayout(kfBtnLayout);
    leftLayout->addWidget(kfGroup);
    
    // Right panel - Keyframe editor + Preview
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    
    // Keyframe editor
    QGroupBox* kfEditGroup = new QGroupBox(tr("Keyframe Editor"));
    QFormLayout* kfEditLayout = new QFormLayout(kfEditGroup);
    
    m_timeSpin = new QDoubleSpinBox();
    m_timeSpin->setRange(0, 24);
    m_timeSpin->setSingleStep(0.25);
    m_timeSpin->setSuffix(" h");
    
    m_typeCombo = new QComboBox();
    m_typeCombo->addItems({"clear", "partly_cloudy", "overcast", "light_rain", "heavy_rain", 
                          "storm", "fog", "snow", "blizzard", "custom"});
    
    m_cloudSpin = new QDoubleSpinBox();
    m_cloudSpin->setRange(0, 1);
    m_cloudSpin->setSingleStep(0.05);
    
    m_rainSpin = new QDoubleSpinBox();
    m_rainSpin->setRange(0, 1);
    m_rainSpin->setSingleStep(0.05);
    
    m_windSpeedSpin = new QDoubleSpinBox();
    m_windSpeedSpin->setRange(0, 50);
    m_windSpeedSpin->setSuffix(" m/s");
    
    m_windDirSpin = new QDoubleSpinBox();
    m_windDirSpin->setRange(0, 360);
    m_windDirSpin->setSuffix("°");
    
    m_tempSpin = new QDoubleSpinBox();
    m_tempSpin->setRange(-40, 50);
    m_tempSpin->setSuffix(" °C");
    
    m_humiditySpin = new QDoubleSpinBox();
    m_humiditySpin->setRange(0, 1);
    m_humiditySpin->setSingleStep(0.05);
    
    m_pressureSpin = new QDoubleSpinBox();
    m_pressureSpin->setRange(900, 1100);
    m_pressureSpin->setSuffix(" hPa");
    
    m_visibilitySpin = new QDoubleSpinBox();
    m_visibilitySpin->setRange(0, 100);
    m_visibilitySpin->setSuffix(" km");
    
    m_transitionCombo = new QComboBox();
    m_transitionCombo->addItems({"linear", "ease_in", "ease_out", "ease_in_out", "step"});
    
    m_particleCombo = new QComboBox();
    m_particleCombo->addItems({"none", "rain", "snow", "hail", "fog", "mist", "dust", "leaves"});
    
    m_skyColorBtn = new QPushButton();
    m_skyColorBtn->setFixedHeight(24);
    m_fogColorBtn = new QPushButton();
    m_fogColorBtn->setFixedHeight(24);
    m_fogDensitySpin = new QDoubleSpinBox();
    m_fogDensitySpin->setRange(0, 1);
    
    kfEditLayout->addRow(tr("Time"), m_timeSpin);
    kfEditLayout->addRow(tr("Type"), m_typeCombo);
    kfEditLayout->addRow(tr("Cloud Coverage"), m_cloudSpin);
    kfEditLayout->addRow(tr("Precipitation"), m_rainSpin);
    kfEditLayout->addRow(tr("Wind Speed"), m_windSpeedSpin);
    kfEditLayout->addRow(tr("Wind Direction"), m_windDirSpin);
    kfEditLayout->addRow(tr("Temperature"), m_tempSpin);
    kfEditLayout->addRow(tr("Humidity"), m_humiditySpin);
    kfEditLayout->addRow(tr("Pressure"), m_pressureSpin);
    kfEditLayout->addRow(tr("Visibility"), m_visibilitySpin);
    kfEditLayout->addRow(tr("Transition"), m_transitionCombo);
    kfEditLayout->addRow(tr("Particle Effect"), m_particleCombo);
    kfEditLayout->addRow(tr("Sky Color"), m_skyColorBtn);
    kfEditLayout->addRow(tr("Fog Color"), m_fogColorBtn);
    kfEditLayout->addRow(tr("Fog Density"), m_fogDensitySpin);
    
    rightLayout->addWidget(kfEditGroup);
    
    // Presets
    QGroupBox* presetGroup = new QGroupBox(tr("Presets"));
    QVBoxLayout* presetLayout = new QVBoxLayout(presetGroup);
    m_presetCombo = new QComboBox();
    m_presetCombo->addItems({"Clear Sky", "Overcast", "Light Rain", "Heavy Rain", "Thunderstorm",
                             "Fog", "Snow", "Blizzard", "Sunset", "Night", "Custom..."});
    presetLayout->addWidget(m_presetCombo);
    
    QHBoxLayout* presetBtnLayout = new QHBoxLayout();
    QPushButton* applyPresetBtn = new QPushButton(tr("Apply"));
    QPushButton* savePresetBtn = new QPushButton(tr("Save as Preset"));
    presetBtnLayout->addWidget(applyPresetBtn);
    presetBtnLayout->addWidget(savePresetBtn);
    presetLayout->addLayout(presetBtnLayout);
    rightLayout->addWidget(presetGroup);
    
    // Preview
    QGroupBox* previewGroup = new QGroupBox(tr("Preview"));
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    m_previewGraphics = new QGraphicsView();
    m_previewGraphics->setMinimumHeight(300);
    m_previewScene = new QGraphicsScene();
    m_previewGraphics->setScene(m_previewScene);
    m_previewGraphics->setRenderHint(QPainter::Antialiasing);
    m_previewGraphics->setDragMode(QGraphicsView::ScrollHandDrag);
    previewLayout->addWidget(m_previewGraphics);
    
    QHBoxLayout* previewControls = new QHBoxLayout();
    m_previewTimeSpin = new QDoubleSpinBox();
    m_previewTimeSpin->setRange(0, 24);
    m_previewTimeSpin->setSingleStep(0.25);
    m_previewTimeSpin->setSuffix(" h");
    m_autoPreviewCheck = new QCheckBox(tr("Auto-update"));
    m_autoPreviewCheck->setChecked(true);
    previewControls->addWidget(new QLabel(tr("Time:")));
    previewControls->addWidget(m_previewTimeSpin);
    previewControls->addWidget(m_autoPreviewCheck);
    previewControls->addStretch();
    previewLayout->addLayout(previewControls);
    rightLayout->addWidget(previewGroup);
    
    rightLayout->addStretch();
    
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    
    // Connect signals
    connect(newBtn, &QPushButton::clicked, this, [this]() {
        m_editor->createNewConfig("New Weather");
    });
    connect(loadBtn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, tr("Load Weather Config"), "",
            tr("Weather Files (*.ini *.lua *.json);;All Files (*)"));
        if (!file.isEmpty()) m_editor->loadConfig(file);
    });
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        if (m_editor->currentFile().isEmpty()) {
            QString file = QFileDialog::getSaveFileName(this, tr("Save Weather Config"), "",
                tr("Weather Config (*.ini);;Lua Config (*.lua);;JSON (*.json)"));
            if (!file.isEmpty()) m_editor->saveConfig(file);
        } else {
            m_editor->saveConfig(m_editor->currentFile());
        }
    });
    connect(saveAsBtn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getSaveFileName(this, tr("Save Weather Config As"), "",
            tr("Weather Config (*.ini);;Lua Config (*.lua);;JSON (*.json)"));
        if (!file.isEmpty()) m_editor->saveConfig(file);
    });
    connect(exportBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Export Directory"));
        if (!dir.isEmpty()) m_editor->exportToCSP(dir);
    });
    connect(previewBtn, &QPushButton::clicked, this, [this]() {
        // Toggle preview
    });
    connect(validateBtn, &QPushButton::clicked, this, [this]() {
        QStringList errors, warnings;
        m_editor->validateConfig(&errors);
        m_editor->checkKeyframeContinuity(&warnings);
        m_editor->checkTimeConflicts(&errors);
        
        QString msg;
        if (!errors.isEmpty()) msg += "Errors:\n" + errors.join("\n") + "\n";
        if (!warnings.isEmpty()) msg += "Warnings:\n" + warnings.join("\n");
        if (msg.isEmpty()) msg = "Configuration is valid!";
        QMessageBox::information(this, tr("Validation"), msg);
    });
    
    connect(addSeqBtn, &QPushButton::clicked, this, [this]() {
        QString id = m_editor->addSequence("New Sequence", 0, 24);
        if (!id.isEmpty()) updateSequenceTree();
    });
    connect(removeSeqBtn, &QPushButton::clicked, this, [this]() {
        QTreeWidgetItem* item = m_sequenceTree->currentItem();
        if (item) {
            QString id = item->text(0);
            if (m_editor->removeSequence(id)) updateSequenceTree();
        }
    });
    connect(dupSeqBtn, &QPushButton::clicked, this, [this]() {
        QTreeWidgetItem* item = m_sequenceTree->currentItem();
        if (item) {
            QString id = item->text(0);
            m_editor->duplicateSequence(id, id + "_copy");
            updateSequenceTree();
        }
    });
    
    connect(m_sequenceTree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
        if (current) {
            QString seqName = current->text(0);
            updateKeyframeTree(seqName);
        }
    });
    
    connect(addKfBtn, &QPushButton::clicked, this, [this]() {
        QTreeWidgetItem* seqItem = m_sequenceTree->currentItem();
        if (seqItem) {
            double time = m_timeSpin->value();
            m_editor->addKeyframe(seqItem->text(0), time, m_typeCombo->currentText());
            updateKeyframeTree(seqItem->text(0));
        }
    });
    connect(removeKfBtn, &QPushButton::clicked, this, [this]() {
        QTreeWidgetItem* kfItem = m_keyframeTree->currentItem();
        if (kfItem) {
            QTreeWidgetItem* seqItem = kfItem->parent();
            if (seqItem) {
                m_editor->removeKeyframe(seqItem->text(0), kfItem->text(0));
                updateKeyframeTree(seqItem->text(0));
            }
        }
    });
    connect(editKfBtn, &QPushButton::clicked, this, [this]() {
        // Edit keyframe - populate editor with selected keyframe
    });
    connect(moveKfBtn, &QPushButton::clicked, this, [this]() {
        // Move keyframe to new time
    });
    
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        // Update UI for type
    });
    connect(m_skyColorBtn, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(Qt::white, this);
        if (color.isValid()) {
            m_skyColorBtn->setStyleSheet(QString("background-color: %1;").arg(color.name()));
        }
    });
    connect(m_fogColorBtn, &QPushButton::clicked, this, [this]() {
        QColor color = QColorDialog::getColor(Qt::white, this);
        if (color.isValid()) {
            m_fogColorBtn->setStyleSheet(QString("background-color: %1;").arg(color.name()));
        }
    });
    
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_presetCombo->currentText() != "Custom...") {
            m_editor->applyPreset(m_presetCombo->currentText());
        }
    });
    connect(applyPresetBtn, &QPushButton::clicked, this, [this]() {
        m_editor->applyPreset(m_presetCombo->currentText());
    });
    connect(savePresetBtn, &QPushButton::clicked, this, [this]() {
        QString name = QInputDialog::getText(this, tr("Save Preset"), tr("Preset Name:"));
        if (!name.isEmpty()) m_editor->saveAsPreset(name);
    });
    
    connect(applyPresetBtn, &QPushButton::clicked, this, [this]() {
        // Generate preview
    });
    connect(m_autoPreviewCheck, &QCheckBox::toggled, this, [this](bool checked) {
        // Toggle auto preview
    });
    connect(m_previewTimeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double time) {
        // Update preview at time
    });
    
    connect(m_editor, &WeatherEditor::configChanged, this, [this]() {
        updateSequenceTree();
    });
    connect(m_editor, &WeatherEditor::sequenceAdded, this, [this](const QString&) { updateSequenceTree(); });
    connect(m_editor, &WeatherEditor::sequenceRemoved, this, [this](const QString&) { updateSequenceTree(); });
    connect(m_editor, &WeatherEditor::keyframeAdded, this, [this](const QString&, const QString&) { updateKeyframeTree(); });
    connect(m_editor, &WeatherEditor::keyframeRemoved, this, [this](const QString&, const QString&) { updateKeyframeTree(); });
    connect(m_editor, &WeatherEditor::keyframeChanged, this, [this](const QString&, const QString&) { updateKeyframeTree(); });
    connect(m_editor, &WeatherEditor::configChanged, this, [this]() {
        updateSequenceTree();
    });
}

void WeatherEditorModule::onConfigChanged() {
    updateSequenceTree();
}

void WeatherEditorModule::updateSequenceTree() {
    m_sequenceTree->clear();
    for (const auto& seq : m_editor->config().sequences) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, seq.name);
        item->setText(1, QString::number(seq.startTime, 'f', 2) + " h");
        item->setText(2, QString::number(seq.duration) + " h");
        item->setText(3, seq.loop ? tr("Yes") : tr("No"));
        item->setData(0, Qt::UserRole, seq.name);
        m_sequenceTree->addTopLevelItem(item);
    }
}

void WeatherEditorModule::updateKeyframeTree(const QString& sequenceName) {
    m_keyframeTree->clear();
    for (const auto& seq : m_editor->config().sequences) {
        if (seq.name == sequenceName) {
            for (const auto& kf : seq.keyframes) {
                QTreeWidgetItem* item = new QTreeWidgetItem();
                item->setText(0, QString::number(kf.time, 'f', 2) + " h");
                item->setText(1, kf.type);
                item->setText(2, QString::number(kf.cloudCoverage, 'f', 2));
                item->setText(3, QString::number(kf.precipitation, 'f', 2));
                item->setText(4, QString("%1 m/s @ %1°").arg(kf.windSpeed).arg(kf.windDirection));
                item->setText(5, QString::number(kf.temperature) + " °C");
                item->setText(6, kf.transitionType);
                item->setData(0, Qt::UserRole, kf.time);
                m_keyframeTree->addTopLevelItem(item);
            }
            break;
        }
    }
}

void WeatherEditorModule::importFile(const QString& filePath) {
    m_editor->loadConfig(filePath);
}

void WeatherEditorModule::exportFile(const QString& filePath) {
    m_editor->saveConfig(filePath);
}

void WeatherEditorModule::onActivation() {
    // Called when module becomes active
}

void WeatherEditorModule::onDeactivation() {
    // Called when module becomes inactive
}

QJsonObject WeatherEditorModule::serializeProject() const {
    QJsonObject obj;
    const auto& cfg = m_editor->config();
    obj["name"] = cfg.name;
    obj["trackName"] = cfg.trackName;
    obj["baseTime"] = cfg.baseTime;
    obj["timeMultiplier"] = cfg.timeMultiplier;
    obj["dynamicWeather"] = cfg.dynamicWeather;
    obj["weatherChangeInterval"] = cfg.weatherChangeInterval;
    obj["solConfigPath"] = cfg.solConfigPath;
    obj["weatherLuaPath"] = cfg.weatherLuaPath;
    QJsonArray seqArray;
    for (const auto& seq : cfg.sequences) {
        QJsonObject seqObj;
        seqObj["name"] = seq.name;
        seqObj["description"] = seq.description;
        seqObj["startTime"] = seq.startTime;
        seqObj["duration"] = seq.duration;
        seqObj["loop"] = seq.loop;
        QJsonArray kfArray;
        for (const auto& kf : seq.keyframes) {
            QJsonObject kfObj;
            kfObj["time"] = kf.time;
            kfObj["type"] = kf.type;
            kfObj["cloudCoverage"] = kf.cloudCoverage;
            kfObj["precipitation"] = kf.precipitation;
            kfObj["windSpeed"] = kf.windSpeed;
            kfObj["windDirection"] = kf.windDirection;
            kfObj["temperature"] = kf.temperature;
            kfObj["humidity"] = kf.humidity;
            kfObj["pressure"] = kf.pressure;
            kfObj["visibility"] = kf.visibility;
            kfObj["transitionType"] = kf.transitionType;
            kfArray.append(kfObj);
        }
        seqObj["keyframes"] = kfArray;
        seqArray.append(seqObj);
    }
    obj["sequences"] = seqArray;
    return obj;
}

void WeatherEditorModule::deserializeProject(const QJsonObject& data) {
    // Deserialize project data
}

QMap<QString, double> WeatherKeyframe::defaultValuesForType(const QString& type)
{
    QMap<QString, double> defaults;
    defaults["cloudCoverage"] = 0.0;
    defaults["precipitation"] = 0.0;
    defaults["windSpeed"] = 5.0;
    defaults["windDirection"] = 0.0;
    defaults["temperature"] = 20.0;
    defaults["humidity"] = 0.5;
    defaults["pressure"] = 1013.25;
    defaults["visibility"] = 10.0;

    if (type == "clear") {
        defaults["cloudCoverage"] = 0.1;
    } else if (type == "cloudy") {
        defaults["cloudCoverage"] = 0.6;
    } else if (type == "overcast") {
        defaults["cloudCoverage"] = 0.9;
    } else if (type == "light_rain") {
        defaults["cloudCoverage"] = 0.7;
        defaults["precipitation"] = 0.3;
    } else if (type == "heavy_rain") {
        defaults["cloudCoverage"] = 0.85;
        defaults["precipitation"] = 0.8;
    } else if (type == "storm") {
        defaults["cloudCoverage"] = 0.95;
        defaults["precipitation"] = 1.0;
        defaults["windSpeed"] = 20.0;
    } else if (type == "fog") {
        defaults["visibility"] = 1.0;
        defaults["humidity"] = 0.95;
    } else if (type == "snow") {
        defaults["cloudCoverage"] = 0.8;
        defaults["precipitation"] = 0.5;
        defaults["temperature"] = -5.0;
    }
    return defaults;
}

bool WeatherKeyframe::isValid() const
{
    return time >= 0.0 && time <= 24.0;
}

// ============================================================================
// WeatherConfigParser (QObject version from WeatherEditorModule.h)
// ============================================================================

WeatherConfigParser::WeatherConfigParser(QObject* parent)
    : QObject(parent)
{
}

bool WeatherConfigParser::parseWeatherConfig(const QString& filePath, WeatherConfig& config, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = "Cannot open file: " + filePath;
        return false;
    }

    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();

    if (ext == "lua") {
        return parseWeatherLua(filePath, config, error);
    }
    if (ext == "json") {
        QByteArray data = file.readAll();
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
        if (parseErr.error != QJsonParseError::NoError) {
            if (error) *error = "JSON parse error: " + parseErr.errorString();
            return false;
        }
        QJsonObject obj = doc.object();
        config.name = obj["name"].toString();
        config.trackName = obj["trackName"].toString();
        config.baseTime = obj["baseTime"].toDouble(12.0);
        config.timeMultiplier = obj["timeMultiplier"].toDouble(1.0);
        config.dynamicWeather = obj["dynamicWeather"].toBool(true);
        config.weatherChangeInterval = obj["weatherChangeInterval"].toDouble(2.0);
        config.solConfigPath = obj["solConfigPath"].toString();
        config.weatherLuaPath = obj["weatherLuaPath"].toString();

        QJsonArray seqArray = obj["sequences"].toArray();
        for (const auto& seqVal : seqArray) {
            QJsonObject seqObj = seqVal.toObject();
            WeatherSequence seq;
            seq.name = seqObj["name"].toString();
            seq.description = seqObj["description"].toString();
            seq.startTime = seqObj["startTime"].toDouble();
            seq.duration = seqObj["duration"].toDouble();
            seq.loop = seqObj["loop"].toBool(true);

            QJsonArray kfArray = seqObj["keyframes"].toArray();
            for (const auto& kfVal : kfArray) {
                QJsonObject kfObj = kfVal.toObject();
                WeatherKeyframe kf;
                kf.time = kfObj["time"].toDouble();
                kf.type = kfObj["type"].toString("clear");
                kf.cloudCoverage = kfObj["cloudCoverage"].toDouble();
                kf.precipitation = kfObj["precipitation"].toDouble();
                kf.windSpeed = kfObj["windSpeed"].toDouble();
                kf.windDirection = kfObj["windDirection"].toDouble();
                kf.temperature = kfObj["temperature"].toDouble();
                kf.humidity = kfObj["humidity"].toDouble();
                kf.pressure = kfObj["pressure"].toDouble();
                kf.visibility = kfObj["visibility"].toDouble();
                kf.transitionType = kfObj["transitionType"].toString("linear");
                seq.keyframes.append(kf);
            }
            config.sequences.append(seq);
        }
        emit parsingFinished(true, "Config loaded from JSON");
        return true;
    }

    // Default: treat as INI
    QTextStream in(&file);
    QMap<QString, QString> kv;
    if (!parseIniSection(in, "WEATHER", kv)) {
        if (error) *error = "Failed to parse INI file";
        return false;
    }
    config.name = fi.baseName();
    config.baseTime = kv.value("TIME", "12").toDouble();
    config.timeMultiplier = kv.value("TIME_MULT", "1").toDouble();
    config.dynamicWeather = kv.value("DYNAMIC_WEATHER", "1").toInt() != 0;
    config.weatherChangeInterval = kv.value("CHANGE_INTERVAL", "2").toDouble();
    config.solConfigPath = kv.value("SOL_CONFIG", "");
    config.weatherLuaPath = kv.value("WEATHER_LUA", "");
    emit parsingFinished(true, "Config loaded from INI");
    return true;
}

bool WeatherConfigParser::parseWeatherLua(const QString& filePath, WeatherConfig& config, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = "Cannot open Lua file: " + filePath;
        return false;
    }
    QString luaCode = file.readAll();
    file.close();

    QJsonObject weatherTable;
    if (!parseLuaTable(luaCode, "weather", weatherTable)) {
        if (error) *error = "Failed to parse Lua weather table";
        return false;
    }
    config.name = weatherTable["name"].toString(QFileInfo(filePath).baseName());
    config.baseTime = weatherTable["baseTime"].toDouble(12.0);
    config.timeMultiplier = weatherTable["timeMultiplier"].toDouble(1.0);
    config.dynamicWeather = weatherTable["dynamicWeather"].toBool(true);
    config.weatherChangeInterval = weatherTable["weatherChangeInterval"].toDouble(2.0);

    QJsonArray seqArray = weatherTable["sequences"].toArray();
    for (const auto& seqVal : seqArray) {
        QJsonObject seqObj = seqVal.toObject();
        WeatherSequence seq;
        seq.name = seqObj["name"].toString();
        seq.startTime = seqObj["startTime"].toDouble();
        seq.duration = seqObj["duration"].toDouble();
        seq.loop = seqObj["loop"].toBool(true);

        QJsonArray kfArray = seqObj["keyframes"].toArray();
        for (const auto& kfVal : kfArray) {
            QJsonObject kfObj = kfVal.toObject();
            WeatherKeyframe kf;
            kf.time = kfObj["time"].toDouble();
            kf.type = kfObj["type"].toString("clear");
            kf.cloudCoverage = kfObj["cloudCoverage"].toDouble();
            kf.precipitation = kfObj["precipitation"].toDouble();
            kf.windSpeed = kfObj["windSpeed"].toDouble();
            kf.windDirection = kfObj["windDirection"].toDouble();
            kf.temperature = kfObj["temperature"].toDouble();
            kf.humidity = kfObj["humidity"].toDouble();
            kf.pressure = kfObj["pressure"].toDouble();
            kf.visibility = kfObj["visibility"].toDouble();
            kf.transitionType = kfObj["transitionType"].toString("linear");
            seq.keyframes.append(kf);
        }
        config.sequences.append(seq);
    }
    emit parsingFinished(true, "Lua config loaded");
    return true;
}

bool WeatherConfigParser::saveCspConfig(const WeatherPreset& preset, const QString& configPath)
{
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&file);
    out << "[WEATHER]" << "\n";
    out << "NAME=" << preset.name << "\n";
    out << "TIME=" << QString::number(preset.timeOfDay, 'f', 1) << "\n";
    out << "TIME_MULT=" << QString::number(preset.timeMultiplier, 'f', 2) << "\n";
    out << "TEMP=" << QString::number(preset.ambientTemperature, 'f', 1) << "\n";
    out << "HUMIDITY=" << QString::number(preset.humidity, 'f', 0) << "\n";
    out << "WIND_SPEED=" << QString::number(preset.windSpeed, 'f', 1) << "\n";
    out << "WIND_DIR=" << QString::number(preset.windDirection, 'f', 0) << "\n";
    out << "RAIN=" << QString::number(preset.rainIntensity, 'f', 2) << "\n";
    out << "CLOUDS=" << QString::number(preset.cloudIntensity, 'f', 2) << "\n";
    file.close();
    return true;
}

bool WeatherConfigParser::savePureConfig(const WeatherPreset& preset, const QString& configPath)
{
    // Pure config uses JSON format
    QJsonObject obj;
    obj["name"] = preset.name;
    obj["description"] = preset.description;
    obj["author"] = preset.author;
    obj["timeOfDay"] = preset.timeOfDay;
    obj["timeMultiplier"] = preset.timeMultiplier;
    obj["ambientTemperature"] = preset.ambientTemperature;
    obj["humidity"] = preset.humidity;
    obj["windSpeed"] = preset.windSpeed;
    obj["windDirection"] = preset.windDirection;
    obj["rainIntensity"] = preset.rainIntensity;
    obj["cloudIntensity"] = preset.cloudIntensity;

    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool WeatherConfigParser::saveSolConfig(const WeatherPreset& preset, const QString& configPath)
{
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&file);
    out << "[SOL_WEATHER]" << "\n";
    out << "NAME=" << preset.name << "\n";
    out << "TIME=" << QString::number(preset.timeOfDay, 'f', 1) << "\n";
    out << "TIME_MULTIPLIER=" << QString::number(preset.timeMultiplier, 'f', 3) << "\n";
    out << "TEMPERATURE_AMBIENT=" << QString::number(preset.ambientTemperature, 'f', 1) << "\n";
    out << "HUMIDITY=" << QString::number(preset.humidity, 'f', 0) << "\n";
    out << "WIND_SPEED_MS=" << QString::number(preset.windSpeed, 'f', 1) << "\n";
    out << "WIND_DIRECTION=" << QString::number(preset.windDirection, 'f', 0) << "\n";
    out << "RAIN_INTENSITY=" << QString::number(preset.rainIntensity, 'f', 2) << "\n";
    out << "CLOUD_CLOUDS=" << QString::number(preset.cloudIntensity, 'f', 2) << "\n";
    file.close();
    return true;
}

bool WeatherConfigParser::parseSOLConfig(const QString& filePath, WeatherConfig& config, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = "Cannot open SOL config: " + filePath;
        return false;
    }
    QTextStream in(&file);
    QMap<QString, QString> kv;
    if (!parseIniSection(in, "SOL_WEATHER", kv)) {
        if (error) *error = "Failed to parse SOL config";
        return false;
    }
    config.name = kv.value("NAME", QFileInfo(filePath).baseName());
    config.baseTime = kv.value("TIME", "12").toDouble();
    config.timeMultiplier = kv.value("TIME_MULTIPLIER", "1").toDouble();
    config.weatherChangeInterval = kv.value("CHANGE_INTERVAL", "2").toDouble();
    if (kv.contains("TEMPERATURE_AMBIENT"))
        config.sequences.first().keyframes.first().temperature = kv["TEMPERATURE_AMBIENT"].toDouble();
    if (kv.contains("HUMIDITY"))
        config.sequences.first().keyframes.first().humidity = kv["HUMIDITY"].toDouble() / 100.0;
    if (kv.contains("WIND_SPEED_MS"))
        config.sequences.first().keyframes.first().windSpeed = kv["WIND_SPEED_MS"].toDouble();
    if (kv.contains("WIND_DIRECTION"))
        config.sequences.first().keyframes.first().windDirection = kv["WIND_DIRECTION"].toDouble();
    emit parsingFinished(true, "SOL config loaded");
    return true;
}

bool WeatherConfigParser::writeWeatherConfig(const QString& filePath, const WeatherConfig& config, QString* error)
{
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    if (ext == "lua") {
        return writeWeatherLua(filePath, config, error);
    }
    if (ext == "json") {
        QJsonObject obj;
        obj["name"] = config.name;
        obj["trackName"] = config.trackName;
        obj["baseTime"] = config.baseTime;
        obj["timeMultiplier"] = config.timeMultiplier;
        obj["dynamicWeather"] = config.dynamicWeather;
        obj["weatherChangeInterval"] = config.weatherChangeInterval;
        obj["solConfigPath"] = config.solConfigPath;
        obj["weatherLuaPath"] = config.weatherLuaPath;
        QJsonArray seqArray;
        for (const auto& seq : config.sequences) {
            QJsonObject seqObj;
            seqObj["name"] = seq.name;
            seqObj["description"] = seq.description;
            seqObj["startTime"] = seq.startTime;
            seqObj["duration"] = seq.duration;
            seqObj["loop"] = seq.loop;
            QJsonArray kfArray;
            for (const auto& kf : seq.keyframes) {
                QJsonObject kfObj;
                kfObj["time"] = kf.time;
                kfObj["type"] = kf.type;
                kfObj["cloudCoverage"] = kf.cloudCoverage;
                kfObj["precipitation"] = kf.precipitation;
                kfObj["windSpeed"] = kf.windSpeed;
                kfObj["windDirection"] = kf.windDirection;
                kfObj["temperature"] = kf.temperature;
                kfObj["humidity"] = kf.humidity;
                kfObj["pressure"] = kf.pressure;
                kfObj["visibility"] = kf.visibility;
                kfObj["transitionType"] = kf.transitionType;
                kfArray.append(kfObj);
            }
            seqObj["keyframes"] = kfArray;
            seqArray.append(seqObj);
        }
        obj["sequences"] = seqArray;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            if (error) *error = "Cannot write file: " + filePath;
            return false;
        }
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

    // Default INI format (CSP-style)
    QString ini = generateIniConfig(config);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = "Cannot write file: " + filePath;
        return false;
    }
    QTextStream out(&file);
    out << ini;
    file.close();
    return true;
}

bool WeatherConfigParser::writeWeatherLua(const QString& filePath, const WeatherConfig& config, QString* error)
{
    QString luaConfig = generateLuaConfig(config);
    if (luaConfig.isEmpty()) {
        if (error) *error = "Failed to generate Lua config";
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = "Cannot write file: " + filePath;
        return false;
    }
    QTextStream out(&file);
    out << luaConfig;
    file.close();
    return true;
}

bool WeatherConfigParser::writeSOLConfig(const QString& filePath, const WeatherConfig& config, QString* error)
{
    QString sol = generateSOLConfig(config);
    if (sol.isEmpty()) {
        if (error) *error = "Failed to generate SOL config";
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = "Cannot write file: " + filePath;
        return false;
    }
    QTextStream out(&file);
    out << sol;
    file.close();
    return true;
}

bool WeatherConfigParser::validateConfig(const WeatherConfig& config, QStringList* errors)
{
    bool valid = true;
    if (config.name.isEmpty()) {
        if (errors) errors->append("Weather config name is empty");
        valid = false;
    }
    if (config.baseTime < 0.0 || config.baseTime > 24.0) {
        if (errors) errors->append("Base time must be between 0 and 24 hours");
        valid = false;
    }
    if (config.sequences.isEmpty()) {
        if (errors) errors->append("No weather sequences defined");
        valid = false;
    }
    for (int i = 0; i < config.sequences.size(); ++i) {
        const auto& seq = config.sequences[i];
        if (seq.name.isEmpty()) {
            if (errors) errors->append(QString("Sequence %1 has no name").arg(i));
            valid = false;
        }
        if (seq.keyframes.isEmpty()) {
            if (errors) errors->append(QString("Sequence '%1' has no keyframes").arg(seq.name));
            valid = false;
        }
        for (int j = 0; j < seq.keyframes.size(); ++j) {
            const auto& kf = seq.keyframes[j];
            if (!kf.isValid()) {
                if (errors) errors->append(QString("Invalid keyframe at index %1 in sequence '%2'").arg(j).arg(seq.name));
                valid = false;
            }
        }
    }
    if (errors && errors->isEmpty()) errors->append("Configuration is valid");
    return valid;
}

bool WeatherConfigParser::parseIniSection(QTextStream& in, const QString& sectionName, QMap<QString, QString>& out)
{
    out.clear();
    bool inTargetSection = false;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';') || line.startsWith('#')) continue;

        if (line.startsWith('[')) {
            inTargetSection = line.compare('[' + sectionName + ']', Qt::CaseInsensitive) == 0;
            continue;
        }
        if (!inTargetSection) continue;

        int eqPos = line.indexOf('=');
        if (eqPos < 0) continue;
        QString key = line.left(eqPos).trimmed().toUpper();
        QString val = line.mid(eqPos + 1).trimmed();
        if (!key.isEmpty()) out[key] = val;
    }
    return !out.isEmpty();
}

bool WeatherConfigParser::parseLuaTable(const QString& luaCode, const QString& tableName, QJsonObject& out)
{
    // Simple Lua table parser for config-style tables
    // Matches patterns like: weather = { name = "foo", ... }
    int tableStart = luaCode.indexOf(tableName + " = {");
    if (tableStart < 0) {
        tableStart = luaCode.indexOf(tableName + "={");
        if (tableStart < 0) return false;
    }
    int braceStart = luaCode.indexOf('{', tableStart);
    if (braceStart < 0) return false;

    // Find matching closing brace
    int depth = 1;
    int braceEnd = braceStart + 1;
    while (depth > 0 && braceEnd < luaCode.length()) {
        if (luaCode[braceEnd] == '{') depth++;
        else if (luaCode[braceEnd] == '}') depth--;
        braceEnd++;
    }
    if (depth != 0) return false;

    QString tableBody = luaCode.mid(braceStart + 1, braceEnd - braceStart - 2);

    // Parse key = value pairs
    QStringList lines = tableBody.split('\n');
    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith("--")) continue;

        // Handle nested tables
        if (line.contains('{')) {
            int eqPos = line.indexOf('=');
            if (eqPos < 0) continue;
            QString key = line.left(eqPos).trimmed();
            // Skip nested tables for now
            continue;
        }

        int eqPos = line.indexOf('=');
        if (eqPos < 0) continue;

        QString key = line.left(eqPos).trimmed();
        QString val = line.mid(eqPos + 1).trimmed();

        // Remove trailing comma
        if (val.endsWith(',')) val.chop(1);
        val = val.trimmed();

        // Remove quotes
        if ((val.startsWith('"') && val.endsWith('"')) ||
            (val.startsWith('\'') && val.endsWith('\''))) {
            val = val.mid(1, val.length() - 2);
            out[key] = val;
        } else if (val == "true") {
            out[key] = true;
        } else if (val == "false") {
            out[key] = false;
        } else {
            bool ok;
            double d = val.toDouble(&ok);
            if (ok) out[key] = d;
            else out[key] = val;
        }
    }
    return true;
}

QString WeatherConfigParser::interpolateLuaString(const QString& str, const QJsonObject& variables)
{
    QString result = str;
    for (auto it = variables.begin(); it != variables.end(); ++it) {
        QString pattern = "${" + it.key() + "}";
        QString value = it.value().toVariant().toString();
        result.replace(pattern, value);
    }
    // Also handle Lua-style format patterns
    QRegularExpression re("%([a-zA-Z_][a-zA-Z0-9_]*)%");
    QRegularExpressionMatchIterator matches = re.globalMatch(result);
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString varName = match.captured(1);
        if (variables.contains(varName)) {
            result.replace(match.captured(0), variables[varName].toVariant().toString());
        }
    }
    return result;
}

QString WeatherConfigParser::generateLuaConfig(const WeatherConfig& config)
{
    QString lua;
    QTextStream out(&lua);
    out << "-- Weather configuration generated by kseditor\n";
    out << "-- " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
    out << "weather = {\n";
    out << "    name = \"" << config.name << "\",\n";
    out << "    baseTime = " << QString::number(config.baseTime, 'f', 2) << ",\n";
    out << "    timeMultiplier = " << QString::number(config.timeMultiplier, 'f', 3) << ",\n";
    out << "    dynamicWeather = " << (config.dynamicWeather ? "true" : "false") << ",\n";
    out << "    weatherChangeInterval = " << QString::number(config.weatherChangeInterval, 'f', 2) << ",\n";

    if (!config.sequences.isEmpty()) {
        out << "    sequences = {\n";
        for (const auto& seq : config.sequences) {
            out << "        {\n";
            out << "            name = \"" << seq.name << "\",\n";
            out << "            startTime = " << QString::number(seq.startTime, 'f', 2) << ",\n";
            out << "            duration = " << QString::number(seq.duration, 'f', 2) << ",\n";
            out << "            loop = " << (seq.loop ? "true" : "false") << ",\n";

            if (!seq.keyframes.isEmpty()) {
                out << "            keyframes = {\n";
                for (const auto& kf : seq.keyframes) {
                    out << "                {\n";
                    out << "                    time = " << QString::number(kf.time, 'f', 2) << ",\n";
                    out << "                    type = \"" << kf.type << "\",\n";
                    out << "                    cloudCoverage = " << QString::number(kf.cloudCoverage, 'f', 2) << ",\n";
                    out << "                    precipitation = " << QString::number(kf.precipitation, 'f', 2) << ",\n";
                    out << "                    windSpeed = " << QString::number(kf.windSpeed, 'f', 1) << ",\n";
                    out << "                    windDirection = " << QString::number(kf.windDirection, 'f', 0) << ",\n";
                    out << "                    temperature = " << QString::number(kf.temperature, 'f', 1) << ",\n";
                    out << "                    humidity = " << QString::number(kf.humidity, 'f', 3) << ",\n";
                    out << "                    pressure = " << QString::number(kf.pressure, 'f', 1) << ",\n";
                    out << "                    visibility = " << QString::number(kf.visibility, 'f', 1) << ",\n";
                    out << "                    transitionType = \"" << kf.transitionType << "\",\n";
                    out << "                },\n";
                }
                out << "            },\n";
            }
            out << "        },\n";
        }
        out << "    },\n";
    }
    out << "}\n";
    return lua;
}

QString WeatherConfigParser::generateIniConfig(const WeatherConfig& config)
{
    QString ini;
    QTextStream out(&ini);
    out << "; Weather configuration generated by kseditor\n";
    out << "; " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
    out << "[WEATHER]\n";
    out << "NAME=" << config.name << "\n";
    out << "TIME=" << QString::number(config.baseTime, 'f', 1) << "\n";
    out << "TIME_MULT=" << QString::number(config.timeMultiplier, 'f', 2) << "\n";
    out << "DYNAMIC_WEATHER=" << (config.dynamicWeather ? "1" : "0") << "\n";
    out << "CHANGE_INTERVAL=" << QString::number(config.weatherChangeInterval, 'f', 1) << "\n";
    out << "SOL_CONFIG=" << config.solConfigPath << "\n";
    out << "WEATHER_LUA=" << config.weatherLuaPath << "\n";

    for (int i = 0; i < config.sequences.size(); ++i) {
        const auto& seq = config.sequences[i];
        out << "\n[SEQUENCE_" << i << "]\n";
        out << "NAME=" << seq.name << "\n";
        out << "START=" << QString::number(seq.startTime, 'f', 2) << "\n";
        out << "DURATION=" << QString::number(seq.duration, 'f', 2) << "\n";
        out << "LOOP=" << (seq.loop ? "1" : "0") << "\n";

        for (int j = 0; j < seq.keyframes.size(); ++j) {
            const auto& kf = seq.keyframes[j];
            out << "\n[KEYFRAME_" << i << "_" << j << "]\n";
            out << "TIME=" << QString::number(kf.time, 'f', 2) << "\n";
            out << "TYPE=" << kf.type << "\n";
            out << "CLOUDS=" << QString::number(kf.cloudCoverage, 'f', 2) << "\n";
            out << "RAIN=" << QString::number(kf.precipitation, 'f', 2) << "\n";
            out << "WIND_SPEED=" << QString::number(kf.windSpeed, 'f', 1) << "\n";
            out << "WIND_DIR=" << QString::number(kf.windDirection, 'f', 0) << "\n";
            out << "TEMP=" << QString::number(kf.temperature, 'f', 1) << "\n";
            out << "HUMIDITY=" << QString::number(kf.humidity, 'f', 2) << "\n";
            out << "PRESSURE=" << QString::number(kf.pressure, 'f', 0) << "\n";
            out << "VISIBILITY=" << QString::number(kf.visibility, 'f', 1) << "\n";
            out << "TRANSITION=" << kf.transitionType << "\n";
        }
    }
    return ini;
}

QString WeatherConfigParser::generateSOLConfig(const WeatherConfig& config)
{
    QString sol;
    QTextStream out(&sol);
    out << "; SOL weather config generated by kseditor\n";
    out << "; " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
    out << "[SOL_WEATHER]\n";
    out << "NAME=" << config.name << "\n";
    out << "TIME=" << QString::number(config.baseTime, 'f', 1) << "\n";
    out << "TIME_MULTIPLIER=" << QString::number(config.timeMultiplier, 'f', 3) << "\n";
    out << "DYNAMIC_WEATHER=" << (config.dynamicWeather ? "1" : "0") << "\n";

    if (!config.sequences.isEmpty() && !config.sequences.first().keyframes.isEmpty()) {
        const auto& kf = config.sequences.first().keyframes.first();
        out << "TEMPERATURE_AMBIENT=" << QString::number(kf.temperature, 'f', 1) << "\n";
        out << "HUMIDITY=" << QString::number(kf.humidity * 100.0, 'f', 0) << "\n";
        out << "WIND_SPEED_MS=" << QString::number(kf.windSpeed, 'f', 1) << "\n";
        out << "WIND_DIRECTION=" << QString::number(kf.windDirection, 'f', 0) << "\n";
        out << "RAIN_INTENSITY=" << QString::number(kf.precipitation, 'f', 2) << "\n";
        out << "CLOUD_CLOUDS=" << QString::number(kf.cloudCoverage, 'f', 2) << "\n";
    }
    return sol;
}

} // namespace weather
} // namespace ks