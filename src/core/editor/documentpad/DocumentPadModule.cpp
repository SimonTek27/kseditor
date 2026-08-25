#include "DocumentPadModule.h"
#include "DocumentPad.h"
#include "../../sys/LogManager.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QDebug>

namespace ks {

DocumentPadModule::DocumentPadModule(QWidget* parent)
    : EditorModule(parent)
    , m_dockWidget(nullptr)
    , m_documentPad(nullptr)
{
}

DocumentPadModule::~DocumentPadModule()
{
    shutdown();
}

bool DocumentPadModule::initialize()
{
    LOG_INFO("DocumentPadModule", "Initializing DocumentPad module");
    m_documentPad = new DocumentPad(this);
    return true;
}

void DocumentPadModule::shutdown()
{
    LOG_INFO("DocumentPadModule", "Shutting down DocumentPad module");
    if (m_dockWidget) {
        m_dockWidget->close();
        m_dockWidget->deleteLater();
        m_dockWidget = nullptr;
    }
    if (m_documentPad) {
        m_documentPad->deleteLater();
        m_documentPad = nullptr;
    }
}

QDockWidget* DocumentPadModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;

    m_dockWidget = new QDockWidget("Document Pad", mainWindow);
    m_dockWidget->setObjectName("DocumentPadDock");
    m_dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_documentPad = new DocumentPad();
    m_dockWidget->setWidget(m_documentPad);

    // Connect signals from DocumentPad
    connect(m_documentPad, &DocumentPad::fileOpened, this, &DocumentPadModule::onFileOpened);
    connect(m_documentPad, &DocumentPad::fileClosed, this, &DocumentPadModule::onFileClosed);
    connect(m_documentPad, &DocumentPad::fileSaved, this, &DocumentPadModule::onFileSaved);
    connect(m_documentPad, &DocumentPad::documentModifiedChanged, this, &DocumentPadModule::onDocumentModifiedChanged);
    connect(m_documentPad, &DocumentPad::selectionChanged, this, &DocumentPadModule::onSelectionChanged);

    return m_dockWidget;
}

void DocumentPadModule::onFileOpened(const QString& path)
{
    emit fileOpened(path);
}

void DocumentPadModule::onFileClosed()
{
    emit fileClosed();
}

void DocumentPadModule::onFileSaved(const QString& path)
{
    emit fileSaved(path);
}

void DocumentPadModule::onDocumentModifiedChanged(bool modified)
{
    m_modified = modified;
    emit modifiedChanged(modified);
}

void DocumentPadModule::onSelectionChanged()
{
    emit selectionChanged();
}

} // namespace ks