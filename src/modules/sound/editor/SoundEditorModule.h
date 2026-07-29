#pragma once

#include <QWidget>
#include <QQuickWidget>
#include "../../../core/editor/EditorModule.h"

namespace ks {
namespace audio {
class ACEventBridge;
class AudioWaveformBridge;
}

class SoundEditorModule : public EditorModule {
    Q_OBJECT

public:
    explicit SoundEditorModule(QWidget* parent = nullptr);
    ~SoundEditorModule() override;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "Sound Editor"; }
    QString moduleId() const override { return "soundEditor"; }
    int getModulePriority() const override { return 50; }

    void exportFile(const QString& filePath) override;
    void importFile(const QString& filePath) override;

    // Load audio file into waveform
    void loadAudioFile(const QString& path);

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private:
    QQuickWidget* m_quickWidget = nullptr;
    audio::AudioWaveformBridge* m_waveformBridge = nullptr;
    ks::audio::ACEventBridge* m_eventBridge = nullptr;
    bool m_initialized = false;
};

} // namespace ks
