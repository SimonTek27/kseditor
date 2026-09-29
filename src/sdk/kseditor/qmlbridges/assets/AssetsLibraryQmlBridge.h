#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace ks {

class AssetsLibraryQmlBridge : public QObject
{
    Q_OBJECT

public:
    static AssetsLibraryQmlBridge* instance();

    explicit AssetsLibraryQmlBridge(QObject* parent = nullptr);
    ~AssetsLibraryQmlBridge() override;

    Q_INVOKABLE QVariantList getAssets(const QString& category, const QString& query, const QString& sort);
    Q_INVOKABLE QVariantList getCategories();
    Q_INVOKABLE QVariantMap getStorageStats();
    Q_INVOKABLE QVariantMap getAsset(const QString& id);
    Q_INVOKABLE bool importAsset(const QString& path, const QString& options);
    Q_INVOKABLE bool exportAsset(const QString& id, const QString& path);
    Q_INVOKABLE void statusMessage(const QString& message);
    Q_INVOKABLE void openInModeler(const QString& id);
    Q_INVOKABLE bool removeAsset(const QString& id);

signals:
    void assetsChanged();
    void scanComplete();
    void assetOpenedInModeler(const QString& path);

private:
    static AssetsLibraryQmlBridge* s_instance;
};

} // namespace ks
