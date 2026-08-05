#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QVector>
#include <QDockWidget>
#include "PaintSystem.h"
#include "PaintPainter.h"
#include "../../core/editor/EditorModule.h"

namespace ks {

class PaintEditorWidget;

class PaintEditor : public QObject {
    Q_OBJECT
public:
    static PaintEditor* instance();

    void setCarPath(const QString& path);
    QString carPath() const { return m_carPath; }

    bool loadSkins();
    bool createSkin(const QString& name);
    bool deleteSkin(const QString& name);
    bool duplicateSkin(const QString& sourceName, const QString& destName);

    bool setCurrentSkin(const QString& skinName);
    QString currentSkin() const { return m_currentSkin; }
    QStringList getSkinNames() const;

    PaintSystem::SkinConfig& currentConfig() { return m_config; }
    PaintPainter* paintPainter() { return &m_paintPainter; }

    bool loadPaintTexture(const QString& skinPath);
    bool savePaintTexture(const QImage& texture, const QString& skinPath);

    bool addLayer(const PaintSystem::SkinLayer& layer);
    bool removeLayer(int index);
    bool moveLayer(int fromIndex, int toIndex);
    bool updateLayer(int index, const PaintSystem::SkinLayer& layer);

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
    void paintModified();

private:
    explicit PaintEditor(QObject* parent = nullptr);
    static PaintEditor* s_instance;

    QString m_carPath;
    QString m_currentSkin;
    PaintSystem::SkinConfig m_config;
    PaintPainter m_paintPainter;
    PaintManager* m_paintManager = nullptr;
};

class PaintEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit PaintEditorModule(QWidget* parent = nullptr);
    ~PaintEditorModule() override = default;

    QString getModuleName() const override { return "Paint Editor"; }
    QString moduleId() const override { return "paintEditor"; }
    QString getModuleIcon() const override { return ":/icons/paint.png"; }
    int getModulePriority() const override { return 40; }
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;

    bool initialize() override;
    void shutdown() override;

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

protected:
    void onActivation() override;
    void onDeactivation() override;

public:
    PaintEditorWidget* editorWidget() const { return m_editorWidget; }

private:
    PaintEditorWidget* ensureWidget();
    QDockWidget* m_dockWidget = nullptr;
    PaintEditorWidget* m_editorWidget = nullptr;
};

} // namespace ks