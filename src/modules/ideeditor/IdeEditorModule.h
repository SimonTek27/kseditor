#pragma once

#include "../../core/editor/EditorModule.h"
#include <QDockWidget>
#include <QJsonObject>

namespace ks {

class IdeEditorWidget;
class FindReplaceDialog;

class IdeEditorModule : public EditorModule
{
    Q_OBJECT
public:
    explicit IdeEditorModule(QWidget* parent = nullptr);
    ~IdeEditorModule() override;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "IDE Editor"; }
    QString moduleId() const override { return "ideEditor"; }
    int getModulePriority() const override { return 55; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    bool canCut() const override;
    bool canCopy() const override;
    bool canPaste() const override;
    bool canDelete() const override;
    void cut() override;
    void copy() override;
    void paste() override;
    void deleteSelected() override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

    IdeEditorWidget* editorWidget() const { return m_editorWidget; }

protected:
    void onActivation() override;
    void onDeactivation() override;

private:
    void setupUI();

    QDockWidget* m_dockWidget = nullptr;
    IdeEditorWidget* m_editorWidget = nullptr;
    bool m_initialized = false;
};

} // namespace ks
