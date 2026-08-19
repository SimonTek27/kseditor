#pragma once

#include <QWidget>
#include <QQuickWidget>
#include "../../core/editor/EditorModule.h"

namespace ks {

class CockpitInstrumentsQmlBridge;

class CockpitInstrumentsModule : public EditorModule {
    Q_OBJECT

public:
    explicit CockpitInstrumentsModule(QWidget* parent = nullptr);
    ~CockpitInstrumentsModule() override;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Display Editor"; }
    QString moduleId() const override { return "displayEditor"; }
    int getModulePriority() const override { return 42; }

    void exportFile(const QString& filePath) override;
    void importFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private:
    QQuickWidget* m_quickWidget = nullptr;
    CockpitInstrumentsQmlBridge* m_bridge = nullptr;
    bool m_initialized = false;
};

} // namespace ks
