#pragma once

// ---------------------------------------------------------------------------
// Transport abstraction for the chat / collaboration modules.
//
// The Qt implementation used QWebSocket + QWebSocketServer + QTimer, i.e. an
// event loop owned by Qt. The engine core is Qt-free, so the socket layer and
// the delayed-execution primitives are provided by the *host* application
// through the interfaces below. Nothing in src/engine/network knows about
// WebSockets any more.
//
// A Qt-free build gets the null implementation (listen()/open() fail with a
// clear message, timers are no-ops) unless the embedding application installs
// real services with ks::net::setHostServices() at startup. The kseditor (Qt)
// application is expected to install a QWebSocket-backed pair of transports
// plus a QTimer-based scheduler -- that adapter lives on the Qt side and is
// deliberately not part of this directory.
//
// Threading contract: implementations must invoke the callbacks on the thread
// that created the transport (the host event-loop thread), which is what the
// previous Qt code assumed as well.
// ---------------------------------------------------------------------------

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace ks::net {

using ConnId = std::uint64_t;
constexpr ConnId INVALID_CONN = 0;

// Server side: accepts connections and exchanges UTF-8 text frames.
class IServerTransport {
public:
    virtual ~IServerTransport() = default;

    // Binds and starts listening. Returns false and fills errorOut on failure.
    virtual bool listen(std::uint16_t port, std::string& errorOut) = 0;
    virtual void close() = 0;

    virtual void sendText(ConnId conn, const std::string& text) = 0;
    virtual void closeConnection(ConnId conn) = 0;

    std::function<void(ConnId)> onClientConnected;
    std::function<void(ConnId)> onClientDisconnected;
    std::function<void(ConnId, const std::string&)> onTextReceived;
};

// Client side: connects to a server and exchanges UTF-8 text frames.
class IClientTransport {
public:
    virtual ~IClientTransport() = default;

    // path is the resource part of the URL ("" or "/collab").
    virtual void open(const std::string& host, std::uint16_t port, const std::string& path) = 0;
    virtual void close() = 0;
    virtual void sendText(const std::string& text) = 0;
    virtual bool isOpen() const = 0;

    std::function<void()> onOpened;
    std::function<void(const std::string& reason)> onClosed;
    std::function<void(const std::string& text)> onTextReceived;
    std::function<void(const std::string& message)> onError;
};

// Everything the engine expects from its host application.
struct HostServices {
    std::function<std::unique_ptr<IServerTransport>()> createServerTransport;
    std::function<std::unique_ptr<IClientTransport>()> createClientTransport;

    // One-shot delayed execution on the host event loop (QTimer in Qt hosts).
    // Returns a non-zero token, or 0 when no scheduler is installed.
    std::function<std::uint64_t(int delayMs, std::function<void()>)> startTimer;
    std::function<void(std::uint64_t)> killTimer;
};

void setHostServices(HostServices services);
void resetHostServices();
const HostServices& hostServices();

inline bool hasServerTransport() {
    return static_cast<bool>(hostServices().createServerTransport);
}
inline bool hasClientTransport() {
    return static_cast<bool>(hostServices().createClientTransport);
}

// Convenience wrappers; return nullptr / 0 when no service is installed.
std::unique_ptr<IServerTransport> makeServerTransport();
std::unique_ptr<IClientTransport> makeClientTransport();

// Returns a token usable with killTimer(); 0 means "nothing was scheduled".
std::uint64_t startTimer(int delayMs, std::function<void()> fn);
void killTimer(std::uint64_t token);

// Repeating timer built on the host one-shot scheduler, equivalent to
// QTimer::setInterval(t) + start(). Does nothing when no scheduler is
// installed (QTimer::start() call sites in the Qt code map onto this).
class RepeatingTimer {
public:
    RepeatingTimer() = default;
    ~RepeatingTimer() { stop(); }
    RepeatingTimer(const RepeatingTimer&) = delete;
    RepeatingTimer& operator=(const RepeatingTimer&) = delete;

    void start(int intervalMs, std::function<void()> fn);
    void stop();
    bool isActive() const { return m_token != 0; }

private:
    void arm();

    int m_intervalMs = 0;
    bool m_active = false;
    std::function<void()> m_fn;
    std::uint64_t m_token = 0;
};

} // namespace ks::net
