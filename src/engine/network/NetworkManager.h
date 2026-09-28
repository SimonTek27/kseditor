#pragma once
#include <string>
#include <cstdint>
#include <functional>
namespace ks { namespace engine { namespace network {
class NetworkManager {
public:
    static NetworkManager& instance() { static NetworkManager s; return s; }
    bool initialize() { return true; }
    void shutdown() {}
    bool startServer(uint16_t /*port*/) { return false; }
    bool connect(const std::string& /*host*/, uint16_t /*port*/) { return false; }
    void update(float /*dt*/) {}
    std::function<void()> onConnected;
};
}}} // namespace
