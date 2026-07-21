#include "DebuggerCore.h"
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#pragma comment(lib, "psapi.lib")
#endif

namespace ks {

// ─── BreakpointManager ────────────────────────────────────────────────────────

BreakpointManager* BreakpointManager::s_instance = nullptr;

BreakpointManager* BreakpointManager::instance()
{
    if (!s_instance) s_instance = new BreakpointManager();
    return s_instance;
}

BreakpointManager::BreakpointManager(QObject* parent) : QObject(parent) {}
BreakpointManager::~BreakpointManager() { s_instance = nullptr; }

QString BreakpointManager::addBreakpoint(const QString& type, const QString& target,
                                          const QString& condition)
{
    Breakpoint bp;
    bp.id        = QString::number(++m_nextId);
    bp.type      = type;
    bp.target    = target;
    bp.condition = condition;
    bp.enabled   = true;
    m_breakpoints.insert(bp.id, bp);
    emit breakpointAdded(bp.id);
    return bp.id;
}

void BreakpointManager::removeBreakpoint(const QString& id)
{
    m_breakpoints.remove(id);
    emit breakpointRemoved(id);
}

void BreakpointManager::enableBreakpoint(const QString& id, bool enabled)
{
    if (m_breakpoints.contains(id)) {
        m_breakpoints[id].enabled = enabled;
        emit breakpointEnabled(id, enabled);
    }
}

bool BreakpointManager::isBreakpointEnabled(const QString& id) const
{
    if (m_breakpoints.contains(id)) {
        return m_breakpoints[id].enabled;
    }
    return false;
}

void BreakpointManager::setBreakpointCondition(const QString& id, const QString& condition)
{
    if (m_breakpoints.contains(id)) {
        m_breakpoints[id].condition = condition;
    }
}

QString BreakpointManager::getBreakpointCondition(const QString& id) const
{
    if (m_breakpoints.contains(id)) {
        return m_breakpoints[id].condition;
    }
    return QString();
}

BreakpointManager::Breakpoint BreakpointManager::getBreakpoint(const QString& id) const
{
    return m_breakpoints.value(id);
}

bool BreakpointManager::check(const QString& type, const QString& target,
                               const QJsonObject& ctx)
{
    for (const auto& bp : m_breakpoints) {
        if (!bp.enabled) continue;
        if (bp.type != type && !bp.type.isEmpty()) continue;
        if (bp.target != target && !bp.target.isEmpty()) continue;
        emit breakpointHit(bp.id, type, target, ctx);
        return true;
    }
    return false;
}

// ─── WatchVariable ────────────────────────────────────────────────────────────

WatchVariable* WatchVariable::s_instance = nullptr;

WatchVariable* WatchVariable::instance()
{
    if (!s_instance) s_instance = new WatchVariable();
    return s_instance;
}

WatchVariable::WatchVariable(QObject* parent) : QObject(parent) {}
WatchVariable::~WatchVariable() { s_instance = nullptr; }

void WatchVariable::addWatch(const QString& name, const QString& expression)
{
    Variable var;
    var.id = QString::number(m_watches.size() + 1);
    var.name = name;
    var.value = "0";
    var.type = "unknown";
    m_watches.insert(var.id, var);
    m_expressions.insert(var.id, expression);
}

void WatchVariable::removeWatch(const QString& watchId)
{
    m_watches.remove(watchId);
    m_expressions.remove(watchId);
}

void WatchVariable::refreshWatches()
{
#ifdef Q_OS_WIN
    Debugger* dbg = Debugger::instance();
    if (!dbg->isAttached()) {
        return;
    }

    qint64 pid = 0;
    // Get the target process handle
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, static_cast<DWORD>(pid));
    if (hProcess) {
        // For each watch, attempt to read memory at the expression address
        // A full implementation would parse expressions and resolve addresses
        for (auto it = m_watches.begin(); it != m_watches.end(); ++it) {
            // Update variable type based on expression analysis
            const QString& expr = m_expressions[it.key()];
            if (!expr.isEmpty()) {
                it.value().type = "evaluated";
            }
        }
        CloseHandle(hProcess);
    }
#endif

    emit watchesRefreshed();
}

void WatchVariable::setMaxWatches(int max)
{
    m_maxWatches = qMax(1, max);
}

void WatchVariable::setAutoRefresh(bool enabled)
{
    m_autoRefresh = enabled;
}

void WatchVariable::setRefreshInterval(int ms)
{
    m_refreshInterval = qMax(100, ms);
}

// ─── Debugger ─────────────────────────────────────────────────────────────────

Debugger* Debugger::s_instance = nullptr;

Debugger* Debugger::instance()
{
    if (!s_instance) s_instance = new Debugger();
    return s_instance;
}

Debugger::Debugger(QObject* parent) : QObject(parent) {}
Debugger::~Debugger()
{
    stop();
    s_instance = nullptr;
}

void Debugger::attachToProcess(qint64 processId)
{
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        static_cast<DWORD>(processId)
    );

    if (!hProcess) {
        qWarning() << "[Debugger] Failed to open process" << processId
                    << "- error:" << GetLastError();
        return;
    }

    // Verify the process is valid by reading a basic property
    DWORD exitCode = 0;
    if (!GetExitCodeProcess(hProcess, &exitCode)) {
        qWarning() << "[Debugger] Invalid process handle for PID" << processId;
        CloseHandle(hProcess);
        return;
    }

    if (exitCode != STILL_ACTIVE) {
        qWarning() << "[Debugger] Process" << processId << "has already exited";
        CloseHandle(hProcess);
        return;
    }

    CloseHandle(hProcess);
#endif

    m_targetPid = processId;
    m_attached = true;
    m_state = State::Running;
    emit stateChanged(m_state);
    qDebug() << "[Debugger] Attached to process" << processId;
}

void Debugger::detach()
{
    m_targetPid = 0;
    m_attached = false;
    m_state = State::Stopped;
    emit stateChanged(m_state);
}

void Debugger::start(const QString& executable, const QStringList& arguments)
{
    if (m_process) {
        stop();
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    m_process->setProgram(executable);
    m_process->setArguments(arguments);

    connect(m_process, &QProcess::started, this, [this]() {
        m_targetPid = m_process->processId();
        m_attached = true;
        m_state = State::Running;
        emit stateChanged(m_state);
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        Q_UNUSED(status);
        m_targetPid = 0;
        m_attached = false;
        m_state = State::Stopped;
        emit stopped(QString("Process exited with code %1").arg(exitCode));
        m_process->deleteLater();
        m_process = nullptr;
    });

    m_process->start();
    if (!m_process->waitForStarted(10000)) {
        m_state = State::Stopped;
        emit stopped("Failed to start process: " + m_process->errorString());
        delete m_process;
        m_process = nullptr;
    }
}

void Debugger::stop()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(5000);
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_targetPid = 0;
    m_attached = false;
    m_state = State::Stopped;
    emit stateChanged(m_state);
}

void Debugger::pause()
{
    if (m_process) {
#ifdef Q_OS_WIN
        // Suspend all threads in the process
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te;
            te.dwSize = sizeof(THREADENTRY32);
            if (Thread32First(hSnapshot, &te)) {
                do {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                    if (hThread) {
                        SuspendThread(hThread);
                        CloseHandle(hThread);
                    }
                } while (Thread32Next(hSnapshot, &te));
            }
            CloseHandle(hSnapshot);
        }
#else
        m_process->kill();
#endif
    }
    m_state = State::Paused;
    emit stateChanged(m_state);
    qDebug() << "[Debugger] Paused";
}

void Debugger::resume()
{
    if (m_process && m_state == State::Paused) {
#ifdef Q_OS_WIN
        // Resume all threads in the process
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te;
            te.dwSize = sizeof(THREADENTRY32);
            if (Thread32First(hSnapshot, &te)) {
                do {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                    if (hThread) {
                        ResumeThread(hThread);
                        CloseHandle(hThread);
                    }
                } while (Thread32Next(hSnapshot, &te));
            }
            CloseHandle(hSnapshot);
        }
#endif
    }
    m_state = State::Running;
    emit stateChanged(m_state);
    emit resumed();
    qDebug() << "[Debugger] Resumed";
}

void Debugger::setBreakpoint(const QString& location)
{
    BreakpointManager::instance()->addBreakpoint("location", location);
}

void Debugger::removeBreakpoint(const QString& location)
{
    // Find and remove breakpoint by location target
    auto bps = BreakpointManager::instance()->getBreakpoints();
    for (const auto& bp : bps) {
        if (bp.target == location) {
            BreakpointManager::instance()->removeBreakpoint(bp.id);
            return;
        }
    }
}

void Debugger::evaluate(const QString& expression)
{
    m_lastExpression = expression;

    if (m_state != State::Paused || !m_attached) {
        qDebug() << "[Debugger] Cannot evaluate: not paused or not attached";
        return;
    }

#ifdef Q_OS_WIN
    // Read the target process memory to evaluate simple expressions
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, static_cast<DWORD>(m_targetPid));
    if (!hProcess) {
        qWarning() << "[Debugger] Failed to open process for evaluation";
        return;
    }

    // For now, log the expression being evaluated
    // A full implementation would parse the expression and read process memory
    qDebug() << "[Debugger] Evaluating expression:" << expression;
    CloseHandle(hProcess);
#else
    qDebug() << "[Debugger] Evaluating expression (platform not supported):" << expression;
#endif
}

void Debugger::watch(const QString& expression, const QString& name)
{
    WatchVariable::instance()->addWatch(name, expression);
}

void Debugger::getCallStack(QVector<QString>& callStack) const
{
    callStack.clear();

    if (!m_attached || m_targetPid == 0) {
        return;
    }

#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        static_cast<DWORD>(m_targetPid)
    );

    if (!hProcess) {
        qWarning() << "[Debugger] Failed to open process for call stack";
        return;
    }

    // Enumerate threads in the target process to get a stack trace
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te;
        te.dwSize = sizeof(THREADENTRY32);

        if (Thread32First(hSnapshot, &te)) {
            do {
                HANDLE hThread = OpenThread(
                    THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                    FALSE,
                    te.th32ThreadID
                );

                if (hThread) {
                    CONTEXT ctx;
                    ctx.ContextFlags = CONTEXT_FULL;
                    if (GetThreadContext(hThread, &ctx)) {
#ifdef _M_X64
                        DWORD64 stackAddrs[128];
                        USHORT frames = 0;
                        // Use StackWalk64 if available, otherwise provide basic info
                        callStack.append(QString("Thread %1 - RIP: 0x%2")
                            .arg(te.th32ThreadID)
                            .arg(ctx.Rip, 0, 16));
#else
                        DWORD stackAddrs[128];
                        USHORT frames = 0;
                        callStack.append(QString("Thread %1 - EIP: 0x%2")
                            .arg(te.th32ThreadID)
                            .arg(ctx.Eip, 0, 16));
#endif
                    }
                    CloseHandle(hThread);
                    break;  // Just get the first thread's info for now
                }
            } while (Thread32Next(hSnapshot, &te));
        }
        CloseHandle(hSnapshot);
    }

    CloseHandle(hProcess);

    if (callStack.isEmpty()) {
        callStack.append("(Call stack unavailable - attach to a running process)");
    }
#else
    callStack.append("(Call stack not implemented on this platform)");
#endif
}

void Debugger::getLocalVariables(QVector<WatchVariable::Variable>& variables) const
{
    variables = WatchVariable::instance()->getWatches();
}

} // namespace ks
