#pragma once

/**
 * @file WeatherModule.h
 * @brief Weather editing module for the editor
 * @copyright KS Physics Engine
 */

#include "../../EngineModule.h"
#include "WeatherEditor.h"
#include "../WeatherConfig.h"

#include <QDockWidget>
#include <QTreeWidget>
#include <QSplitter>

namespace ks {
namespace physics {

/**
 * @class WeatherModule
 * @brief Editor module for weather configuration and editing
 */
class WeatherModule : public EditorModule {
    Q_OBJECT
    
public:
    explicit WeatherModule(QWidget* parent = nullptr);
    ~WeatherModule() override;
    
    bool initialize() override;
    void shutdown() override;
    QDockWidget* getOrCreateDockWidget(QMainWindow* mainWindow) override;
    
    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;
    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& project) override;
    
    config::WeatherConfig& weatherConfig() { return m_config; }
    const config::WeatherConfig& weatherConfig() const { return m_config; }
    
signals:
    void configChanged();
    
public slots:
    void loadWeatherFile(const QString& filePath);
    void saveWeatherFile(const QString& filePath);
    void loadWeatherPreset(const QString& presetName);
    
private slots:
    void onConfigChanged();
    void onSequenceSelectionChanged();
    void onKeyframeSelectionChanged();
    void onAddSequence();
    void onRemoveSequence();
    void onAddKeyframe();
    void onRemoveKeyframe();
    
private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupConnections();
    void updateUI();
    void updateSequenceTree();
    void updateKeyframeTree(const QString& sequenceName);
    
    config::WeatherConfig m_config;
    WeatherEditor* m_editor = nullptr;
    QDockWidget* m_dockWidget = nullptr;
    QWidget* m_centralWidget = nullptr;
    QSplitter* m_splitter = nullptr;
    QTreeWidget* m_sequenceTree = nullptr;
    QTreeWidget* m_keyframeTree = nullptr;
    bool m_initialized = false;
};

} // namespace physics
} // namespace ks