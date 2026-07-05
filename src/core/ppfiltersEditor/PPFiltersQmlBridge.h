#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include "../editor/EditorModule.h"
#include "PPFilterColorGrading.h"

namespace ks {

class PPFiltersQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentFilter READ currentFilter WRITE setCurrentFilter NOTIFY currentFilterChanged)
    Q_PROPERTY(int filterCount READ filterCount NOTIFY filterCountChanged)
    Q_PROPERTY(bool isPreviewActive READ isPreviewActive NOTIFY previewActiveChanged)

public:
    static PPFiltersQmlBridge* instance();

    QString currentFilter() const { return m_currentFilter; }
    int filterCount() const { return m_filterCount; }
    bool isPreviewActive() const { return m_isPreviewActive; }

    Q_INVOKABLE void setCurrentFilter(const QString& filter);
    Q_INVOKABLE void loadFiltersFromDirectory(const QString& path);
    Q_INVOKABLE void loadFilter(const QString& path);
    Q_INVOKABLE void saveFilter(const QString& path);
    Q_INVOKABLE void exportFilter(const QString& path);
    Q_INVOKABLE QVariantList getFilters();
    Q_INVOKABLE QVariantMap getFilter(int index);
    Q_INVOKABLE QVariantList getParameters();
    Q_INVOKABLE QVariantMap getParameterMap(const QString& name);
    Q_INVOKABLE void setParameter(const QString& name, float value);
    Q_INVOKABLE float getParameterValue(const QString& name);
    Q_INVOKABLE void resetParameters();
    Q_INVOKABLE void applyPreset(const QString& presetName);
    Q_INVOKABLE QStringList getPresets();
    Q_INVOKABLE void startPreview();
    Q_INVOKABLE void stopPreview();
    Q_INVOKABLE void reloadFilter();

    // Color grading
    Q_INVOKABLE void setColorGradingParam(const QString& name, float value);
    Q_INVOKABLE QVariantMap getColorGradingParams() const;
    Q_INVOKABLE void applyColorGradingPreset(const QString& name);
    Q_INVOKABLE QVariantList getColorGradingPresets() const;
    Q_INVOKABLE bool exportCubeLUT(const QString& path);
    Q_INVOKABLE QString getColorGradingJson() const;
    Q_INVOKABLE void loadColorGradingJson(const QString& json);
    Q_INVOKABLE bool exportToAC(const QString& acPath, const QString& filterName);
    Q_INVOKABLE void exportToACDialog();

signals:
    void currentFilterChanged();
    void filterCountChanged();
    void previewActiveChanged();
    void parameterChanged(const QString& name, float value);
    void filterLoaded();
    void filterSaved();
    void colorGradingChanged();

private:
    static PPFiltersQmlBridge* s_instance;
    PPFiltersQmlBridge(QObject* parent = nullptr) : QObject(parent) {}

    QVariantMap parseFilterINI(const QString& path);
    void writeFilterINI(const QString& path, const QVariantMap& params);
    QVariantMap getPresetData(const QString& presetName);
    void syncColorGradingToUI();

    PPFilterColorGrading m_colorGrading;
    QString m_currentFilter;
    int m_filterCount = 0;
    bool m_isPreviewActive = false;
    QVariantList m_filters;
    QVariantMap m_currentParams;
};

class PPFiltersEditorModule : public EditorModule {
    Q_OBJECT
public:
    explicit PPFiltersEditorModule(QWidget* parent = nullptr);
    ~PPFiltersEditorModule() override = default;

    bool initialize() override;
    void shutdown() override;
    QString moduleName() const override { return "PP Filters Editor"; }
    QString moduleId() const override { return "ppFiltersEditor"; }
    int getModulePriority() const override { return 35; }

    void importFile(const QString& filePath) override;
    void exportFile(const QString& filePath) override;

    QJsonObject serializeProject() const override;
    void deserializeProject(const QJsonObject& data) override;
};

} // namespace ks
