#include "TransactionManager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>

namespace ks {

static TransactionManager* s_instance = nullptr;

TransactionManager* TransactionManager::instance()
{
    if (!s_instance) s_instance = new TransactionManager();
    return s_instance;
}

TransactionManager::TransactionManager(QObject* parent)
    : QObject(parent)
{
}

TransactionManager::~TransactionManager()
{
    s_instance = nullptr;
}

QString TransactionManager::beginTransaction(const QString& description)
{
    QMutexLocker lock(&m_mutex);
    
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    Transaction tx;
    tx.id = id;
    tx.description = description.isEmpty() ? QString("Transaction %1").arg(m_transactions.size() + 1) : description;
    tx.startTime = QDateTime::currentDateTime();
    
    m_transactions[id] = tx;
    m_transactionStack.push(id);
    
    emit transactionStarted(id, tx.description);
    
    qInfo() << "[TransactionManager] Started transaction:" << id << tx.description;
    return id;
}

void TransactionManager::commitTransaction(const QString& transactionId)
{
    QMutexLocker lock(&m_mutex);
    
    if (!m_transactions.contains(transactionId)) {
        qWarning() << "[TransactionManager] Transaction not found:" << transactionId;
        return;
    }
    
    if (!m_transactionStack.isEmpty() && m_transactionStack.top() != transactionId) {
        qWarning() << "[TransactionManager] Cannot commit nested transaction out of order";
        return;
    }
    
    if (!m_transactionStack.isEmpty()) m_transactionStack.pop();
    
    emit transactionCommitted(transactionId);
    
    qInfo() << "[TransactionManager] Committed transaction:" << transactionId;
}

void TransactionManager::rollbackTransaction(const QString& transactionId)
{
    QMutexLocker lock(&m_mutex);
    
    if (!m_transactions.contains(transactionId)) {
        qWarning() << "[TransactionManager] Transaction not found:" << transactionId;
        return;
    }
    
    // Only allow rollback of top transaction
    if (!m_transactionStack.isEmpty() && m_transactionStack.top() != transactionId) {
        qWarning() << "[TransactionManager] Cannot rollback nested transaction out of order";
        return;
    }
    
    applyRollback(transactionId);
    
    if (!m_transactionStack.isEmpty()) m_transactionStack.pop();
    
    emit transactionRolledBack(transactionId, "User requested rollback");
    
    qInfo() << "[TransactionManager] Rolled back transaction:" << transactionId;
}

void TransactionManager::applyRollback(const QString& transactionId)
{
    auto it = m_transactions.find(transactionId);
    if (it == m_transactions.end()) return;
    
    const auto& changes = it->changes;
    
    // Apply in reverse order
    for (int i = changes.size() - 1; i >= 0; --i) {
        const auto& change = changes[i];
        if (m_restoreCallbacks.contains(change.moduleId)) {
            QJsonObject state;
            state[change.property] = change.oldValue;
            m_restoreCallbacks[change.moduleId](change.moduleId, state);
        }
    }
    
    m_transactions.erase(it);
}

void TransactionManager::registerModule(const QString& moduleId, QObject* module)
{
    Q_UNUSED(module)
    // Module registration for future direct integration
}

void TransactionManager::unregisterModule(const QString& moduleId)
{
    m_snapshotCallbacks.remove(moduleId);
    m_restoreCallbacks.remove(moduleId);
}

QVector<TransactionManager::Change> TransactionManager::getChanges(const QString& transactionId) const
{
    QMutexLocker lock(&m_mutex);
    return m_transactions.value(transactionId).changes;
}

QVector<TransactionManager::Change> TransactionManager::getAllChanges() const
{
    QMutexLocker lock(&m_mutex);
    QVector<Change> all;
    for (const auto& tx : m_transactions) {
        all.append(tx.changes);
    }
    return all;
}

QJsonObject TransactionManager::serializeTransaction(const QString& transactionId) const
{
    QMutexLocker lock(&m_mutex);
    
    auto it = m_transactions.find(transactionId);
    if (it == m_transactions.end()) return {};
    
    QJsonObject obj;
    obj["id"] = it->id;
    obj["description"] = it->description;
    obj["startTime"] = it->startTime.toString(Qt::ISODate);
    
    QJsonArray changesArray;
    for (const auto& c : it->changes) {
        QJsonObject changeObj;
        changeObj["moduleId"] = c.moduleId;
        changeObj["property"] = c.property;
        changeObj["oldValue"] = c.oldValue;
        changeObj["newValue"] = c.newValue;
        changeObj["description"] = c.description;
        changeObj["timestamp"] = c.timestamp.toString(Qt::ISODate);
        changesArray.append(changeObj);
    }
    obj["changes"] = changesArray;
    
    return obj;
}

void TransactionManager::deserializeTransaction(const QJsonObject& data)
{
    QMutexLocker lock(&m_mutex);
    
    Transaction tx;
    tx.id = data["id"].toString();
    tx.description = data["description"].toString();
    tx.startTime = QDateTime::fromString(data["startTime"].toString(), Qt::ISODate);
    
    QJsonArray changesArray = data["changes"].toArray();
    for (const auto& v : changesArray) {
        QJsonObject c = v.toObject();
        Change change;
        change.transactionId = tx.id;
        change.moduleId = c["moduleId"].toString();
        change.property = c["property"].toString();
        change.oldValue = c["oldValue"];
        change.newValue = c["newValue"];
        change.description = c["description"].toString();
        change.timestamp = QDateTime::fromString(c["timestamp"].toString(), Qt::ISODate);
        tx.changes.append(change);
    }
    
    m_transactions[tx.id] = tx;
}

void TransactionManager::setSnapshotCallback(const QString& moduleId, ModuleSnapshotCallback cb)
{
    m_snapshotCallbacks[moduleId] = cb;
}

void TransactionManager::setRestoreCallback(const QString& moduleId, ModuleRestoreCallback cb)
{
    m_restoreCallbacks[moduleId] = cb;
}

} // namespace ks