#pragma once

#include "../EditorModule.h"
#include <QDockWidget>
#include <QMainWindow>

namespace ks {

class DocumentPad;

class DocumentPadModule : public EditorModule {
    Q_OBJECT
public:
    explicit DocumentPadModule(QWidget* parent = nullptr);
    ~DocumentPadModule() override;

    QString moduleName() const override { return "Document Pad"; }
    QString moduleId() const override { return "documentPad"; }
    int getModulePriority() const override { return 70; }

    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;
    bool initialize() override;
    void shutdown() override;

signals:
    void fileOpened(const QString& path);
    void fileClosed();
    void fileSaved(const QString& path);
    void modifiedChanged(bool modified);
    void selectionChanged();

private slots:
    void onFileOpened(const QString& path);
    void onFileClosed();
    void onFileSaved(const QString& path);
    void onDocumentModifiedChanged(bool modified);
    void onSelectionChanged();

private:
    QDockWidget* m_dockWidget = nullptr;
    DocumentPad* m_documentPad = nullptr;
    bool m_modified = false;
};

} // namespace ks