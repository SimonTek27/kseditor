#include "CareerEditorModule.h"
#include "../../../core/sys/LogManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QDir>
#include <QTextStream>
#include <QJsonArray>

namespace ks {

CareerEditorModule::CareerEditorModule(QWidget* parent) : EditorModule(parent) {}
bool CareerEditorModule::initialize() { LOG_INFO("CareerEditorModule", "Initialized"); return true; }
void CareerEditorModule::shutdown()
{
    m_series.clear();
    m_seriesList->clear();
    m_selectedIndex = -1;
    m_dir.clear();
}

QDockWidget* CareerEditorModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;
    m_dockWidget = new QDockWidget("Career Editor", mainWindow);
    m_dockWidget->setObjectName("CareerEditorDock");

    auto* centralWidget = new QWidget();
    auto* mainLayout = new QHBoxLayout(centralWidget);

    auto* listWidget = new QWidget(); auto* listLayout = new QVBoxLayout(listWidget);
    m_seriesList = new QListWidget(); listLayout->addWidget(m_seriesList);
    mainLayout->addWidget(listWidget);

    auto* propsWidget = new QWidget(); auto* propsLayout = new QGridLayout(propsWidget);
    m_seriesNameEdit = new QLineEdit(); propsLayout->addWidget(new QLabel("Name:"), 0, 0); propsLayout->addWidget(m_seriesNameEdit, 0, 1);
    m_seriesInfoEdit = new QTextEdit(); propsLayout->addWidget(new QLabel("Info:"), 1, 0); propsLayout->addWidget(m_seriesInfoEdit, 1, 1);
    mainLayout->addWidget(propsWidget);

    auto* vMain = new QVBoxLayout(); vMain->addLayout(mainLayout);
    auto* actionLayout = new QHBoxLayout();
    m_loadBtn = new QPushButton("Load Directory");
    actionLayout->addWidget(m_loadBtn); vMain->addLayout(actionLayout);
    m_statusLabel = new QLabel("Ready"); vMain->addWidget(m_statusLabel);

    auto* wrapper = new QWidget(); wrapper->setLayout(vMain);
    m_dockWidget->setWidget(wrapper);

    connect(m_seriesList, &QListWidget::currentRowChanged, this, &CareerEditorModule::onSeriesSelected);
    connect(m_loadBtn, &QPushButton::clicked, this, &CareerEditorModule::onLoadDir);
    connect(m_seriesNameEdit, &QLineEdit::textChanged, this, &CareerEditorModule::onSeriesNameChanged);

    return m_dockWidget;
}

void CareerEditorModule::importFile(const QString& f) { m_dir = f; loadDirToUI(); }
void CareerEditorModule::exportFile(const QString& f)
{
    if (f.isEmpty()) return;
    QDir dir(f);
    for (const auto& series : m_series) {
        QString seriesDir = dir.absoluteFilePath(series.first);
        QDir().mkpath(seriesDir);
        QString iniPath = seriesDir + "/series.ini";
        QFile file(iniPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "NAME=" << series.first << "\n";
            file.close();
        }
    }
    m_statusLabel->setText(QString("Exported %1 series to %2").arg(m_series.size()).arg(f));
}
void CareerEditorModule::onActivation() { if (m_statusLabel) m_statusLabel->setText("Active"); }
void CareerEditorModule::onDeactivation() { if (m_statusLabel) m_statusLabel->setText("Inactive"); }
void CareerEditorModule::onSeriesSelected(int r) { if (r >= 0 && r < m_series.size()) { m_selectedIndex = r; m_seriesNameEdit->setText(m_series[r].first); } }
void CareerEditorModule::onSeriesNameChanged(const QString& t) { if (m_selectedIndex >= 0) { m_series[m_selectedIndex].first = t; m_seriesList->item(m_selectedIndex)->setText(t); } }

void CareerEditorModule::onLoadDir()
{
    QString d = QFileDialog::getExistingDirectory(this, "Open career directory");
    if (!d.isEmpty()) { m_dir = d; loadDirToUI(); }
}

void CareerEditorModule::setupUi() { if (m_statusLabel) m_statusLabel->setText("UI Ready"); }

void CareerEditorModule::loadDirToUI()
{
    m_series.clear(); m_seriesList->clear();
    QDir dir(m_dir);
    for (const QString& sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        m_series.append({sub, dir.absoluteFilePath(sub + "/series.ini")});
        m_seriesList->addItem(sub);
    }
    m_statusLabel->setText(QString("Loaded %1 series").arg(m_series.size()));
}

QJsonObject CareerEditorModule::serializeProject() const
{
    QJsonObject data;
    data["dir"] = m_dir;
    data["selectedIndex"] = m_selectedIndex;
    QJsonArray seriesArray;
    for (const auto& s : m_series) {
        QJsonObject obj;
        obj["name"] = s.first;
        obj["info"] = s.second;
        seriesArray.append(obj);
    }
    data["series"] = seriesArray;
    return data;
}

void CareerEditorModule::deserializeProject(const QJsonObject& data)
{
    m_dir = data["dir"].toString();
    m_selectedIndex = data["selectedIndex"].toInt(-1);
    m_series.clear();
    for (const auto& v : data["series"].toArray()) {
        QJsonObject obj = v.toObject();
        m_series.append(qMakePair(obj["name"].toString(), obj["info"].toString()));
    }
}

} // namespace ks
