#include "ServerConfigEditorModule.h"
#include "../sys/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>

namespace ks {

ServerConfigEditorModule::ServerConfigEditorModule(QWidget* parent)
    : EditorModule(parent) {}

bool ServerConfigEditorModule::initialize()
{
    LOG_INFO("ServerConfigEditorModule", "Initialized");
    return true;
}

void ServerConfigEditorModule::shutdown()
{
    m_entries.clear();
    m_serverPath.clear();
}
QDockWidget* ServerConfigEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;
    m_dockWidget = new QDockWidget(tr("Server Config Editor"), mainWindow);
    m_dockWidget->setObjectName("ServerConfigEditorDock");

    auto* wrapper = new QWidget();
    auto* mainLayout = new QVBoxLayout(wrapper);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    m_tabWidget = new QTabWidget();

    auto* settingsTab = new QWidget();
    auto* settingsScroll = new QScrollArea();
    settingsScroll->setWidgetResizable(true);
    auto* settingsContent = new QWidget();
    auto* settingsLayout = new QVBoxLayout(settingsContent);

    auto* basicGb = new QGroupBox(tr("Basic"));
    auto* basicL = new QGridLayout(basicGb);
    m_nameEdit = new QLineEdit();
    m_descEdit = new QLineEdit();
    m_passwordEdit = new QLineEdit(); m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_adminPwEdit = new QLineEdit(); m_adminPwEdit->setEchoMode(QLineEdit::Password);
    int r = 0;
    basicL->addWidget(new QLabel(tr("Name:")), r, 0); basicL->addWidget(m_nameEdit, r++, 1);
    basicL->addWidget(new QLabel(tr("Description:")), r, 0); basicL->addWidget(m_descEdit, r++, 1);
    basicL->addWidget(new QLabel(tr("Password:")), r, 0); basicL->addWidget(m_passwordEdit, r++, 1);
    basicL->addWidget(new QLabel(tr("Admin Password:")), r, 0); basicL->addWidget(m_adminPwEdit, r++, 1);
    settingsLayout->addWidget(basicGb);

    auto* netGb = new QGroupBox(tr("Network"));
    auto* netL = new QGridLayout(netGb);
    m_ipEdit = new QLineEdit(); m_ipEdit->setPlaceholderText("0.0.0.0");
    m_portSpin = new QSpinBox(); m_portSpin->setRange(1024, 65535);
    m_httpPortSpin = new QSpinBox(); m_httpPortSpin->setRange(1024, 65535);
    m_maxClientsSpin = new QSpinBox(); m_maxClientsSpin->setRange(1, 100);
    m_slotCountSpin = new QSpinBox(); m_slotCountSpin->setRange(1, 100);
    r = 0;
    netL->addWidget(new QLabel(tr("IP:")), r, 0); netL->addWidget(m_ipEdit, r++, 1);
    netL->addWidget(new QLabel(tr("Port:")), r, 0); netL->addWidget(m_portSpin, r++, 1);
    netL->addWidget(new QLabel(tr("HTTP Port:")), r, 0); netL->addWidget(m_httpPortSpin, r++, 1);
    netL->addWidget(new QLabel(tr("Max Clients:")), r, 0); netL->addWidget(m_maxClientsSpin, r++, 1);
    netL->addWidget(new QLabel(tr("Slot Count:")), r, 0); netL->addWidget(m_slotCountSpin, r++, 1);
    settingsLayout->addWidget(netGb);

    auto* sessGb = new QGroupBox(tr("Session"));
    auto* sessL = new QGridLayout(sessGb);
    m_sessionTypeCombo = new QComboBox();
    m_sessionTypeCombo->addItems({tr("Practice"), tr("Qualifying"), tr("Race")});
    m_sessionDurationSpin = new QSpinBox(); m_sessionDurationSpin->setRange(0, 86400);
    m_sessionDurationSpin->setSuffix(tr(" sec"));
    m_lapsCountSpin = new QSpinBox(); m_lapsCountSpin->setRange(0, 999);
    m_waitTimeSpin = new QSpinBox(); m_waitTimeSpin->setRange(0, 300);
    m_waitTimeSpin->setSuffix(tr(" sec"));
    r = 0;
    sessL->addWidget(new QLabel(tr("Type:")), r, 0); sessL->addWidget(m_sessionTypeCombo, r++, 1);
    sessL->addWidget(new QLabel(tr("Duration:")), r, 0); sessL->addWidget(m_sessionDurationSpin, r++, 1);
    sessL->addWidget(new QLabel(tr("Laps:")), r, 0); sessL->addWidget(m_lapsCountSpin, r++, 1);
    sessL->addWidget(new QLabel(tr("Wait Time:")), r, 0); sessL->addWidget(m_waitTimeSpin, r++, 1);
    settingsLayout->addWidget(sessGb);
    auto* wthGb = new QGroupBox(tr("Weather"));
    auto* wthL = new QGridLayout(wthGb);
    m_weatherCombo = new QComboBox();
    m_weatherCombo->addItems({"clear_01", "cloudy_01", "light_rain_01", "heavy_rain_01", "storm_01", "fog_01"});
    m_timeOfDaySpin = new QDoubleSpinBox(); m_timeOfDaySpin->setRange(0, 24);
    m_useRealWeatherCheck = new QCheckBox(tr("Use Real Weather"));
    m_timeMultSpin = new QSpinBox(); m_timeMultSpin->setRange(1, 24);
    r = 0;
    wthL->addWidget(new QLabel(tr("Weather:")), r, 0); wthL->addWidget(m_weatherCombo, r++, 1);
    wthL->addWidget(new QLabel(tr("Time of Day:")), r, 0); wthL->addWidget(m_timeOfDaySpin, r++, 1);
    wthL->addWidget(m_useRealWeatherCheck, r++, 0, 1, 2);
    wthL->addWidget(new QLabel(tr("Time Mult:")), r, 0); wthL->addWidget(m_timeMultSpin, r++, 1);
    settingsLayout->addWidget(wthGb);
    settingsLayout->addStretch();
    settingsScroll->setWidget(settingsContent);
    auto* settingsTabLayout = new QVBoxLayout(settingsTab);
    settingsTabLayout->setContentsMargins(0, 0, 0, 0);
    settingsTabLayout->addWidget(settingsScroll);
    m_tabWidget->addTab(settingsTab, tr("Server Settings"));

    auto* entryTab = new QWidget();
    auto* entryLayout = new QVBoxLayout(entryTab);
    m_entryTable = new QTableWidget(0, 6);
    m_entryTable->setHorizontalHeaderLabels({tr("Name"), tr("Team"), tr("GUID"), tr("Car Model"), tr("Skin"), tr("Ballast")});
    m_entryTable->horizontalHeader()->setStretchLastSection(true);
    m_entryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_entryTable->setAlternatingRowColors(true);
    entryLayout->addWidget(m_entryTable);
    auto* entryBtns = new QHBoxLayout();
    m_addEntryBtn = new QPushButton(tr("Add Entry"));
    m_removeEntryBtn = new QPushButton(tr("Remove Entry"));
    entryBtns->addWidget(m_addEntryBtn);
    entryBtns->addWidget(m_removeEntryBtn);
    entryBtns->addStretch();
    entryLayout->addLayout(entryBtns);
    m_tabWidget->addTab(entryTab, tr("Entry List"));

    mainLayout->addWidget(m_tabWidget);

    auto* actionLayout = new QHBoxLayout();
    m_loadBtn = new QPushButton(tr("Load Config"));
    m_saveBtn = new QPushButton(tr("Save Config"));
    m_presetCombo = new QComboBox();
    m_presetCombo->addItems({tr("Sprint"), tr("Endurance"), tr("Drift"), tr("Cruise")});
    m_applyPresetBtn = new QPushButton(tr("Apply Preset"));
    actionLayout->addWidget(m_loadBtn);
    actionLayout->addWidget(m_saveBtn);
    actionLayout->addStretch();
    actionLayout->addWidget(m_presetCombo);
    actionLayout->addWidget(m_applyPresetBtn);
    mainLayout->addLayout(actionLayout);

    m_statusLabel = new QLabel(tr("Ready"));
    m_statusLabel->setStyleSheet("color: #aaa; padding: 4px;");
    mainLayout->addWidget(m_statusLabel);

    m_dockWidget->setWidget(wrapper);

    connect(m_loadBtn, &QPushButton::clicked, this, &ServerConfigEditorModule::onLoadConfig);
    connect(m_saveBtn, &QPushButton::clicked, this, &ServerConfigEditorModule::onSaveConfig);
    connect(m_applyPresetBtn, &QPushButton::clicked, this, &ServerConfigEditorModule::onApplyPreset);
    connect(m_addEntryBtn, &QPushButton::clicked, this, &ServerConfigEditorModule::onAddEntry);
    connect(m_removeEntryBtn, &QPushButton::clicked, this, &ServerConfigEditorModule::onRemoveEntry);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &ServerConfigEditorModule::onServerPropChanged);
    connect(m_portSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigEditorModule::onServerPropChanged);
    connect(m_sessionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ServerConfigEditorModule::onServerPropChanged);

    return m_dockWidget;
}
void ServerConfigEditorModule::onLoadConfig()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select Server Directory"));
    if (path.isEmpty()) return;
    m_serverPath = path;
    m_config = ServerConfigEditor::loadConfig(path);
    m_entries = ServerConfigEditor::loadEntryList(path);
    loadConfigToUI();
    populateEntryTable();
    m_statusLabel->setText(tr("Loaded: %1").arg(QFileInfo(path).fileName()));
}

void ServerConfigEditorModule::onSaveConfig()
{
    if (m_serverPath.isEmpty()) {
        QString path = QFileDialog::getExistingDirectory(this, tr("Select Server Directory"));
        if (path.isEmpty()) return;
        m_serverPath = path;
    }
    saveConfigFromUI();
    bool ok = ServerConfigEditor::saveConfig(m_config, m_serverPath);
    ok &= ServerConfigEditor::saveEntryList(m_entries, m_serverPath);
    m_statusLabel->setText(ok ? tr("Saved: %1").arg(QFileInfo(m_serverPath).fileName()) : tr("Save failed!"));
}

void ServerConfigEditorModule::onApplyPreset()
{
    int idx = m_presetCombo->currentIndex();
    switch (idx) {
    case 0: m_config = ServerConfigEditor::getPresetSprint(); break;
    case 1: m_config = ServerConfigEditor::getPresetEndurance(); break;
    case 2: m_config = ServerConfigEditor::getPresetDrift(); break;
    case 3: m_config = ServerConfigEditor::getPresetCruise(); break;
    }
    loadConfigToUI();
    m_statusLabel->setText(tr("Applied preset: %1").arg(m_presetCombo->currentText()));
}

void ServerConfigEditorModule::onAddEntry()
{
    ServerConfigEditor::EntryInfo entry;
    entry.name = tr("New Driver");
    entry.guid = "GUID_" + QString::number(m_entries.size() + 1);
    entry.carModel = "abarth500";
    m_entries.append(entry);
    populateEntryTable();
    m_statusLabel->setText(tr("Added entry"));
}

void ServerConfigEditorModule::onRemoveEntry()
{
    int row = m_entryTable->currentRow();
    if (row < 0 || row >= m_entries.size()) return;
    m_entries.removeAt(row);
    populateEntryTable();
    m_statusLabel->setText(tr("Removed entry"));
}

void ServerConfigEditorModule::onServerPropChanged()
{
    if (m_updating) return;
    saveConfigFromUI();
    m_statusLabel->setText(tr("Modified"));
}
void ServerConfigEditorModule::loadConfigToUI()
{
    m_updating = true;
    m_nameEdit->setText(m_config.name);
    m_descEdit->setText(m_config.description);
    m_passwordEdit->setText(m_config.password);
    m_adminPwEdit->setText(m_config.adminPassword);
    m_ipEdit->setText(m_config.ip);
    m_portSpin->setValue(m_config.port);
    m_httpPortSpin->setValue(m_config.httpPort);
    m_maxClientsSpin->setValue(m_config.maxClients);
    m_slotCountSpin->setValue(m_config.slotCount);
    m_sessionTypeCombo->setCurrentIndex(qBound(0, m_config.sessionType, 2));
    m_sessionDurationSpin->setValue(m_config.sessionDuration);
    m_lapsCountSpin->setValue(m_config.lapsCount);
    m_waitTimeSpin->setValue(m_config.waitTime);
    int weatherIdx = m_weatherCombo->findText(m_config.weather);
    if (weatherIdx >= 0) m_weatherCombo->setCurrentIndex(weatherIdx);
    m_timeOfDaySpin->setValue(m_config.timeOfDay);
    m_useRealWeatherCheck->setChecked(m_config.useRealWeather);
    m_timeMultSpin->setValue(m_config.timeMultiplier);
    m_updating = false;
}

void ServerConfigEditorModule::saveConfigFromUI()
{
    m_config.name = m_nameEdit->text();
    m_config.description = m_descEdit->text();
    m_config.password = m_passwordEdit->text();
    m_config.adminPassword = m_adminPwEdit->text();
    m_config.ip = m_ipEdit->text();
    m_config.port = m_portSpin->value();
    m_config.httpPort = m_httpPortSpin->value();
    m_config.maxClients = m_maxClientsSpin->value();
    m_config.slotCount = m_slotCountSpin->value();
    m_config.sessionType = m_sessionTypeCombo->currentIndex();
    m_config.sessionDuration = m_sessionDurationSpin->value();
    m_config.lapsCount = m_lapsCountSpin->value();
    m_config.waitTime = m_waitTimeSpin->value();
    m_config.weather = m_weatherCombo->currentText();
    m_config.timeOfDay = (float)m_timeOfDaySpin->value();
    m_config.useRealWeather = m_useRealWeatherCheck->isChecked();
    m_config.timeMultiplier = m_timeMultSpin->value();
}

void ServerConfigEditorModule::populateEntryTable()
{
    m_entryTable->setRowCount(m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i) {
        const auto& e = m_entries[i];
        m_entryTable->setItem(i, 0, new QTableWidgetItem(e.name));
        m_entryTable->setItem(i, 1, new QTableWidgetItem(e.team));
        m_entryTable->setItem(i, 2, new QTableWidgetItem(e.guid));
        m_entryTable->setItem(i, 3, new QTableWidgetItem(e.carModel));
        m_entryTable->setItem(i, 4, new QTableWidgetItem(e.skin));
        m_entryTable->setItem(i, 5, new QTableWidgetItem(QString::number(e.ballast)));
    }
}
void ServerConfigEditorModule::importFile(const QString& f)
{
    QFileInfo fi(f);
    if (fi.isDir()) {
        m_serverPath = f;
        m_config = ServerConfigEditor::loadConfig(f);
        m_entries = ServerConfigEditor::loadEntryList(f);
        loadConfigToUI();
        populateEntryTable();
    }
}

void ServerConfigEditorModule::exportFile(const QString& f)
{
    if (f.isEmpty()) return;
    saveConfigFromUI();
    ServerConfigEditor::saveConfig(m_config, f);
    ServerConfigEditor::saveEntryList(m_entries, f);
}

void ServerConfigEditorModule::onActivation()
{
    if (m_statusLabel) m_statusLabel->setText(tr("Active"));
}

void ServerConfigEditorModule::onDeactivation()
{
    if (m_statusLabel) m_statusLabel->setText(tr("Inactive"));
}

QJsonObject ServerConfigEditorModule::serializeProject() const
{
    QJsonObject data;
    data["serverPath"] = m_serverPath;
    QJsonObject cfg;
    cfg["name"] = m_config.name;
    cfg["description"] = m_config.description;
    cfg["password"] = m_config.password;
    cfg["adminPassword"] = m_config.adminPassword;
    cfg["ip"] = m_config.ip;
    cfg["port"] = m_config.port;
    cfg["httpPort"] = m_config.httpPort;
    cfg["maxClients"] = m_config.maxClients;
    cfg["slotCount"] = m_config.slotCount;
    cfg["track"] = m_config.track;
    cfg["sessionType"] = m_config.sessionType;
    cfg["sessionDuration"] = m_config.sessionDuration;
    cfg["lapsCount"] = m_config.lapsCount;
    cfg["waitTime"] = m_config.waitTime;
    cfg["weather"] = m_config.weather;
    cfg["timeOfDay"] = m_config.timeOfDay;
    cfg["sunAngle"] = m_config.sunAngle;
    cfg["useRealWeather"] = m_config.useRealWeather;
    cfg["timeMultiplier"] = m_config.timeMultiplier;
    cfg["trackConfig"] = m_config.trackConfig;
    cfg["gripModifier"] = m_config.gripModifier;
    cfg["dynamicTrack"] = m_config.dynamicTrack;
    cfg["trackConditions"] = m_config.trackConditions;
    cfg["isLocked"] = m_config.isLocked;
    cfg["allowAutopilot"] = m_config.allowAutopilot;
    cfg["allowVirtualMirror"] = m_config.allowVirtualMirror;
    cfg["maxBallast"] = m_config.maxBallast;
    cfg["dumpInterval"] = m_config.dumpInterval;
    cfg["enableCsp"] = m_config.enableCsp;
    cfg["cspVersion"] = m_config.cspVersion;
    data["config"] = cfg;
    return data;
}

void ServerConfigEditorModule::deserializeProject(const QJsonObject& data)
{
    m_serverPath = data["serverPath"].toString();
    QJsonObject cfg = data["config"].toObject();
    m_config.name = cfg["name"].toString();
    m_config.description = cfg["description"].toString();
    m_config.password = cfg["password"].toString();
    m_config.adminPassword = cfg["adminPassword"].toString();
    m_config.ip = cfg["ip"].toString();
    m_config.port = cfg["port"].toInt(9600);
    m_config.httpPort = cfg["httpPort"].toInt(8080);
    m_config.maxClients = cfg["maxClients"].toInt(20);
    m_config.slotCount = cfg["slotCount"].toInt(30);
    m_config.track = cfg["track"].toInt(0);
    m_config.sessionType = cfg["sessionType"].toInt(0);
    m_config.sessionDuration = cfg["sessionDuration"].toInt(600);
    m_config.lapsCount = cfg["lapsCount"].toInt(0);
    m_config.waitTime = cfg["waitTime"].toInt(15);
    m_config.weather = cfg["weather"].toString();
    m_config.timeOfDay = (float)cfg["timeOfDay"].toDouble(12.0);
    m_config.sunAngle = (float)cfg["sunAngle"].toDouble(0.0);
    m_config.useRealWeather = cfg["useRealWeather"].toBool(false);
    m_config.timeMultiplier = cfg["timeMultiplier"].toInt(1);
    m_config.trackConfig = cfg["trackConfig"].toString();
    m_config.gripModifier = (float)cfg["gripModifier"].toDouble(1.0);
    m_config.dynamicTrack = cfg["dynamicTrack"].toBool(true);
    m_config.trackConditions = cfg["trackConditions"].toInt(1);
    m_config.isLocked = cfg["isLocked"].toBool(false);
    m_config.allowAutopilot = cfg["allowAutopilot"].toBool(false);
    m_config.allowVirtualMirror = cfg["allowVirtualMirror"].toBool(true);
    m_config.maxBallast = cfg["maxBallast"].toInt(0);
    m_config.dumpInterval = cfg["dumpInterval"].toInt(0);
    m_config.enableCsp = cfg["enableCsp"].toBool(false);
    m_config.cspVersion = cfg["cspVersion"].toString();
    m_config.cspSettings.clear();
    loadConfigToUI();
}

} // namespace ks
