#include "ScriptDebugger.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

namespace ks {
namespace scripting {

static DebugWatch toDebugWatch(const WatchExpression& w) {
    DebugWatch dw;
    dw.id = w.id.toString();
    dw.expression = w.expression;
    dw.value = w.value.toString();
    dw.type = w.type;
    dw.error = w.error;
    return dw;
}

static DebugVariable toDebugVariable(const Variable& v) {
    DebugVariable dv;
    dv.name = v.name;
    dv.value = v.value.toString();
    dv.type = v.type;
    dv.address = v.address;
    dv.children = v.childrenCount;
    return dv;
}

static DebugStackFrame toDebugStackFrame(const StackFrame& sf) {
    DebugStackFrame dsf;
    dsf.level = sf.level;
    dsf.function = sf.function;
    dsf.file = sf.file;
    dsf.line = sf.line;
    return dsf;
}

static DebugVariable toDebugVariable(const Variable& v);

DebuggerFrontend::DebuggerFrontend(QObject* parent) : QObject(parent)
{
    m_backend = nullptr;
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
    
    if (lang == DebugSession::Language::Lua) {
        return m_backend->launch(script);
    } else if (lang == DebugSession::Language::Python) {
        return m_backend->launch(script);
    }
    return false;
}

bool DebuggerFrontend::attachToProcess(int pid, DebugSession::Language lang)
{
    if (!m_backend) return false;
    
    if (lang == DebugSession::Language::Lua) {
        return m_backend->attach(pid);
    } else if (lang == DebugSession::Language::Python) {
        return m_backend->attach(pid);
    }
    return false;
}

void DebuggerFrontend::stopDebugging()
{
    if (m_backend) {
        m_backend->stop();
    }
}

DebugState DebuggerFrontend::state() const
{
    return m_backend ? m_backend->state() : DebugState::Detached;
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
    if (m_backend) {
        m_backend->setBreakpoint(file, line, QString());
    }
}

void DebuggerFrontend::setBreakpointCondition(const QString& file, int line, const QString& condition)
{
    if (m_backend) {
        m_backend->setBreakpoint(file, line, condition);
    }
}

QVector<DebugBreakpoint> DebuggerFrontend::breakpointsForFile(const QString& file) const
{
    Q_UNUSED(file);
    return QVector<DebugBreakpoint>();
}

QString DebuggerFrontend::addWatch(const QString& expression)
{
    if (m_backend) {
        return m_backend->addWatch(expression).toString();
    }
    return QString();
}

void DebuggerFrontend::removeWatch(const QString& watchId)
{
    if (m_backend) {
        m_backend->removeWatch(QUuid(watchId));
    }
}

void DebuggerFrontend::refreshWatches()
{
    if (m_backend) {
        m_backend->refreshWatches();
    }
}

QVector<DebugWatch> DebuggerFrontend::watches() const
{
    if (m_backend) {
        QVector<WatchExpression> backendWatches = m_backend->watches();
        QVector<DebugWatch> result;
        for (const auto& w : backendWatches) {
            result.append(toDebugWatch(w));
        }
        return result;
    }
    return QVector<DebugWatch>();
}

QVector<DebugVariable> DebuggerFrontend::variables() const
{
    if (m_backend) {
        QVector<Variable> backendVars = m_backend->variables();
        QVector<DebugVariable> result;
        for (const auto& v : backendVars) {
            result.append(toDebugVariable(v));
        }
        return result;
    }
    return QVector<DebugVariable>();
}

QVector<DebugVariable> DebuggerFrontend::locals() const
{
    if (m_backend) {
        QVector<Variable> backendVars = m_backend->locals();
        QVector<DebugVariable> result;
        for (const auto& v : backendVars) {
            result.append(toDebugVariable(v));
        }
        return result;
    }
    return QVector<DebugVariable>();
}

QVector<DebugVariable> DebuggerFrontend::globals() const
{
    if (m_backend) {
        QVector<Variable> backendVars = m_backend->globals();
        QVector<DebugVariable> result;
        for (const auto& v : backendVars) {
            result.append(toDebugVariable(v));
        }
        return result;
    }
    return QVector<DebugVariable>();
}

QVector<DebugStackFrame> DebuggerFrontend::callStack() const
{
    if (m_backend) {
        QVector<StackFrame> backendFrames = m_backend->callStack();
        QVector<DebugStackFrame> result;
        for (const auto& sf : backendFrames) {
            result.append(toDebugStackFrame(sf));
        }
        return result;
    }
    return QVector<DebugStackFrame>();
}

QVariant DebuggerFrontend::evaluate(const QString& expression)
{
    if (m_backend) {
        return m_backend->evaluate(expression);
    }
    return QVariant();
}

QVariant DebuggerFrontend::evaluateSelection(const QString& text)
{
    Q_UNUSED(text);
    return QVariant();
}

void DebuggerFrontend::onBackendStateChanged(DebugState state)
{
    m_currentState = state;
    emit stateChanged(state);
}

void DebuggerFrontend::onBackendStopped(const QString& reason)
{
    Q_UNUSED(reason);
    emit stopped(reason);
}

void DebuggerFrontend::onBackendBreakpointHit(const QUuid& id, int line, const QString& file)
{
    m_currentFile = file;
    m_currentLine = line;
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