#pragma once

#include "../EditorModule.h"
#include <QDockWidget>
#include <QMainWindow>

namespace ks {

class SheetPad;

class SheetPadModule : public EditorModule {
    Q_OBJECT
public:
    explicit SheetPadModule(QWidget* parent = nullptr);
    ~SheetPadModule() override;

    QString moduleName() const override { return "Sheet Pad"; }
    QString moduleId() const override { return "sheetPad"; }
    int getModulePriority() const override { return 71; }

    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;
    bool initialize() override;
    void shutdown() override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

signals:
    void fileOpened(const QString& path);
    void fileClosed();
    void fileSaved(const QString& path);
    void modifiedChanged(bool modified);
    void cellSelected(int row, int col);

private slots:
    void onFileOpened(const QString& path);
    void onFileClosed();
    void onFileSaved(const QString& path);
    void onDocumentModifiedChanged(bool modified);
    void onCellSelected(int row, int col);

private:
    QDockWidget* m_dockWidget = nullptr;
    SheetPad* m_sheetPad = nullptr;
    bool m_modified = false;
};

} // namespace ks
