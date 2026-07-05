#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QJsonObject>

class QProcess;

namespace ks {

class BreakpointManager : public QObject
{
    Q_OBJECT

public:
    static BreakpointManager* instance();

    struct Breakpoint {
        QString id;
        QString type;
        QString condition;
        QString target;
        bool enabled;
        int hitCount;
        int ignoreCount;
    };

    QString addBreakpoint(const QString& type, const QString& target,
                          const QString& condition = QString());
    void removeBreakpoint(const QString& breakpointId);

    QVector<Breakpoint> getBreakpoints() const { return m_breakpoints.values(); }
    Breakpoint getBreakpoint(const QString& breakpointId) const;

    void enableBreakpoint(const QString& breakpointId, bool enabled);
    bool isBreakpointEnabled(const QString& breakpointId) const;

    void setBreakpointCondition(const QString& breakpointId, const QString& condition);
    QString getBreakpointCondition(const QString& breakpointId) const;

    bool check(const QString& type, const QString& target, const QJsonObject& ctx);

signals:
    void breakpointHit(const QString& breakpointId);
    void breakpointHit(const QString& id, const QString& type, const QString& target, const QJsonObject& ctx);
    void breakpointAdded(const QString& breakpointId);
    void breakpointRemoved(const QString& breakpointId);
    void breakpointEnabled(const QString& breakpointId, bool enabled);

private:
    BreakpointManager(QObject* parent = nullptr);
    ~BreakpointManager();
    Q_DISABLE_COPY(BreakpointManager)

    static BreakpointManager* s_instance;

    QMap<QString, Breakpoint> m_breakpoints;
    int m_nextId = 0;
};

class WatchVariable : public QObject
{
    Q_OBJECT

public:
    static WatchVariable* instance();

    struct Variable {
        QString id;
        QString name;
        QString value;
        QString type;
        QString address;
    };

    void addWatch(const QString& name, const QString& expression);
    void removeWatch(const QString& watchId);

    QVector<Variable> getWatches() const { return m_watches.values(); }
    void refreshWatches();

    void setMaxWatches(int max);
    int getMaxWatches() const { return m_maxWatches; }

    void setAutoRefresh(bool enabled);
    bool isAutoRefreshEnabled() const { return m_autoRefresh; }

    void setRefreshInterval(int ms);
    int getRefreshInterval() const { return m_refreshInterval; }

signals:
    void variableValueChanged(const QString& watchId, const QString& value);
    void watchesRefreshed();

private:
    WatchVariable(QObject* parent = nullptr);
    ~WatchVariable();
    Q_DISABLE_COPY(WatchVariable)

    static WatchVariable* s_instance;

    int m_maxWatches = 100;
    bool m_autoRefresh = false;
    int m_refreshInterval = 1000;

    QMap<QString, Variable> m_watches;
    QMap<QString, QString> m_expressions;
};

class Debugger : public QObject
{
    Q_OBJECT

public:
    static Debugger* instance();

    enum class State {
        Stopped,
        Running,
        Paused,
        Stepping
    };

    void attachToProcess(qint64 processId);
    void detach();

    void start(const QString& executable, const QStringList& arguments = QStringList());
    void stop();

    void pause();
    void resume();

    State getState() const { return m_state; }

    void setBreakpoint(const QString& location);
    void removeBreakpoint(const QString& location);

    void evaluate(const QString& expression);
    void watch(const QString& expression, const QString& name);

    void getCallStack(QVector<QString>& callStack) const;
    void getLocalVariables(QVector<WatchVariable::Variable>& variables) const;

    bool isAttached() const { return m_attached; }

signals:
    void stateChanged(State state);
    void stopped(const QString& reason);
    void resumed();
    void breakpointHit(const QString& location);

private:
    Debugger(QObject* parent = nullptr);
    ~Debugger();
    Q_DISABLE_COPY(Debugger)

    static Debugger* s_instance;

    State m_state = State::Stopped;
    bool m_attached = false;
    QProcess* m_process = nullptr;
    qint64 m_targetPid = 0;
    QString m_lastExpression;
};

} // namespace ks
