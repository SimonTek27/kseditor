#pragma once

#include <QObject>

namespace ks {

class AudioEditorModule : public QObject
{
    Q_OBJECT
public:
    explicit AudioEditorModule(QObject* parent = nullptr);
    ~AudioEditorModule() override;

    static AudioEditorModule* instance() { return s_instance; }

    bool initialize();
    void shutdown();
    QString moduleName() const { return "Sound Editor"; }
    QString moduleId() const { return "ks.sound_editor"; }

signals:
    void soundChanged();

public slots:
    void onNewProject();
    void onOpenProject();
    void onSaveProject();
    void onImportAsset();
    void onExportAsset();
    void onBuildBanks();

private:
    static AudioEditorModule* s_instance;
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace ks