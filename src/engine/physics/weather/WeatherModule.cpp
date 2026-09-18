#include "WeatherModule.h"
#include "../WeatherConfig.h"
#include "WeatherEditor.h"

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {
namespace physics {

// ============================================================================
// Constructor/Destructor
// ============================================================================

WeatherModule::WeatherModule(QWidget* parent)
    : EditorModule(parent) {
}

WeatherModule::~WeatherModule() {
    shutdown();
}

// ============================================================================
// EditorModule Interface
// ============================================================================

bool WeatherModule::initialize() {
    if (m_initialized) return true;
    
    setupUI();
    setupConnections();
    updateUI();
    
    m_initialized = true;
    return true;
}

void WeatherModule::shutdown() {
    if (!m_initialized) return;
    
    m_initialized = false;
}

QDockWidget* WeatherModule::getOrCreateDockWidget(QMainWindow* mainWindow) {
    if (m_dockWidget) return m_dockWidget;
    
    m_dockWidget = new QDockWidget(tr("Weather Editor"), mainWindow);
    m_dockWidget->setObjectName("WeatherModuleDock");
    m_dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    m_centralWidget = new QWidget(m_dockWidget);
    QVBoxLayout* layout = new QVBoxLayout(m_centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_splitter = new QSplitter(Qt::Vertical, m_centralWidget);
    
    // Sequence tree
    QWidget* sequenceWidget = new QWidget();
    QVBoxLayout* sequenceLayout = new QVBoxLayout(sequenceWidget);
    sequenceLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* sequenceLabel = new QLabel(tr("Sequences:"));
    sequenceLayout->addWidget(sequenceLabel);
    
    m_sequenceTree = new QTreeWidget();
    m_sequenceTree->setHeaderLabel(tr("Sequences"));
    m_sequenceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    sequenceLayout->addWidget(m_sequenceTree);
    
    m_splitter->addWidget(sequenceWidget);
    
    // Keyframe tree
    QWidget* keyframeWidget = new QWidget();
    QVBoxLayout* keyframeLayout = new QVBoxLayout(keyframeWidget);
    keyframeLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* keyframeLabel = new QLabel(tr("Keyframes:"));
    keyframeLayout->addWidget(keyframeLabel);
    
    m_keyframeTree = new QTreeWidget();
    m_keyframeTree->setHeaderLabel(tr("Keyframes"));
    m_keyframeTree->setSelectionMode(QAbstractItemView::SingleSelection);
    keyframeLayout->addWidget(m_keyframeTree);
    
    m_splitter->addWidget(keyframeWidget);
    
    // Editor
    m_editor = new WeatherEditor(m_centralWidget);
    m_splitter->addWidget(m_editor);
    
    // Set splitter sizes
    m_splitter->setSizes({100, 100, 300});
    
    layout->addWidget(m_splitter);
    
    m_dockWidget->setWidget(m_centralWidget);
    
    mainWindow->addDockWidget(Qt::LeftDockWidgetArea, m_dockWidget);
    
    return m_dockWidget;
}

void WeatherModule::importFile(const QString& filePath) {
    loadWeatherFile(filePath);
}

void WeatherModule::exportFile(const QString& filePath) {
    saveWeatherFile(filePath);
}

QJsonObject WeatherModule::serializeProject() const {
    QJsonObject project;
    project["weather"] = m_config.toJson();
    return project;
}

void WeatherModule::deserializeProject(const QJsonObject& project) {
    if (project.contains("weather")) {
        m_config = WeatherConfig::fromJson(project["weather"].toObject());
        updateUI();
    }
}

// ============================================================================
// Public Slots
// ============================================================================

void WeatherModule::loadWeatherFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        QMessageBox::warning(this, tr("Error"),
            tr("File not found: %1").arg(filePath));
        return;
    }
    
    // Try to load as JSON
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot open file: %1").arg(filePath));
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("Error"),
            tr("JSON parse error: %1").arg(parseError.errorString()));
        return;
    }
    
    m_config = WeatherConfig::fromJson(doc.object());
    updateUI();
    
    emit configChanged();
}

void WeatherModule::saveWeatherFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot write to file: %1").arg(filePath));
        return;
    }
    
    QJsonObject json = m_config.toJson();
    QJsonDocument doc(json);
    
    file.write(doc.toJson());
    file.close();
}

void WeatherModule::loadWeatherPreset(const QString& presetName) {
    // Try to load preset from standard locations
    QStringList searchPaths = {
        ":/weather/presets",
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/kseditor/weather/presets",
        "weather/presets"
    };
    
    for (const QString& path : searchPaths) {
        QString presetPath = path + "/" + presetName + ".json";
        if (QFileInfo::exists(presetPath)) {
            loadWeatherFile(presetPath);
            return;
        }
    }
    
    QMessageBox::information(this, tr("Preset Not Found"),
        tr("Could not find preset: %1").arg(presetName));
}

// ============================================================================
// Private Slots
// ============================================================================

void WeatherModule::onConfigChanged() {
    updateUI();
    emit configChanged();
}

void WeatherModule::onSequenceSelectionChanged() {
    QList<QTreeWidgetItem*> selected = m_sequenceTree->selectedItems();
    if (selected.isEmpty()) return;
    
    QString sequenceName = selected.first()->text(0);
    updateKeyframeTree(sequenceName);
}

void WeatherModule::onKeyframeSelectionChanged() {
    // Handle keyframe selection
}

void WeatherModule::onAddSequence() {
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Sequence"),
        tr("Sequence name:"), QLineEdit::Normal, QString(), &ok);
    
    if (ok && !name.isEmpty()) {
        m_config.addSequence(name);
        updateSequenceTree();
        emit configChanged();
    }
}

void WeatherModule::onRemoveSequence() {
    QList<QTreeWidgetItem*> selected = m_sequenceTree->selectedItems();
    if (selected.isEmpty()) return;
    
    QString sequenceName = selected.first()->text(0);
    
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        tr("Remove Sequence"),
        tr("Are you sure you want to remove sequence '%1'?").arg(sequenceName),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_config.removeSequence(sequenceName);
        updateSequenceTree();
        emit configChanged();
    }
}

void WeatherModule::onAddKeyframe() {
    QList<QTreeWidgetItem*> selected = m_sequenceTree->selectedItems();
    if (selected.isEmpty()) return;
    
    QString sequenceName = selected.first()->text(0);
    
    bool ok;
    double time = QInputDialog::getDouble(this, tr("Add Keyframe"),
        tr("Time (seconds):"), 0.0, 0.0, 3600.0, 2, &ok);
    
    if (ok) {
        m_config.addKeyframe(sequenceName, time);
        updateKeyframeTree(sequenceName);
        emit configChanged();
    }
}

void WeatherModule::onRemoveKeyframe() {
    QList<QTreeWidgetItem*> sequenceSelected = m_sequenceTree->selectedItems();
    QList<QTreeWidgetItem*> keyframeSelected = m_keyframeTree->selectedItems();
    
    if (sequenceSelected.isEmpty() || keyframeSelected.isEmpty()) return;
    
    QString sequenceName = sequenceSelected.first()->text(0);
    double time = keyframeSelected.first()->text(0).toDouble();
    
    m_config.removeKeyframe(sequenceName, time);
    updateKeyframeTree(sequenceName);
    emit configChanged();
}

// ============================================================================
// Private Methods
// ============================================================================

void WeatherModule::setupUI() {
    // UI setup is done in getOrCreateDockWidget
}

void WeatherModule::setupMenuBar() {
    // Menu bar setup would be done by the main editor
}

void WeatherModule::setupToolBar() {
    // Toolbar setup would be done by the main editor
}

void WeatherModule::setupConnections() {
    if (m_editor) {
        connect(m_editor, &WeatherEditor::configChanged,
                this, &WeatherModule::onConfigChanged);
    }
    
    if (m_sequenceTree) {
        connect(m_sequenceTree, &QTreeWidget::itemSelectionChanged,
                this, &WeatherModule::onSequenceSelectionChanged);
    }
    
    if (m_keyframeTree) {
        connect(m_keyframeTree, &QTreeWidget::itemSelectionChanged,
                this, &WeatherModule::onKeyframeSelectionChanged);
    }
}

void WeatherModule::updateUI() {
    if (m_editor) {
        m_editor->setWeatherConfig(m_config);
    }
    
    updateSequenceTree();
}

void WeatherModule::updateSequenceTree() {
    if (!m_sequenceTree) return;
    
    m_sequenceTree->clear();
    
    for (const QString& sequenceName : m_config.sequenceNames()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_sequenceTree);
        item->setText(0, sequenceName);
        item->setData(0, Qt::UserRole, sequenceName);
    }
}

void WeatherModule::updateKeyframeTree(const QString& sequenceName) {
    if (!m_keyframeTree) return;
    
    m_keyframeTree->clear();
    
    QVector<double> keyframes = m_config.keyframes(sequenceName);
    
    for (double time : keyframes) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_keyframeTree);
        item->setText(0, QString::number(time, 'f', 2));
        item->setData(0, Qt::UserRole, time);
    }
}

} // namespace physics
} // namespace ks