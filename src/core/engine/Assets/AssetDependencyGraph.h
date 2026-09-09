#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QJsonObject>
#include <functional>

namespace ks {

enum class DepAssetType {
    Unknown = 0,
    Texture,
    Material,
    Mesh,
    Animation,
    Audio,
    Config,
    Script,
    Shader
};

struct AssetInfo {
    QString id;
    QString name;
    DepAssetType type = DepAssetType::Unknown;
    QString path;
};

class AssetDependencyGraph : public QObject
{
    Q_OBJECT

public:
    explicit AssetDependencyGraph(QObject* parent = nullptr);
    ~AssetDependencyGraph() override;

    void addAsset(const QString& assetId, const AssetInfo& info);
    void removeAsset(const QString& assetId);
    void addDependency(const QString& fromAsset, const QString& toAsset);
    void removeDependency(const QString& fromAsset, const QString& toAsset);
    bool hasDependency(const QString& fromAsset, const QString& toAsset) const;

    QSet<QString> getDependencies(const QString& assetId) const;
    QSet<QString> getDependents(const QString& assetId) const;

    QVector<QString> getLoadOrder() const;
    QVector<QString> getUnloadOrder() const;
    bool validate() const;
    QVector<QVector<QString>> getCycles() const;

    QVector<QString> getTransitiveDependencies(const QString& assetId) const;
    QVector<QString> getTransitiveDependents(const QString& assetId) const;

    int getDepth(const QString& assetId) const;
    QVector<QString> getAssetsAtDepth(int depth) const;
    int getMaxDepth() const;

    void rebuild();

    bool canLoad(const QString& assetId) const;
    QVector<QString> getLoadingBatch(int batchSize) const;

    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& obj);

    void clear();

    void setAssetInfo(const QString& assetId, const AssetInfo& info);
    AssetInfo getAssetInfo(const QString& assetId) const;

signals:
    void assetAdded(const QString& assetId);
    void assetRemoved(const QString& assetId);
    void dependencyAdded(const QString& fromId, const QString& toId);
    void dependencyRemoved(const QString& fromId, const QString& toId);
    void graphChanged();

private:
    QMap<QString, AssetInfo> m_assets;
    QMap<QString, QSet<QString>> m_adjacency;
    QMap<QString, QSet<QString>> m_reverseAdjacency;
    QVector<QString> m_loadOrder;
    QVector<QVector<QString>> m_cycles;
    QMap<QString, int> m_depths;
    bool m_dirty = false;
};

} // namespace ks
