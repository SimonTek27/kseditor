#include "ScriptDebugger.h"
#include <QTcpSocket>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ks {
namespace scripting {

DebugSession::DebugSession(QObject* parent) : QObject(parent)
{
}

DebugSession::~DebugSession() = default;

bool DebugSession::attach(int pid, const QString& host, int port)
{
    if (pid <= 0) return false;
    m_debugPid = pid;
    m_debugHost = host;
    m_debugPort = port;
    m_state = DebugState::Paused;
    m_attached = true;
    emit stateChanged(m_state);
    return true;
}

bool DebugSession::launch(const QString& scriptPath, const QStringList& args)
{
    if (scriptPath.isEmpty()) return false;
    m_scriptPath = scriptPath;
    m_scriptArgs = args;
    m_state = DebugState::Running;
    m_attached = true;
    emit stateChanged(m_state);
    emit outputReceived(QStringLiteral("Debug session started: %1").arg(scriptPath));
    return true;
}

void DebugSession::detach() { stop(); }

void DebugSession::stop() { m_state = DebugState::Detached; }

DebugState DebugSession::state() const { return m_state; }

DebugSession::Language DebugSession::language() const { return m_language; }

QUuid DebugSession::setBreakpoint(const QString& file, int line, const QString& condition)
{
    Q_UNUSED(file); Q_UNUSED(line); Q_UNUSED(condition);
    QUuid id = QUuid::createUuid();
    Breakpoint bp;
    bp.id = id;
    bp.file = file;
    bp.line = line;
    bp.condition = condition;
    m_breakpoints[id] = bp;
    emit breakpointAdded(id);
    return id;
}

void DebugSession::removeBreakpoint(const QUuid& breakpointId)
{
    if (m_breakpoints.remove(breakpointId) > 0) {
        emit breakpointRemoved(breakpointId);
    }
}

void DebugSession::enableBreakpoint(const QUuid& breakpointId, bool enabled)
{
    if (m_breakpoints.contains(breakpointId)) {
        m_breakpoints[breakpointId].enabled = enabled;
        emit breakpointChanged(breakpointId);
    }
}

void DebugSession::setBreakpointCondition(const QUuid& breakpointId, const QString& condition)
{
    if (m_breakpoints.contains(breakpointId)) {
        m_breakpoints[breakpointId].condition = condition;
        emit breakpointChanged(breakpointId);
    }
}

QVector<Breakpoint> DebugSession::breakpoints() const
{
    return m_breakpoints.values();
}

Breakpoint DebugSession::getBreakpoint(const QUuid& id) const
{
    return m_breakpoints.value(id, Breakpoint());
}

void DebugSession::continueExecution() { m_state = DebugState::Running; emit stateChanged(m_state); }
void DebugSession::stepOver() { m_state = DebugState::SteppingOver; emit stateChanged(m_state); }
void DebugSession::stepInto() { m_state = DebugState::Stepping; emit stateChanged(m_state); }
void DebugSession::stepOut() { m_state = DebugState::SteppingOut; emit stateChanged(m_state); }
void DebugSession::pause() { m_state = DebugState::Paused; emit stateChanged(m_state); }

QVector<StackFrame> DebugSession::callStack() const { return m_callStack; }

QVector<Variable> DebugSession::variables(int frameLevel) const
{
    Q_UNUSED(frameLevel);
    return m_locals.value(frameLevel, QVector<Variable>());
}

QVector<Variable> DebugSession::globals() const { return m_globals; }

QVector<Variable> DebugSession::locals(int frameLevel) const
{
    Q_UNUSED(frameLevel);
    return QVector<Variable>();
}

QVariant DebugSession::evaluate(const QString& expression, int frameLevel)
{
    Q_UNUSED(frameLevel);
    if (expression.isEmpty()) return QVariant();
    // Evaluate against stored local variables at the given frame level
    QVector<Variable> frameVars = m_locals.value(frameLevel, QVector<Variable>());
    for (const auto& var : frameVars) {
        if (var.name == expression) {
            return var.value;
        }
    }
    // Try global scope
    for (const auto& var : m_globals) {
        if (var.name == expression) {
            return var.value;
        }
    }
    return QVariant();
}

QVariant DebugSession::evaluateInContext(const QString& expression, const QMap<QString, QVariant>& context)
{
    if (expression.isEmpty()) return QVariant();
    // Check the provided context first
    if (context.contains(expression)) {
        return context[expression];
    }
    // Fall back to regular evaluate
    return evaluate(expression);
}

QUuid DebugSession::addWatch(const QString& expression)
{
    Q_UNUSED(expression);
    QUuid id = QUuid::createUuid();
    WatchExpression w;
    w.id = id;
    w.expression = expression;
    m_watches[id] = w;
    return id;
}

void DebugSession::removeWatch(const QUuid& watchId) { m_watches.remove(watchId); }

QVector<WatchExpression> DebugSession::watches() const { return m_watches.values(); }

void DebugSession::refreshWatches()
{
    for (auto it = m_watches.begin(); it != m_watches.end(); ++it) {
        if (!it.value().enabled) continue;
        QVariant result = evaluate(it.value().expression);
        if (result.isValid()) {
            it.value().value = result;
            it.value().error.clear();
        } else {
            it.value().error = QStringLiteral("Evaluation returned invalid result");
        }
        emit watchChanged(it.key().toString());
    }
    emit variablesUpdated();
}

QString DebugSession::sourceFile(int frameLevel) const
{
    if (frameLevel >= 0 && frameLevel < m_callStack.size())
        return m_callStack[frameLevel].file;
    return m_scriptPath;
}

int DebugSession::sourceLine(int frameLevel) const
{
    if (frameLevel >= 0 && frameLevel < m_callStack.size())
        return m_callStack[frameLevel].line;
    return -1;
}

QStringList DebugSession::sourceLines(const QString& file, int startLine, int count) const
{
    QStringList lines;
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return lines;
    QTextStream in(&f);
    int lineNum = 1;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (lineNum >= startLine && lineNum < startLine + count) {
            lines.append(line);
        }
        ++lineNum;
    }
    f.close();
    return lines;
}

QVector<StackFrame> DebugSession::coroutineStack(const QUuid& coroutineId) const
{
    Q_UNUSED(coroutineId);
    // In a real debugger, each coroutine would have its own call stack
    // For now, return the main call stack
    return m_callStack;
}

void DebugSession::onStateChanged(DebugState newState)
{
    Q_UNUSED(newState);
    emit stateChanged(newState);
}

void DebugSession::onBreakpointHit(const QUuid& breakpointId, int line, const QString& file)
{
    Q_UNUSED(breakpointId); Q_UNUSED(line); Q_UNUSED(file);
    emit breakpointHit(breakpointId, line, file);
}

void DebugSession::onOutput(const QString& text, bool isError)
{
    Q_UNUSED(text); Q_UNUSED(isError);
    emit outputReceived(text, isError);
}

void DebugSession::onVariablesChanged(int frameLevel)
{
    Q_UNUSED(frameLevel);
    emit variablesChanged(frameLevel);
    emit variablesUpdated();
}

LuaDebugBackend::LuaDebugBackend(QObject* parent) : DebugSession(parent)
{
    m_language = Language::Lua;
}

LuaDebugBackend::~LuaDebugBackend() = default;

bool LuaDebugBackend::attach(int pid, const QString& host, int port)
{
    Q_UNUSED(pid); Q_UNUSED(host); Q_UNUSED(port);
    m_attached = true;
    m_state = DebugState::Paused;
    emit stateChanged(m_state);
    return true;
}

bool LuaDebugBackend::launch(const QString& scriptPath, const QStringList& args)
{
    Q_UNUSED(scriptPath); Q_UNUSED(args);
    m_state = DebugState::Running;
    m_attached = true;
    emit stateChanged(m_state);
    emit outputReceived(QStringLiteral("Lua debug session started for: %1").arg(scriptPath));
    return true;
}

void LuaDebugBackend::detach()
{
    m_attached = false;
    m_state = DebugState::Detached;
    emit stateChanged(m_state);
}

void LuaDebugBackend::stop()
{
    m_attached = false;
    m_state = DebugState::Stopped;
    emit stateChanged(m_state);
}

void LuaDebugBackend::continueExecution() { DebugSession::continueExecution(); }
void LuaDebugBackend::stepOver() { DebugSession::stepOver(); }
void LuaDebugBackend::stepInto() { DebugSession::stepInto(); }
void LuaDebugBackend::stepOut() { DebugSession::stepOut(); }
void LuaDebugBackend::pause() { DebugSession::pause(); }

QVector<Variable> LuaDebugBackend::variables(int frameLevel) const
{
    return DebugSession::variables(frameLevel);
}

QVariant LuaDebugBackend::evaluate(const QString& expression, int frameLevel)
{
    if (!m_connection || !m_connection->connected)
        return DebugSession::evaluate(expression, frameLevel);
    return QVariant();
}

void LuaDebugBackend::onStateChanged(DebugState newState)
{
    DebugSession::onStateChanged(newState);
}

void LuaDebugBackend::onBreakpointHit(const QUuid& breakpointId, int line, const QString& file)
{
    DebugSession::onBreakpointHit(breakpointId, line, file);
}

void LuaDebugBackend::onOutput(const QString& text, bool isError)
{
    DebugSession::onOutput(text, isError);
}

void LuaDebugBackend::refreshWatches()
{
    for (auto it = m_watches.begin(); it != m_watches.end(); ++it) {
        if (!it.value().enabled) continue;
        QVariant result = evaluate(it.value().expression);
        if (result.isValid()) {
            it.value().value = result;
            it.value().error.clear();
        } else {
            it.value().error = QStringLiteral("Evaluation returned invalid result");
        }
        emit watchChanged(it.key().toString());
    }
    emit variablesUpdated();
}

PythonDebugBackend::PythonDebugBackend(QObject* parent) : DebugSession(parent)
{
    m_language = Language::Python;
}

PythonDebugBackend::~PythonDebugBackend() = default;

bool PythonDebugBackend::attach(int pid, const QString& host, int port)
{
    Q_UNUSED(pid); Q_UNUSED(host); Q_UNUSED(port);
    m_attached = true;
    m_state = DebugState::Paused;
    emit stateChanged(m_state);
    return true;
}

bool PythonDebugBackend::launch(const QString& scriptPath, const QStringList& args)
{
    Q_UNUSED(scriptPath); Q_UNUSED(args);
    m_state = DebugState::Running;
    m_attached = true;
    emit stateChanged(m_state);
    emit outputReceived(QStringLiteral("Python debug session started for: %1").arg(scriptPath));
    return true;
}

void PythonDebugBackend::detach()
{
    m_attached = false;
    m_state = DebugState::Detached;
    emit stateChanged(m_state);
}

void PythonDebugBackend::stop()
{
    m_attached = false;
    m_state = DebugState::Stopped;
    emit stateChanged(m_state);
}

void PythonDebugBackend::continueExecution() { DebugSession::continueExecution(); }
void PythonDebugBackend::stepOver() { DebugSession::stepOver(); }
void PythonDebugBackend::stepInto() { DebugSession::stepInto(); }
void PythonDebugBackend::stepOut() { DebugSession::stepOut(); }
void PythonDebugBackend::pause() { DebugSession::pause(); }

QVector<Variable> PythonDebugBackend::variables(int frameLevel) const
{
    if (m_connection && m_connection->connected) {
        return DebugSession::variables(frameLevel);
    }
    return DebugSession::variables(frameLevel);
}

QVariant PythonDebugBackend::evaluate(const QString& expression, int frameLevel)
{
    if (!m_connection || !m_connection->connected)
        return DebugSession::evaluate(expression, frameLevel);
    return QVariant();
}

void PythonDebugBackend::onStateChanged(DebugState newState)
{
    DebugSession::onStateChanged(newState);
}

void PythonDebugBackend::onOutput(const QString& text, bool isError)
{
    DebugSession::onOutput(text, isError);
}

void PythonDebugBackend::refreshWatches()
{
    for (auto it = m_watches.begin(); it != m_watches.end(); ++it) {
        if (!it.value().enabled) continue;
        QVariant result = evaluate(it.value().expression);
        if (result.isValid()) {
            it.value().value = result;
            it.value().error.clear();
        } else {
            it.value().error = QStringLiteral("Evaluation returned invalid result");
        }
        emit watchChanged(it.key().toString());
    }
    emit variablesUpdated();
}

} // namespace scripting
} // namespace ks