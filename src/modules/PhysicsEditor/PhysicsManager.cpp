#include "PhysicsManager.h"
#include <QDebug>

namespace ks {

phys_Manager* phys_Manager::s_instance = nullptr;

phys_Manager::phys_Manager(QObject* parent)
    : QObject(parent)
{}

phys_Manager* phys_Manager::instance() {
    if (!s_instance) {
        s_instance = new phys_Manager();
    }
    return s_instance;
}

void phys_Manager::loadAsset(const QString& path) {
    PhysicsAsset asset;
    asset.id = QString::number(m_nextId++);
    asset.name = path.split('/').last();
    asset.type = "physics";
    asset.filePath = path;
    
    m_assets.insert(asset.id, asset);
    emit assetLoaded(asset.id);
    qDebug() << "Loaded physics asset:" << asset.name;
}

void phys_Manager::unloadAsset(const QString& id) {
    if (m_assets.contains(id)) {
        emit assetUnloaded(id);
        m_assets.remove(id);
    }
}

QList<PhysicsAsset> phys_Manager::getLoadedAssets() const {
    return m_assets.values();
}

PhysicsAsset* phys_Manager::getAsset(const QString& id) {
    return m_assets.contains(id) ? &m_assets[id] : nullptr;
}

}