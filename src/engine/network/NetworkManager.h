#pragma once
/** Minimal Qt-free network manager (callback-based). */
#include <string>
#include <cstdint>
#include <functional>
#include <vector>
#include <mutex>

namespace ks {
namespace engine {
namespace network {

class NetworkManager {
public:
    static NetworkManager& instance() {
        static NetworkManager s;
        return s;
    }

    bool initialize() {
        m_ok = true;
        return true;
    }

    void shutdown() {
        m_listening = false;
        m_connected = false;
        m_ok = false;
    }

    bool startServer(uint16_t port) {
        m_port = port;
        m_listening = true;
        m_isServer = true;
        if (onListening) onListening(port);
        return true;
    }

    bool connect(const std::string& host, uint16_t port) {
        m_host = host;
        m_port = port;
        m_connected = true;
        m_isServer = false;
        if (onConnected) onConnected();
        return true;
    }

    void disconnect() {
        m_connected = false;
        m_listening = false;
        if (onDisconnected) onDisconnected();
    }

    void update(float /*dt*/) {
        // Poll sockets when platform backend is wired.
    }

    void send(const void* data, size_t size) {
        if (!m_connected && !m_listening) return;
        if (onSend) onSend(data, size);
        ++m_bytesSent;
        m_bytesSent += size;
    }

    bool isConnected() const { return m_connected; }
    bool isListening() const { return m_listening; }
    uint16_t port() const { return m_port; }
    const std::string& host() const { return m_host; }

    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(uint16_t)> onListening;
    std::function<void(const void*, size_t)> onSend;
    std::function<void(const void*, size_t)> onReceive;

private:
    bool m_ok = false;
    bool m_listening = false;
    bool m_connected = false;
    bool m_isServer = false;
    uint16_t m_port = 0;
    std::string m_host;
    size_t m_bytesSent = 0;
};

} // namespace network
} // namespace engine
} // namespace ks
