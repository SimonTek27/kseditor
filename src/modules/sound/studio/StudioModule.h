#pragma once

#include <QWidget>
#include <QQuickWidget>
#include "../../../core/editor/EditorModule.h"

namespace ks {
namespace audio {
class AudioStudioBridge;
}

class StudioModule : public EditorModule {
    Q_OBJECT

public:
    explicit StudioModule(QWidget* parent = nullptr);
    ~StudioModule() override;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "ksAudioStudio"; }
    QString moduleId() const override { return "ksAudioStudio"; }
    int getModulePriority() const override { return 55; }

    void exportFile(const QString& filePath) override;
    void importFile(const QString& filePath) override;

    // Load/save studio project
    void loadProject(const QString& path);
    void saveProject(const QString& path);

    // Bank operations
    void importBank(const QString& bankPath);
    void exportBank(const QString& bankPath);

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

signals:
    void statusMessage(const QString& message);

protected:
    void onActivation() override;
    void onDeactivation() override;

private:
    QQuickWidget* m_quickWidget = nullptr;
    audio::AudioStudioBridge* m_audioStudioBridge = nullptr;
    QString m_currentProjectPath;
    bool m_initialized = false;
};

} // namespace ks