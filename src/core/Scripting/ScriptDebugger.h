#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <QUuid>
#include <functional>
#include <memory>
#include <optional>
#include <map>
#include <QThreadPool>
#include <QFileSystemWatcher>
#include <QTcpSocket>

namespace ks {
namespace scripting {

// ─── Debug Protocol ────────────────────────────────────────────────────────

struct Breakpoint {
    QUuid id;
    QString file;
    int line = -1;
    QString condition;
    bool enabled = true;
    int hitCount = 0;
    int ignoreCount = 0;
    bool logExpression = false;
    QString logMessage;
};

struct StackFrame {
    int level = 0;
    QString function;
    QString file;
    int line = -1;
    int column = -1;
    QMap<QString, QVariant> locals;
    QMap<QString, QVariant> upvalues;
};

struct Variable {
    QString name;
    QVariant value;
    QString type;
    QString address;
    bool isTable = false;
    bool isFunction = false;
    int childrenCount = 0;
    QVector<Variable> children;
    bool expanded = false;
};

struct WatchExpression {
    QUuid id;
    QString expression;
    QVariant value;
    QString error;
    QString type;
    bool enabled = true;
};

enum class DebugState {
    Detached,
    Stopped,
    Running,
    Paused,
    Stepping,
    SteppingOver,
    SteppingOut
};

enum class DebugEvent {
    BreakpointHit,
    StepComplete,
    Output,
    Error,
    ThreadStarted,
    ThreadStopped,
    VariableChanged,
    CallStackChanged
};

struct DebugBreakpoint {
    QString file;
    int line = -1;
    bool enabled = true;
    QString condition;
};

struct DebugWatch {
    QString id;
    QString expression;
    QString value;
    QString type;
    QString error;
};

struct DebugVariable {
    QString name;
    QString value;
    QString type;
    QString address;
    int children = 0;
};

struct DebugStackFrame {
    int level = 0;
    QString function;
    QString file;
    int line = -1;
};

// ─── Debug Adapter Protocol ────────────────────────────────────────────────

class DebugSession : public QObject {
    Q_OBJECT

public:
    enum class Language { Lua, Python, Unknown };
    using State = DebugState;

    explicit DebugSession(QObject* parent = nullptr);
    ~DebugSession() override;

    // Session management
    virtual bool attach(int pid, const QString& host = "localhost", int port = 8123);
    virtual bool launch(const QString& scriptPath, const QStringList& args = {});
    virtual void detach();
    virtual void stop();

    DebugState state() const;
    Language language() const;

    // Breakpoints
    QUuid setBreakpoint(const QString& file, int line, const QString& condition = QString());
    void removeBreakpoint(const QUuid& breakpointId);
    void enableBreakpoint(const QUuid& breakpointId, bool enabled);
    void setBreakpointCondition(const QUuid& breakpointId, const QString& condition);
    QVector<Breakpoint> breakpoints() const;
    Breakpoint getBreakpoint(const QUuid& id) const;

    // Execution control
    virtual void continueExecution();
    virtual void stepOver();
    virtual void stepInto();
    virtual void stepOut();
    virtual void pause();

    // Stack & variables
    QVector<StackFrame> callStack() const;
    virtual QVector<Variable> variables(int frameLevel = 0) const;
    QVector<Variable> globals() const;
    QVector<Variable> locals(int frameLevel = 0) const;

    // Evaluation
    virtual QVariant evaluate(const QString& expression, int frameLevel = 0);
    QVariant evaluateInContext(const QString& expression, const QMap<QString, QVariant>& context);
    QUuid addWatch(const QString& expression);
    void removeWatch(const QUuid& watchId);
    QVector<WatchExpression> watches() const;
    void refreshWatches();

    // Source
    QString sourceFile(int frameLevel = 0) const;
    int sourceLine(int frameLevel = 0) const;
    QStringList sourceLines(const QString& file, int startLine, int count) const;

    // Coroutine support
    QVector<QUuid> activeCoroutines() const { return m_coroutines; }
    QVector<StackFrame> coroutineStack(const QUuid& coroutineId) const;

signals:
    void stateChanged(DebugState state);
    void stopped(const QString& reason, int frameLevel = 0);
    void breakpointHit(const QUuid& breakpointId, int line, const QString& file);
    void breakpointAdded(const QUuid& breakpointId);
    void breakpointRemoved(const QUuid& breakpointId);
    void breakpointChanged(const QUuid& breakpointId);
    void outputReceived(const QString& text, bool isError = false);
    void variablesChanged(int frameLevel);
    void watchChanged(const QString& watchId);
    void callStackChanged();
    void stackUpdated();
    void variablesUpdated();
    void coroutineEvent(const QUuid& coroutineId, const QString& event);

protected:
    virtual void onStateChanged(DebugState newState);
    virtual void onBreakpointHit(const QUuid& breakpointId, int line, const QString& file);
    virtual void onOutput(const QString& text, bool isError);
    virtual void onVariablesChanged(int frameLevel);

    DebugState m_state = DebugState::Detached;
    Language m_language = Language::Unknown;
    QMap<QUuid, Breakpoint> m_breakpoints;
    QVector<StackFrame> m_callStack;
    QVector<Variable> m_globals;
    QMap<int, QVector<Variable>> m_locals;
    QMap<QUuid, WatchExpression> m_watches;
    QMap<QUuid, QVector<StackFrame>> m_coroutineStacks;
    QVector<QUuid> m_coroutines;

    // Session state
    bool m_attached = false;
    int m_debugPid = -1;
    QString m_debugHost;
    int m_debugPort = -1;
    QString m_scriptPath;
    QStringList m_scriptArgs;
};

// ─── Lua Debug Backend ────────────────────────────────────────────────────

class LuaDebugBackend : public DebugSession {
    Q_OBJECT

public:
    explicit LuaDebugBackend(QObject* parent = nullptr);
    ~LuaDebugBackend() override;

    bool attach(int pid, const QString& host = "localhost", int port = 8123) override;
    bool launch(const QString& scriptPath, const QStringList& args = {}) override;
    void detach() override;
    void stop() override;

    void continueExecution() override;
    void stepOver() override;
    void stepInto() override;
    void stepOut() override;
    void pause() override;

    QVector<Variable> variables(int frameLevel = 0) const override;
    QVariant evaluate(const QString& expression, int frameLevel = 0) override;

protected:
    void onStateChanged(DebugState newState) override;
    void onBreakpointHit(const QUuid& breakpointId, int line, const QString& file) override;
    void onOutput(const QString& text, bool isError) override;
    void refreshWatches();

private:
    struct LuaDebugConnection {
        QTcpSocket* socket = nullptr;
        bool connected = false;
        QString buffer;
    };
    std::unique_ptr<LuaDebugConnection> m_connection;

    void connectToDebugger(int port);
    void processMessages();
    void sendCommand(const QString& command, const QJsonObject& args);
    void handleResponse(const QJsonObject& response);
    void handleEvent(const QJsonObject& event);

    bool m_attached = false;
    int m_nextRequestId = 1;
    QMap<int, std::function<void(const QJsonObject&)>> m_pendingRequests;
};

// ─── Python Debug Backend (debugpy) ────────────────────────────────────────

class PythonDebugBackend : public DebugSession {
    Q_OBJECT

public:
    explicit PythonDebugBackend(QObject* parent = nullptr);
    ~PythonDebugBackend() override;

    bool attach(int pid, const QString& host = "localhost", int port = 5678) override;
    bool launch(const QString& scriptPath, const QStringList& args = {}) override;
    void detach() override;
    void stop() override;

    void continueExecution() override;
    void stepOver() override;
    void stepInto() override;
    void stepOut() override;
    void pause() override;

    QVector<Variable> variables(int frameLevel = 0) const override;
    QVariant evaluate(const QString& expression, int frameLevel = 0) override;

protected:
    void onStateChanged(DebugState newState) override;
    void onOutput(const QString& text, bool isError) override;
    void refreshWatches();

private:
    struct PyDebugConnection {
        QTcpSocket* socket = nullptr;
        bool connected = false;
        QString buffer;
    };
    std::unique_ptr<PyDebugConnection> m_connection;

    void connectToDebugger(int port);
    void processMessages();
    void sendCommand(const QString& command, const QJsonObject& args);
    void handleResponse(const QJsonObject& response);
    void handleEvent(const QJsonObject& event);

    bool m_attached = false;
    int m_nextRequestId = 1;
    QMap<int, std::function<void(const QJsonObject&)>> m_pendingRequests;
};

// ─── Debugger Frontend ─────────────────────────────────────────────────────

class DebuggerFrontend : public QObject {
    Q_OBJECT

public:
    explicit DebuggerFrontend(QObject* parent = nullptr);
    ~DebuggerFrontend() override;

    void setBackend(DebugSession* backend);
    DebugSession* backend() const;

    bool startDebugging(const QString& script, DebugSession::Language lang);
    bool attachToProcess(int pid, DebugSession::Language lang);
    void stopDebugging();

    DebugState state() const;
    QString currentFile() const;
    int currentLine() const;

    void toggleBreakpoint(const QString& file, int line);
    void setBreakpointCondition(const QString& file, int line, const QString& condition);
    QVector<DebugBreakpoint> breakpointsForFile(const QString& file) const;

    QString addWatch(const QString& expression);
    void removeWatch(const QString& watchId);
    void refreshWatches();
    QVector<DebugWatch> watches() const;

    QVector<DebugVariable> variables() const;
    QVector<DebugVariable> locals() const;
    QVector<DebugVariable> globals() const;
    QVector<DebugStackFrame> callStack() const;

    QVariant evaluate(const QString& expression);
    QVariant evaluateSelection(const QString& text);

signals:
    void stateChanged(DebugState state);
    void stopped(const QString& reason);
    void breakpointHit(const QString& file, int line);
    void watchChanged(const QString& watchId);
    void callStackUpdated();
    void variablesUpdated();
    void outputReceived(const QString& text, bool isError);

private slots:
    void onBackendStateChanged(DebugState state);
    void onBackendStopped(const QString& reason);
    void onBackendBreakpointHit(const QUuid& id, int line, const QString& file);
    void onBackendWatchChanged(const QString& watchId);
    void onBackendStackUpdated();
    void onBackendVariablesUpdated();
    void onBackendOutput(const QString& text, bool isError);

private:
    DebugSession* m_backend = nullptr;
    QString m_currentFile;
    int m_currentLine = 0;
    DebugState m_currentState = DebugState::Detached;
};

// ─── Hot Reload Manager ────────────────────────────────────────────────────

class HotReloadManager : public QObject {
    Q_OBJECT

public:
    struct ReloadConfig {
        QStringList watchPaths;
        QStringList ignorePatterns;
        int debounceMs = 500;
        bool reloadOnSave = true;
        bool preserveState = true;
        std::function<void()> onBeforeReload;
        std::function<void()> onAfterReload;
    };

    explicit HotReloadManager(QObject* parent = nullptr);
    ~HotReloadManager() override;

    void addWatchPath(const QString& path);
    void removeWatchPath(const QString& path);
    QStringList watchPaths() const { return m_config.watchPaths; }

    void setConfig(const ReloadConfig& config);
    ReloadConfig config() const;

    void triggerReload(const QString& filePath = QString());
    void cancelPendingReload();

    // Module reloading
    void registerModule(const QString& name, QObject* module);
    void unregisterModule(const QString& name);
    void reloadModule(const QString& name);

    // State preservation
    void saveModuleState(const QString& name, const QVariantMap& state);
    QVariantMap restoreModuleState(const QString& name);

signals:
    void reloadStarted(const QString& filePath);
    void reloadFinished(bool success, const QString& message);
    void moduleReloaded(const QString& moduleName);
    void fileChanged(const QString& filePath);

private:
    void setupFileWatchers();
    void onFileChanged(const QString& filePath);
    void debouncedReload();

    ReloadConfig m_config;
    QFileSystemWatcher* m_watcher = nullptr;
    QTimer* m_debounceTimer = nullptr;
    QString m_lastChangedFile;
    QMap<QString, QVariantMap> m_moduleStates;
    QMap<QString, QObject*> m_modules;
};

// ─── Async Coroutine Support ──────────────────────────────────────────────

template<typename T>
class Promise {
public:
    using ResolveFunc = std::function<void(const T&)>;
    using RejectFunc = std::function<void(const QString&)>;

    Promise() = default;
    Promise(ResolveFunc resolve, RejectFunc reject)
        : m_resolve(std::move(resolve)), m_reject(std::move(reject)) {}

    void resolve(const T& value) {
        if (m_resolve) m_resolve(value);
        m_resolved = true;
    }

    void reject(const QString& error) {
        if (m_reject) m_reject(error);
        m_rejected = true;
    }

    bool isResolved() const { return m_resolved; }
    bool isRejected() const { return m_rejected; }
    bool isPending() const { return !m_resolved && !m_rejected; }

    // Then chaining
    template<typename U>
    Promise<U> then(std::function<Promise<U>(const T&)> onResolved) {
        Promise<U> nextPromise;
        auto self = this;
        auto resolve = [self, onResolved, nextPromise = &nextPromise](const T& value) mutable {
            auto next = onResolved(value);
            next.then([nextPromise](const U& v) { nextPromise->resolve(v); })
                .catch_([nextPromise](const QString& e) { nextPromise->reject(e); });
        };
        auto reject = [nextPromise = &nextPromise](const QString& error) {
            nextPromise->reject(error);
        };
        // Store callbacks for when this promise resolves
        if (m_resolved) {
            // Already resolved, execute immediately
            auto next = onResolved(m_resolvedValue);
            next.then([nextPromise](const U& v) { nextPromise->resolve(v); })
                .catch_([nextPromise](const QString& e) { nextPromise->reject(e); });
        } else if (m_rejected) {
            nextPromise->reject(m_rejectedError);
        } else {
            m_thenCallbacks.push_back([onResolved, nextPromise](const T& value) {
                auto next = onResolved(value);
                next.then([nextPromise](const U& v) { nextPromise->resolve(v); })
                    .catch_([nextPromise](const QString& e) { nextPromise->reject(e); });
            });
            m_catchCallbacks.push_back([nextPromise](const QString& error) {
                nextPromise->reject(error);
            });
        }
        return nextPromise;
    }

    Promise<T> catch_(std::function<void(const QString&)> onRejected) {
        if (m_rejected) {
            onRejected(m_rejectedError);
        } else {
            m_catchCallbacks.push_back(onRejected);
        }
        return *this;
    }

    void finally(std::function<void()> onFinally) {
        if (m_resolved || m_rejected) {
            onFinally();
        } else {
            m_finallyCallbacks.push_back(onFinally);
        }
    }

    T m_resolvedValue;
    QString m_rejectedError;
    bool m_resolved = false;
    bool m_rejected = false;

private:
    ResolveFunc m_resolve;
    RejectFunc m_reject;
    std::vector<std::function<void(const T&)>> m_thenCallbacks;
    std::vector<std::function<void(const QString&)>> m_catchCallbacks;
    std::vector<std::function<void()>> m_finallyCallbacks;
};

// Coroutine utilities
class CoroutineManager : public QObject {
    Q_OBJECT

public:
    explicit CoroutineManager(QObject* parent = nullptr);
    ~CoroutineManager() override;

    static CoroutineManager* instance();

    // Create a coroutine from a generator function
    QUuid startCoroutine(std::function<QVariant()> generator, const QString& name = QString());

    void resumeCoroutine(const QUuid& id);
    void yieldCoroutine(const QUuid& id, const QVariant& value = QVariant());
    void stopCoroutine(const QUuid& id);
    bool isCoroutineRunning(const QUuid& id) const;

    // Async utilities
    template<typename T>
    static Promise<T> async(std::function<T()> func) {
        Promise<T> promise;
        QThreadPool::globalInstance()->start([func, promise]() mutable {
            try {
                T result = func();
                promise.resolve(result);
            } catch (const std::exception& e) {
                promise.reject(QString::fromUtf8(e.what()));
            }
        });
        return promise;
    }

    template<typename T>
    static Promise<T> delay(int ms, const T& value = T()) {
        Promise<T> promise;
        QTimer::singleShot(ms, [promise, value]() mutable {
            promise.resolve(value);
        });
        return promise;
    }

    static Promise<QVector<QVariant>> all(const QVector<Promise<QVariant>>& promises);
    static Promise<QVariant> race(const QVector<Promise<QVariant>>& promises);

signals:
    void coroutineStarted(const QUuid& id);
    void coroutineFinished(const QUuid& id);
    void coroutineError(const QUuid& id, const QString& error);
    void coroutineYielded(const QUuid& id, const QVariant& value);

private:
    enum class CoroutineState { Running, Yielded, Finished, Error };

    struct CoroutineBase {
        QUuid id;
        QString name;
        CoroutineState state = CoroutineState::Running;
        QVariant lastYieldValue;
    };

    template<typename T>
    struct CoroutineImpl : CoroutineBase {
        std::function<QVariant()> generator;
        std::optional<T> result;
        QString error;
    };

    template<>
    struct CoroutineImpl<void> : CoroutineBase {
        std::function<QVariant()> generator;
        QString error;
    };

    std::map<QUuid, std::unique_ptr<CoroutineBase>> m_coroutines;
};

} // namespace scripting
} // namespace ks