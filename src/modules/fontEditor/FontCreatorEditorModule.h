#pragma once

#include <QWidget>
#include <QQuickWidget>
#include "../../core/editor/EditorModule.h"

namespace ks {

class FontCreatorQmlBridge;

class FontCreatorEditorModule : public EditorModule {
    Q_OBJECT

public:
    explicit FontCreatorEditorModule(QWidget* parent = nullptr);
    ~FontCreatorEditorModule() override;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Font Creator"; }
    QString moduleId() const override { return "fontCreator"; }
    int getModulePriority() const override { return 41; }

    void exportFile(const QString& filePath) override;
    void importFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private:
    QQuickWidget* m_quickWidget = nullptr;
    FontCreatorQmlBridge* m_bridge = nullptr;
    bool m_initialized = false;
};

} // namespace ks
