#include "ChatTransport.h"

#include "../sys/LogManager.h"

namespace ks::net {

namespace {

const char* kNoServerService =
    "no WebSocket server transport is installed in this build; "
    "call ks::net::setHostServices() from the host application";

const char* kNoClientService =
    "no WebSocket client transport is installed in this build; "
    "call ks::net::setHostServices() from the host application";

class NullServerTransport : public IServerTransport {
public:
    bool listen(std::uint16_t, std::string& errorOut) override {
        errorOut = kNoServerService;
        LOG_WARNING("ChatTransport", kNoServerService);
        return false;
    }

    void close() override {}
    void sendText(ConnId, const std::string&) override {}
    void closeConnection(ConnId) override {}
};

class NullClientTransport : public IClientTransport {
public:
    void open(const std::string&, std::uint16_t, const std::string&) override {
        LOG_WARNING("ChatTransport", kNoClientService);
        if (onError) onError(kNoClientService);
    }

    void close() override {}
    void sendText(const std::string&) override {}
    bool isOpen() const override { return false; }
};

HostServices& services() {
    static HostServices instance;
    return instance;
}

} // namespace

void setHostServices(HostServices newServices) {
    services() = std::move(newServices);
}

void resetHostServices() {
    services() = HostServices{};
}

const HostServices& hostServices() {
    return services();
}

std::unique_ptr<IServerTransport> makeServerTransport() {
    auto& s = services();
    if (s.createServerTransport) return s.createServerTransport();
    return std::make_unique<NullServerTransport>();
}

std::unique_ptr<IClientTransport> makeClientTransport() {
    auto& s = services();
    if (s.createClientTransport) return s.createClientTransport();
    return std::make_unique<NullClientTransport>();
}

std::uint64_t startTimer(int delayMs, std::function<void()> fn) {
    auto& s = services();
    if (!s.startTimer || !fn) return 0;
    return s.startTimer(delayMs, std::move(fn));
}

void killTimer(std::uint64_t token) {
    auto& s = services();
    if (s.killTimer && token != 0) s.killTimer(token);
}

void RepeatingTimer::start(int intervalMs, std::function<void()> fn) {
    stop();
    m_intervalMs = intervalMs;
    m_fn = std::move(fn);
    m_active = true;
    arm();
}

void RepeatingTimer::stop() {
    m_active = false;
    if (m_token != 0) {
        killTimer(m_token);
        m_token = 0;
    }
}

void RepeatingTimer::arm() {
    if (!m_active) return;
    std::function<void()> fn = m_fn;
    m_token = ks::net::startTimer(m_intervalMs, [this, fn]() {
        m_token = 0;
        if (fn) fn();
        if (m_active) arm();
    });
}

} // namespace ks::net
