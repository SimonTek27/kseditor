#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QVector>
#include <QDockWidget>
#include "LiverySystem.h"
#include "LiveryPainter.h"
#include "../../core/editor/EditorModule.h"

namespace ks {

class LiveryEditorWidget;

class LiveryEditor : public QObject {
    Q_OBJECT
public:
    static LiveryEditor* instance();

    void setCarPath(const QString& path);
    QString carPath() const { return m_carPath; }

    bool loadSkins();
    bool createSkin(const QString& name);
    bool deleteSkin(const QString& name);
    bool duplicateSkin(const QString& sourceName, const QString& destName);

    bool setCurrentSkin(const QString& skinName);
    QString currentSkin() const { return m_currentSkin; }
    QStringList getSkinNames() const;

    LiverySystem::SkinConfig& currentConfig() { return m_config; }
    LiveryPainter* liveryPainter() { return &m_liveryPainter; }

    bool loadLiveryTexture(const QString& skinPath);
    bool saveLiveryTexture(const QImage& texture, const QString& skinPath);

    bool addLayer(const LiverySystem::LiveryLayer& layer);
    bool removeLayer(int index);
    bool moveLayer(int fromIndex, int toIndex);
    bool updateLayer(int index, const LiverySystem::LiveryLayer& layer);

    bool generateLicensePlate(const QString& text, const QString& country);

    bool saveCurrentSkin();
    bool exportSkin(const QString& outputPath);
    bool importSkin(const QString& importPath);

signals:
    void skinLoaded(const QString& skinName);
    void skinSaved(const QString& skinName);
    void skinListChanged();
    void textureLoaded(const QImage& texture);
    void textureSaved(const QString& path);
    void liveryModified();

private:
    explicit LiveryEditor(QObject* parent = nullptr);
    static LiveryEditor* s_instance;

    QString m_carPath;
    QString m_currentSkin;
    LiverySystem::SkinConfig m_config;
    LiveryPainter m_liveryPainter;
    LiveryManager* m_liveryManager = nullptr;
};

class LiveryEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit LiveryEditorModule(QWidget* parent = nullptr);
    ~LiveryEditorModule() override = default;

    QString getModuleName() const override { return "Livery Editor"; }
    QString moduleId() const override { return "liveryEditor"; }
    QString getModuleIcon() const override { return ":/icons/livery.png"; }
    int getModulePriority() const override { return 40; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    bool initialize() override;
    void shutdown() override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

private:
    QDockWidget* m_dockWidget = nullptr;
    LiveryEditorWidget* m_editorWidget = nullptr;
};

} // namespace ks
