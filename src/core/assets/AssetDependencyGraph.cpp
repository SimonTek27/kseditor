#include "AssetDependencyGraph.h"
#include <QDebug>
#include <QJsonArray>
#include <QQueue>
#include <QSet>
#include <QStack>

namespace ks {

AssetDependencyGraph::AssetDependencyGraph(QObject* parent)
    : QObject(parent)
{
}

AssetDependencyGraph::~AssetDependencyGraph()
{
}

void AssetDependencyGraph::addAsset(const QString& assetId, const AssetInfo& info)
{
    if (m_assets.contains(assetId)) return;
    m_assets[assetId] = info;
    m_adjacency[assetId] = QSet<QString>();
    m_reverseAdjacency[assetId] = QSet<QString>();
    m_dirty = true;
}

void AssetDependencyGraph::removeAsset(const QString& assetId)
{
    if (!m_assets.contains(assetId)) return;
    
    // Remove all edges
    for (const QString& dep : m_adjacency[assetId]) {
        m_reverseAdjacency[dep].remove(assetId);
    }
    for (const QString& rev : m_reverseAdjacency[assetId]) {
        m_adjacency[rev].remove(assetId);
    }
    
    m_adjacency.remove(assetId);
    m_reverseAdjacency.remove(assetId);
    m_assets.remove(assetId);
    m_dirty = true;
}

void AssetDependencyGraph::addDependency(const QString& fromAsset, const QString& toAsset)
{
    if (!m_assets.contains(fromAsset) || !m_assets.contains(toAsset)) {
        qWarning() << "AssetDependencyGraph: Asset not found" << fromAsset << toAsset;
        return;
    }
    
    if (fromAsset == toAsset) return; // No self-dependencies
    
    m_adjacency[fromAsset].insert(toAsset);
    m_reverseAdjacency[toAsset].insert(fromAsset);
    m_dirty = true;
}

void AssetDependencyGraph::removeDependency(const QString& fromAsset, const QString& toAsset)
{
    m_adjacency[fromAsset].remove(toAsset);
    m_reverseAdjacency[toAsset].remove(fromAsset);
    m_dirty = true;
}

bool AssetDependencyGraph::hasDependency(const QString& fromAsset, const QString& toAsset) const
{
    return m_adjacency.value(fromAsset).contains(toAsset);
}

QSet<QString> AssetDependencyGraph::getDependencies(const QString& assetId) const
{
    return m_adjacency.value(assetId);
}

QSet<QString> AssetDependencyGraph::getDependents(const QString& assetId) const
{
    return m_reverseAdjacency.value(assetId);
}

QVector<QString> AssetDependencyGraph::getLoadOrder() const
{
    if (m_dirty) const_cast<AssetDependencyGraph*>(this)->rebuild();
    return m_loadOrder;
}

QVector<QString> AssetDependencyGraph::getUnloadOrder() const
{
    if (m_dirty) const_cast<AssetDependencyGraph*>(this)->rebuild();
    QVector<QString> unload = m_loadOrder;
    std::reverse(unload.begin(), unload.end());
    return unload;
}

bool AssetDependencyGraph::validate() const
{
    if (m_dirty) const_cast<AssetDependencyGraph*>(this)->rebuild();
    return m_cycles.isEmpty();
}

QVector<QVector<QString>> AssetDependencyGraph::getCycles() const
{
    if (m_dirty) const_cast<AssetDependencyGraph*>(this)->rebuild();
    return m_cycles;
}

QVector<QString> AssetDependencyGraph::getTransitiveDependencies(const QString& assetId) const
{
    QVector<QString> result;
    QSet<QString> visited;
    QQueue<QString> queue;
    
    queue.enqueue(assetId);
    visited.insert(assetId);
    
    while (!queue.isEmpty()) {
        QString current = queue.dequeue();
        for (const QString& dep : m_adjacency.value(current)) {
            if (!visited.contains(dep)) {
                visited.insert(dep);
                result.append(dep);
                queue.enqueue(dep);
            }
        }
    }
    
    return result;
}

QVector<QString> AssetDependencyGraph::getTransitiveDependents(const QString& assetId) const
{
    QVector<QString> result;
    QSet<QString> visited;
    QQueue<QString> queue;
    
    queue.enqueue(assetId);
    visited.insert(assetId);
    
    while (!queue.isEmpty()) {
        QString current = queue.dequeue();
        for (const QString& dep : m_reverseAdjacency.value(current)) {
            if (!visited.contains(dep)) {
                visited.insert(dep);
                result.append(dep);
                queue.enqueue(dep);
            }
        }
    }
    
    return result;
}

int AssetDependencyGraph::getDepth(const QString& assetId) const
{
    if (m_dirty) const_cast<AssetDependencyGraph*>(this)->rebuild();
    return m_depths.value(assetId, -1);
}

QVector<QString> AssetDependencyGraph::getAssetsAtDepth(int depth) const
{
    if (m_dirty) const_cast<AssetDependencyGraph*>(this)->rebuild();
    QVector<QString> result;
    for (auto it = m_depths.constBegin(); it != m_depths.constEnd(); ++it) {
        if (it.value() == depth) result.append(it.key());
    }
    return result;
}

int AssetDependencyGraph::getMaxDepth() const
{
    if (m_dirty) const_cast<AssetDependencyGraph*>(this)->rebuild();
    int maxD = 0;
    for (int d : m_depths) if (d > maxD) maxD = d;
    return maxD;
}

void AssetDependencyGraph::rebuild()
{
    // Topological sort (Kahn's algorithm)
    QMap<QString, int> inDegree;
    QMap<QString, QSet<QString>> adj = m_adjacency;
    
    for (const QString& asset : m_assets.keys()) {
        inDegree[asset] = 0;
    }
    for (const QString& from : m_assets.keys()) {
        for (const QString& to : adj[from]) {
            inDegree[to]++;
        }
    }
    
    QQueue<QString> queue;
    for (auto it = inDegree.constBegin(); it != inDegree.constEnd(); ++it) {
        if (it.value() == 0) queue.enqueue(it.key());
    }
    
    m_loadOrder.clear();
    m_cycles.clear();
    m_depths.clear();
    
    while (!queue.isEmpty()) {
        QString current = queue.dequeue();
        m_loadOrder.append(current);
        
        for (const QString& neighbor : adj[current]) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.enqueue(neighbor);
            }
        }
    }
    
    // Check for cycles
    if (m_loadOrder.size() != m_assets.size()) {
        // Find cycles using DFS
        QSet<QString> visited;
        QSet<QString> recStack;
        QVector<QString> path;
        
        std::function<void(const QString&)> dfs = [&](const QString& node) {
            visited.insert(node);
            recStack.insert(node);
            path.append(node);
            
            for (const QString& neighbor : adj[node]) {
                if (!visited.contains(neighbor)) {
                    dfs(neighbor);
                } else if (recStack.contains(neighbor)) {
                    // Found cycle
                    int idx = path.indexOf(neighbor);
                    if (idx >= 0) {
                        QVector<QString> cycle = path.mid(idx);
                        cycle.append(neighbor);
                        m_cycles.append(cycle);
                    }
                }
            }
            
            recStack.remove(node);
            path.removeLast();
        };
        
        for (const QString& asset : m_assets.keys()) {
            if (!visited.contains(asset)) {
                dfs(asset);
            }
        }
    }
    
    // Compute depths
    m_depths.clear();
    for (const QString& asset : m_loadOrder) {
        int maxDepDepth = -1;
        for (const QString& dep : m_reverseAdjacency[asset]) {
            if (m_depths.contains(dep)) {
                maxDepDepth = qMax(maxDepDepth, m_depths[dep]);
            }
        }
        m_depths[asset] = maxDepDepth + 1;
    }
    
    m_dirty = false;
}

bool AssetDependencyGraph::canLoad(const QString& assetId) const
{
    if (m_dirty) const_cast<AssetDependencyGraph*>(this)->rebuild();
    
    // Can load if all dependencies are already loaded
    for (const QString& dep : m_adjacency.value(assetId)) {
        if (!m_loadOrder.contains(dep)) return false;
    }
    return true;
}

QVector<QString> AssetDependencyGraph::getLoadingBatch(int batchSize) const
{
    if (m_dirty) const_cast<AssetDependencyGraph*>(this)->rebuild();
    
    QVector<QString> batch;
    for (const QString& asset : m_loadOrder) {
        if (canLoad(asset)) {
            batch.append(asset);
            if (batch.size() >= batchSize) break;
        }
    }
    return batch;
}

QJsonObject AssetDependencyGraph::toJson() const
{
    QJsonObject obj;
    
    QJsonArray assetsArr;
    for (auto it = m_assets.constBegin(); it != m_assets.constEnd(); ++it) {
        QJsonObject a;
        a["id"] = it.key();
        a["name"] = it.value().name;
        a["type"] = static_cast<int>(it.value().type);
        a["path"] = it.value().path;
        assetsArr.append(a);
    }
    obj["assets"] = assetsArr;
    
    QJsonArray edgesArr;
    for (auto it = m_adjacency.constBegin(); it != m_adjacency.constEnd(); ++it) {
        for (const QString& to : it.value()) {
            QJsonObject e;
            e["from"] = it.key();
            e["to"] = to;
            edgesArr.append(e);
        }
    }
    obj["dependencies"] = edgesArr;
    
    if (!m_cycles.isEmpty()) {
        QJsonArray cyclesArr;
        for (const auto& cycle : m_cycles) {
            QJsonArray c;
            for (const QString& node : cycle) c.append(node);
            cyclesArr.append(c);
        }
        obj["cycles"] = cyclesArr;
    }
    
    return obj;
}

bool AssetDependencyGraph::fromJson(const QJsonObject& obj)
{
    clear();
    
    for (const auto& v : obj["assets"].toArray()) {
        QJsonObject a = v.toObject();
        AssetInfo info;
        info.id = a["id"].toString();
        info.name = a["name"].toString();
        info.type = static_cast<DepAssetType>(a["type"].toInt());
        info.path = a["path"].toString();
        addAsset(info.id, info);
    }
    
    for (const auto& v : obj["dependencies"].toArray()) {
        QJsonObject e = v.toObject();
        addDependency(e["from"].toString(), e["to"].toString());
    }
    
    rebuild();
    return true;
}

void AssetDependencyGraph::clear()
{
    m_assets.clear();
    m_adjacency.clear();
    m_reverseAdjacency.clear();
    m_loadOrder.clear();
    m_cycles.clear();
    m_depths.clear();
    m_dirty = false;
}

void AssetDependencyGraph::setAssetInfo(const QString& assetId, const AssetInfo& info)
{
    if (m_assets.contains(assetId)) {
        m_assets[assetId] = info;
    }
}

AssetInfo AssetDependencyGraph::getAssetInfo(const QString& assetId) const
{
    return m_assets.value(assetId);
}

} // namespace ks