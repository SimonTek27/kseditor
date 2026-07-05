#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <memory>
#include "LiverySystem.h"

namespace ks {

class LiveryEditorQmlBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString carPath READ carPath NOTIFY carPathChanged)
    Q_PROPERTY(QString currentSkin READ currentSkin NOTIFY currentSkinChanged)
    Q_PROPERTY(int skinCount READ skinCount NOTIFY skinListChanged)

public:
    static LiveryEditorQmlBridge* instance();

    QString carPath() const { return m_carPath; }
    QString currentSkin() const { return m_currentSkin; }
    int skinCount() const { return m_skins.size(); }

    Q_INVOKABLE bool loadCar(const QString& carPath);
    Q_INVOKABLE QVariantList getSkins();
    Q_INVOKABLE bool createSkin(const QString& name);
    Q_INVOKABLE bool deleteSkin(const QString& name);
    Q_INVOKABLE bool duplicateSkin(const QString& sourceName, const QString& destName);
    Q_INVOKABLE bool selectSkin(const QString& name);

    Q_INVOKABLE QVariantList getLayers();
    Q_INVOKABLE bool addLayer(const QVariantMap& layer);
    Q_INVOKABLE bool removeLayer(int index);
    Q_INVOKABLE bool moveLayer(int from, int to);
    Q_INVOKABLE bool updateLayer(int index, const QVariantMap& layer);

    Q_INVOKABLE bool exportSkin(const QString& outputPath);
    Q_INVOKABLE bool importSkin(const QString& importPath);

    Q_INVOKABLE bool generateLicensePlate(const QString& text, const QString& country);
    Q_INVOKABLE QStringList getSupportedCountries();

    Q_INVOKABLE QVariantMap getSkinConfig();

signals:
    void carPathChanged();
    void currentSkinChanged();
    void skinListChanged();
    void skinLoaded();

private:
    static LiveryEditorQmlBridge* s_instance;
    LiveryEditorQmlBridge(QObject* parent = nullptr) : QObject(parent) {}

    QVariantMap layerToVariant(const LiverySystem::LiveryLayer& layer) const;
    LiverySystem::LiveryLayer variantToLayer(const QVariantMap& v) const;

    QString m_carPath;
    QString m_currentSkin;
    QVector<LiverySystem::SkinInfo> m_skins;
    std::unique_ptr<LiveryManager> m_manager;
};

} // namespace ks
