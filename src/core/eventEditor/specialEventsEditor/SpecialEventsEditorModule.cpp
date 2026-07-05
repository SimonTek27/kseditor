#include "SpecialEventsEditorModule.h"
#include "../../../core/sys/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QDir>
#include <QTextStream>
#include <QJsonArray>

namespace ks {

SpecialEventsEditorModule::SpecialEventsEditorModule(QWidget* parent) : EditorModule(parent) {}
bool SpecialEventsEditorModule::initialize() { LOG_INFO("SpecialEventsEditorModule", "Initialized"); return true; }
void SpecialEventsEditorModule::shutdown()
{
    m_events.clear();
    m_eventList->clear();
    m_selectedIndex = -1;
    m_dir.clear();
}

QDockWidget* SpecialEventsEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;
    m_dockWidget = new QDockWidget("Special Events Editor", mainWindow);
    m_dockWidget->setObjectName("SpecialEventsEditorDock");

    auto* centralWidget = new QWidget();
    auto* mainLayout = new QHBoxLayout(centralWidget);

    auto* listWidget = new QWidget(); auto* listLayout = new QVBoxLayout(listWidget);
    m_eventList = new QListWidget(); listLayout->addWidget(m_eventList);
    auto* listBtns = new QHBoxLayout();
    m_addBtn = new QPushButton("Add"); m_removeBtn = new QPushButton("Remove");
    listBtns->addWidget(m_addBtn); listBtns->addWidget(m_removeBtn); listBtns->addStretch();
    listLayout->addLayout(listBtns);
    mainLayout->addWidget(listWidget);

    auto* propsWidget = new QWidget(); auto* propsLayout = new QGridLayout(propsWidget);
    m_eventNameEdit = new QLineEdit();
    propsLayout->addWidget(new QLabel("Name:"), 0, 0); propsLayout->addWidget(m_eventNameEdit, 0, 1);
    mainLayout->addWidget(propsWidget);

    auto* vMain = new QVBoxLayout(); vMain->addLayout(mainLayout);
    auto* actionLayout = new QHBoxLayout();
    m_loadBtn = new QPushButton("Load Directory"); m_saveBtn = new QPushButton("Save");
    actionLayout->addWidget(m_loadBtn); actionLayout->addWidget(m_saveBtn);
    vMain->addLayout(actionLayout);
    m_statusLabel = new QLabel("Ready"); vMain->addWidget(m_statusLabel);

    auto* wrapper = new QWidget(); wrapper->setLayout(vMain);
    m_dockWidget->setWidget(wrapper);

    connect(m_eventList, &QListWidget::currentRowChanged, this, &SpecialEventsEditorModule::onEventSelected);
    connect(m_addBtn, &QPushButton::clicked, this, &SpecialEventsEditorModule::onAddEvent);
    connect(m_removeBtn, &QPushButton::clicked, this, &SpecialEventsEditorModule::onRemoveEvent);
    connect(m_loadBtn, &QPushButton::clicked, this, &SpecialEventsEditorModule::onLoadDir);
    connect(m_saveBtn, &QPushButton::clicked, this, &SpecialEventsEditorModule::onSaveDir);
    connect(m_eventNameEdit, &QLineEdit::textChanged, this, &SpecialEventsEditorModule::onEventNameChanged);

    return m_dockWidget;
}

void SpecialEventsEditorModule::importFile(const QString& f) { m_dir = f; loadDirToUI(); }
void SpecialEventsEditorModule::exportFile(const QString& f)
{
    if (f.isEmpty()) return;
    QDir dir(f);
    for (const auto& event : m_events) {
        QString eventDir = dir.absoluteFilePath(event.name);
        QDir().mkpath(eventDir);
        QString iniPath = eventDir + "/event.ini";
        QFile file(iniPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "NAME=" << event.name << "\n";
            for (auto it = event.settings.constBegin(); it != event.settings.constEnd(); ++it)
                out << it.key() << "=" << it.value() << "\n";
            file.close();
        }
    }
    m_statusLabel->setText(QString("Exported %1 events to %2").arg(m_events.size()).arg(f));
}
void SpecialEventsEditorModule::onActivation() { if (m_statusLabel) m_statusLabel->setText("Active"); }
void SpecialEventsEditorModule::onDeactivation() { if (m_statusLabel) m_statusLabel->setText("Inactive"); }
void SpecialEventsEditorModule::onEventSelected(int r) { if (r >= 0 && r < m_events.size()) { m_selectedIndex = r; m_eventNameEdit->setText(m_events[r].name); } }
void SpecialEventsEditorModule::onAddEvent() { SpecialEvent e; e.name = "New Event"; m_events.append(e); m_eventList->addItem(e.name); }
void SpecialEventsEditorModule::onRemoveEvent() { if (m_selectedIndex >= 0) { m_events.removeAt(m_selectedIndex); delete m_eventList->takeItem(m_selectedIndex); } }
void SpecialEventsEditorModule::onEventNameChanged(const QString& t) { if (m_selectedIndex >= 0) { m_events[m_selectedIndex].name = t; m_eventList->item(m_selectedIndex)->setText(t); } }

void SpecialEventsEditorModule::onLoadDir()
{
    QString d = QFileDialog::getExistingDirectory(this, "Open specialevents directory");
    if (!d.isEmpty()) { m_dir = d; loadDirToUI(); }
}

void SpecialEventsEditorModule::onSaveDir()
{
    if (m_dir.isEmpty()) return;
    QDir dir(m_dir);
    for (const auto& event : m_events) {
        QString eventDir = dir.absoluteFilePath(event.name);
        QDir().mkpath(eventDir);
        if (!event.iniPath.isEmpty()) {
            QFile file(event.iniPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << "NAME=" << event.name << "\n";
                for (auto it = event.settings.constBegin(); it != event.settings.constEnd(); ++it)
                    out << it.key() << "=" << it.value() << "\n";
                file.close();
            }
        }
    }
    m_statusLabel->setText(QString("Saved %1 events to %2").arg(m_events.size()).arg(m_dir));
}

void SpecialEventsEditorModule::setupUi() { if (m_statusLabel) m_statusLabel->setText("UI Ready"); }

void SpecialEventsEditorModule::loadDirToUI()
{
    m_events.clear(); m_eventList->clear();
    QDir dir(m_dir);
    for (const QString& sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        SpecialEvent e;
        e.name = sub;
        e.iniPath = dir.absoluteFilePath(sub + "/event.ini");
        e.previewPath = dir.absoluteFilePath(sub + "/preview.png");
        m_events.append(e);
        m_eventList->addItem(sub);
    }
    m_statusLabel->setText(QString("Loaded %1 events").arg(m_events.size()));
}

QJsonObject SpecialEventsEditorModule::serializeProject() const
{
    QJsonObject data;
    data["dir"] = m_dir;
    data["selectedIndex"] = m_selectedIndex;
    QJsonArray eventsArray;
    for (const auto& ev : m_events) {
        QJsonObject obj;
        obj["name"] = ev.name;
        obj["iniPath"] = ev.iniPath;
        obj["previewPath"] = ev.previewPath;
        QJsonObject settingsObj;
        for (auto it = ev.settings.constBegin(); it != ev.settings.constEnd(); ++it)
            settingsObj[it.key()] = it.value();
        obj["settings"] = settingsObj;
        eventsArray.append(obj);
    }
    data["events"] = eventsArray;
    return data;
}

void SpecialEventsEditorModule::deserializeProject(const QJsonObject& data)
{
    m_dir = data["dir"].toString();
    m_selectedIndex = data["selectedIndex"].toInt(-1);
    m_events.clear();
    for (const auto& v : data["events"].toArray()) {
        QJsonObject obj = v.toObject();
        SpecialEvent ev;
        ev.name = obj["name"].toString();
        ev.iniPath = obj["iniPath"].toString();
        ev.previewPath = obj["previewPath"].toString();
        QJsonObject settingsObj = obj["settings"].toObject();
        for (auto it = settingsObj.constBegin(); it != settingsObj.constEnd(); ++it)
            ev.settings[it.key()] = it.value().toString();
        m_events.append(ev);
    }
}

} // namespace ks
