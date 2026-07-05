#pragma once
#include <QObject>
#include <QString>
#include <QMap>
#include <QList>

namespace ks {

struct PhysicsAsset {
    QString id;
    QString name;
    QString type;
    QString filePath;
};

class phys_Manager : public QObject {
    Q_OBJECT

public:
    static phys_Manager* instance();
    
    void loadAsset(const QString& path);
    void unloadAsset(const QString& id);
    QList<PhysicsAsset> getLoadedAssets() const;
    
    PhysicsAsset* getAsset(const QString& id);

signals:
    void assetLoaded(const QString& id);
    void assetUnloaded(const QString& id);

private:
    explicit phys_Manager(QObject* parent = nullptr);
    static phys_Manager* s_instance;
    
    QMap<QString, PhysicsAsset> m_assets;
    int m_nextId = 1;
};

}