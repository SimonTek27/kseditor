#include "DebuggerCore.h"
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QProcess>
#include <QThread>
#include <QTimer>

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
    m_targetPid = processId;
    m_attached = true;
    m_state = State::Running;
    emit stateChanged(m_state);
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
        m_process->kill(); // On Windows we can't easily pause
    }
    m_state = State::Paused;
    emit stateChanged(m_state);
}

void Debugger::resume()
{
    m_state = State::Running;
    emit stateChanged(m_state);
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
    // Store for evaluation; actual evaluation requires process interaction
    m_lastExpression = expression;
}

void Debugger::watch(const QString& expression, const QString& name)
{
    WatchVariable::instance()->addWatch(name, expression);
}

void Debugger::getCallStack(QVector<QString>& callStack) const
{
    callStack.clear();
    // On Windows, call stack retrieval requires reading debuggee memory
    // via ReadProcessMemory or DBGHELP. Not implemented in QProcess mode.
}

void Debugger::getLocalVariables(QVector<WatchVariable::Variable>& variables) const
{
    variables = WatchVariable::instance()->getWatches();
}

} // namespace ks
