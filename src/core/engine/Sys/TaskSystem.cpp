#include "TaskSystem.h"
#include <QThreadPool>
#include <QUuid>
#include <QDebug>
#include <QTimer>

namespace ks {

// ─── Task ─────────────────────────────────────────────────────────────────────

Task::Task(QObject* parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    setAutoDelete(false);
}

Task::~Task() {}

void Task::cancel()
{
    if (m_state == TaskState::Running || m_state == TaskState::Paused) {
        m_cancelled = true;
        m_state = TaskState::Cancelled;
        emit stateChanged(m_state);
    }
}

void Task::pause()
{
    if (m_state != TaskState::Running || !m_canPause) return;
    m_paused = true;
    m_state = TaskState::Paused;
    emit stateChanged(m_state);
}

void Task::resume()
{
    if (m_state != TaskState::Paused) return;
    m_paused = false;
    m_state = TaskState::Running;
    emit stateChanged(m_state);
}

void Task::setInput(const QJsonObject& input) { m_input = input; }

void Task::run()
{
    if (m_cancelled) return;
    m_state = TaskState::Running;
    emit stateChanged(m_state);

    try {
        execute();
        if (!m_cancelled) {
            m_state = TaskState::Completed;
            emit stateChanged(m_state);
            emit completed();
        }
    } catch (const std::exception& e) {
        m_error = QString::fromStdString(e.what());
        m_state = TaskState::Failed;
        emit stateChanged(m_state);
        emit failed(m_error);
    }
}

void Task::reportProgress(float progress, const QString& status)
{
    m_progress = qBound(0.f, progress, 1.f);
    m_status   = status;
    emit progressChanged(m_progress);
    if (!status.isEmpty()) emit statusChanged(status);
}

// ─── TaskManager ──────────────────────────────────────────────────────────────

TaskManager* TaskManager::s_instance = nullptr;

TaskManager* TaskManager::instance()
{
    if (!s_instance) s_instance = new TaskManager();
    return s_instance;
}

TaskManager::TaskManager(QObject* parent)
    : QObject(parent)
{
    // Cap at 4 worker threads for background tasks on Win11
    QThreadPool::globalInstance()->setMaxThreadCount(
        qMax(2, QThread::idealThreadCount() - 1));
}

TaskManager::~TaskManager()
{
    cancelAll();
    s_instance = nullptr;
}

void TaskManager::submit(Task* task, TaskPriority priority)
{
    if (!task) return;
    m_tasks.insert(task->getId(), task);

    connect(task, &Task::completed, this, [this, task](){
        emit taskCompleted(task->getId());
        cleanup(task->getId());
    });
    connect(task, &Task::failed, this, [this, task](const QString& err){
        emit taskFailed(task->getId(), err);
        cleanup(task->getId());
    });
    connect(task, &Task::progressChanged, this, [this, task](float p){
        emit taskProgressChanged(task->getId(), p);
    });

    int prio = 0;
    switch (priority) {
    case TaskPriority::Low:      prio = QThread::LowestPriority;  break;
    case TaskPriority::Normal:   prio = QThread::NormalPriority;  break;
    case TaskPriority::High:     prio = QThread::HighPriority;    break;
    case TaskPriority::Critical: prio = QThread::TimeCriticalPriority; break;
    }

    emit taskSubmitted(task->getId());
    QThreadPool::globalInstance()->start(task, prio);
}

void TaskManager::cancel(const QString& taskId)
{
    if (auto* t = m_tasks.value(taskId)) t->cancel();
}

void TaskManager::cancelAll()
{
    for (auto* t : m_tasks) t->cancel();
}

Task* TaskManager::getTask(const QString& taskId) const
{
    return m_tasks.value(taskId, nullptr);
}

QVector<Task*> TaskManager::getActiveTasks() const
{
    QVector<Task*> active;
    for (auto* t : m_tasks)
        if (t->getState() == TaskState::Running ||
            t->getState() == TaskState::Queued  ||
            t->getState() == TaskState::Paused)
            active << t;
    return active;
}

int TaskManager::getActiveCount() const { return getActiveTasks().size(); }

bool TaskManager::waitForAll(int timeoutMs)
{
    return QThreadPool::globalInstance()->waitForDone(timeoutMs);
}

void TaskManager::cleanup(const QString& taskId)
{
    // Defer deletion so signals finish delivering
    QTimer::singleShot(500, this, [this, taskId](){
        if (auto* t = m_tasks.take(taskId)) {
            t->deleteLater();
        }
    });
}

// ─── TaskQueue ──────────────────────────────────────────────────────────────

TaskQueue::TaskQueue(QObject* parent)
    : QObject(parent)
{
    m_pool = QThreadPool::globalInstance();
}

TaskQueue::~TaskQueue() {}

void TaskQueue::setMaxConcurrentTasks(int max)
{
    m_maxConcurrent = qMax(1, max);
}

void TaskQueue::setThreadPool(QThreadPool* pool)
{
    m_pool = pool ? pool : QThreadPool::globalInstance();
}

void TaskQueue::enqueue(Task* task)
{
    if (!task) return;
    m_tasks.insert(task->getId(), task);
    m_pendingQueue.append(task->getId());

    connect(task, &Task::progressChanged, this, &TaskQueue::onTaskProgress);
    connect(task, &Task::completed, this, &TaskQueue::onTaskCompleted);
    connect(task, &Task::failed, this, &TaskQueue::onTaskFailed);
    connect(task, &Task::cancelled, this, &TaskQueue::onTaskCancelled);

    emit taskEnqueued(task->getId());

    // Start tasks if under concurrency limit
    while (m_runningQueue.size() < m_maxConcurrent && !m_pendingQueue.isEmpty()) {
        QString nextId = m_pendingQueue.takeFirst();
        Task* next = m_tasks.value(nextId);
        if (next) {
            m_runningQueue.append(nextId);
            next->setState(TaskState::Queued);
            emit taskStarted(nextId);
            m_pool->start(next);
        }
    }
}

void TaskQueue::enqueueFront(Task* task)
{
    if (!task) return;
    m_tasks.insert(task->getId(), task);
    m_pendingQueue.prepend(task->getId());

    connect(task, &Task::progressChanged, this, &TaskQueue::onTaskProgress);
    connect(task, &Task::completed, this, &TaskQueue::onTaskCompleted);
    connect(task, &Task::failed, this, &TaskQueue::onTaskFailed);
    connect(task, &Task::cancelled, this, &TaskQueue::onTaskCancelled);

    emit taskEnqueued(task->getId());

    while (m_runningQueue.size() < m_maxConcurrent && !m_pendingQueue.isEmpty()) {
        QString nextId = m_pendingQueue.takeFirst();
        Task* next = m_tasks.value(nextId);
        if (next) {
            m_runningQueue.append(nextId);
            next->setState(TaskState::Queued);
            emit taskStarted(nextId);
            m_pool->start(next);
        }
    }
}

void TaskQueue::cancel(const QString& taskId)
{
    if (auto* t = m_tasks.value(taskId)) t->cancel();
}

void TaskQueue::cancelAll()
{
    for (auto* t : m_tasks) t->cancel();
}

void TaskQueue::pause(const QString& taskId)
{
    if (auto* t = m_tasks.value(taskId)) t->pause();
}

void TaskQueue::resume(const QString& taskId)
{
    if (auto* t = m_tasks.value(taskId)) t->resume();
}

void TaskQueue::remove(const QString& taskId)
{
    m_tasks.remove(taskId);
    m_pendingQueue.removeAll(taskId);
    m_runningQueue.removeAll(taskId);
    m_completedQueue.removeAll(taskId);
}

Task* TaskQueue::getTask(const QString& taskId) const
{
    return m_tasks.value(taskId);
}

QVector<Task*> TaskQueue::getPendingTasks() const
{
    QVector<Task*> result;
    for (const auto& id : m_pendingQueue)
        if (auto* t = m_tasks.value(id)) result.append(t);
    return result;
}

QVector<Task*> TaskQueue::getRunningTasks() const
{
    QVector<Task*> result;
    for (const auto& id : m_runningQueue)
        if (auto* t = m_tasks.value(id)) result.append(t);
    return result;
}

QVector<Task*> TaskQueue::getCompletedTasks() const
{
    QVector<Task*> result;
    for (const auto& id : m_completedQueue)
        if (auto* t = m_tasks.value(id)) result.append(t);
    return result;
}

int TaskQueue::getPendingCount() const { return m_pendingQueue.size(); }
int TaskQueue::getRunningCount() const { return m_runningQueue.size(); }

void TaskQueue::setMaxHistory(int max)
{
    m_maxHistory = qMax(1, max);
}

Task* TaskQueue::takeFirstCompleted()
{
    if (m_completedQueue.isEmpty()) return nullptr;
    auto* t = m_tasks.take(m_completedQueue.first());
    m_completedQueue.removeFirst();
    return t;
}

void TaskQueue::clearHistory()
{
    m_completedQueue.clear();
}

void TaskQueue::onTaskProgress() {
    auto* task = qobject_cast<Task*>(sender());
    if (!task) return;
    emit taskProgress(task->getId(), task->getProgress());
}

void TaskQueue::onTaskCompleted()
{
    auto* task = qobject_cast<Task*>(sender());
    if (!task) return;
    m_runningQueue.removeAll(task->getId());
    m_completedQueue.append(task->getId());
    while (m_completedQueue.size() > m_maxHistory)
        m_completedQueue.removeFirst();
    emit taskCompleted(task->getId());

    // Start next pending task if under concurrency limit
    while (m_runningQueue.size() < m_maxConcurrent && !m_pendingQueue.isEmpty()) {
        QString nextId = m_pendingQueue.takeFirst();
        Task* next = m_tasks.value(nextId);
        if (next) {
            m_runningQueue.append(nextId);
            next->setState(TaskState::Queued);
            emit taskStarted(nextId);
            m_pool->start(next);
        }
    }

    if (m_pendingQueue.isEmpty() && m_runningQueue.isEmpty())
        emit allTasksCompleted();
}

void TaskQueue::onTaskFailed(const QString& error)
{
    auto* task = qobject_cast<Task*>(sender());
    if (!task) return;
    m_runningQueue.removeAll(task->getId());
    emit taskFailed(task->getId(), error);
}

void TaskQueue::onTaskCancelled()
{
    auto* task = qobject_cast<Task*>(sender());
    if (!task) return;
    m_runningQueue.removeAll(task->getId());
    emit taskCancelled(task->getId());
}

Task* TaskQueue::takeFirst(bool pending)
{
    QMutexLocker lock(&m_mutex);
    if (pending) {
        if (m_pendingQueue.isEmpty()) return nullptr;
        QString id = m_pendingQueue.takeFirst();
        return m_tasks.value(id);
    } else {
        if (m_completedQueue.isEmpty()) return nullptr;
        QString id = m_completedQueue.takeFirst();
        return m_tasks.take(id);
    }
}

// ─── BatchTask ───────────────────────────────────────────────────────────────

BatchTask::BatchTask(const QString& id, QObject* parent)
    : Task(parent)
{
    setName(id);
}

BatchTask::~BatchTask() {}

void BatchTask::addSubtask(Task* subtask)
{
    if (subtask) {
        m_subtasks.append(subtask);
        connect(subtask, &Task::completed, this, [this, subtask]() {
            ++m_completedCount;
        });
    }
}

void BatchTask::addSubtasks(const QVector<Task*>& subtasks)
{
    for (auto* t : subtasks) addSubtask(t);
}

void BatchTask::execute()
{
    for (int i = 0; i < m_subtasks.size(); ++i) {
        if (isCancelled()) break;
        m_subtasks[i]->run();
        reportProgress(float(i + 1) / m_subtasks.size());
    }
}

// ─── Workflow ─────────────────────────────────────────────────────────────────

Workflow::Workflow(const QString& id, QObject* parent)
    : QObject(parent)
    , m_id(id) {}

Workflow::~Workflow() {}

void Workflow::setName(const QString& name)
{
    m_name = name;
}

void Workflow::addStep(const QString& stepId, const QString& name, Task* task)
{
    Step step;
    step.id = stepId;
    step.name = name;
    step.task = task;
    m_steps.insert(stepId, step);
    m_executionOrder.append(stepId);
}

void Workflow::addDependency(const QString& stepId, const QString& dependsOn)
{
    if (m_steps.contains(stepId) && m_steps.contains(dependsOn))
        m_steps[stepId].dependsOn.append(dependsOn);
}

void Workflow::setStepEnabled(const QString& stepId, bool enabled)
{
    if (m_steps.contains(stepId))
        m_steps[stepId].optional = !enabled;
}

bool Workflow::isStepEnabled(const QString& stepId) const
{
    return m_steps.contains(stepId) && !m_steps[stepId].optional;
}

bool Workflow::execute()
{
    m_running = true;
    for (const auto& stepId : m_executionOrder) {
        auto& step = m_steps[stepId];
        if (step.task) {
            emit stepStarted(stepId);
            step.task->run();
            emit stepCompleted(stepId);
        }
    }
    m_running = false;
    m_complete = true;
    emit workflowCompleted();
    return true;
}

void Workflow::cancel()
{
    m_running = false;
    for (auto& step : m_steps) {
        if (step.task) step.task->cancel();
    }
}

float Workflow::getProgress() const
{
    if (m_steps.isEmpty()) return 1.0f;
    int completed = 0;
    for (const auto& step : m_steps)
        if (step.task && step.task->getState() == TaskState::Completed)
            ++completed;
    return float(completed) / m_steps.size();
}

QString Workflow::getCurrentStep() const
{
    for (const auto& stepId : m_executionOrder) {
        const auto& step = m_steps[stepId];
        if (step.task && step.task->getState() == TaskState::Running)
            return stepId;
    }
    return QString();
}

} // namespace ks
