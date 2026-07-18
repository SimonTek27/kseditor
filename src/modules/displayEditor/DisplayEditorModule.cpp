#include "DisplayEditorModule.h"
#include "DisplayEditorQmlBridge.h"
#include "../../core/sys/LogManager.h"
#include <QVBoxLayout>
#include <QUrl>
#include <QQmlContext>
#include <QJsonObject>

namespace ks {

DisplayEditorModule::DisplayEditorModule(QWidget* parent)
    : EditorModule(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    layout->addWidget(m_quickWidget);

    setLayout(layout);
}

DisplayEditorModule::~DisplayEditorModule()
{
}

bool DisplayEditorModule::initialize()
{
    if (!m_initialized) {
        m_bridge = DisplayEditorQmlBridge::instance();
        auto* ctx = m_quickWidget->rootContext();
        ctx->setContextProperty("displayBridge", m_bridge);
        m_quickWidget->setSource(QUrl("qrc:///qml/modules/car_DisplayEditor.qml"));
        m_initialized = true;
    }

    LOG_INFO("DisplayEditorModule", "Display Editor module initialized");
    return true;
}

void DisplayEditorModule::shutdown()
{
    LOG_INFO("DisplayEditorModule", "Shutting down Display Editor module");
    if (m_initialized) {
        m_bridge = nullptr;
        m_quickWidget->setSource(QUrl());
        m_initialized = false;
    }
}

void DisplayEditorModule::exportFile(const QString& filePath)
{
    if (m_bridge) {
        m_bridge->saveToFile(filePath);
    }
}

void DisplayEditorModule::importFile(const QString& filePath)
{
    if (m_bridge) {
        m_bridge->loadFromFile(filePath);
    }
}

void DisplayEditorModule::onActivation()
{
    if (!m_initialized) {
        initialize();
    }
    EditorModule::onActivation();
}

void DisplayEditorModule::onDeactivation()
{
    EditorModule::onDeactivation();
}

QJsonObject DisplayEditorModule::serializeProject() const
{
    QJsonObject data;
    if (m_bridge) {
        data["displayName"] = m_bridge->displayName();
        data["elementCount"] = m_bridge->elementCount();
        data["currentFile"] = m_bridge->currentFile();
    }
    return data;
}

void DisplayEditorModule::deserializeProject(const QJsonObject& data)
{
    if (data.contains("currentFile")) {
        if (!m_initialized) initialize();
        if (m_bridge) {
            m_bridge->loadFromFile(data["currentFile"].toString());
        }
    }
}

} // namespace ks
