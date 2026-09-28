#pragma once
#include <string>
#include <cstdint>
namespace ks { namespace engine { namespace network {
struct NetworkConfig {
    std::string host = "127.0.0.1";
    uint16_t port = 9600;
    int maxClients = 16;
};
}}} // namespace
