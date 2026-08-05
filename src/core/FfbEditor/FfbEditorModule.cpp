#include "FfbEditorModule.h"
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
#include <QFileInfo>
#include <QStandardPaths>
#include <QProgressBar>
#include <QSettings>
#include <QApplication>
#include <QThread>

namespace ks {
namespace ffb {

FfbEditorModule::FfbEditorModule(QWidget* parent)
    : ModuleGuiBase(parent)
    , m_tabWidget(nullptr)
    , m_deviceTab(nullptr)
    , m_deviceCombo(nullptr)
    , m_deviceInfoLabel(nullptr)
    , m_testBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_testProgress(nullptr)
    , m_effectsTab(nullptr)
    , m_effectTypeCombo(nullptr)
    , m_gainSpin(nullptr)
    , m_springSpin(nullptr)
    , m_damperSpin(nullptr)
    , m_frictionSpin(nullptr)
    , m_inertiaSpin(nullptr)
    , m_springSlider(nullptr)
    , m_damperSlider(nullptr)
    , m_frictionSlider(nullptr)
    , m_profilesTab(nullptr)
    , m_profileTable(nullptr)
    , m_loadProfileBtn(nullptr)
    , m_saveProfileBtn(nullptr)
    , m_resetDefaultsBtn(nullptr)
{
    setObjectName("FfbEditorModule");
}

bool FfbEditorModule::initialize() {
    if (m_uiBuilt) return true;
    ModuleGuiBase::initialize();
    return true;
}

void FfbEditorModule::shutdown() {
    m_uiBuilt = false;
}

void FfbEditorModule::importFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    if (fi.suffix().toLower() == "ffbprofile") {
        QSettings ps(filePath, QSettings::IniFormat);
        ps.beginGroup("FFB");
        m_gainSpin->setValue(ps.value("Gain", 75.0).toDouble());
        m_springSpin->setValue(ps.value("Spring", 50.0).toDouble());
        m_damperSpin->setValue(ps.value("Damper", 50.0).toDouble());
        m_frictionSpin->setValue(ps.value("Friction", 30.0).toDouble());
        m_inertiaSpin->setValue(ps.value("Inertia", 20.0).toDouble());
        m_springSlider->setValue(static_cast<int>(m_springSpin->value()));
        m_damperSlider->setValue(static_cast<int>(m_damperSpin->value()));
        m_frictionSlider->setValue(static_cast<int>(m_frictionSpin->value()));
        int devIdx = m_deviceCombo->findText(ps.value("Device", "").toString());
        if (devIdx >= 0) m_deviceCombo->setCurrentIndex(devIdx);
        ps.endGroup();
        logSuccess(QString("FFB profile loaded: %1").arg(filePath));
    } else {
        logError(QString("Unsupported FFB profile format: %1").arg(filePath));
    }
}

void FfbEditorModule::exportFile(const QString& filePath) {
    if (filePath.isEmpty()) return;
    QSettings ps(filePath, QSettings::IniFormat);
    ps.beginGroup("FFB");
    ps.setValue("Gain", m_gainSpin->value());
    ps.setValue("Spring", m_springSpin->value());
    ps.setValue("Damper", m_damperSpin->value());
    ps.setValue("Friction", m_frictionSpin->value());
    ps.setValue("Inertia", m_inertiaSpin->value());
    ps.setValue("Device", m_deviceCombo->currentText());
    ps.endGroup();
    ps.sync();
    logSuccess(QString("FFB profile saved: %1").arg(filePath));
}

void FfbEditorModule::onActivation() {}
void FfbEditorModule::onDeactivation() {}

void FfbEditorModule::buildUI() {
    m_tabWidget = new QTabWidget();

    setupDeviceTab();
    setupEffectsTab();
    setupProfilesTab();

    m_mainLayout->addWidget(m_tabWidget);
    m_mainLayout->addWidget(m_logOutput);
}

void FfbEditorModule::setupDeviceTab() {
    m_deviceTab = new QWidget();
    auto* layout = new QVBoxLayout(m_deviceTab);

    auto* deviceGroup = createGroupBox("Wheel Device");
    auto* deviceLayout = new QFormLayout(deviceGroup);
    m_deviceCombo = createComboBox({"No Device Connected", "Logitech G29", "Logitech G923", "Fanatec CSL DD", "Fanatec DD1/DD2", "Thrustmaster T300", "Thrustmaster TS-PC", "Moza R9", "Moza R16", "Simucube 2 Pro"});
    deviceLayout->addRow("Device:", m_deviceCombo);
    m_deviceInfoLabel = createLabel("Select a device to view information");
    deviceLayout->addRow("Info:", m_deviceInfoLabel);
    layout->addWidget(deviceGroup);

    auto* testGroup = createGroupBox("Test");
    auto* testLayout = new QVBoxLayout(testGroup);
    auto* testBtnLayout = new QHBoxLayout();
    m_testBtn = createButton("Test Force Feedback");
    m_stopBtn = createButton("Stop");
    testBtnLayout->addWidget(m_testBtn);
    testBtnLayout->addWidget(m_stopBtn);
    testBtnLayout->addStretch();
    testLayout->addLayout(testBtnLayout);
    m_testProgress = new QProgressBar();
    m_testProgress->setVisible(false);
    testLayout->addWidget(m_testProgress);
    layout->addWidget(testGroup);

    layout->addStretch();

    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FfbEditorModule::onDeviceSelected);
    connect(m_testBtn, &QPushButton::clicked, this, &FfbEditorModule::onTestFFB);
    connect(m_stopBtn, &QPushButton::clicked, this, &FfbEditorModule::onStopFFB);

    populateDeviceList();
    m_tabWidget->addTab(m_deviceTab, "Device");
}

void FfbEditorModule::setupEffectsTab() {
    m_effectsTab = new QWidget();
    auto* layout = new QVBoxLayout(m_effectsTab);

    auto* effectGroup = createGroupBox("Effect Parameters");
    auto* effectLayout = new QFormLayout(effectGroup);
    m_effectTypeCombo = createComboBox({"Constant", "Periodic (Sine)", "Periodic (Square)", "Periodic (Sawtooth)", "Spring", "Damper", "Friction", "Inertia"});
    effectLayout->addRow("Effect Type:", m_effectTypeCombo);

    m_gainSpin = createDoubleSpinBox(0.0, 100.0, 75.0, 1, "%");
    effectLayout->addRow("Overall Gain:", m_gainSpin);

    m_springSpin = createDoubleSpinBox(0.0, 100.0, 50.0, 1, "%");
    m_springSlider = new QSlider(Qt::Horizontal);
    m_springSlider->setRange(0, 100);
    m_springSlider->setValue(50);
    effectLayout->addRow("Spring Effect:", m_springSpin);
    effectLayout->addRow("", m_springSlider);

    m_damperSpin = createDoubleSpinBox(0.0, 100.0, 50.0, 1, "%");
    m_damperSlider = new QSlider(Qt::Horizontal);
    m_damperSlider->setRange(0, 100);
    m_damperSlider->setValue(50);
    effectLayout->addRow("Damper Effect:", m_damperSpin);
    effectLayout->addRow("", m_damperSlider);

    m_frictionSpin = createDoubleSpinBox(0.0, 100.0, 30.0, 1, "%");
    m_frictionSlider = new QSlider(Qt::Horizontal);
    m_frictionSlider->setRange(0, 100);
    m_frictionSlider->setValue(30);
    effectLayout->addRow("Friction:", m_frictionSpin);
    effectLayout->addRow("", m_frictionSlider);

    m_inertiaSpin = createDoubleSpinBox(0.0, 100.0, 20.0, 1, "%");
    effectLayout->addRow("Inertia:", m_inertiaSpin);
    layout->addWidget(effectGroup);

    layout->addStretch();

    connect(m_effectTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FfbEditorModule::onEffectTypeChanged);
    connect(m_gainSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FfbEditorModule::onGainChanged);
    connect(m_springSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FfbEditorModule::onSpringStrengthChanged);
    connect(m_springSlider, &QSlider::valueChanged, this, [this](int val) { m_springSpin->setValue(val); });
    connect(m_damperSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FfbEditorModule::onDamperStrengthChanged);
    connect(m_damperSlider, &QSlider::valueChanged, this, [this](int val) { m_damperSpin->setValue(val); });
    connect(m_frictionSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FfbEditorModule::onFrictionChanged);
    connect(m_frictionSlider, &QSlider::valueChanged, this, [this](int val) { m_frictionSpin->setValue(val); });
    connect(m_inertiaSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FfbEditorModule::onInertiaChanged);

    m_tabWidget->addTab(m_effectsTab, "Effects");
}

void FfbEditorModule::setupProfilesTab() {
    m_profilesTab = new QWidget();
    auto* layout = new QVBoxLayout(m_profilesTab);

    auto* btnLayout = new QHBoxLayout();
    m_loadProfileBtn = createButton("Load Profile");
    m_saveProfileBtn = createButton("Save Profile");
    m_resetDefaultsBtn = createButton("Reset to Defaults");
    btnLayout->addWidget(m_loadProfileBtn);
    btnLayout->addWidget(m_saveProfileBtn);
    btnLayout->addWidget(m_resetDefaultsBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_profileTable = new QTableWidget(0, 3);
    m_profileTable->setHorizontalHeaderLabels({"Profile Name", "Device", "Last Modified"});
    m_profileTable->horizontalHeader()->setStretchLastSection(true);
    m_profileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_profileTable->setAlternatingRowColors(true);
    layout->addWidget(m_profileTable);

    layout->addStretch();

    connect(m_loadProfileBtn, &QPushButton::clicked, this, &FfbEditorModule::onLoadProfile);
    connect(m_saveProfileBtn, &QPushButton::clicked, this, &FfbEditorModule::onSaveProfile);
    connect(m_resetDefaultsBtn, &QPushButton::clicked, this, &FfbEditorModule::onResetDefaults);

    m_tabWidget->addTab(m_profilesTab, "Profiles");
}

void FfbEditorModule::populateDeviceList() {
    m_deviceCombo->setCurrentIndex(0);
}

void FfbEditorModule::onDeviceSelected(int index) {
    if (index > 0) {
        QString device = m_deviceCombo->currentText();
        log(QString("Selected device: %1").arg(device));
        m_deviceInfoLabel->setText(QString("Connected to %1").arg(device));
        QSettings s; s.setValue("FFBEditor/Device", device);
    } else {
        m_deviceInfoLabel->setText("No device selected");
    }
}

void FfbEditorModule::onEffectTypeChanged(int index) {
    QSettings s; s.setValue("FFBEditor/EffectType", m_effectTypeCombo->currentText());
}

void FfbEditorModule::onGainChanged(double value) {
    QSettings s; s.setValue("FFBEditor/Gain", value);
    log(QString("Overall gain set to: %1%").arg(value, 0, 'f', 1));
}

void FfbEditorModule::onSpringStrengthChanged(double value) {
    QSettings s; s.setValue("FFBEditor/Spring", value);
    log(QString("Spring strength set to: %1%").arg(value, 0, 'f', 1));
}

void FfbEditorModule::onDamperStrengthChanged(double value) {
    QSettings s; s.setValue("FFBEditor/Damper", value);
    log(QString("Damper strength set to: %1%").arg(value, 0, 'f', 1));
}

void FfbEditorModule::onFrictionChanged(double value) {
    QSettings s; s.setValue("FFBEditor/Friction", value);
    log(QString("Friction set to: %1%").arg(value, 0, 'f', 1));
}

void FfbEditorModule::onInertiaChanged(double value) {
    QSettings s; s.setValue("FFBEditor/Inertia", value);
    log(QString("Inertia set to: %1%").arg(value, 0, 'f', 1));
}

void FfbEditorModule::onTestFFB() {
    log("Testing force feedback...");
    m_testProgress->setVisible(true);
    m_testProgress->setRange(0, 100);
    m_testProgress->setValue(0);
    // Simulate a sweep test
    for (int i = 0; i <= 100; i += 10) {
        m_testProgress->setValue(i);
        QApplication::processEvents();
        QThread::msleep(50);
    }
    m_testProgress->setVisible(false);
    logSuccess("Force feedback test completed");
}

void FfbEditorModule::onStopFFB() {
    log("Force feedback test stopped");
    m_testProgress->setVisible(false);
}

void FfbEditorModule::onLoadProfile() {
    QString path = selectFile("Load FFB Profile", "FFB Profiles (*.ffbprofile);;All Files (*)");
    if (!path.isEmpty()) {
        importFile(path);
        // Add to profile table
        int row = m_profileTable->rowCount();
        m_profileTable->insertRow(row);
        m_profileTable->setItem(row, 0, new QTableWidgetItem(QFileInfo(path).baseName()));
        m_profileTable->setItem(row, 1, new QTableWidgetItem(m_deviceCombo->currentText()));
        m_profileTable->setItem(row, 2, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")));
    }
}

void FfbEditorModule::onSaveProfile() {
    QString path = selectFile("Save FFB Profile", "FFB Profiles (*.ffbprofile)");
    if (!path.isEmpty()) {
        exportFile(path);
        int row = m_profileTable->rowCount();
        m_profileTable->insertRow(row);
        m_profileTable->setItem(row, 0, new QTableWidgetItem(QFileInfo(path).baseName()));
        m_profileTable->setItem(row, 1, new QTableWidgetItem(m_deviceCombo->currentText()));
        m_profileTable->setItem(row, 2, new QTableWidgetItem(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm")));
    }
}

void FfbEditorModule::onResetDefaults() {
    if (confirmAction("Reset Settings", "Reset all FFB settings to defaults?")) {
        m_gainSpin->setValue(75.0);
        m_springSpin->setValue(50.0);
        m_springSlider->setValue(50);
        m_damperSpin->setValue(50.0);
        m_damperSlider->setValue(50);
        m_frictionSpin->setValue(30.0);
        m_frictionSlider->setValue(30);
        m_inertiaSpin->setValue(20.0);
        QSettings s; s.remove("FFBEditor");
        logSuccess("FFB settings reset to defaults");
    }
}

} // namespace ffb
} // namespace ks

