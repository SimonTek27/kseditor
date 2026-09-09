#pragma once
#include "../EngineModule.h"
#include "NetworkConfig.h"
#include <QObject>

namespace ks::engine::network {

class NetSystem : public QObject, public EngineModule {
    Q_OBJECT
public:
    static NetSystem& instance(){ static NetSystem s; return s; }
    QString moduleName() const override { return "NetSystem"; }
    QString moduleId() const override { return "ks.net"; }
    bool initialize() override { m_initialized=true; return true; }
    void shutdown() override { m_initialized=false; }
    bool isServer() const { return m_isServer; }
    bool isConnected() const { return m_connected; }
    void startServer(uint16_t port){ Q_UNUSED(port); m_isServer=true; m_connected=true; }
    void connectTo(const QString& addr, uint16_t port){ Q_UNUSED(addr); Q_UNUSED(port); m_connected=true; }
    void disconnect(){ m_connected=false; m_isServer=false; }
    void tick(double dt){ Q_UNUSED(dt); }

private:
    bool m_connected=false, m_isServer=false;
};

} // namespace ks::engine::network
