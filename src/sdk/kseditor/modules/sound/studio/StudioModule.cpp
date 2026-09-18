#include "StudioModule.h"
#include "AudioStudioBridge.h"
#include <QQuickWidget>
#include <QQmlContext>
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QVBoxLayout>

namespace ks {

StudioModule::StudioModule(QWidget* parent)
    : EditorModule(parent)
{
    setObjectName("StudioModule");
}

StudioModule::~StudioModule()
{
    shutdown();
}

bool StudioModule::initialize()
{
    if (m_initialized) return true;

    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setAttribute(Qt::WA_AlwaysStackOnTop);
    m_quickWidget->setAttribute(Qt::WA_OpaquePaintEvent);

    // Create bridge object
    m_audioStudioBridge = new audio::AudioStudioBridge(this);

    // Expose to QML
    m_quickWidget->rootContext()->setContextProperty("AudioStudioBridge", m_audioStudioBridge);

    // Load QML
    QString qmlPath = "qrc:/ui/qml/pages/page_ksAudioStudio.qml";
    m_quickWidget->setSource(QUrl(qmlPath));

    if (m_quickWidget->status() == QQuickWidget::Error) {
        qWarning() << "Failed to load Studio QML:" << m_quickWidget->errors();
    }

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_quickWidget);

    m_initialized = true;
    return true;
}

void StudioModule::shutdown()
{
    if (!m_initialized) return;

    if (m_quickWidget) {
        m_quickWidget->setSource(QUrl());
        m_quickWidget->deleteLater();
        m_quickWidget = nullptr;
    }

    if (m_audioStudioBridge) {
        m_audioStudioBridge->deleteLater();
        m_audioStudioBridge = nullptr;
    }

    m_initialized = false;
}

void StudioModule::exportFile(const QString& filePath)
{
    if (filePath.endsWith(".ksaudio", Qt::CaseInsensitive)) {
        saveProject(filePath);
    } else if (filePath.endsWith(".bank", Qt::CaseInsensitive)) {
        exportBank(filePath);
    }
}

void StudioModule::importFile(const QString& filePath)
{
    if (filePath.endsWith(".ksaudio", Qt::CaseInsensitive)) {
        loadProject(filePath);
    } else if (filePath.endsWith(".bank", Qt::CaseInsensitive) || filePath.endsWith(".fspro", Qt::CaseInsensitive)) {
        importBank(filePath);
    }
}

void StudioModule::loadProject(const QString& path)
{
    if (m_audioStudioBridge) {
        if (m_audioStudioBridge->loadProject(path)) {
            m_currentProjectPath = path;
            emit statusMessage("Project loaded: " + path);
        } else {
            emit statusMessage("Failed to load project: " + path);
        }
    }
}

void StudioModule::saveProject(const QString& path)
{
    if (m_audioStudioBridge) {
        if (m_audioStudioBridge->saveProject(path)) {
            m_currentProjectPath = path;
            emit statusMessage("Project saved: " + path);
        } else {
            emit statusMessage("Failed to save project: " + path);
        }
    }
}

void StudioModule::importBank(const QString& bankPath)
{
    if (m_audioStudioBridge) {
        if (m_audioStudioBridge->importBank(bankPath)) {
            emit statusMessage("Bank imported: " + bankPath);
        } else {
            emit statusMessage("Failed to import bank: " + bankPath);
        }
    }
}

void StudioModule::exportBank(const QString& bankPath)
{
    if (m_audioStudioBridge) {
        if (m_audioStudioBridge->exportBank(bankPath)) {
            emit statusMessage("Bank exported: " + bankPath);
        } else {
            emit statusMessage("Failed to export bank: " + bankPath);
        }
    }
}

QJsonObject StudioModule::serializeProject() const
{
    QJsonObject obj;
    obj["moduleId"] = moduleId();
    obj["projectPath"] = m_currentProjectPath;
    return obj;
}

void StudioModule::deserializeProject(const QJsonObject& data)
{
    QString path = data["projectPath"].toString();
    if (!path.isEmpty()) {
        loadProject(path);
    }
}

void StudioModule::onActivation()
{
    if (m_quickWidget) {
        m_quickWidget->show();
    }
    emit statusMessage("ksAudioStudio activated");
}

void StudioModule::onDeactivation()
{
    if (m_quickWidget) {
        m_quickWidget->hide();
    }
    emit statusMessage("ksAudioStudio deactivated");
}

} // namespace ks