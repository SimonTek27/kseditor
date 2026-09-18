#include "SheetPadModule.h"
#include "SheetPad.h"
#include "../../sys/LogManager.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QDebug>

namespace ks {

SheetPadModule::SheetPadModule(QWidget* parent)
    : EditorModule(parent)
    , m_dockWidget(nullptr)
    , m_sheetPad(nullptr)
{
}

SheetPadModule::~SheetPadModule()
{
    shutdown();
}

bool SheetPadModule::initialize()
{
    LOG_INFO("SheetPadModule", "Initializing SheetPad module");
    m_sheetPad = new SheetPad(this);
    return true;
}

void SheetPadModule::shutdown()
{
    LOG_INFO("SheetPadModule", "Shutting down SheetPad module");
    if (m_dockWidget) {
        m_dockWidget->close();
        m_dockWidget->deleteLater();
        m_dockWidget = nullptr;
    }
    if (m_sheetPad) {
        m_sheetPad->deleteLater();
        m_sheetPad = nullptr;
    }
}

QDockWidget* SheetPadModule::getOrCreateDockWidget(QMainWindow* mainWindow)
{
    if (m_dockWidget) return m_dockWidget;

    m_dockWidget = new QDockWidget("Sheet Pad", mainWindow);
    m_dockWidget->setObjectName("SheetPadDock");
    m_dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    m_sheetPad = new SheetPad();
    m_dockWidget->setWidget(m_sheetPad);

    connect(m_sheetPad, &SheetPad::fileOpened, this, &SheetPadModule::onFileOpened);
    connect(m_sheetPad, &SheetPad::fileClosed, this, &SheetPadModule::onFileClosed);
    connect(m_sheetPad, &SheetPad::fileSaved, this, &SheetPadModule::onFileSaved);
    connect(m_sheetPad, &SheetPad::documentModifiedChanged, this, &SheetPadModule::onDocumentModifiedChanged);
    connect(m_sheetPad, &SheetPad::cellSelected, this, &SheetPadModule::onCellSelected);

    return m_dockWidget;
}

void SheetPadModule::importFile(const QString& filePath)
{
    if (!m_sheetPad) return;

    if (filePath.endsWith(".csv", Qt::CaseInsensitive)) {
        m_sheetPad->importCSV();
    }
}

void SheetPadModule::exportFile(const QString& filePath)
{
    if (!m_sheetPad) return;

    if (filePath.endsWith(".csv", Qt::CaseInsensitive)) {
        m_sheetPad->exportCSV();
    }
}

void SheetPadModule::onFileOpened(const QString& path)
{
    emit fileOpened(path);
}

void SheetPadModule::onFileClosed()
{
    emit fileClosed();
}

void SheetPadModule::onFileSaved(const QString& path)
{
    emit fileSaved(path);
}

void SheetPadModule::onDocumentModifiedChanged(bool modified)
{
    m_modified = modified;
    emit modifiedChanged(modified);
}

void SheetPadModule::onCellSelected(int row, int col)
{
    emit cellSelected(row, col);
}

} // namespace ks
