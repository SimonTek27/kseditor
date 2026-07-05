#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QRunnable>
#include <QThreadPool>
#include <QMutex>
#include <QWaitCondition>

namespace ks {

enum class TaskPriority {
    Low = 0,
    Normal = 5,
    High = 10,
    Critical = 15
};

enum class TaskState {
    Pending,
    Queued,
    Running,
    Paused,
    Completed,
    Failed,
    Cancelled
};

class Task : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit Task(QObject* parent = nullptr);
    virtual ~Task();

    QString getId() const { return m_id; }
    QString getName() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    TaskState getState() const { return m_state; }
    void setState(TaskState state) { m_state = state; emit stateChanged(state); }
    TaskPriority getPriority() const { return m_priority; }

    float getProgress() const { return m_progress; }
    QString getStatus() const { return m_status; }

    void setInput(const QJsonObject& input);
    QJsonObject getInput() const { return m_input; }

    QString getError() const { return m_error; }

    bool canPause() const { return m_canPause; }
    void setPausable(bool pausable) { m_canPause = pausable; }

    void cancel();
    void pause();
    void resume();
    void reportProgress(float progress, const QString& status = QString());

    virtual void run() override;

signals:
    void progressChanged(float progress);
    void statusChanged(const QString& status);
    void stateChanged(TaskState state);
    void completed();
    void failed(const QString& error);
    void cancelled();

protected:
    virtual void execute() = 0;

    bool isCancelled() const { return m_cancelled; }
    bool isPaused() const { return m_paused; }

    QJsonObject m_input;

private:
    QString m_id;
    QString m_name;
    TaskState m_state = TaskState::Pending;
    TaskPriority m_priority = TaskPriority::Normal;

    float m_progress = 0.0f;
    QString m_status;

    bool m_canPause = false;
    bool m_cancelled = false;
    bool m_paused = false;

    QString m_error;
};

class TaskManager : public QObject
{
    Q_OBJECT

public:
    static TaskManager* instance();

    void submit(Task* task, TaskPriority priority = TaskPriority::Normal);
    void cancel(const QString& taskId);
    void cancelAll();

    Task* getTask(const QString& taskId) const;
    QVector<Task*> getActiveTasks() const;
    int getActiveCount() const;

    bool waitForAll(int timeoutMs = 30000);

signals:
    void taskSubmitted(const QString& taskId);
    void taskCompleted(const QString& taskId);
    void taskFailed(const QString& taskId, const QString& error);
    void taskProgressChanged(const QString& taskId, float progress);

private:
    TaskManager(QObject* parent = nullptr);
    ~TaskManager();
    Q_DISABLE_COPY(TaskManager)

    void cleanup(const QString& taskId);

    static TaskManager* s_instance;

    QMap<QString, Task*> m_tasks;
};

class TaskQueue : public QObject
{
    Q_OBJECT

public:
    explicit TaskQueue(QObject* parent = nullptr);
    ~TaskQueue();

    void setMaxConcurrentTasks(int max);
    int getMaxConcurrentTasks() const { return m_maxConcurrent; }

    void setThreadPool(QThreadPool* pool);
    QThreadPool* getThreadPool() const { return m_pool; }

    void enqueue(Task* task);
    void enqueueFront(Task* task);

    void cancel(const QString& taskId);
    void cancelAll();

    void pause(const QString& taskId);
    void resume(const QString& taskId);

    void remove(const QString& taskId);

    Task* getTask(const QString& taskId) const;
    QVector<Task*> getTasks() const { return m_tasks.values(); }
    QVector<Task*> getPendingTasks() const;
    QVector<Task*> getRunningTasks() const;
    QVector<Task*> getCompletedTasks() const;

    int getPendingCount() const;
    int getRunningCount() const;

    void setMaxHistory(int max);
    int getMaxHistory() const { return m_maxHistory; }

    Task* takeFirstCompleted();
    void clearHistory();

signals:
    void taskEnqueued(const QString& taskId);
    void taskStarted(const QString& taskId);
    void taskProgress(const QString& taskId, float progress);
    void taskCompleted(const QString& taskId);
    void taskFailed(const QString& taskId, const QString& error);
    void taskCancelled(const QString& taskId);

    void queueEmpty();
    void allTasksCompleted();

private slots:
    void onTaskProgress();
    void onTaskCompleted();
    void onTaskFailed(const QString& error);
    void onTaskCancelled();

private:
    Task* takeFirst(bool pending);

    QThreadPool* m_pool;
    int m_maxConcurrent = 4;
    int m_maxHistory = 100;

    QMap<QString, Task*> m_tasks;
    QStringList m_pendingQueue;
    QStringList m_runningQueue;
    QStringList m_completedQueue;

    QMutex m_mutex;
};

class BatchTask : public Task
{
    Q_OBJECT

public:
    explicit BatchTask(const QString& id, QObject* parent = nullptr);
    ~BatchTask();

    void addSubtask(Task* subtask);
    void addSubtasks(const QVector<Task*>& subtasks);

    int getSubtaskCount() const { return m_subtasks.size(); }
    int getCompletedSubtaskCount() const { return m_completedCount; }

signals:
    void subtaskStarted(int index);
    void subtaskCompleted(int index);
    void subtaskFailed(int index, const QString& error);

protected:
    void execute() override;

private:
    QVector<Task*> m_subtasks;
    int m_completedCount = 0;
};

class Workflow : public QObject
{
    Q_OBJECT

public:
    explicit Workflow(const QString& id, QObject* parent = nullptr);
    ~Workflow();

    QString getId() const { return m_id; }
    void setName(const QString& name);
    QString getName() const { return m_name; }

    struct Step {
        QString id;
        QString name;
        Task* task;
        QVector<QString> dependsOn;
        bool optional = false;
    };

    void addStep(const QString& stepId, const QString& name, Task* task);
    void addDependency(const QString& stepId, const QString& dependsOn);

    void setStepEnabled(const QString& stepId, bool enabled);
    bool isStepEnabled(const QString& stepId) const;

    bool execute();
    void cancel();

    float getProgress() const;
    QString getCurrentStep() const;

    bool isRunning() const { return m_running; }
    bool isComplete() const { return m_complete; }

    QVector<Step> getSteps() const { return m_steps.values(); }

signals:
    void stepStarted(const QString& stepId);
    void stepCompleted(const QString& stepId);
    void stepFailed(const QString& stepId, const QString& error);
    void workflowCompleted();

private:
    QString m_id;
    QString m_name;
    bool m_running = false;
    bool m_complete = false;

    QMap<QString, Step> m_steps;
    QVector<QString> m_executionOrder;
};

} // namespace ks