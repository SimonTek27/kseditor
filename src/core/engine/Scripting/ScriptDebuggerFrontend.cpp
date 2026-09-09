#include "ScriptDebugger.h"
#include <QFileInfo>
#include <QDir>
#include <QTimer>

namespace ks {
namespace scripting {

// ─── DebuggerFrontend ─────────────────────────────────────────────────────

DebuggerFrontend::DebuggerFrontend(QObject* parent) : QObject(parent)
{
}

DebuggerFrontend::~DebuggerFrontend() = default;

void DebuggerFrontend::setBackend(DebugSession* backend)
{
    if (m_backend) {
        disconnect(m_backend, nullptr, this, nullptr);
    }
    m_backend = backend;
    if (m_backend) {
        connect(m_backend, &DebugSession::stateChanged, this, &DebuggerFrontend::onBackendStateChanged);
        connect(m_backend, &DebugSession::stopped, this, &DebuggerFrontend::onBackendStopped);
        connect(m_backend, &DebugSession::breakpointHit, this, &DebuggerFrontend::onBackendBreakpointHit);
        connect(m_backend, &DebugSession::watchChanged, this, &DebuggerFrontend::onBackendWatchChanged);
        connect(m_backend, &DebugSession::stackUpdated, this, &DebuggerFrontend::onBackendStackUpdated);
        connect(m_backend, &DebugSession::variablesUpdated, this, &DebuggerFrontend::onBackendVariablesUpdated);
        connect(m_backend, &DebugSession::outputReceived, this, &DebuggerFrontend::onBackendOutput);
    }
}

DebugSession* DebuggerFrontend::backend() const
{
    return m_backend;
}

bool DebuggerFrontend::startDebugging(const QString& script, DebugSession::Language lang)
{
    if (!m_backend) return false;
    bool ok = false;
    if (lang == DebugSession::Language::Lua) {
        m_backend = new LuaDebugBackend(this);
        ok = m_backend->launch(script);
    } else if (lang == DebugSession::Language::Python) {
        m_backend = new PythonDebugBackend(this);
        ok = m_backend->launch(script);
    } else {
        ok = m_backend->launch(script);
    }
    setBackend(m_backend);
    m_currentFile = script;
    m_currentState = m_backend->state();
    return ok;
}

bool DebuggerFrontend::attachToProcess(int pid, DebugSession::Language lang)
{
    if (!m_backend) {
        if (lang == DebugSession::Language::Lua) {
            m_backend = new LuaDebugBackend(this);
        } else if (lang == DebugSession::Language::Python) {
            m_backend = new PythonDebugBackend(this);
        } else {
            return false;
        }
        setBackend(m_backend);
    }
    bool ok = m_backend->attach(pid);
    if (ok) m_currentState = m_backend->state();
    return ok;
}

void DebuggerFrontend::stopDebugging()
{
    if (m_backend) {
        m_backend->stop();
        m_currentState = DebugState::Detached;
    }
}

DebugState DebuggerFrontend::state() const
{
    return m_currentState;
}

QString DebuggerFrontend::currentFile() const
{
    return m_currentFile;
}

int DebuggerFrontend::currentLine() const
{
    return m_currentLine;
}

void DebuggerFrontend::toggleBreakpoint(const QString& file, int line)
{
    if (!m_backend) return;
    // Check if breakpoint already exists
    for (const auto& bp : m_backend->breakpoints()) {
        if (bp.file == file && bp.line == line) {
            m_backend->removeBreakpoint(bp.id);
            return;
        }
    }
    m_backend->setBreakpoint(file, line);
}

void DebuggerFrontend::setBreakpointCondition(const QString& file, int line, const QString& condition)
{
    if (!m_backend) return;
    for (const auto& bp : m_backend->breakpoints()) {
        if (bp.file == file && bp.line == line) {
            m_backend->setBreakpointCondition(bp.id, condition);
            return;
        }
    }
}

QVector<DebugBreakpoint> DebuggerFrontend::breakpointsForFile(const QString& file) const
{
    QVector<DebugBreakpoint> result;
    if (!m_backend) return result;
    for (const auto& bp : m_backend->breakpoints()) {
        if (bp.file == file) {
            DebugBreakpoint dbp;
            dbp.file = bp.file;
            dbp.line = bp.line;
            dbp.enabled = bp.enabled;
            dbp.condition = bp.condition;
            result.append(dbp);
        }
    }
    return result;
}

QString DebuggerFrontend::addWatch(const QString& expression)
{
    if (!m_backend) return QString();
    QUuid id = m_backend->addWatch(expression);
    return id.toString(QUuid::WithoutBraces);
}

void DebuggerFrontend::removeWatch(const QString& watchId)
{
    if (!m_backend) return;
    m_backend->removeWatch(QUuid(watchId));
}

void DebuggerFrontend::refreshWatches()
{
    if (m_backend) m_backend->refreshWatches();
}

QVector<DebugWatch> DebuggerFrontend::watches() const
{
    QVector<DebugWatch> result;
    if (!m_backend) return result;
    for (const auto& w : m_backend->watches()) {
        DebugWatch dw;
        dw.id = w.id.toString(QUuid::WithoutBraces);
        dw.expression = w.expression;
        dw.value = w.value.toString();
        dw.type = w.type;
        dw.error = w.error;
        result.append(dw);
    }
    return result;
}

QVector<DebugVariable> DebuggerFrontend::variables() const
{
    QVector<DebugVariable> result;
    if (!m_backend) return result;
    for (const auto& v : m_backend->variables()) {
        DebugVariable dv;
        dv.name = v.name;
        dv.value = v.value.toString();
        dv.type = v.type;
        dv.address = v.address;
        dv.children = v.childrenCount;
        result.append(dv);
    }
    return result;
}

QVector<DebugVariable> DebuggerFrontend::locals() const
{
    QVector<DebugVariable> result;
    if (!m_backend) return result;
    for (const auto& v : m_backend->locals()) {
        DebugVariable dv;
        dv.name = v.name;
        dv.value = v.value.toString();
        dv.type = v.type;
        result.append(dv);
    }
    return result;
}

QVector<DebugVariable> DebuggerFrontend::globals() const
{
    QVector<DebugVariable> result;
    if (!m_backend) return result;
    for (const auto& v : m_backend->globals()) {
        DebugVariable dv;
        dv.name = v.name;
        dv.value = v.value.toString();
        dv.type = v.type;
        result.append(dv);
    }
    return result;
}

QVector<DebugStackFrame> DebuggerFrontend::callStack() const
{
    QVector<DebugStackFrame> result;
    if (!m_backend) return result;
    for (const auto& sf : m_backend->callStack()) {
        DebugStackFrame dsf;
        dsf.level = sf.level;
        dsf.function = sf.function;
        dsf.file = sf.file;
        dsf.line = sf.line;
        result.append(dsf);
    }
    return result;
}

QVariant DebuggerFrontend::evaluate(const QString& expression)
{
    if (!m_backend) return QVariant();
    return m_backend->evaluate(expression);
}

QVariant DebuggerFrontend::evaluateSelection(const QString& text)
{
    if (!m_backend) return QVariant();
    return m_backend->evaluate(text);
}

void DebuggerFrontend::onBackendStateChanged(DebugState state)
{
    m_currentState = state;
    emit stateChanged(state);
}

void DebuggerFrontend::onBackendStopped(const QString& reason)
{
    m_currentState = DebugState::Stopped;
    emit stopped(reason);
}

void DebuggerFrontend::onBackendBreakpointHit(const QUuid& id, int line, const QString& file)
{
    m_currentLine = line;
    m_currentFile = file;
    m_currentState = DebugState::Paused;
    emit breakpointHit(file, line);
}

void DebuggerFrontend::onBackendWatchChanged(const QString& watchId)
{
    emit watchChanged(watchId);
}

void DebuggerFrontend::onBackendStackUpdated()
{
    emit callStackUpdated();
}

void DebuggerFrontend::onBackendVariablesUpdated()
{
    emit variablesUpdated();
}

void DebuggerFrontend::onBackendOutput(const QString& text, bool isError)
{
    emit outputReceived(text, isError);
}

} // namespace scripting
} // namespace ks